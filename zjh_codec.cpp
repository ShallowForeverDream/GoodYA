
#include "stdafx.h"
#include "zjh_codec.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <queue>
#include <string>
#include <vector>
#include <cstring>

namespace {

// ============================================================================
// ZJH 编解码模块说明（论文注释版）
// 1) 当前模块负责 .zjh 文件的压缩、解压、密码封装与包信息探测。
// 2) 压缩核心采用“按 len 位分组 + 哈夫曼编码 + 可重前缀唯一可解校验/调整”。
// 3) 生成格式优先使用 ENC2；解码兼容 ENC2、ENC1 以及历史 ZJH1 容器。
// 4) 当启用密码时，先对编码负载做异或流混淆，再在文件尾写入密码尾标记：
//    [ "ZJHPWD1"(7字节) | flag(1字节) | hash32(4字节) ]。
// 5) 代码面向 VC6，避免使用 C++11 新语法；注释使用中文，便于论文阐述实现细节。
// ============================================================================

// len 的合法范围：每个“符号”由 1~10 个 bit 组成。
const int MAX_LEN = 10;
const int MAX_SYMBOLS = (1 << MAX_LEN);

// 三类负载魔数：
// ENC1: 早期表项自带码字位串版本；
// ENC2: 新版“表头 + 表位流 + 数据位流”版本；
// ZJH1: 更早期/历史容器格式（本模块仅做兼容解码）。
const char MAGIC_ENC1[4] = {'E', 'N', 'C', '1'};
const char MAGIC_ENC2[4] = {'E', 'N', 'C', '2'};
const char MAGIC_LEGACY[4] = {'Z', 'J', 'H', '1'};

// 密码尾部标记。存在该尾部时，表示文件负载已加密，需密码参与解码。
const char TAIL_MAGIC[] = "ZJHPWD1";
const int TAIL_MAGIC_LEN = 7;
const int TAIL_SIZE = TAIL_MAGIC_LEN + 1 + 4;

// 历史 ZJH1 头结构（与旧实现保持二进制兼容）。
struct LegacyHeader
{
	char magic[4];
	unsigned long payloadLen;
	unsigned long originalLen;
	unsigned char methodLen;
	unsigned char reserved[3];
};

// ============================================================================
// 进度回调上下文（论文说明）
// 1) 压缩过程由多个阶段组成，不同阶段工作量差异较大；
// 2) 本实现把总进度映射到 [0,100]，并通过阶段枚举解释当前处理状态；
// 3) ETA 采用“线性外推”估计：eta = elapsed * (100 - p) / p。
//    该方式实现简单、开销极低，适合 VC6 单线程压缩流程。
// ============================================================================
struct ProgressContext
{
	ZJH_ProgressCallback callback;
	void* userData;
	DWORD startTick;
	int lastPercent;
	int lastStage;

	ProgressContext(ZJH_ProgressCallback cb, void* ud)
		: callback(cb), userData(ud), startTick(::GetTickCount()), lastPercent(-1), lastStage(-1)
	{
	}
};

static int ClampPercent(int percent)
{
	if (percent < 0) return 0;
	if (percent > 100) return 100;
	return percent;
}

static unsigned long CalcElapsedMs(DWORD startTick)
{
	DWORD now = ::GetTickCount();
	return (unsigned long)(now - startTick);
}

static unsigned long CalcEtaMs(int percent, unsigned long elapsedMs)
{
	if (percent <= 0 || percent >= 100)
		return 0;
	unsigned __int64 remain = (unsigned __int64)elapsedMs * (unsigned __int64)(100 - percent);
	return (unsigned long)(remain / (unsigned __int64)percent);
}

static void ReportProgress(ProgressContext* ctx, int percent, int stage, bool force)
{
	if (ctx == NULL || ctx->callback == NULL)
		return;

	percent = ClampPercent(percent);
	if (!force && ctx->lastPercent == percent && ctx->lastStage == stage)
		return;

	unsigned long elapsedMs = CalcElapsedMs(ctx->startTick);
	unsigned long etaMs = CalcEtaMs(percent, elapsedMs);
	ctx->callback(percent, stage, elapsedMs, etaMs, ctx->userData);
	ctx->lastPercent = percent;
	ctx->lastStage = stage;
}

static void ReportStageProgress(ProgressContext* ctx,
	int stage,
	unsigned __int64 done,
	unsigned __int64 total,
	int startPercent,
	int endPercent,
	bool force)
{
	if (ctx == NULL || ctx->callback == NULL)
		return;

	if (total == 0)
	{
		ReportProgress(ctx, endPercent, stage, force);
		return;
	}

	if (done > total)
		done = total;

	unsigned __int64 width = (unsigned __int64)(endPercent - startPercent);
	int percent = startPercent + (int)((done * width) / total);
	ReportProgress(ctx, percent, stage, force);
}

template <typename T>
static void AppendPod(std::vector<unsigned char>& out, const T& value)
{
	// 将 POD 类型按“内存字节序”直接附加到缓冲区末尾。
	// 注意：该项目的读写双方运行在同平台/同编译器环境，采用原生字节序。
	size_t oldSize = out.size();
	out.resize(oldSize + sizeof(T));
	memcpy(&out[oldSize], &value, sizeof(T));
}

template <typename T>
static bool ReadPod(const std::vector<unsigned char>& in, size_t& offset, T& value)
{
	// 从 in[offset...] 读取一个 POD 对象，读取成功后推进 offset。
	if (offset + sizeof(T) > in.size())
		return false;
	memcpy(&value, &in[offset], sizeof(T));
	offset += sizeof(T);
	return true;
}

static unsigned __int64 BytesForBits(unsigned __int64 bitCount)
{
	// bit 数转字节数（向上取整）。
	return (bitCount + 7) / 8;
}

static int GetBitFromBytes(const unsigned char* bytes, unsigned __int64 bitIndex)
{
	// 位序约定：高位在前（MSB first）。
	// 即每个字节按 bit7, bit6, ... bit0 的顺序读取。
	unsigned __int64 byteIndex64 = bitIndex >> 3;
	int bitInByte = 7 - (int)(bitIndex & 7);
	size_t byteIndex = (size_t)byteIndex64;
	return (bytes[byteIndex] >> bitInByte) & 1;
}

static bool ReadAllBytes(const std::string& path, std::vector<unsigned char>& out)
{
	// 全文件二进制读取。
	out.clear();
	std::ifstream fin(path.c_str(), std::ios::binary);
	if (!fin)
		return false;

	fin.seekg(0, std::ios::end);
	std::streamoff sz = fin.tellg();
	if (sz < 0)
		return false;
	fin.seekg(0, std::ios::beg);

	out.resize((size_t)sz);
	if (!out.empty())
	{
		fin.read((char*)&out[0], (std::streamsize)out.size());
		if (!fin)
			return false;
	}
	return true;
}

static bool WriteAllBytes(const std::string& path, const std::vector<unsigned char>& data)
{
	// 全文件二进制覆盖写入。
	std::ofstream fout(path.c_str(), std::ios::binary | std::ios::trunc);
	if (!fout)
		return false;
	if (!data.empty())
	{
		fout.write((const char*)&data[0], (std::streamsize)data.size());
		if (!fout)
			return false;
	}
	return true;
}

static bool WriteAllBytesWithProgress(const std::string& path,
	const std::vector<unsigned char>& data,
	ProgressContext* progressCtx,
	int stage,
	int startPercent,
	int endPercent)
{
	// 分块写盘并上报进度，避免“长时间写文件阶段”界面无响应。
	std::ofstream fout(path.c_str(), std::ios::binary | std::ios::trunc);
	if (!fout)
		return false;

	if (data.empty())
	{
		ReportProgress(progressCtx, endPercent, stage, true);
		return true;
	}

	const size_t chunk = 64 * 1024;
	size_t offset = 0;
	while (offset < data.size())
	{
		size_t left = data.size() - offset;
		size_t n = (left < chunk) ? left : chunk;
		fout.write((const char*)(&data[offset]), (std::streamsize)n);
		if (!fout)
			return false;
		offset += n;
		ReportStageProgress(progressCtx,
			stage,
			(unsigned __int64)offset,
			(unsigned __int64)data.size(),
			startPercent,
			endPercent,
			(offset == data.size()));
	}

	return true;
}

static bool HasSuffixIgnoreCase(const std::string& text, const std::string& suffix)
{
	// 不区分大小写的后缀判断，主要用于验证 .zjh 扩展名。
	if (text.size() < suffix.size())
		return false;

	size_t base = text.size() - suffix.size();
	for (size_t i = 0; i < suffix.size(); ++i)
	{
		char a = text[base + i];
		char b = suffix[i];
		if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
		if (a != b)
			return false;
	}
	return true;
}

static unsigned long PasswordHash32(const char* password)
{
	// FNV-1a 32 位哈希：
	// - 用于“密码是否正确”的快速校验，不承担抗碰撞的密码学安全职责；
	// - 真正的数据保护由后续 XorCryptPayload 的流混淆实现（工程级轻量方案）。
	if (password == NULL || password[0] == '\0')
		return 0;

	unsigned long hash = 2166136261UL;
	const unsigned char* p = (const unsigned char*)password;
	while (*p)
	{
		hash ^= (unsigned long)(*p);
		hash *= 16777619UL;
		++p;
	}
	return hash;
}

static void XorCryptPayload(std::vector<unsigned char>& data,
	const char* password,
	ProgressContext* progressCtx,
	int stage,
	int startPercent,
	int endPercent)
{
	// 轻量异或流混淆（可逆）：
	// 同一函数可同时用于加密和解密（XOR 自反性）。
	// 论文可描述为“基于口令扰动的伪随机密钥流”。
	if (data.empty() || password == NULL || password[0] == '\0')
	{
		ReportProgress(progressCtx, endPercent, stage, true);
		return;
	}

	int passLen = (int)strlen(password);
	if (passLen <= 0)
	{
		ReportProgress(progressCtx, endPercent, stage, true);
		return;
	}

	unsigned long seed = 0xA5C39E7DUL;
	for (size_t i = 0; i < data.size(); ++i)
	{
		seed = seed * 1664525UL + 1013904223UL + (unsigned char)password[i % (size_t)passLen];
		unsigned char key = (unsigned char)(
			((seed >> 24) & 0xFF) ^
			(unsigned char)password[(i * 7 + 3) % (size_t)passLen] ^
			(i & 0xFF));
		data[i] ^= key;

		if (((i & 0x3FFF) == 0) || (i + 1 == data.size()))
		{
			ReportStageProgress(progressCtx,
				stage,
				(unsigned __int64)(i + 1),
				(unsigned __int64)data.size(),
				startPercent,
				endPercent,
				(i + 1 == data.size()));
		}
	}
}

static bool ParseTailInfo(const std::vector<unsigned char>& fileData,
	bool* hasPassword,
	unsigned long* hashOut,
	size_t* payloadSize)
{
	// 尝试解析密码尾标记：
	// - 若尾部不是 TAIL_MAGIC，则视为“无密码尾”，payload 为全文件；
	// - 若匹配，则提取 hasPassword/hash/payload 截止位置。
	// 返回 true 表示“解析过程无 I/O 级异常”，并不代表密码一定正确。
	if (hasPassword) *hasPassword = false;
	if (hashOut) *hashOut = 0;
	if (payloadSize) *payloadSize = fileData.size();

	if (fileData.size() < (size_t)TAIL_SIZE)
		return true;

	size_t tailOffset = fileData.size() - (size_t)TAIL_SIZE;
	const unsigned char* tail = &fileData[tailOffset];
	if (memcmp(tail, TAIL_MAGIC, TAIL_MAGIC_LEN) != 0)
		return true;

	bool hasPwd = ((tail[TAIL_MAGIC_LEN] & 1) != 0);
	unsigned long hash = 0;
	memcpy(&hash, tail + TAIL_MAGIC_LEN + 1, 4);

	if (hasPassword) *hasPassword = hasPwd;
	if (hashOut) *hashOut = hash;
	if (payloadSize) *payloadSize = tailOffset;
	return true;
}

struct BitWriter
{
	// 位写入器：将离散 bit 序列按 MSB-first 打包到 bytes。
	std::vector<unsigned char> bytes;
	unsigned char current;
	int used;
	unsigned __int64 bitCount;

	BitWriter()
		: current(0), used(0), bitCount(0)
	{
	}

	void pushBit(int bit)
	{
		// 每写入 8bit 输出一个字节。
		current = (unsigned char)((current << 1) | (bit & 1));
		++used;
		++bitCount;
		if (used == 8)
		{
			bytes.push_back(current);
			current = 0;
			used = 0;
		}
	}

	void pushCode(const std::string& code)
	{
		// 码字是由 '0'/'1' 字符组成的字符串表示。
		for (size_t i = 0; i < code.size(); ++i)
			pushBit(code[i] == '1' ? 1 : 0);
	}

	void flush()
	{
		// 末字节不足 8 位时左移补 0。
		if (used == 0)
			return;
		current = (unsigned char)(current << (8 - used));
		bytes.push_back(current);
		current = 0;
		used = 0;
	}
};

struct EncTableEntry
{
	// 编码表项：符号值 + 码字。
	unsigned short symbol;
	std::string code;
};

class EncoderWorker
{
public:
	// 核心入口：读取原文 -> 统计符号 -> 哈夫曼编码 -> 唯一可解调整 -> 输出 ENC2 负载。
	EncoderWorker()
		: len_(0), symbolLimit_(0), root_(NULL), originalBits_(0), progressCtx_(NULL)
	{
	}

	~EncoderWorker()
	{
		clearNodes();
	}

	bool run(const std::string& inputPath,
		int len,
		std::vector<unsigned char>& outPayload,
		ProgressContext* progressCtx)
	{
		// 初始化运行时状态。
		progressCtx_ = progressCtx;
		ReportProgress(progressCtx_, 0, ZJH_PROGRESS_STAGE_PREPARE, true);
		len_ = len;
		symbolLimit_ = (1 << len_);
		freq_.assign((size_t)symbolLimit_, 0);
		codes_.assign((size_t)symbolLimit_, std::string());
		usedSymbols_.clear();
		inputSymbols_.clear();
		clearNodes();
		root_ = NULL;
		originalBits_ = 0;
		checkBits_.clear();
		ways_.clear();
		acNodes_.clear();

		if (!readInput(inputPath))
			return false;

		if (!inputSymbols_.empty())
		{
			// 非空输入时构建编码树与码表；空输入直接输出空负载结构。
			ReportProgress(progressCtx_, 32, ZJH_PROGRESS_STAGE_BUILD_TREE, true);
			buildHuffmanTree();
			assignCodes(root_, "");
			if (usedSymbols_.size() == 1)
				codes_[(size_t)usedSymbols_[0]] = "0";
			ReportProgress(progressCtx_, 45, ZJH_PROGRESS_STAGE_BUILD_TREE, true);
			relaxTreeForUniqueDecode();
		}
		else
		{
			ReportProgress(progressCtx_, 45, ZJH_PROGRESS_STAGE_BUILD_TREE, true);
			ReportProgress(progressCtx_, 65, ZJH_PROGRESS_STAGE_OPTIMIZE_CODE, true);
		}

		return writeOutputBytes(outPayload);
	}

private:
	struct Node
	{
		// 哈夫曼树节点：
		// symbol == -1 表示内部节点；否则为叶子节点（对应一个符号）。
		int symbol;
		Node* left;
		Node* right;
		std::string pathBits;

		Node(int symbolIn, Node* leftIn, Node* rightIn)
			: symbol(symbolIn), left(leftIn), right(rightIn)
		{
		}
	};

	struct QueueInfo
	{
		// 优先队列项：按频次升序合并最小两棵子树。
		unsigned __int64 count;
		Node* node;

		QueueInfo()
			: count(0), node(NULL)
		{
		}

		QueueInfo(unsigned __int64 countIn, Node* nodeIn)
			: count(countIn), node(nodeIn)
		{
		}
	};

	struct QueueInfoGreater
	{
		// std::priority_queue 默认大顶堆，这里自定义为“小频次优先”。
		bool operator()(const QueueInfo& a, const QueueInfo& b) const
		{
			return a.count > b.count;
		}
	};

	struct ACNode
	{
		// AC 自动机节点，用于“给定码字集合”上的线性 DP 分词计数。
		// 目的：检测当前码表对目标位串是否“唯一可解”。
		int next0;
		int next1;
		int fail;
		std::vector<unsigned short> outLengths;

		ACNode()
			: next0(-1), next1(-1), fail(0)
		{
		}
	};

private:
	int len_;
	int symbolLimit_;
	std::vector<int> freq_;
	std::vector<std::string> codes_;
	std::vector<int> usedSymbols_;
	std::vector<int> inputSymbols_;
	std::vector<Node*> nodes_;
	Node* root_;
	unsigned __int64 originalBits_;

	std::vector<unsigned char> checkBits_;
	std::vector<unsigned char> ways_;
	std::vector<ACNode> acNodes_;
	ProgressContext* progressCtx_;

	void clearNodes()
	{
		// 释放哈夫曼树节点，避免内存泄漏（VC6 无智能指针）。
		for (size_t i = 0; i < nodes_.size(); ++i)
			delete nodes_[i];
		nodes_.clear();
	}

	Node* makeNode(int symbol, Node* left, Node* right)
	{
		// 工厂方法：统一记录节点，便于统一释放。
		Node* n = new Node(symbol, left, right);
		nodes_.push_back(n);
		return n;
	}

	bool readInput(const std::string& inputPath)
	{
		// 按 len 位切片读取：
		// - 每 len bit 形成一个 symbol；
		// - 末尾不足 len 位时右侧补 0，再形成最后一个 symbol。
		std::ifstream fin(inputPath.c_str(), std::ios::binary);
		if (!fin)
			return false;

		fin.seekg(0, std::ios::end);
		std::streamoff fileSize = fin.tellg();
		if (fileSize < 0)
			return false;
		fin.seekg(0, std::ios::beg);

		unsigned int current = 0;
		int currentLen = 0;
		char ch = 0;
		unsigned __int64 bytesRead = 0;
		while (fin.get(ch))
		{
			unsigned char byte = (unsigned char)ch;
			++bytesRead;
			originalBits_ += 8;
			for (int bit = 7; bit >= 0; --bit)
			{
				current = (current << 1) | ((byte >> bit) & 1u);
				++currentLen;
				if (currentLen == len_)
				{
					int symbol = (int)current;
					inputSymbols_.push_back(symbol);
					++freq_[(size_t)symbol];
					current = 0;
					currentLen = 0;
				}
			}

			if (((bytesRead & 0x0FFF) == 0) || (bytesRead == (unsigned __int64)fileSize))
			{
				ReportStageProgress(progressCtx_,
					ZJH_PROGRESS_STAGE_READ_INPUT,
					bytesRead,
					(unsigned __int64)fileSize,
					1,
					30,
					(bytesRead == (unsigned __int64)fileSize));
			}
		}

		if (currentLen > 0)
		{
			current <<= (len_ - currentLen);
			int symbol = (int)current;
			inputSymbols_.push_back(symbol);
			++freq_[(size_t)symbol];
		}

		for (int symbol = 0; symbol < symbolLimit_; ++symbol)
		{
			if (freq_[(size_t)symbol] > 0)
				usedSymbols_.push_back(symbol);
		}
		if (fileSize == 0)
			ReportProgress(progressCtx_, 30, ZJH_PROGRESS_STAGE_READ_INPUT, true);
		return true;
	}

	void buildHuffmanTree()
	{
		// 标准哈夫曼建树（按频次合并）。
		std::priority_queue<QueueInfo, std::vector<QueueInfo>, QueueInfoGreater> pq;
		for (size_t i = 0; i < usedSymbols_.size(); ++i)
		{
			int symbol = usedSymbols_[i];
			pq.push(QueueInfo((unsigned __int64)freq_[(size_t)symbol], makeNode(symbol, NULL, NULL)));
		}

		while (!pq.empty())
		{
			QueueInfo first = pq.top();
			pq.pop();
			if (pq.empty())
			{
				root_ = first.node;
				break;
			}
			QueueInfo second = pq.top();
			pq.pop();
			pq.push(QueueInfo(first.count + second.count, makeNode(-1, first.node, second.node)));
		}
	}

	void assignCodes(Node* node, const std::string& current)
	{
		// DFS 生成码表：左边追加 0，右边追加 1。
		if (node == NULL)
			return;

		node->pathBits = current;
		if (node->symbol != -1)
			codes_[(size_t)node->symbol] = current;

		assignCodes(node->left, current + "0");
		assignCodes(node->right, current + "1");
	}

	bool buildCheckBitstream()
	{
		// 将“输入符号序列”按当前码表展开为一条位串，用于唯一可解性验证。
		size_t bitCount = 0;
		for (size_t i = 0; i < inputSymbols_.size(); ++i)
		{
			const std::string& code = codes_[(size_t)inputSymbols_[i]];
			if (code.empty())
				return false;
			bitCount += code.size();
		}

		checkBits_.clear();
		checkBits_.reserve(bitCount);

		for (size_t j = 0; j < inputSymbols_.size(); ++j)
		{
			const std::string& code = codes_[(size_t)inputSymbols_[j]];
			for (size_t k = 0; k < code.size(); ++k)
				checkBits_.push_back((unsigned char)(code[k] == '1'));
		}
		return true;
	}

	bool buildCheckAutomaton()
	{
		// 把“所有码字”构建为 AC 自动机：
		// 运行时可在 O(m + 匹配数) 内找出每个位置的可结束码字长度。
		acNodes_.clear();
		acNodes_.push_back(ACNode());

		for (size_t i = 0; i < usedSymbols_.size(); ++i)
		{
			int symbol = usedSymbols_[i];
			const std::string& code = codes_[(size_t)symbol];
			if (code.empty())
				return false;

			int node = 0;
			for (size_t j = 0; j < code.size(); ++j)
			{
				int bit = (code[j] == '1') ? 1 : 0;
				int nextRef = (bit == 0) ? acNodes_[(size_t)node].next0 : acNodes_[(size_t)node].next1;
				if (nextRef == -1)
				{
					nextRef = (int)acNodes_.size();
					if (bit == 0)
						acNodes_[(size_t)node].next0 = nextRef;
					else
						acNodes_[(size_t)node].next1 = nextRef;
					acNodes_.push_back(ACNode());
				}
				node = nextRef;
			}
			acNodes_[(size_t)node].outLengths.push_back((unsigned short)code.size());
		}

		std::queue<int> q;
		for (int bit = 0; bit < 2; ++bit)
		{
			int child = (bit == 0) ? acNodes_[0].next0 : acNodes_[0].next1;
			if (child == -1)
			{
				if (bit == 0)
					acNodes_[0].next0 = 0;
				else
					acNodes_[0].next1 = 0;
				continue;
			}
			acNodes_[(size_t)child].fail = 0;
			q.push(child);
		}

		while (!q.empty())
		{
			int node = q.front();
			q.pop();

			for (int bit = 0; bit < 2; ++bit)
			{
				int child = (bit == 0) ? acNodes_[(size_t)node].next0 : acNodes_[(size_t)node].next1;
				if (child == -1)
				{
					int failState = acNodes_[(size_t)node].fail;
					int jump = (bit == 0) ? acNodes_[(size_t)failState].next0 : acNodes_[(size_t)failState].next1;
					if (bit == 0)
						acNodes_[(size_t)node].next0 = jump;
					else
						acNodes_[(size_t)node].next1 = jump;
					continue;
				}

				int failState = acNodes_[(size_t)node].fail;
				int nextFail = (bit == 0) ? acNodes_[(size_t)failState].next0 : acNodes_[(size_t)failState].next1;
				acNodes_[(size_t)child].fail = nextFail;

				std::vector<unsigned short>& childOut = acNodes_[(size_t)child].outLengths;
				const std::vector<unsigned short>& failOut = acNodes_[(size_t)nextFail].outLengths;
				for (size_t oi = 0; oi < failOut.size(); ++oi)
					childOut.push_back(failOut[oi]);
				q.push(child);
			}
		}

		return true;
	}

	bool checkUniqueDecodeFast()
	{
		// 唯一可解判定（核心）：
		// ways[i] 表示前 i 位的可分解方案数（截断到 0/1/2，2 表示“至少两种”）。
		// 结论：ways[m] == 1 才是“唯一可解”。
		if (!buildCheckBitstream())
			return false;
		if (checkBits_.empty())
			return true;
		if (!buildCheckAutomaton())
			return false;

		size_t m = checkBits_.size();
		ways_.assign(m + 1, 0);
		ways_[0] = 1;

		int state = 0;
		for (size_t i = 0; i < m; ++i)
		{
			state = checkBits_[i] ? acNodes_[(size_t)state].next1 : acNodes_[(size_t)state].next0;
			unsigned char waysHere = 0;
			const std::vector<unsigned short>& outs = acNodes_[(size_t)state].outLengths;
			for (size_t j = 0; j < outs.size(); ++j)
			{
				unsigned short length = outs[j];
				if (length == 0 || (size_t)length > i + 1)
					continue;
				int sum = (int)waysHere + (int)ways_[i + 1 - (size_t)length];
				if (sum > 2) sum = 2;
				waysHere = (unsigned char)sum;
				if (waysHere == 2)
					break;
			}
			ways_[i + 1] = waysHere;
		}

		return ways_[m] == 1;
	}

	void relaxTreeForUniqueDecode()
	{
		// 可重前缀调整策略：
		// 在 BFS 节点序上尝试“内部节点与叶子节点符号互换”，并实时做唯一可解校验；
		// 一旦找到满足 unique decode 的方案即停止。
		if (root_ == NULL)
		{
			ReportProgress(progressCtx_, 65, ZJH_PROGRESS_STAGE_OPTIMIZE_CODE, true);
			return;
		}

		ReportProgress(progressCtx_, 46, ZJH_PROGRESS_STAGE_OPTIMIZE_CODE, true);

		std::vector<Node*> bfsNodes;
		std::queue<Node*> q;
		q.push(root_);
		while (!q.empty())
		{
			Node* current = q.front();
			q.pop();
			bfsNodes.push_back(current);
			if (current->left != NULL) q.push(current->left);
			if (current->right != NULL) q.push(current->right);
		}

		std::map<std::string, int> codeCount;
		for (size_t i = 0; i < usedSymbols_.size(); ++i)
		{
			const std::string& key = codes_[(size_t)usedSymbols_[i]];
			codeCount[key] = codeCount[key] + 1;
		}

		int nodeCount = (int)bfsNodes.size();
		unsigned __int64 totalChecks = 0;
		for (int ti = 0; ti < nodeCount; ++ti)
			totalChecks += (unsigned __int64)(nodeCount - ti - 1);
		unsigned __int64 checked = 0;

		for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			Node* internal = bfsNodes[(size_t)nodeIndex];
			if (internal->symbol != -1 || internal->pathBits.empty())
				continue;

			for (int j = nodeCount - 1; j > nodeIndex; --j)
			{
				++checked;
				if (((checked & 0x3F) == 0) || checked == totalChecks)
				{
					ReportStageProgress(progressCtx_,
						ZJH_PROGRESS_STAGE_OPTIMIZE_CODE,
						checked,
						(totalChecks == 0) ? 1 : totalChecks,
						46,
						65,
						(checked == totalChecks));
				}

				Node* leaf = bfsNodes[(size_t)j];
				if (leaf->symbol == -1)
					continue;

				int leafSymbol = leaf->symbol;
				const std::string previousCode = codes_[(size_t)leafSymbol];
				const std::string& candidateCode = internal->pathBits;
				if (candidateCode == previousCode)
					continue;

				std::map<std::string, int>::iterator dupIt = codeCount.find(candidateCode);
				if (dupIt != codeCount.end() && dupIt->second > 0)
					continue;

				std::map<std::string, int>::iterator prevIt = codeCount.find(previousCode);
				if (prevIt != codeCount.end())
				{
					prevIt->second -= 1;
					if (prevIt->second == 0)
						codeCount.erase(prevIt);
				}
				codeCount[candidateCode] = codeCount[candidateCode] + 1;

				codes_[(size_t)leafSymbol] = candidateCode;
				std::swap(internal->symbol, leaf->symbol);

				if (checkUniqueDecodeFast())
				{
					ReportProgress(progressCtx_, 65, ZJH_PROGRESS_STAGE_OPTIMIZE_CODE, true);
					return;
				}

				std::swap(internal->symbol, leaf->symbol);
				codes_[(size_t)leafSymbol] = previousCode;

				std::map<std::string, int>::iterator candIt = codeCount.find(candidateCode);
				if (candIt != codeCount.end())
				{
					candIt->second -= 1;
					if (candIt->second == 0)
						codeCount.erase(candIt);
				}
				codeCount[previousCode] = codeCount[previousCode] + 1;
			}
		}

		ReportProgress(progressCtx_, 65, ZJH_PROGRESS_STAGE_OPTIMIZE_CODE, true);
	}

	bool writeOutputBytes(std::vector<unsigned char>& outPayload)
	{
		// ENC2 输出布局：
		// [magic=ENC2]
		// [len(1)][reserved(1)][tableCnt(2)][symbolCnt(8)][originalBits(8)][tableBits(8)][encodedBits(8)]
		// [table meta: symbol(2)+codeLen(2)] * tableCnt
		// [table bitstream][data bitstream]
		std::vector<EncTableEntry> table;
		table.reserve(usedSymbols_.size());
		for (size_t iUsed = 0; iUsed < usedSymbols_.size(); ++iUsed)
		{
			int symbol = usedSymbols_[iUsed];
			const std::string& code = codes_[(size_t)symbol];
			if (!code.empty())
			{
				EncTableEntry t;
				t.symbol = (unsigned short)symbol;
				t.code = code;
				table.push_back(t);
			}
		}
		ReportProgress(progressCtx_, 68, ZJH_PROGRESS_STAGE_PACK_OUTPUT, true);

		BitWriter tableWriter;
		for (size_t iTbl = 0; iTbl < table.size(); ++iTbl)
		{
			tableWriter.pushCode(table[iTbl].code);
			if (((iTbl & 0x3F) == 0) || (iTbl + 1 == table.size()))
			{
				ReportStageProgress(progressCtx_,
					ZJH_PROGRESS_STAGE_PACK_OUTPUT,
					(unsigned __int64)(iTbl + 1),
					(unsigned __int64)(table.empty() ? 1 : table.size()),
					68,
					74,
					(iTbl + 1 == table.size()));
			}
		}
		unsigned __int64 tableBits = tableWriter.bitCount;
		tableWriter.flush();

		BitWriter dataWriter;
		for (size_t iData = 0; iData < inputSymbols_.size(); ++iData)
		{
			int symbol = inputSymbols_[iData];
			const std::string& code = codes_[(size_t)symbol];
			if (code.empty())
				return false;
			dataWriter.pushCode(code);
			if (((iData & 0x3FF) == 0) || (iData + 1 == inputSymbols_.size()))
			{
				ReportStageProgress(progressCtx_,
					ZJH_PROGRESS_STAGE_PACK_OUTPUT,
					(unsigned __int64)(iData + 1),
					(unsigned __int64)(inputSymbols_.empty() ? 1 : inputSymbols_.size()),
					74,
					85,
					(iData + 1 == inputSymbols_.size()));
			}
		}
		unsigned __int64 encodedBits = dataWriter.bitCount;
		dataWriter.flush();

		outPayload.clear();
		outPayload.reserve(64 + tableWriter.bytes.size() + dataWriter.bytes.size() + table.size() * 4);
		outPayload.push_back((unsigned char)MAGIC_ENC2[0]);
		outPayload.push_back((unsigned char)MAGIC_ENC2[1]);
		outPayload.push_back((unsigned char)MAGIC_ENC2[2]);
		outPayload.push_back((unsigned char)MAGIC_ENC2[3]);

		unsigned char lenU8 = (unsigned char)len_;
		unsigned char reserved = 0;
		unsigned short tableCnt = (unsigned short)table.size();
		unsigned __int64 symbolCnt = (unsigned __int64)inputSymbols_.size();

		AppendPod(outPayload, lenU8);
		AppendPod(outPayload, reserved);
		AppendPod(outPayload, tableCnt);
		AppendPod(outPayload, symbolCnt);
		AppendPod(outPayload, originalBits_);
		AppendPod(outPayload, tableBits);
		AppendPod(outPayload, encodedBits);

		for (size_t iMeta = 0; iMeta < table.size(); ++iMeta)
		{
			unsigned short codeLen = (unsigned short)table[iMeta].code.size();
			AppendPod(outPayload, table[iMeta].symbol);
			AppendPod(outPayload, codeLen);
		}

		if (!tableWriter.bytes.empty())
			outPayload.insert(outPayload.end(), tableWriter.bytes.begin(), tableWriter.bytes.end());
		if (!dataWriter.bytes.empty())
			outPayload.insert(outPayload.end(), dataWriter.bytes.begin(), dataWriter.bytes.end());

		ReportProgress(progressCtx_, 85, ZJH_PROGRESS_STAGE_PACK_OUTPUT, true);

		return true;
	}
};

struct DecTableEntry
{
	// 解码侧码表项（与 EncTableEntry 含义一致）。
	unsigned short symbol;
	std::string code;
};

struct DecodeOutput
{
	// AC 终止输出：在当前位置可匹配到的“码字长度 + 对应符号”。
	unsigned short length;
	unsigned short symbol;
};

struct DecodeACNode
{
	// 解码 AC 自动机节点。
	int next0;
	int next1;
	int fail;
	std::vector<DecodeOutput> outs;

	DecodeACNode()
		: next0(-1), next1(-1), fail(0)
	{
	}
};

static bool AddDecodeOutputUnique(std::vector<DecodeOutput>& outs, unsigned short length, unsigned short symbol)
{
	// 同一节点可能因为 fail 链合并产生重复输出，这里去重。
	for (size_t i = 0; i < outs.size(); ++i)
	{
		if (outs[i].length == length && outs[i].symbol == symbol)
			return true;
	}
	DecodeOutput o;
	o.length = length;
	o.symbol = symbol;
	outs.push_back(o);
	return true;
}

static bool BuildDecodeAutomaton(const std::vector<DecTableEntry>& table,
	std::vector<DecodeACNode>& nodes)
{
	// 基于码表构建 AC 自动机，供“位串动态规划分词”使用。
	nodes.clear();
	nodes.push_back(DecodeACNode());

	for (size_t i = 0; i < table.size(); ++i)
	{
		const std::string& code = table[i].code;
		if (code.empty())
			return false;

		int node = 0;
		for (size_t j = 0; j < code.size(); ++j)
		{
			int bit = (code[j] == '1') ? 1 : 0;
			int nextRef = (bit == 0) ? nodes[(size_t)node].next0 : nodes[(size_t)node].next1;
			if (nextRef == -1)
			{
				nextRef = (int)nodes.size();
				if (bit == 0)
					nodes[(size_t)node].next0 = nextRef;
				else
					nodes[(size_t)node].next1 = nextRef;
				nodes.push_back(DecodeACNode());
			}
			node = nextRef;
		}
		if (!AddDecodeOutputUnique(nodes[(size_t)node].outs,
			(unsigned short)code.size(),
			table[i].symbol))
			return false;
	}

	std::queue<int> q;
	for (int bit = 0; bit < 2; ++bit)
	{
		int child = (bit == 0) ? nodes[0].next0 : nodes[0].next1;
		if (child == -1)
		{
			if (bit == 0)
				nodes[0].next0 = 0;
			else
				nodes[0].next1 = 0;
			continue;
		}
		nodes[(size_t)child].fail = 0;
		q.push(child);
	}

	while (!q.empty())
	{
		int node = q.front();
		q.pop();
		for (int bit = 0; bit < 2; ++bit)
		{
			int child = (bit == 0) ? nodes[(size_t)node].next0 : nodes[(size_t)node].next1;
			if (child == -1)
			{
				int failState = nodes[(size_t)node].fail;
				int jump = (bit == 0) ? nodes[(size_t)failState].next0 : nodes[(size_t)failState].next1;
				if (bit == 0)
					nodes[(size_t)node].next0 = jump;
				else
					nodes[(size_t)node].next1 = jump;
				continue;
			}

			int failState = nodes[(size_t)node].fail;
			int nextFail = (bit == 0) ? nodes[(size_t)failState].next0 : nodes[(size_t)failState].next1;
			nodes[(size_t)child].fail = nextFail;

			const std::vector<DecodeOutput>& failOuts = nodes[(size_t)nextFail].outs;
			for (size_t oi = 0; oi < failOuts.size(); ++oi)
			{
				if (!AddDecodeOutputUnique(nodes[(size_t)child].outs,
					failOuts[oi].length,
					failOuts[oi].symbol))
					return false;
			}
			q.push(child);
		}
	}

	return true;
}

static bool DecodeSymbolsFromBits(const std::vector<DecTableEntry>& table,
	const unsigned char* bitsData,
	size_t bitsDataBytes,
	unsigned __int64 encodedBits,
	unsigned __int64 expectedSymbolCount,
	std::vector<unsigned short>& outSymbols)
{
	// 关键解码算法（论文重点）：
	// 1) 先用 AC 自动机在位串上做线性扫描；
	// 2) 对每个前缀位置做 DP 计数（0/1/2+），并记录唯一前驱；
	// 3) 仅当 ways[m] == 1 时回溯重建符号序列。
	// 该算法能够正确处理“可重前缀码”，避免贪心匹配误解码。
	outSymbols.clear();

	if (encodedBits == 0)
		return expectedSymbolCount == 0;
	if (bitsData == NULL || bitsDataBytes == 0)
		return false;

	size_t bitCount = (size_t)encodedBits;
	if ((unsigned __int64)bitCount != encodedBits)
		return false;
	if (bitCount >= 0x7FFFFFF0)
		return false;

	std::vector<DecodeACNode> acNodes;
	if (!BuildDecodeAutomaton(table, acNodes))
		return false;

	std::vector<unsigned char> ways(bitCount + 1, 0);
	std::vector<unsigned char> hasPrev(bitCount + 1, 0);
	std::vector<unsigned long> prevPos(bitCount + 1, 0);
	std::vector<unsigned short> prevSymbol(bitCount + 1, 0);

	ways[0] = 1;
	int state = 0;

	for (size_t i = 0; i < bitCount; ++i)
	{
		// AC 状态转移到当前位置后，枚举所有可结束码字，更新 DP。
		int bit = GetBitFromBytes(bitsData, (unsigned __int64)i);
		state = (bit == 0) ? acNodes[(size_t)state].next0 : acNodes[(size_t)state].next1;

		size_t pos = i + 1;
		unsigned char waysHere = 0;
		unsigned char uniqueCount = 0;
		unsigned long uniquePrevPos = 0;
		unsigned short uniquePrevSym = 0;

		const std::vector<DecodeOutput>& outs = acNodes[(size_t)state].outs;
		for (size_t oi = 0; oi < outs.size(); ++oi)
		{
			unsigned short length = outs[oi].length;
			if (length == 0 || (size_t)length > pos)
				continue;
			size_t fromPos = pos - (size_t)length;
			unsigned char fromWays = ways[fromPos];
			if (fromWays == 0)
				continue;

			int sum = (int)waysHere + (int)fromWays;
			if (sum > 2) sum = 2;
			waysHere = (unsigned char)sum;

			if (fromWays == 1)
			{
				if (uniqueCount == 0)
				{
					uniqueCount = 1;
					uniquePrevPos = (unsigned long)fromPos;
					uniquePrevSym = outs[oi].symbol;
				}
				else if (uniquePrevPos != (unsigned long)fromPos || uniquePrevSym != outs[oi].symbol)
				{
					uniqueCount = 2;
				}
			}
			else
			{
				uniqueCount = 2;
			}
		}

		ways[pos] = waysHere;
		if (waysHere == 1 && uniqueCount == 1)
		{
			hasPrev[pos] = 1;
			prevPos[pos] = uniquePrevPos;
			prevSymbol[pos] = uniquePrevSym;
		}
	}

	if (ways[bitCount] != 1)
		return false;

	// 通过唯一前驱链回溯出符号序列。
	std::vector<unsigned short> reversed;
	reversed.reserve((size_t)expectedSymbolCount > 0 ? (size_t)expectedSymbolCount : 16);

	size_t pos = bitCount;
	while (pos > 0)
	{
		if (!hasPrev[pos])
			return false;
		reversed.push_back(prevSymbol[pos]);
		pos = (size_t)prevPos[pos];
	}

	outSymbols.reserve(reversed.size());
	for (size_t r = reversed.size(); r > 0; --r)
		outSymbols.push_back(reversed[r - 1]);

	if ((unsigned __int64)outSymbols.size() != expectedSymbolCount)
		return false;
	return true;
}

static bool RebuildOriginalBytes(const std::vector<unsigned short>& symbols,
	int len,
	unsigned __int64 originalBits,
	std::vector<unsigned char>& outBytes)
{
	// 将符号序列按每符号 len 位拼回原始字节流，并按 originalBits 截断到真实位数。
	outBytes.clear();

	if (len <= 0 || len > MAX_LEN)
		return false;

	unsigned __int64 expectedBytes = BytesForBits(originalBits);
	outBytes.reserve((size_t)expectedBytes);

	unsigned __int64 producedBits = 0;
	unsigned char current = 0;
	int used = 0;

	for (size_t i = 0; i < symbols.size(); ++i)
	{
		unsigned short symbol = symbols[i];
		for (int b = len - 1; b >= 0; --b)
		{
			if (producedBits >= originalBits)
				break;
			int bit = (symbol >> b) & 1;
			current = (unsigned char)((current << 1) | (bit & 1));
			++used;
			++producedBits;
			if (used == 8)
			{
				outBytes.push_back(current);
				current = 0;
				used = 0;
			}
		}
		if (producedBits >= originalBits)
			break;
	}

	if (producedBits != originalBits)
		return false;

	if (used != 0)
	{
		current = (unsigned char)(current << (8 - used));
		outBytes.push_back(current);
		used = 0;
	}

	if ((unsigned __int64)outBytes.size() != expectedBytes)
		return false;

	return true;
}

static bool DecodeENC2PayloadToFile(const std::vector<unsigned char>& payload, const std::string& outputPath)
{
	// 按 ENC2 协议解析并解码到输出文件。
	if (payload.size() < 4 + 1 + 1 + 2 + 8 + 8 + 8 + 8)
		return false;
	if (memcmp(&payload[0], MAGIC_ENC2, 4) != 0)
		return false;

	size_t offset = 4;
	unsigned char lenU8 = 0;
	unsigned char reserved = 0;
	unsigned short tableCnt = 0;
	unsigned __int64 symbolCnt = 0;
	unsigned __int64 originalBits = 0;
	unsigned __int64 tableBits = 0;
	unsigned __int64 encodedBits = 0;

	if (!ReadPod(payload, offset, lenU8)) return false;
	if (!ReadPod(payload, offset, reserved)) return false;
	if (!ReadPod(payload, offset, tableCnt)) return false;
	if (!ReadPod(payload, offset, symbolCnt)) return false;
	if (!ReadPod(payload, offset, originalBits)) return false;
	if (!ReadPod(payload, offset, tableBits)) return false;
	if (!ReadPod(payload, offset, encodedBits)) return false;

	int len = (int)lenU8;
	if (len <= 0 || len > MAX_LEN)
		return false;

	std::vector<unsigned short> tableSymbols;
	std::vector<unsigned short> tableCodeLens;
	tableSymbols.reserve((size_t)tableCnt);
	tableCodeLens.reserve((size_t)tableCnt);

	for (unsigned short i = 0; i < tableCnt; ++i)
	{
		unsigned short symbol = 0;
		unsigned short codeLen = 0;
		if (!ReadPod(payload, offset, symbol)) return false;
		if (!ReadPod(payload, offset, codeLen)) return false;
		tableSymbols.push_back(symbol);
		tableCodeLens.push_back(codeLen);
	}

	unsigned __int64 tableBytes64 = BytesForBits(tableBits);
	size_t tableBytes = (size_t)tableBytes64;
	if ((unsigned __int64)tableBytes != tableBytes64)
		return false;
	if (offset + tableBytes > payload.size())
		return false;

	const unsigned char* tableBitsData = tableBytes > 0 ? &payload[offset] : NULL;
	offset += tableBytes;

	std::vector<DecTableEntry> table;
	table.reserve((size_t)tableCnt);

	unsigned __int64 tableBitOffset = 0;
	for (unsigned short ti = 0; ti < tableCnt; ++ti)
	{
		// 从 table 位流中按 codeLen 逐位恢复码字字符串。
		unsigned short codeLen = tableCodeLens[(size_t)ti];
		if (tableBitOffset + (unsigned __int64)codeLen > tableBits)
			return false;

		DecTableEntry e;
		e.symbol = tableSymbols[(size_t)ti];
		e.code.reserve((size_t)codeLen);
		for (unsigned short b = 0; b < codeLen; ++b)
		{
			int bit = GetBitFromBytes(tableBitsData, tableBitOffset + (unsigned __int64)b);
			e.code += (bit ? '1' : '0');
		}
		tableBitOffset += (unsigned __int64)codeLen;
		table.push_back(e);
	}
	if (tableBitOffset != tableBits)
		return false;

	unsigned __int64 dataBytes64 = BytesForBits(encodedBits);
	size_t dataBytes = (size_t)dataBytes64;
	if ((unsigned __int64)dataBytes != dataBytes64)
		return false;
	if (offset + dataBytes > payload.size())
		return false;
	const unsigned char* dataBits = dataBytes > 0 ? &payload[offset] : NULL;

	std::vector<unsigned short> symbols;
	if (!DecodeSymbolsFromBits(table, dataBits, dataBytes, encodedBits, symbolCnt, symbols))
		return false;

	std::vector<unsigned char> decodedBytes;
	if (!RebuildOriginalBytes(symbols, len, originalBits, decodedBytes))
		return false;

	return WriteAllBytes(outputPath, decodedBytes);
}

static bool DecodeENC1PayloadToFile(const std::vector<unsigned char>& payload, const std::string& outputPath)
{
	// 按 ENC1 协议解析并解码到输出文件（兼容旧编码版本）。
	if (payload.size() < 4 + 1 + 1 + 2 + 8 + 8 + 8)
		return false;
	if (memcmp(&payload[0], MAGIC_ENC1, 4) != 0)
		return false;

	size_t offset = 4;
	unsigned char lenU8 = 0;
	unsigned char reserved = 0;
	unsigned short tableCnt = 0;
	unsigned __int64 symbolCnt = 0;
	unsigned __int64 originalBits = 0;
	unsigned __int64 encodedBits = 0;

	if (!ReadPod(payload, offset, lenU8)) return false;
	if (!ReadPod(payload, offset, reserved)) return false;
	if (!ReadPod(payload, offset, tableCnt)) return false;
	if (!ReadPod(payload, offset, symbolCnt)) return false;
	if (!ReadPod(payload, offset, originalBits)) return false;
	if (!ReadPod(payload, offset, encodedBits)) return false;

	int len = (int)lenU8;
	if (len <= 0 || len > MAX_LEN)
		return false;

	std::vector<DecTableEntry> table;
	table.reserve((size_t)tableCnt);

	for (unsigned short i = 0; i < tableCnt; ++i)
	{
		unsigned short symbol = 0;
		unsigned short codeLen = 0;
		if (!ReadPod(payload, offset, symbol)) return false;
		if (!ReadPod(payload, offset, codeLen)) return false;

		unsigned __int64 codeBytes64 = BytesForBits((unsigned __int64)codeLen);
		size_t codeBytes = (size_t)codeBytes64;
		if ((unsigned __int64)codeBytes != codeBytes64)
			return false;
		if (offset + codeBytes > payload.size())
			return false;

		DecTableEntry e;
		e.symbol = symbol;
		e.code.reserve((size_t)codeLen);
		for (unsigned short b = 0; b < codeLen; ++b)
		{
			int bit = GetBitFromBytes(&payload[offset], (unsigned __int64)b);
			e.code += (bit ? '1' : '0');
		}
		table.push_back(e);
		offset += codeBytes;
	}

	unsigned __int64 dataBytes64 = BytesForBits(encodedBits);
	size_t dataBytes = (size_t)dataBytes64;
	if ((unsigned __int64)dataBytes != dataBytes64)
		return false;
	if (offset + dataBytes > payload.size())
		return false;
	const unsigned char* dataBits = dataBytes > 0 ? &payload[offset] : NULL;

	std::vector<unsigned short> symbols;
	if (!DecodeSymbolsFromBits(table, dataBits, dataBytes, encodedBits, symbolCnt, symbols))
		return false;

	std::vector<unsigned char> decodedBytes;
	if (!RebuildOriginalBytes(symbols, len, originalBits, decodedBytes))
		return false;

	return WriteAllBytes(outputPath, decodedBytes);
}

static bool DecodeLegacyPayloadToFile(const std::vector<unsigned char>& payload,
	const std::string& outputPath,
	const char* password)
{
	// 历史 ZJH1 容器兼容解码：
	// 结构为 [LegacyHeader][payload]，payload 可能做过口令异或混淆。
	if (payload.size() < sizeof(LegacyHeader))
		return false;

	LegacyHeader header;
	memcpy(&header, &payload[0], sizeof(header));
	if (memcmp(header.magic, MAGIC_LEGACY, 4) != 0)
		return false;

	size_t dataOffset = sizeof(LegacyHeader);
	if ((size_t)header.payloadLen > payload.size() - dataOffset)
		return false;

	std::vector<unsigned char> raw;
	raw.assign(payload.begin() + dataOffset,
		payload.begin() + dataOffset + (size_t)header.payloadLen);

	if (password != NULL && password[0] != '\0')
		XorCryptPayload(raw, password, NULL, ZJH_PROGRESS_STAGE_ENCRYPT_PAYLOAD, 0, 0);

	if (header.originalLen < raw.size())
		raw.resize((size_t)header.originalLen);

	return WriteAllBytes(outputPath, raw);
}

static bool EncodeEnc2Payload(const std::string& path,
	int len,
	std::vector<unsigned char>& payload,
	ProgressContext* progressCtx)
{
	// 仅生成 ENC2 负载（不含密码尾与扩展名处理）。
	if (len <= 0 || len > MAX_LEN)
		return false;

	EncoderWorker worker;
	return worker.run(path, len, payload, progressCtx);
}

static bool EncodeToFile(const std::string& path,
	int len,
	const char* password,
	ZJH_ProgressCallback progressCallback,
	void* userData)
{
	// 压缩总入口：
	// 1) 生成 ENC2 负载；
	// 2) 若启用密码则混淆负载；
	// 3) 追加密码尾标记并写成 .zjh 文件。
	ProgressContext progressCtx(progressCallback, userData);
	std::vector<unsigned char> payload;
	if (!EncodeEnc2Payload(path, len, payload, &progressCtx))
		return false;

	bool hasPassword = (password != NULL && password[0] != '\0');
	if (hasPassword)
	{
		XorCryptPayload(payload,
			password,
			&progressCtx,
			ZJH_PROGRESS_STAGE_ENCRYPT_PAYLOAD,
			86,
			92);
	}
	else
	{
		ReportProgress(&progressCtx, 92, ZJH_PROGRESS_STAGE_ENCRYPT_PAYLOAD, true);
	}

	std::vector<unsigned char> fileData = payload;
	if (hasPassword)
	{
		unsigned char flag = 1;
		unsigned long hash = PasswordHash32(password);
		for (int ti = 0; ti < TAIL_MAGIC_LEN; ++ti)
			fileData.push_back((unsigned char)TAIL_MAGIC[ti]);
		fileData.push_back(flag);
		size_t oldSize = fileData.size();
		fileData.resize(oldSize + 4);
		memcpy(&fileData[oldSize], &hash, 4);
	}

	if (!WriteAllBytesWithProgress(path + ".zjh",
		fileData,
		&progressCtx,
		ZJH_PROGRESS_STAGE_WRITE_FILE,
		92,
		99))
	{
		return false;
	}

	ReportProgress(&progressCtx, 100, ZJH_PROGRESS_STAGE_DONE, true);
	return true;
}

static bool DecodeToFile(const std::string& path,
	const std::string& outputPath,
	const char* password)
{
	// 解压总入口：
	// 1) 解析密码尾标记；
	// 2) 校验口令哈希（如有密码）；
	// 3) 自动分流到 ENC2 / ENC1 / Legacy 解码器。
	std::vector<unsigned char> fileData;
	if (!ReadAllBytes(path, fileData))
		return false;

	bool hasPassword = false;
	unsigned long storedHash = 0;
	size_t payloadSize = fileData.size();
	if (!ParseTailInfo(fileData, &hasPassword, &storedHash, &payloadSize))
		return false;
	if (payloadSize == 0)
		return false;

	std::vector<unsigned char> payload;
	payload.assign(fileData.begin(), fileData.begin() + payloadSize);

	bool isLegacy = false;
	if (payload.size() >= 4 && memcmp(&payload[0], MAGIC_LEGACY, 4) == 0)
		isLegacy = true;

	if (isLegacy)
	{
		if (hasPassword)
		{
			if (password == NULL || password[0] == '\0')
				return false;
			if (PasswordHash32(password) != storedHash)
				return false;
		}
		return DecodeLegacyPayloadToFile(payload, outputPath, hasPassword ? password : NULL);
	}

	if (hasPassword)
	{
		if (password == NULL || password[0] == '\0')
			return false;
		if (PasswordHash32(password) != storedHash)
			return false;
		XorCryptPayload(payload, password, NULL, ZJH_PROGRESS_STAGE_ENCRYPT_PAYLOAD, 0, 0);
	}

	if (payload.size() >= 4 && memcmp(&payload[0], MAGIC_ENC2, 4) == 0)
		return DecodeENC2PayloadToFile(payload, outputPath);
	if (payload.size() >= 4 && memcmp(&payload[0], MAGIC_ENC1, 4) == 0)
		return DecodeENC1PayloadToFile(payload, outputPath);
	if (payload.size() >= 4 && memcmp(&payload[0], MAGIC_LEGACY, 4) == 0)
		return DecodeLegacyPayloadToFile(payload, outputPath, NULL);

	return false;
}

} // namespace

bool ZJH_encrypto::encrypto(const std::string& path,
	int len,
	ZJH_ProgressCallback progressCallback,
	void* userData)
{
	// 无密码压缩接口（默认入口）。
	return EncodeToFile(path, len, NULL, progressCallback, userData);
}

bool ZJH_encrypto1::encrypto(const std::string& path,
	int len,
	const char* password,
	ZJH_ProgressCallback progressCallback,
	void* userData)
{
	// 带密码压缩接口（与界面“设置密码”选项对应）。
	return EncodeToFile(path, len, password, progressCallback, userData);
}

bool ZJH_encrypto2::encrypto(const std::string& path,
	int len,
	ZJH_ProgressCallback progressCallback,
	void* userData)
{
	// 兼容保留接口：当前行为与 ZJH_encrypto::encrypto 一致。
	return EncodeToFile(path, len, NULL, progressCallback, userData);
}

bool ZJH_decrypto::decrypto(const std::string& path, const char* password)
{
	// 默认解压：输出路径为“去掉 .zjh 后缀”的同名文件。
	if (!HasSuffixIgnoreCase(path, ".zjh"))
		return false;

	std::string outPath = path.substr(0, path.size() - 4);
	return DecodeToFile(path, outPath, password);
}

bool ZJH_decrypto::decrypto_to(const std::string& path, const std::string& output_path, const char* password)
{
	// 指定输出路径的解压接口，供“测试解压到临时文件”等场景使用。
	return DecodeToFile(path, output_path, password);
}

bool ZJH_GetPackageInfo(const std::string& path, bool* hasPassword)
{
	// 包信息探测接口：
	// - 输出是否存在密码尾标记；
	// - 在可判断的情况下验证负载魔数（ENC1/ENC2/Legacy）；
	// - 若文件已加密，负载头会被混淆，此时仅返回“存在密码”信息。
	if (hasPassword)
		*hasPassword = false;

	std::vector<unsigned char> fileData;
	if (!ReadAllBytes(path, fileData))
		return false;

	bool hasPwd = false;
	size_t payloadSize = fileData.size();
	if (!ParseTailInfo(fileData, &hasPwd, NULL, &payloadSize))
		return false;

	if (hasPassword)
		*hasPassword = hasPwd;

	if (payloadSize < 4)
		return false;

	const unsigned char* payload = &fileData[0];
	if (memcmp(payload, MAGIC_ENC2, 4) == 0)
		return true;
	if (memcmp(payload, MAGIC_ENC1, 4) == 0)
		return true;
	if (memcmp(payload, MAGIC_LEGACY, 4) == 0)
		return true;

	if (hasPwd)
		return true;

	return false;
}

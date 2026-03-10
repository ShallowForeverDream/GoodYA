// Huffman.cpp : 哈夫曼压缩/解压实现文件。
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Huffman.h"
#include <iostream>
#include <fstream>
#include <string>
#include <memory.h>

using namespace std;

// 文件尾部标记：用于记录“是否加密 + 原文 CRC32 + 密码 MD5”
static const char kTailMagic[] = "GYMD5!";
static const int kTailMagicLen = 6;
static const int kMd5HexLen = 32;
static const int kTailSize = kTailMagicLen + 1 + 4 + kMd5HexLen;

// 功能：向目标字符串末尾追加指定长度的原始字节。
static void AppendBytes(string& buf, const void* data, int len)
{
	// 当输入指针为空或长度非法时直接返回，避免越界追加。
	if (data == NULL || len <= 0)
		return;
	buf.append((const char*)data, len);
}

// 功能：从字节流读取固定长度数据，并同步推进读取指针。
static BOOL ReadBytes(const unsigned char*& p, int& remain, void* out, int len)
{
	// 先做剩余长度检查，再拷贝并更新指针与剩余计数。
	if (len < 0 || remain < len)
		return FALSE;
	if (len > 0)
	{
		memcpy(out, p, len);
		p += len;
		remain -= len;
	}
	return TRUE;
}

// 计算缓冲区 CRC32（多项式 0xEDB88320）
// 功能：计算内存缓冲区的 CRC32 校验值。
static unsigned long CalcCRC32Buffer(const unsigned char* data, int len)
{
	// 首次调用时初始化 CRC32 查表，随后按字节滚动计算。
	static BOOL s_inited = FALSE;
	static unsigned long s_table[256];
	if (!s_inited)
	{
		int i, j;
		for (i = 0; i < 256; ++i)
		{
			unsigned long crc = (unsigned long)i;
			for (j = 0; j < 8; ++j)
			{
				if ((crc & 1) != 0)
					crc = (crc >> 1) ^ 0xEDB88320UL;
				else
					crc >>= 1;
			}
			s_table[i] = crc;
		}
		s_inited = TRUE;
	}

	unsigned long crc32 = 0xFFFFFFFFUL;
	int k;
	for (k = 0; k < len; ++k)
	{
		crc32 = s_table[(crc32 ^ data[k]) & 0xFF] ^ (crc32 >> 8);
	}
	return crc32 ^ 0xFFFFFFFFUL;
}

// ------------------------- MD5 实现（RFC1321） -------------------------
typedef struct
{
	unsigned int state[4];
	unsigned int count[2];
	unsigned char buffer[64];
} MD5_CTX;

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

inline static unsigned int F(unsigned int x, unsigned int y, unsigned int z) { return (x & y) | (~x & z); }
inline static unsigned int G(unsigned int x, unsigned int y, unsigned int z) { return (x & z) | (y & ~z); }
inline static unsigned int H(unsigned int x, unsigned int y, unsigned int z) { return x ^ y ^ z; }
inline static unsigned int I(unsigned int x, unsigned int y, unsigned int z) { return y ^ (x | ~z); }
inline static unsigned int ROTATE_LEFT(unsigned int x, int n) { return (x << n) | (x >> (32 - n)); }

// 功能：执行 MD5 第 1 轮的单步变换。
inline static void FF(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	// 按 RFC1321 规则更新寄存器 a。
	a += F(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}
// 功能：执行 MD5 第 2 轮的单步变换。
inline static void GG(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	// 按 RFC1321 规则更新寄存器 a。
	a += G(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}
// 功能：执行 MD5 第 3 轮的单步变换。
inline static void HH(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	// 按 RFC1321 规则更新寄存器 a。
	a += H(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}
// 功能：执行 MD5 第 4 轮的单步变换。
inline static void II(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	// 按 RFC1321 规则更新寄存器 a。
	a += I(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}

// 功能：把 32 位整数数组按小端序编码为字节数组。
static void MD5Encode(unsigned char* output, const unsigned int* input, unsigned int len)
{
	// 每 4 字节拆分一个 32 位字，供 MD5 输出阶段使用。
	unsigned int i, j;
	for (i = 0, j = 0; j < len; ++i, j += 4)
	{
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

// 功能：把字节数组按小端序解码为 32 位整数数组。
static void MD5Decode(unsigned int* output, const unsigned char* input, unsigned int len)
{
	// 每 4 字节合并一个 32 位字，供 MD5 变换阶段使用。
	unsigned int i, j;
	for (i = 0, j = 0; j < len; ++i, j += 4)
	{
		output[i] = ((unsigned int)input[j]) | (((unsigned int)input[j + 1]) << 8) |
			(((unsigned int)input[j + 2]) << 16) | (((unsigned int)input[j + 3]) << 24);
	}
}

// 功能：执行一次 64 字节分组的 MD5 核心压缩变换。
static void MD5Transform(unsigned int state[4], const unsigned char block[64])
{
	// 依次完成四轮 64 步运算并回写 state。
	unsigned int a = state[0], b = state[1], c = state[2], d = state[3];
	unsigned int x[16];
	MD5Decode(x, block, 64);

	FF(a, b, c, d, x[0], S11, 0xd76aa478);
	FF(d, a, b, c, x[1], S12, 0xe8c7b756);
	FF(c, d, a, b, x[2], S13, 0x242070db);
	FF(b, c, d, a, x[3], S14, 0xc1bdceee);
	FF(a, b, c, d, x[4], S11, 0xf57c0faf);
	FF(d, a, b, c, x[5], S12, 0x4787c62a);
	FF(c, d, a, b, x[6], S13, 0xa8304613);
	FF(b, c, d, a, x[7], S14, 0xfd469501);
	FF(a, b, c, d, x[8], S11, 0x698098d8);
	FF(d, a, b, c, x[9], S12, 0x8b44f7af);
	FF(c, d, a, b, x[10], S13, 0xffff5bb1);
	FF(b, c, d, a, x[11], S14, 0x895cd7be);
	FF(a, b, c, d, x[12], S11, 0x6b901122);
	FF(d, a, b, c, x[13], S12, 0xfd987193);
	FF(c, d, a, b, x[14], S13, 0xa679438e);
	FF(b, c, d, a, x[15], S14, 0x49b40821);

	GG(a, b, c, d, x[1], S21, 0xf61e2562);
	GG(d, a, b, c, x[6], S22, 0xc040b340);
	GG(c, d, a, b, x[11], S23, 0x265e5a51);
	GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
	GG(a, b, c, d, x[5], S21, 0xd62f105d);
	GG(d, a, b, c, x[10], S22, 0x02441453);
	GG(c, d, a, b, x[15], S23, 0xd8a1e681);
	GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
	GG(a, b, c, d, x[9], S21, 0x21e1cde6);
	GG(d, a, b, c, x[14], S22, 0xc33707d6);
	GG(c, d, a, b, x[3], S23, 0xf4d50d87);
	GG(b, c, d, a, x[8], S24, 0x455a14ed);
	GG(a, b, c, d, x[13], S21, 0xa9e3e905);
	GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
	GG(c, d, a, b, x[7], S23, 0x676f02d9);
	GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

	HH(a, b, c, d, x[5], S31, 0xfffa3942);
	HH(d, a, b, c, x[8], S32, 0x8771f681);
	HH(c, d, a, b, x[11], S33, 0x6d9d6122);
	HH(b, c, d, a, x[14], S34, 0xfde5380c);
	HH(a, b, c, d, x[1], S31, 0xa4beea44);
	HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
	HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
	HH(b, c, d, a, x[10], S34, 0xbebfbc70);
	HH(a, b, c, d, x[13], S31, 0x289b7ec6);
	HH(d, a, b, c, x[0], S32, 0xeaa127fa);
	HH(c, d, a, b, x[3], S33, 0xd4ef3085);
	HH(b, c, d, a, x[6], S34, 0x04881d05);
	HH(a, b, c, d, x[9], S31, 0xd9d4d039);
	HH(d, a, b, c, x[12], S32, 0xe6db99e5);
	HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
	HH(b, c, d, a, x[2], S34, 0xc4ac5665);

	II(a, b, c, d, x[0], S41, 0xf4292244);
	II(d, a, b, c, x[7], S42, 0x432aff97);
	II(c, d, a, b, x[14], S43, 0xab9423a7);
	II(b, c, d, a, x[5], S44, 0xfc93a039);
	II(a, b, c, d, x[12], S41, 0x655b59c3);
	II(d, a, b, c, x[3], S42, 0x8f0ccc92);
	II(c, d, a, b, x[10], S43, 0xffeff47d);
	II(b, c, d, a, x[1], S44, 0x85845dd1);
	II(a, b, c, d, x[8], S41, 0x6fa87e4f);
	II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
	II(c, d, a, b, x[6], S43, 0xa3014314);
	II(b, c, d, a, x[13], S44, 0x4e0811a1);
	II(a, b, c, d, x[4], S41, 0xf7537e82);
	II(d, a, b, c, x[11], S42, 0xbd3af235);
	II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
	II(b, c, d, a, x[9], S44, 0xeb86d391);

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	memset((unsigned char*)x, 0, sizeof(x));
}

// 功能：初始化 MD5 上下文状态与计数器。
static void MD5Init(MD5_CTX* context)
{
	// 写入 RFC1321 规定的初始常量。
	context->count[0] = context->count[1] = 0;
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

// 功能：增量喂入数据到 MD5 上下文。
static void MD5Update(MD5_CTX* context, const unsigned char* input, unsigned int inputLen)
{
	// 按 64 字节分组处理并缓存不足一块的尾部数据。
	unsigned int i, index, partLen;
	index = (unsigned int)((context->count[0] >> 3) & 0x3F);

	if ((context->count[0] += (inputLen << 3)) < (inputLen << 3))
		context->count[1]++;
	context->count[1] += (inputLen >> 29);

	partLen = 64 - index;

	if (inputLen >= partLen)
	{
		memcpy((unsigned char*)&context->buffer[index], (unsigned char*)input, partLen);
		MD5Transform(context->state, context->buffer);
		for (i = partLen; i + 63 < inputLen; i += 64)
			MD5Transform(context->state, &input[i]);
		index = 0;
	}
	else
		i = 0;

	memcpy((unsigned char*)&context->buffer[index], (unsigned char*)&input[i], inputLen - i);
}

// 功能：完成 MD5 填充并输出最终 16 字节摘要。
static void MD5Final(unsigned char digest[16], MD5_CTX* context)
{
	// 补齐长度信息后进行最后一轮变换并导出 digest。
	static unsigned char PADDING[64] = { 0x80 };
	unsigned char bits[8];
	unsigned int index, padLen;

	MD5Encode(bits, context->count, 8);
	index = (unsigned int)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	MD5Update(context, PADDING, padLen);
	MD5Update(context, bits, 8);
	MD5Encode(digest, context->state, 16);

	memset((unsigned char*)context, 0, sizeof(*context));
}

// 计算口令 MD5（32位十六进制字符串）
// 功能：计算字符串的 MD5，并输出 32 位十六进制文本。
static BOOL CalcMD5Hex(const char* text, char outHex[33])
{
	// 先算 16 字节摘要，再转成可持久化比较的 hex 字符串。
	if (outHex == NULL)
		return FALSE;

	const char* p = (text != NULL) ? text : "";
	MD5_CTX ctx;
	unsigned char digest[16];
	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char*)p, (unsigned int)strlen(p));
	MD5Final(digest, &ctx);

	int i;
	for (i = 0; i < 16; ++i)
		sprintf(outHex + i * 2, "%02x", (unsigned int)digest[i]);
	outHex[32] = '\0';
	return TRUE;
}

// 使用口令对负载做异或流加/解密（同一函数可双向）
// 功能：按密码做简单异或加解密（同算法可逆）。
static void XorCryptPayload(unsigned char* data, int len, const char* password)
{
	// 循环使用密码字节与数据逐字节异或。
	if (data == NULL || len <= 0 || password == NULL || password[0] == '\0')
		return;

	int passLen = (int)strlen(password);
	unsigned long seed = 0x9E3779B9UL;
	int i;
	for (i = 0; i < len; ++i)
	{
		seed = seed * 1664525UL + 1013904223UL + (unsigned char)password[i % passLen];
		unsigned char key = (unsigned char)(((seed >> 24) & 0xFF) ^ (unsigned char)password[(i * 7 + 3) % passLen] ^ (i & 0xFF));
		data[i] ^= key;
	}
}

//////////////////////////////////////////////////////////////////////
// 构造与析构
//////////////////////////////////////////////////////////////////////

// 功能：构造 Huffman 对象并初始化成员状态。
Huffman::Huffman()
{
	huffTree = NULL;
	chars = NULL;
	n = 0;
	m_hasPassword = FALSE;
	m_storedCrc32 = 0;
	m_progressCallback = NULL;
	m_progressUserData = NULL;
	m_progressStartTick = 0;
	m_lastProgressPercent = -1;
	m_lastProgressStage = -1;
	codeBitLength = 0;
}

// 功能：析构 Huffman 对象并释放动态资源。
Huffman::~Huffman()
{
	ResetData();
}

// 功能：设置 Huffman 压缩进度回调函数与上下文。
void Huffman::SetProgressCallback(HUF_ProgressCallback callback, void* userData)
{
	// 仅更新回调配置，不立即触发上报；真正开始由 BeginProgressSession 控制。
	m_progressCallback = callback;
	m_progressUserData = userData;
}

// 功能：启动一次新的压缩进度会话。
void Huffman::BeginProgressSession()
{
	// 重置时间基线与去抖状态，保证每次压缩都从 0% 开始。
	m_progressStartTick = ::GetTickCount();
	m_lastProgressPercent = -1;
	m_lastProgressStage = -1;
	codeBitLength = 0;
	ReportProgress(0, HUF_PROGRESS_STAGE_PREPARE, TRUE);
}

// 功能：计算自会话开始以来的已耗时毫秒。
unsigned long Huffman::CalcElapsedMs() const
{
	if (m_progressStartTick == 0)
		return 0;
	DWORD now = ::GetTickCount();
	return (unsigned long)(now - m_progressStartTick);
}

// 功能：按线性外推估算剩余时间。
unsigned long Huffman::CalcEtaMs(int percent, unsigned long elapsedMs) const
{
	if (percent <= 0 || percent >= 100)
		return 0;
	unsigned __int64 remain = (unsigned __int64)elapsedMs * (unsigned __int64)(100 - percent);
	return (unsigned long)(remain / (unsigned __int64)percent);
}

// 功能：上报一个总进度点（含阶段、耗时和 ETA）。
void Huffman::ReportProgress(int percent, int stage, BOOL force)
{
	if (m_progressCallback == NULL)
		return;

	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;
	if (!force && m_lastProgressPercent == percent && m_lastProgressStage == stage)
		return;

	unsigned long elapsedMs = CalcElapsedMs();
	unsigned long etaMs = CalcEtaMs(percent, elapsedMs);
	m_progressCallback(percent, stage, elapsedMs, etaMs, m_progressUserData);
	m_lastProgressPercent = percent;
	m_lastProgressStage = stage;
}

// 功能：把阶段内完成度映射到总进度区间并上报。
void Huffman::ReportStageProgress(int stage,
	unsigned __int64 done,
	unsigned __int64 total,
	int startPercent,
	int endPercent,
	BOOL force)
{
	if (m_progressCallback == NULL)
		return;

	if (total == 0)
	{
		ReportProgress(endPercent, stage, force);
		return;
	}

	if (done > total)
		done = total;

	unsigned __int64 width = (unsigned __int64)(endPercent - startPercent);
	int percent = startPercent + (int)((done * width) / total);
	ReportProgress(percent, stage, force);
}

// 功能：重置内部数据结构，避免重复压缩/解压时残留状态。
void Huffman::ResetData()
{
	// 释放 Huffman 树与编码表，并清空文本/编码缓存。
	if (huffTree != NULL)
	{
		delete[] huffTree;
		huffTree = NULL;
	}

	if (chars != NULL)
	{
		int i;
		for (i = 0; i <= n; ++i)
		{
			if (chars[i].code != NULL)
			{
				delete[] chars[i].code;
				chars[i].code = NULL;
			}
		}
		delete[] chars;
		chars = NULL;
	}

	n = 0;
	text = "";
	codeBitsPacked = "";
	codeBitLength = 0;
	m_hasPassword = FALSE;
	m_storedCrc32 = 0;
}

// 功能：根据字符编码表把原文 text 编码为 code。
void Huffman::Encode()
{
	// 按字符逐个查表并直接写入按位打包缓冲，避免构造超大 "0/1" 字符串导致内存爆炸。
	codeBitsPacked = "";
	codeBitLength = 0;
	if (n <= 0 || chars == NULL)
	{
		ReportProgress(85, HUF_PROGRESS_STAGE_ENCODE_DATA, TRUE);
		return;
	}

	ReportProgress(69, HUF_PROGRESS_STAGE_ENCODE_DATA, TRUE);

	const char* codeLookup[256];
	int codeLenLookup[256];
	int idx;
	for (idx = 0; idx < 256; ++idx)
	{
		codeLookup[idx] = NULL;
		codeLenLookup[idx] = 0;
	}

	for (idx = 1; idx <= n; ++idx)
	{
		if (chars[idx].code != NULL)
		{
			unsigned char ch = (unsigned char)chars[idx].c;
			codeLookup[ch] = chars[idx].code;
			codeLenLookup[ch] = (int)strlen(chars[idx].code);
		}
	}

	if (!text.empty())
		codeBitsPacked.reserve(text.size());

	unsigned char current = 0;
	int used = 0;
	string::size_type i;
	for (i = 0; i != text.size(); ++i)
	{
		unsigned char ch = (unsigned char)text[i];
		const char* symCode = codeLookup[ch];
		int symLen = codeLenLookup[ch];
		if (symCode != NULL && symLen > 0)
		{
			int k;
			for (k = 0; k < symLen; ++k)
			{
				current = (unsigned char)(current << 1);
				if (symCode[k] == '1')
					current = (unsigned char)(current + 1);
				++used;
				++codeBitLength;
				if (used == 8)
				{
					codeBitsPacked += (char)current;
					current = 0;
					used = 0;
				}
			}
		}

		if (((i & 0x0FFF) == 0) || (i + 1 == text.size()))
		{
			ReportStageProgress(HUF_PROGRESS_STAGE_ENCODE_DATA,
				(unsigned __int64)(i + 1),
				(unsigned __int64)(text.empty() ? 1 : text.size()),
				69,
				85,
				(i + 1 == text.size()) ? TRUE : FALSE);
		}
	}

	if (used != 0)
	{
		current = (unsigned char)(current << (8 - used));
		codeBitsPacked += (char)current;
	}

	ReportProgress(85, HUF_PROGRESS_STAGE_ENCODE_DATA, TRUE);
}
// 功能：调试输出字符权重表。
void Huffman::PrintCharWeight()
{
	// 逐项打印字符与对应权重。
	if (chars == NULL || n <= 0)
		return;

	CharMapNode cc;
	int p, j;
	for (p = 0; p <= n; ++p)
	{
		for (j = 0; j <= n - 1; ++j)
		{
			if (chars[j].weight > chars[j + 1].weight)
			{
				cc = chars[j];
				chars[j] = chars[j + 1];
				chars[j + 1] = cc;
			}
		}
	}

	cout << " |字符|---|权值|" << endl;
	for (p = 1; p <= n; ++p)
	{
		switch (chars[p].c)
		{
		case '\t':
			cout << "  |" << "\\t" << "|";
			break;
		case '\n':
			cout << "  |" << "\\n" << "|";
			break;
		default:
			cout << "  |" << chars[p].c << "|";
			break;
		}
		cout << "--|" << chars[p].weight << "|" << endl;
	}
}

// 功能：调试输出字符到编码的映射结果。
void Huffman::PrintCharCode()
{
	// 逐项打印字符及其 Huffman 编码。
	if (chars == NULL || n <= 0)
		return;

	cout << " |字符|---|哈夫曼编码|" << endl;
	int i;
	for (i = 1; i <= n; ++i)
	{
		switch (chars[i].c)
		{
		case '\t':
			cout << "  | " << "\\t" << " |";
			break;
		case '\n':
			cout << "  | " << "\\n" << " |";
			break;
		default:
			cout << "  | " << chars[i].c << " |";
			break;
		}
		cout << "-----| " << ((chars[i].code != NULL) ? chars[i].code : "") << " |" << endl;
	}
}

// 功能：在候选节点中选出两个最小权值节点用于建树。
void Huffman::select(int nNode, int &s1, int &s2)
{
	// 仅挑选 parent==-1 的节点，并返回最小与次小下标。
	s1 = s2 = 0;
	int i;
	for (i = 1; i <= nNode; ++i)
	{
		if (huffTree[i].parent != 0)
			continue;

		if (s1 == 0)
			s1 = i;
		else if (s2 == 0)
		{
			if (huffTree[i].weight < huffTree[s1].weight)
			{
				s2 = s1;
				s1 = i;
			}
			else
				s2 = i;
		}
		else
		{
			if (huffTree[i].weight < huffTree[s1].weight)
			{
				s2 = s1;
				s1 = i;
			}
			else if (huffTree[i].weight < huffTree[s2].weight)
				s2 = i;
		}
	}
}

// 功能：调试打印当前压缩编码串。
void Huffman::PrintCode()
{
	// 直接输出当前编码位数（位流按字节打包后不再展示完整 0/1 串）。
	CString msg;
	msg.Format(_T("当前编码位数：%u"), codeBitLength);
	AfxMessageBox(msg);
}

// 功能：调试打印当前原文文本。
void Huffman::PrintText()
{
	// 直接输出成员 text 内容。
	AfxMessageBox(CString(text.c_str()));
}

// 功能：根据权重统计结果构建 Huffman 树并生成编码表。
void Huffman::MakeCharMap()
{
	// 完成建树后逆向回溯每个叶子节点得到最终编码。
	if (n <= 0 || chars == NULL)
	{
		ReportProgress(68, HUF_PROGRESS_STAGE_BUILD_TREE, TRUE);
		return;
	}

	ReportProgress(53, HUF_PROGRESS_STAGE_BUILD_TREE, TRUE);

	if (huffTree != NULL)
	{
		delete[] huffTree;
		huffTree = NULL;
	}

	if (n == 1)
	{
		if (chars[1].code != NULL)
			delete[] chars[1].code;
		chars[1].code = new char[2];
		chars[1].code[0] = '0';
		chars[1].code[1] = '\0';
		ReportProgress(68, HUF_PROGRESS_STAGE_BUILD_TREE, TRUE);
		return;
	}

	int m = 2 * n - 1;
	huffTree = new HuffTreeNode[m + 1];

	int i;
	for (i = 1; i <= n; ++i)
	{
		huffTree[i].weight = chars[i].weight;
		huffTree[i].parent = 0;
		huffTree[i].lchild = 0;
		huffTree[i].rchild = 0;
	}
	for (i = n + 1; i <= m; ++i)
	{
		huffTree[i].weight = 0;
		huffTree[i].parent = 0;
		huffTree[i].lchild = 0;
		huffTree[i].rchild = 0;
	}

	for (i = n + 1; i <= m; ++i)
	{
		int s1, s2;
		select(i - 1, s1, s2);
		huffTree[s1].parent = huffTree[s2].parent = i;
		huffTree[i].lchild = s1;
		huffTree[i].rchild = s2;
		huffTree[i].weight = huffTree[s1].weight + huffTree[s2].weight;

		ReportStageProgress(HUF_PROGRESS_STAGE_BUILD_TREE,
			(unsigned __int64)(i - n),
			(unsigned __int64)(m - n),
			53,
			62,
			(i == m) ? TRUE : FALSE);
	}

	char *cd = new char[n + 1];
	cd[n] = '\0';
	for (i = 1; i <= n; ++i)
	{
		int start = n;
		int c, f;
		for (c = i, f = huffTree[i].parent; f != 0; c = f, f = huffTree[f].parent)
		{
			if (huffTree[f].lchild == c)
				cd[--start] = '0';
			else
				cd[--start] = '1';
		}

		if (chars[i].code != NULL)
		{
			delete[] chars[i].code;
			chars[i].code = NULL;
		}
		chars[i].code = new char[n - start + 1];
		strcpy(chars[i].code, &cd[start]);

		if (((i & 0x3F) == 0) || i == n)
		{
			ReportStageProgress(HUF_PROGRESS_STAGE_BUILD_TREE,
				(unsigned __int64)i,
				(unsigned __int64)(n > 0 ? n : 1),
				62,
				68,
				(i == n) ? TRUE : FALSE);
		}
	}
	delete[] cd;
	ReportProgress(68, HUF_PROGRESS_STAGE_BUILD_TREE, TRUE);
}

// 功能：读取源文本文件到 text 缓冲。
void Huffman::ReadTextFromFile(char *filename)
{
	// 按字节读入文件内容，为后续统计和压缩做准备。
	BeginProgressSession();
	ifstream infile(filename, ios::binary);
	if (!infile)
	{
		AfxMessageBox(_T("无法打开文件！"));
		text = "";
		return;
	}

	infile.seekg(0, ios::end);
	long fileSize = (long)infile.tellg();
	if (fileSize < 0)
		fileSize = 0;
	infile.seekg(0, ios::beg);

	text = "";
	if (fileSize > 0)
		text.reserve((size_t)fileSize);
	char c;
	unsigned __int64 readBytes = 0;
	while (infile.get(c))
	{
		text += c;
		++readBytes;
		if (((readBytes & 0x0FFF) == 0) || readBytes == (unsigned __int64)fileSize)
		{
			ReportStageProgress(HUF_PROGRESS_STAGE_READ_INPUT,
				readBytes,
				(fileSize > 0) ? (unsigned __int64)fileSize : 1,
				1,
				30,
				(readBytes == (unsigned __int64)fileSize) ? TRUE : FALSE);
		}
	}
	infile.close();
	if (fileSize == 0)
		ReportProgress(30, HUF_PROGRESS_STAGE_READ_INPUT, TRUE);
}

// 功能：将 Huffman 结构与编码数据序列化为压缩包负载。
BOOL Huffman::BuildPayload(string& payload)
{
	// 依次写入节点数、节点数组、编码长度和编码字节。
	payload = "";
	if (n <= 0 || chars == NULL)
		return FALSE;

	ReportProgress(86, HUF_PROGRESS_STAGE_WRITE_FILE, TRUE);

	int i;
	AppendBytes(payload, &n, sizeof(int));
	for (i = 1; i <= n; ++i)
	{
		unsigned char ch = (unsigned char)chars[i].c;
		int codeLen = (chars[i].code == NULL) ? 0 : (int)strlen(chars[i].code);
		AppendBytes(payload, &ch, sizeof(unsigned char));
		AppendBytes(payload, &codeLen, sizeof(int));
		if (codeLen > 0)
			AppendBytes(payload, chars[i].code, codeLen);

		if (((i & 0x3F) == 0) || i == n)
		{
			ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE,
				(unsigned __int64)i,
				(unsigned __int64)(n > 0 ? n : 1),
				86,
				89,
				(i == n) ? TRUE : FALSE);
		}
	}

	unsigned int length = codeBitLength;
	AppendBytes(payload, &length, sizeof(unsigned int));

	int bytesCount = (int)((length + 7U) / 8U);
	if (bytesCount > 0)
	{
		if ((int)codeBitsPacked.size() < bytesCount)
			return FALSE;

		int offset = 0;
		const int kChunk = 64 * 1024;
		while (offset < bytesCount)
		{
			int nAppend = bytesCount - offset;
			if (nAppend > kChunk)
				nAppend = kChunk;
			AppendBytes(payload, codeBitsPacked.c_str() + offset, nAppend);
			offset += nAppend;

			ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE,
				(unsigned __int64)offset,
				(unsigned __int64)bytesCount,
				89,
				92,
				(offset == bytesCount) ? TRUE : FALSE);
		}
	}
	else
	{
		ReportProgress(92, HUF_PROGRESS_STAGE_WRITE_FILE, TRUE);
	}

	// 释放编码中间缓冲，降低后续写盘阶段峰值内存。
	string emptyCode;
	codeBitsPacked.swap(emptyCode);
	codeBitLength = 0;

	ReportProgress(92, HUF_PROGRESS_STAGE_WRITE_FILE, TRUE);
	return TRUE;
}
// 功能：保存压缩包文件，并附加密码摘要与 CRC 尾信息。
BOOL Huffman::SaveCodeToFile(char *filename, const char* password, unsigned long sourceCrc32)
{
	// 构建负载后按需加密，再写入尾部校验元数据。
	// 先释放原文缓存，避免大文件压缩时峰值内存过高。
	if (!text.empty())
	{
		string emptyText;
		text.swap(emptyText);
	}

	string payload;
	if (!BuildPayload(payload))
	{
		AfxMessageBox(_T("压缩数据为空，无法保存！"));
		return FALSE;
	}

	BOOL hasPassword = (password != NULL && password[0] != '\0');
	if (hasPassword)
	{
		// 论文说明：加密阶段采用与解密完全对称的异或流，边处理边上报进度。
		int passLen = (int)strlen(password);
		unsigned long seed = 0x9E3779B9UL;
		int i;
		for (i = 0; i < (int)payload.size(); ++i)
		{
			seed = seed * 1664525UL + 1013904223UL + (unsigned char)password[i % passLen];
			unsigned char key = (unsigned char)(((seed >> 24) & 0xFF) ^
				(unsigned char)password[(i * 7 + 3) % passLen] ^
				(i & 0xFF));
			payload[(size_t)i] = (char)(((unsigned char)payload[(size_t)i]) ^ key);

			if (((i & 0x3FFF) == 0) || (i + 1 == (int)payload.size()))
			{
				ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE,
					(unsigned __int64)(i + 1),
					(unsigned __int64)(payload.empty() ? 1 : payload.size()),
					92,
					94,
					(i + 1 == (int)payload.size()) ? TRUE : FALSE);
			}
		}
	}
	else
	{
		ReportProgress(94, HUF_PROGRESS_STAGE_WRITE_FILE, TRUE);
	}

	char md5Hex[33] = {0};
	CalcMD5Hex(password, md5Hex);

	FILE *f = fopen(filename, "wb");
	if (f == NULL)
	{
		AfxMessageBox(_T("无法打开文件！"));
		return FALSE;
	}

	unsigned __int64 totalWrite = (unsigned __int64)payload.size() + (unsigned __int64)kTailSize;
	unsigned __int64 written = 0;

	if (!payload.empty())
	{
		const unsigned char* p = (const unsigned char*)payload.data();
		int remain = (int)payload.size();
		const int kChunk = 64 * 1024;
		while (remain > 0)
		{
			int nWrite = (remain > kChunk) ? kChunk : remain;
			if (fwrite(p, 1, nWrite, f) != (size_t)nWrite)
			{
				fclose(f);
				return FALSE;
			}
			p += nWrite;
			remain -= nWrite;
			written += (unsigned __int64)nWrite;
			ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE, written, totalWrite, 94, 99, FALSE);
		}
	}

	// 尾部结构：magic(6) + flag(1) + crc32(4) + md5hex(32)
	if (fwrite(kTailMagic, sizeof(char), kTailMagicLen, f) != (size_t)kTailMagicLen)
	{
		fclose(f);
		return FALSE;
	}
	written += (unsigned __int64)kTailMagicLen;
	ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE, written, totalWrite, 94, 99, FALSE);

	{
		unsigned char flag = hasPassword ? 1 : 0;
		if (fwrite(&flag, sizeof(unsigned char), 1, f) != 1)
		{
			fclose(f);
			return FALSE;
		}
	}
	written += 1;
	ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE, written, totalWrite, 94, 99, FALSE);

	if (fwrite(&sourceCrc32, sizeof(unsigned long), 1, f) != 1)
	{
		fclose(f);
		return FALSE;
	}
	written += sizeof(unsigned long);
	ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE, written, totalWrite, 94, 99, FALSE);

	if (fwrite(md5Hex, sizeof(char), kMd5HexLen, f) != (size_t)kMd5HexLen)
	{
		fclose(f);
		return FALSE;
	}
	written += (unsigned __int64)kMd5HexLen;
	ReportStageProgress(HUF_PROGRESS_STAGE_WRITE_FILE, written, totalWrite, 94, 99, TRUE);

	fclose(f);
	m_hasPassword = hasPassword;
	m_storedCrc32 = sourceCrc32;
	ReportProgress(100, HUF_PROGRESS_STAGE_DONE, TRUE);
	return TRUE;
}
// 功能：预读取压缩包尾部元信息。
BOOL Huffman::GetPackageInfo(char *filename, BOOL* hasPassword, unsigned long* storedCrc32, char md5HexOut[33])
{
	// 解析魔数、密码标记、CRC32 和 MD5 文本供解压前判断。
	if (hasPassword != NULL)
		*hasPassword = FALSE;
	if (storedCrc32 != NULL)
		*storedCrc32 = 0;
	if (md5HexOut != NULL)
		md5HexOut[0] = '\0';

	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return FALSE;

	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return FALSE;
	}
	long fileSize = ftell(f);
	if (fileSize <= 0)
	{
		fclose(f);
		return FALSE;
	}

	if (fileSize >= kTailSize)
	{
		if (fseek(f, fileSize - kTailSize, SEEK_SET) == 0)
		{
			unsigned char tail[kTailSize];
			if (fread(tail, 1, kTailSize, f) == (size_t)kTailSize)
			{
				if (memcmp(tail, kTailMagic, kTailMagicLen) == 0)
				{
					BOOL hasPwd = (tail[kTailMagicLen] & 1) ? TRUE : FALSE;
					unsigned long crc = 0;
					memcpy(&crc, tail + kTailMagicLen + 1, 4);
					if (hasPassword != NULL)
						*hasPassword = hasPwd;
					if (storedCrc32 != NULL)
						*storedCrc32 = crc;
					if (md5HexOut != NULL)
					{
						memcpy(md5HexOut, tail + kTailMagicLen + 1 + 4, kMd5HexLen);
						md5HexOut[kMd5HexLen] = '\0';
					}
					fclose(f);
					return TRUE;
				}
			}
		}
	}

	fclose(f);
	return TRUE;
}

// 功能：从压缩包负载解析并恢复 Huffman 结构。
BOOL Huffman::ParsePayload(const unsigned char* payload, int payloadSize)
{
	// 校验边界后回填树节点、编码串及内部状态。
	if (payload == NULL || payloadSize <= 0)
		return FALSE;

	const unsigned char* p = payload;
	int remain = payloadSize;

	int readN = 0;
	if (!ReadBytes(p, remain, &readN, sizeof(int)))
		return FALSE;
	if (readN <= 0 || readN > 256)
		return FALSE;

	n = readN;
	chars = new CharMapNode[n + 1];
	int i;
	for (i = 0; i <= n; ++i)
	{
		chars[i].c = 0;
		chars[i].weight = 0;
		chars[i].code = NULL;
	}

	for (i = 1; i <= n; ++i)
	{
		unsigned char ch = 0;
		int codeLen = 0;
		if (!ReadBytes(p, remain, &ch, sizeof(unsigned char)) ||
			!ReadBytes(p, remain, &codeLen, sizeof(int)))
			return FALSE;
		if (codeLen < 0 || codeLen > 8192)
			return FALSE;

		chars[i].c = (char)ch;
		chars[i].code = new char[codeLen + 1];
		if (codeLen > 0)
		{
			if (!ReadBytes(p, remain, chars[i].code, codeLen))
				return FALSE;
		}
		chars[i].code[codeLen] = '\0';
	}

	unsigned int bitLen = 0;
	if (!ReadBytes(p, remain, &bitLen, sizeof(unsigned int)))
		return FALSE;

	int bytesCount = (int)((bitLen + 7U) / 8U);
	if (bytesCount > remain)
		return FALSE;

	codeBitLength = bitLen;
	codeBitsPacked = "";
	if (bytesCount > 0)
	{
		codeBitsPacked.assign((const char*)p, bytesCount);
		p += bytesCount;
		remain -= bytesCount;
	}

	return TRUE;
}

// 功能：读取压缩包并在需要时验证密码后还原编码数据。
BOOL Huffman::ReadCodeFromFile(char *filename, const char* password)
{
	// 先读尾部信息，再按密码解密并解析 payload。
	ResetData();

	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		AfxMessageBox(_T("无法打开文件！"));
		return FALSE;
	}

	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return FALSE;
	}
	long fileSize = ftell(f);
	if (fileSize <= 0)
	{
		fclose(f);
		return FALSE;
	}
	if (fseek(f, 0, SEEK_SET) != 0)
	{
		fclose(f);
		return FALSE;
	}

	unsigned char* fileData = new unsigned char[fileSize];
	if (fread(fileData, 1, fileSize, f) != (size_t)fileSize)
	{
		delete[] fileData;
		fclose(f);
		return FALSE;
	}
	fclose(f);

	BOOL hasPassword = FALSE;
	unsigned long storedCrc = 0;
	char md5HexInFile[33] = {0};
	int payloadSize = (int)fileSize;

	if (fileSize >= kTailSize)
	{
		const unsigned char* tail = fileData + fileSize - kTailSize;
		if (memcmp(tail, kTailMagic, kTailMagicLen) == 0)
		{
			hasPassword = ((tail[kTailMagicLen] & 1) != 0) ? TRUE : FALSE;
			memcpy(&storedCrc, tail + kTailMagicLen + 1, 4);
			memcpy(md5HexInFile, tail + kTailMagicLen + 1 + 4, kMd5HexLen);
			md5HexInFile[kMd5HexLen] = '\0';
			payloadSize = (int)fileSize - kTailSize;
		}
	}

	if (payloadSize <= 0)
	{
		delete[] fileData;
		return FALSE;
	}

	if (hasPassword)
	{
		if (password == NULL || password[0] == '\0')
		{
			delete[] fileData;
			return FALSE;
		}

		char md5HexInput[33] = {0};
		CalcMD5Hex(password, md5HexInput);
		if (_stricmp(md5HexInput, md5HexInFile) != 0)
		{
			delete[] fileData;
			return FALSE;
		}

		XorCryptPayload(fileData, payloadSize, password);
	}

	BOOL ok = ParsePayload(fileData, payloadSize);
	delete[] fileData;
	if (!ok)
	{
		ResetData();
		return FALSE;
	}

	m_hasPassword = hasPassword;
	m_storedCrc32 = storedCrc;
	return TRUE;
}

// 功能：将 code 按 Huffman 树解码回原文 text。
void Huffman::Decode()
{
	// 从按位打包缓冲解码回原文，避免先展开成超大 "0/1" 字符串。
	text = "";
	if (n <= 0 || chars == NULL || codeBitLength == 0 || codeBitsPacked.empty())
		return;

	int maxNodes = 1;
	int i;
	for (i = 1; i <= n; ++i)
	{
		if (chars[i].code != NULL)
			maxNodes += (int)strlen(chars[i].code);
	}
	if (maxNodes <= 1)
		return;

	struct DecodeNode
	{
		int next0;
		int next1;
		int symbolIndex;
	};

	DecodeNode* nodes = new DecodeNode[maxNodes + 1];
	for (i = 0; i <= maxNodes; ++i)
	{
		nodes[i].next0 = 0;
		nodes[i].next1 = 0;
		nodes[i].symbolIndex = 0;
	}

	int nodeCount = 1;
	for (i = 1; i <= n; ++i)
	{
		if (chars[i].code == NULL)
			continue;
		int cur = 1;
		int k;
		int codeLen = (int)strlen(chars[i].code);
		for (k = 0; k < codeLen; ++k)
		{
			int bit = (chars[i].code[k] == '1') ? 1 : 0;
			int next = (bit == 0) ? nodes[cur].next0 : nodes[cur].next1;
			if (next == 0)
			{
				++nodeCount;
				nodes[nodeCount].next0 = 0;
				nodes[nodeCount].next1 = 0;
				nodes[nodeCount].symbolIndex = 0;
				if (bit == 0)
					nodes[cur].next0 = nodeCount;
				else
					nodes[cur].next1 = nodeCount;
				next = nodeCount;
			}
			cur = next;
		}
		nodes[cur].symbolIndex = i;
	}

	text.reserve((size_t)((codeBitLength + 7U) / 8U));
	int cur = 1;
	unsigned int bitPos;
	for (bitPos = 0; bitPos < codeBitLength; ++bitPos)
	{
		unsigned int byteIndex = bitPos >> 3;
		int bitInByte = 7 - (int)(bitPos & 7U);
		unsigned char inByte = (unsigned char)codeBitsPacked[(size_t)byteIndex];
		int bit = ((inByte >> bitInByte) & 1) ? 1 : 0;

		int next = (bit == 0) ? nodes[cur].next0 : nodes[cur].next1;
		if (next == 0)
			break;
		cur = next;

		if (nodes[cur].symbolIndex > 0)
		{
			text += chars[nodes[cur].symbolIndex].c;
			cur = 1;
		}
	}

	delete[] nodes;

	// 解码后不再需要编码位流，及时释放内存。
	string emptyBits;
	codeBitsPacked.swap(emptyBits);
	codeBitLength = 0;
}
// 功能：统计原文字符频次并换算为权重。
void Huffman::CountCharsWeight()
{
	// 遍历 text 累加计数，再归一化为每个字符权重。
	if (text.empty())
	{
		ReportProgress(52, HUF_PROGRESS_STAGE_COUNT_WEIGHT, TRUE);
		return;
	}

	if (chars != NULL)
	{
		int t;
		for (t = 0; t <= n; ++t)
		{
			if (chars[t].code != NULL)
			{
				delete[] chars[t].code;
				chars[t].code = NULL;
			}
		}
		delete[] chars;
		chars = NULL;
	}
	n = 0;
	ReportProgress(32, HUF_PROGRESS_STAGE_COUNT_WEIGHT, TRUE);

	int i = 0;
	chars = new CharMapNode[2];
	chars[0].c = 0;
	chars[0].weight = 0;
	chars[0].code = NULL;
	chars[1].code = NULL;
	chars[1].c = text[i];
	chars[1].weight = 1;
	++n;

	int total = 1;
	int textSize = (int)text.size();
	for (i = 1; i != (int)text.size(); ++i)
	{
		int j;
		for (j = 1; j <= n; ++j)
		{
			if (text[i] == chars[j].c)
			{
				++total;
				++chars[j].weight;
				break;
			}
		}

		if (j > n)
		{
			++total;
			++n;
			CharMap newchars = new CharMapNode[n + 1];
			memcpy(newchars, chars, n * sizeof(CharMapNode));
			delete[] chars;
			chars = newchars;
			chars[n].c = text[i];
			chars[n].weight = 1;
			chars[n].code = NULL;
		}

		if (((i & 0x0FFF) == 0) || i + 1 == textSize)
		{
			ReportStageProgress(HUF_PROGRESS_STAGE_COUNT_WEIGHT,
				(unsigned __int64)(i + 1),
				(unsigned __int64)(textSize > 0 ? textSize : 1),
				32,
				52,
				(i + 1 == textSize) ? TRUE : FALSE);
		}
	}

	int p;
	for (p = 0; p < n + 1; ++p)
	{
		chars[p].weight = chars[p].weight / total;
	}
	ReportProgress(52, HUF_PROGRESS_STAGE_COUNT_WEIGHT, TRUE);
}

// 功能：计算当前 text 内容的 CRC32。
unsigned long Huffman::GetTextCRC32() const
{
	// 复用统一 CRC32 逻辑，供压缩后校验一致性。
	if (text.empty())
		return 0;
	return CalcCRC32Buffer((const unsigned char*)text.data(), (int)text.length());
}

// 功能：将解压得到的 text 写入目标文本文件。
void Huffman::SaveTextToFile(char *filename)
{
	// 按字节顺序输出字符串内容。
	ofstream outfile(filename, ios::binary);
	if (!outfile)
	{
		AfxMessageBox(_T("保存文件出错！"));
		return;
	}
	outfile.write(text.data(), text.length());
	outfile.close();
}

// 功能：获取指定文件的字节长度。
int Huffman::FileSize(char* path)
{
	// 打开文件并返回长度，失败时返回 0。
	ifstream in(path, ios::binary);
	if (!in)
		return 0;
	in.seekg(0, ios::end);
	int i = in.tellg();
	in.close();
	return i;
}

// 功能：手动输入字符与权重（调试或教学场景）。
void Huffman::InputCharsWeight()
{
	// 从标准输入读取映射数据并填充 chars 表。
	// 当前版本不从控制台输入权值，统一由 CountCharsWeight 自动统计。
}

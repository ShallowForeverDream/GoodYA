
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

const int MAX_LEN = 10;
const int MAX_SYMBOLS = (1 << MAX_LEN);

const char MAGIC_ENC1[4] = {'E', 'N', 'C', '1'};
const char MAGIC_ENC2[4] = {'E', 'N', 'C', '2'};
const char MAGIC_LEGACY[4] = {'Z', 'J', 'H', '1'};

const char TAIL_MAGIC[] = "ZJHPWD1";
const int TAIL_MAGIC_LEN = 7;
const int TAIL_SIZE = TAIL_MAGIC_LEN + 1 + 4;

struct LegacyHeader
{
	char magic[4];
	unsigned long payloadLen;
	unsigned long originalLen;
	unsigned char methodLen;
	unsigned char reserved[3];
};

template <typename T>
static void AppendPod(std::vector<unsigned char>& out, const T& value)
{
	size_t oldSize = out.size();
	out.resize(oldSize + sizeof(T));
	memcpy(&out[oldSize], &value, sizeof(T));
}

template <typename T>
static bool ReadPod(const std::vector<unsigned char>& in, size_t& offset, T& value)
{
	if (offset + sizeof(T) > in.size())
		return false;
	memcpy(&value, &in[offset], sizeof(T));
	offset += sizeof(T);
	return true;
}

static unsigned __int64 BytesForBits(unsigned __int64 bitCount)
{
	return (bitCount + 7) / 8;
}

static int GetBitFromBytes(const unsigned char* bytes, unsigned __int64 bitIndex)
{
	unsigned __int64 byteIndex64 = bitIndex >> 3;
	int bitInByte = 7 - (int)(bitIndex & 7);
	size_t byteIndex = (size_t)byteIndex64;
	return (bytes[byteIndex] >> bitInByte) & 1;
}

static bool ReadAllBytes(const std::string& path, std::vector<unsigned char>& out)
{
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

static bool HasSuffixIgnoreCase(const std::string& text, const std::string& suffix)
{
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

static void XorCryptPayload(std::vector<unsigned char>& data, const char* password)
{
	if (data.empty() || password == NULL || password[0] == '\0')
		return;

	int passLen = (int)strlen(password);
	if (passLen <= 0)
		return;

	unsigned long seed = 0xA5C39E7DUL;
	for (size_t i = 0; i < data.size(); ++i)
	{
		seed = seed * 1664525UL + 1013904223UL + (unsigned char)password[i % (size_t)passLen];
		unsigned char key = (unsigned char)(
			((seed >> 24) & 0xFF) ^
			(unsigned char)password[(i * 7 + 3) % (size_t)passLen] ^
			(i & 0xFF));
		data[i] ^= key;
	}
}

static bool ParseTailInfo(const std::vector<unsigned char>& fileData,
	bool* hasPassword,
	unsigned long* hashOut,
	size_t* payloadSize)
{
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
		for (size_t i = 0; i < code.size(); ++i)
			pushBit(code[i] == '1' ? 1 : 0);
	}

	void flush()
	{
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
	unsigned short symbol;
	std::string code;
};

class EncoderWorker
{
public:
	EncoderWorker()
		: len_(0), symbolLimit_(0), root_(NULL), originalBits_(0)
	{
	}

	~EncoderWorker()
	{
		clearNodes();
	}

	bool run(const std::string& inputPath, int len, std::vector<unsigned char>& outPayload)
	{
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
			buildHuffmanTree();
			assignCodes(root_, "");
			if (usedSymbols_.size() == 1)
				codes_[(size_t)usedSymbols_[0]] = "0";
			relaxTreeForUniqueDecode();
		}

		return writeOutputBytes(outPayload);
	}

private:
	struct Node
	{
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
		bool operator()(const QueueInfo& a, const QueueInfo& b) const
		{
			return a.count > b.count;
		}
	};

	struct ACNode
	{
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

	void clearNodes()
	{
		for (size_t i = 0; i < nodes_.size(); ++i)
			delete nodes_[i];
		nodes_.clear();
	}

	Node* makeNode(int symbol, Node* left, Node* right)
	{
		Node* n = new Node(symbol, left, right);
		nodes_.push_back(n);
		return n;
	}

	bool readInput(const std::string& inputPath)
	{
		std::ifstream fin(inputPath.c_str(), std::ios::binary);
		if (!fin)
			return false;

		unsigned int current = 0;
		int currentLen = 0;
		char ch = 0;
		while (fin.get(ch))
		{
			unsigned char byte = (unsigned char)ch;
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
		return true;
	}

	void buildHuffmanTree()
	{
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
		if (root_ == NULL)
			return;

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
		for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			Node* internal = bfsNodes[(size_t)nodeIndex];
			if (internal->symbol != -1 || internal->pathBits.empty())
				continue;

			for (int j = nodeCount - 1; j > nodeIndex; --j)
			{
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
					return;

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
	}

	bool writeOutputBytes(std::vector<unsigned char>& outPayload) const
	{
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

		BitWriter tableWriter;
		for (size_t iTbl = 0; iTbl < table.size(); ++iTbl)
			tableWriter.pushCode(table[iTbl].code);
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

		return true;
	}
};

struct DecTableEntry
{
	unsigned short symbol;
	std::string code;
};

struct DecodeOutput
{
	unsigned short length;
	unsigned short symbol;
};

struct DecodeACNode
{
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
		XorCryptPayload(raw, password);

	if (header.originalLen < raw.size())
		raw.resize((size_t)header.originalLen);

	return WriteAllBytes(outputPath, raw);
}

static bool EncodeEnc2Payload(const std::string& path,
	int len,
	std::vector<unsigned char>& payload)
{
	if (len <= 0 || len > MAX_LEN)
		return false;

	EncoderWorker worker;
	return worker.run(path, len, payload);
}

static bool EncodeToFile(const std::string& path, int len, const char* password)
{
	std::vector<unsigned char> payload;
	if (!EncodeEnc2Payload(path, len, payload))
		return false;

	bool hasPassword = (password != NULL && password[0] != '\0');
	if (hasPassword)
		XorCryptPayload(payload, password);

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

	return WriteAllBytes(path + ".zjh", fileData);
}

static bool DecodeToFile(const std::string& path,
	const std::string& outputPath,
	const char* password)
{
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
		XorCryptPayload(payload, password);
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

bool ZJH_encrypto::encrypto(const std::string& path, int len)
{
	return EncodeToFile(path, len, NULL);
}

bool ZJH_encrypto1::encrypto(const std::string& path, int len, const char* password)
{
	return EncodeToFile(path, len, password);
}

bool ZJH_encrypto2::encrypto(const std::string& path, int len)
{
	return EncodeToFile(path, len, NULL);
}

bool ZJH_decrypto::decrypto(const std::string& path, const char* password)
{
	if (!HasSuffixIgnoreCase(path, ".zjh"))
		return false;

	std::string outPath = path.substr(0, path.size() - 4);
	return DecodeToFile(path, outPath, password);
}

bool ZJH_decrypto::decrypto_to(const std::string& path, const std::string& output_path, const char* password)
{
	return DecodeToFile(path, output_path, password);
}

bool ZJH_GetPackageInfo(const std::string& path, bool* hasPassword)
{
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

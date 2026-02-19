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

static void AppendBytes(string& buf, const void* data, int len)
{
	if (data == NULL || len <= 0)
		return;
	buf.append((const char*)data, len);
}

static BOOL ReadBytes(const unsigned char*& p, int& remain, void* out, int len)
{
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
static unsigned long CalcCRC32Buffer(const unsigned char* data, int len)
{
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

inline static void FF(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	a += F(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}
inline static void GG(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	a += G(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}
inline static void HH(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	a += H(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}
inline static void II(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, int s, unsigned int ac)
{
	a += I(b, c, d) + x + ac;
	a = ROTATE_LEFT(a, s);
	a += b;
}

static void MD5Encode(unsigned char* output, const unsigned int* input, unsigned int len)
{
	unsigned int i, j;
	for (i = 0, j = 0; j < len; ++i, j += 4)
	{
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

static void MD5Decode(unsigned int* output, const unsigned char* input, unsigned int len)
{
	unsigned int i, j;
	for (i = 0, j = 0; j < len; ++i, j += 4)
	{
		output[i] = ((unsigned int)input[j]) | (((unsigned int)input[j + 1]) << 8) |
			(((unsigned int)input[j + 2]) << 16) | (((unsigned int)input[j + 3]) << 24);
	}
}

static void MD5Transform(unsigned int state[4], const unsigned char block[64])
{
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

static void MD5Init(MD5_CTX* context)
{
	context->count[0] = context->count[1] = 0;
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

static void MD5Update(MD5_CTX* context, const unsigned char* input, unsigned int inputLen)
{
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

static void MD5Final(unsigned char digest[16], MD5_CTX* context)
{
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
static BOOL CalcMD5Hex(const char* text, char outHex[33])
{
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
static void XorCryptPayload(unsigned char* data, int len, const char* password)
{
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

Huffman::Huffman()
{
	huffTree = NULL;
	chars = NULL;
	n = 0;
	m_hasPassword = FALSE;
	m_storedCrc32 = 0;
}

Huffman::~Huffman()
{
	ResetData();
}

void Huffman::ResetData()
{
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
	code = "";
	m_hasPassword = FALSE;
	m_storedCrc32 = 0;
}

void Huffman::Encode()
{
	code = "";
	if (n <= 0 || chars == NULL)
		return;

	string::size_type i;
	for (i = 0; i != text.size(); ++i)
	{
		int j;
		for (j = 1; j <= n; ++j)
		{
			if (chars[j].c == text[i])
			{
				if (chars[j].code != NULL)
					code += chars[j].code;
				break;
			}
		}
	}
}

void Huffman::PrintCharWeight()
{
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

void Huffman::PrintCharCode()
{
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

void Huffman::select(int nNode, int &s1, int &s2)
{
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

void Huffman::PrintCode()
{
	AfxMessageBox(CString(code.c_str()));
}

void Huffman::PrintText()
{
	AfxMessageBox(CString(text.c_str()));
}

void Huffman::MakeCharMap()
{
	if (n <= 0 || chars == NULL)
		return;

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
	}
	delete[] cd;
}

void Huffman::ReadTextFromFile(char *filename)
{
	ifstream infile(filename, ios::binary);
	if (!infile)
	{
		AfxMessageBox(_T("无法打开文件！"));
		text = "";
		return;
	}

	text = "";
	char c;
	while (infile.get(c))
	{
		text += c;
	}
	infile.close();
}

BOOL Huffman::BuildPayload(string& payload)
{
	payload = "";
	if (n <= 0 || chars == NULL)
		return FALSE;

	int i, j;
	AppendBytes(payload, &n, sizeof(int));
	for (i = 1; i <= n; ++i)
	{
		unsigned char ch = (unsigned char)chars[i].c;
		int codeLen = (chars[i].code == NULL) ? 0 : (int)strlen(chars[i].code);
		AppendBytes(payload, &ch, sizeof(unsigned char));
		AppendBytes(payload, &codeLen, sizeof(int));
		if (codeLen > 0)
			AppendBytes(payload, chars[i].code, codeLen);
	}

	int length = (int)code.length();
	AppendBytes(payload, &length, sizeof(int));

	int times = (length + 7) / 8;
	int num = 0;
	for (i = 0; i < times; ++i)
	{
		unsigned char outByte = 0;
		for (j = 0; j < 8; ++j)
		{
			outByte = (unsigned char)(outByte << 1);
			if (num < length && code[num] == '1')
				outByte = (unsigned char)(outByte + 1);
			++num;
		}
		AppendBytes(payload, &outByte, sizeof(unsigned char));
	}

	return TRUE;
}

BOOL Huffman::SaveCodeToFile(char *filename, const char* password, unsigned long sourceCrc32)
{
	string payload;
	if (!BuildPayload(payload))
	{
		AfxMessageBox(_T("压缩数据为空，无法保存！"));
		return FALSE;
	}

	BOOL hasPassword = (password != NULL && password[0] != '\0');
	string outPayload = payload;
	if (hasPassword)
	{
		XorCryptPayload((unsigned char*)&outPayload[0], (int)outPayload.size(), password);
	}

	char md5Hex[33] = {0};
	CalcMD5Hex(password, md5Hex);

	FILE *f = fopen(filename, "wb");
	if (f == NULL)
	{
		AfxMessageBox(_T("无法打开文件！"));
		return FALSE;
	}

	if (!outPayload.empty())
		fwrite(outPayload.data(), sizeof(char), outPayload.size(), f);

	// 尾部结构：magic(6) + flag(1) + crc32(4) + md5hex(32)
	fwrite(kTailMagic, sizeof(char), kTailMagicLen, f);
	{
		unsigned char flag = hasPassword ? 1 : 0;
		fwrite(&flag, sizeof(unsigned char), 1, f);
	}
	fwrite(&sourceCrc32, sizeof(unsigned long), 1, f);
	fwrite(md5Hex, sizeof(char), kMd5HexLen, f);

	fclose(f);
	m_hasPassword = hasPassword;
	m_storedCrc32 = sourceCrc32;
	return TRUE;
}

BOOL Huffman::GetPackageInfo(char *filename, BOOL* hasPassword, unsigned long* storedCrc32, char md5HexOut[33])
{
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

BOOL Huffman::ParsePayload(const unsigned char* payload, int payloadSize)
{
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

	int bitLen = 0;
	if (!ReadBytes(p, remain, &bitLen, sizeof(int)))
		return FALSE;
	if (bitLen < 0)
		return FALSE;

	int bytesCount = (bitLen + 7) / 8;
	if (bytesCount > remain)
		return FALSE;

	code = "";
	code.reserve(bitLen);

	for (i = 0; i < bytesCount; ++i)
	{
		unsigned char inByte = 0;
		if (!ReadBytes(p, remain, &inByte, sizeof(unsigned char)))
			return FALSE;

		int j;
		for (j = 0; j < 8 && (int)code.length() < bitLen; ++j)
		{
			if (((inByte >> (7 - j)) & 1) != 0)
				code += '1';
			else
				code += '0';
		}
	}

	return TRUE;
}

BOOL Huffman::ReadCodeFromFile(char *filename, const char* password)
{
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

void Huffman::Decode()
{
	text = "";
	if (n <= 0 || chars == NULL || code.empty())
		return;

	string::size_type i = 0;
	while (i < code.size())
	{
		BOOL matched = FALSE;
		int j;
		for (j = 1; j <= n; ++j)
		{
			int codeLen = (chars[j].code == NULL) ? 0 : (int)strlen(chars[j].code);
			if (codeLen <= 0)
				continue;
			if (i + codeLen <= code.size() && code.compare(i, codeLen, chars[j].code) == 0)
			{
				text += chars[j].c;
				i += codeLen;
				matched = TRUE;
				break;
			}
		}
		if (!matched)
			break;
	}
}

void Huffman::CountCharsWeight()
{
	if (text.empty())
		return;

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
	}

	int p;
	for (p = 0; p < n + 1; ++p)
	{
		chars[p].weight = chars[p].weight / total;
	}
}

unsigned long Huffman::GetTextCRC32() const
{
	if (text.empty())
		return 0;
	return CalcCRC32Buffer((const unsigned char*)text.data(), (int)text.length());
}

void Huffman::SaveTextToFile(char *filename)
{
	ofstream outfile(filename, ios::binary);
	if (!outfile)
	{
		AfxMessageBox(_T("保存文件出错！"));
		return;
	}
	outfile.write(text.data(), text.length());
	outfile.close();
}

int Huffman::FileSize(char* path)
{
	ifstream in(path, ios::binary);
	if (!in)
		return 0;
	in.seekg(0, ios::end);
	int i = in.tellg();
	in.close();
	return i;
}

void Huffman::InputCharsWeight()
{
	// 当前版本不从控制台输入权值，统一由 CountCharsWeight 自动统计。
}
// Huffman.h: Huffman 压缩类声明
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_)
#define AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
using namespace std;

// Huffman 树节点
typedef struct
{
	float weight;
	int parent;
	int lchild;
	int rchild;
} HuffTreeNode, *HuffTree;

// 字符权重与编码映射节点
typedef struct
{
	char c;
	float weight;
	char *code;
} CharMapNode, *CharMap;

class Huffman
{
private:
	void select(int n, int &s1, int &s2);
	void ResetData();
	BOOL BuildPayload(string& payload);
	BOOL ParsePayload(const unsigned char* payload, int payloadSize);

	HuffTree huffTree;  // Huffman 树
	CharMap chars;      // 字符表
	int n;              // 不同字符个数
	string text;        // 原文
	string code;        // 编码串

	BOOL m_hasPassword;             // 当前读取文件是否带密码
	unsigned long m_storedCrc32;    // 文件中记录的 CRC32（用于校验）

public:
	void InputCharsWeight();
	void CountCharsWeight();
	void Decode();
	void ReadTextFromFile(char *filename);
	BOOL ReadCodeFromFile(char *filename, const char* password = NULL);
	void SaveTextToFile(char *filename);
	BOOL SaveCodeToFile(char *filename, const char* password, unsigned long sourceCrc32);
	void PrintCode();
	void MakeCharMap();
	void PrintText();
	void PrintCharCode();
	void PrintCharWeight();
	void Encode();
	int FileSize(char *path);

	// 计算当前 text 的 CRC32，供“测试压缩文件”对比使用
	unsigned long GetTextCRC32() const;

	// 预读取文件尾信息，判断是否带密码并获取文件记录 CRC32
	BOOL GetPackageInfo(char *filename, BOOL* hasPassword, unsigned long* storedCrc32, char md5HexOut[33]);
	BOOL HasPassword() const { return m_hasPassword; }
	unsigned long GetStoredCrc32() const { return m_storedCrc32; }

	Huffman();
	virtual ~Huffman();
};

#endif // !defined(AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_)
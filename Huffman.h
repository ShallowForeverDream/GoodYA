// Huffman.h: Huffman 压缩器声明
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
	void select(int n, int &s1, int &s2); // 在前 n 个节点里选出权值最小且无父节点的两个下标
	void ResetData(); // 清空内部缓存，释放动态内存并恢复初始状态
	BOOL BuildPayload(string& payload); // 把当前 Huffman 树和编码结果打包成可写入文件的负载
	BOOL ParsePayload(const unsigned char* payload, int payloadSize); // 从负载字节流还原 Huffman 树和编码数据

	HuffTree huffTree;  // Huffman 树
	CharMap chars;      // 字符映射表
	int n;              // 不同字符数量
	string text;        // 原文文本
	string code;        // 压缩后比特串（以字符形式暂存）

	BOOL m_hasPassword;             // 当前读取包是否带密码校验信息
	unsigned long m_storedCrc32;    // 压缩包中记录的原文 CRC32

public:
	void InputCharsWeight(); // 交互输入字符及权重（调试辅助）
	void CountCharsWeight(); // 统计原文中每个字符的出现频率
	void Decode(); // 按当前 Huffman 树把编码串还原为原文
	void ReadTextFromFile(char *filename); // 读取源文本文件内容到 text
	BOOL ReadCodeFromFile(char *filename, const char* password = NULL); // 读取压缩包并校验密码/尾部信息
	void SaveTextToFile(char *filename); // 把解压后的 text 写回文本文件
	BOOL SaveCodeToFile(char *filename, const char* password, unsigned long sourceCrc32); // 保存压缩包并附加密码摘要和 CRC 信息
	void PrintCode(); // 打印当前编码串（调试）
	void MakeCharMap(); // 根据权重构建字符映射并分配编码数组
	void PrintText(); // 打印当前原文（调试）
	void PrintCharCode(); // 打印每个字符的 Huffman 编码（调试）
	void PrintCharWeight(); // 打印字符权重表（调试）
	void Encode(); // 根据 Huffman 树把 text 编码为 code
	int FileSize(char *path); // 获取指定文件字节长度

	unsigned long GetTextCRC32() const; // 计算当前 text 的 CRC32

	BOOL GetPackageInfo(char *filename, BOOL* hasPassword, unsigned long* storedCrc32, char md5HexOut[33]); // 预读取压缩包尾部信息
	BOOL HasPassword() const { return m_hasPassword; }
	unsigned long GetStoredCrc32() const { return m_storedCrc32; }

	Huffman(); // 构造并初始化压缩器对象
	virtual ~Huffman(); // 析构并释放内部动态资源
};

#endif // !defined(AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_)
// Huffman.h: interface for the Huffman class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_)
#define AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
using namespace std;

/***********************数据结构***********************/

//哈弗曼树节点
typedef struct 
{
	float weight;
	int parent;
	int lchild;
	int rchild;
}HuffTreeNode,*HuffTree;

//字符-权值-编码映射
typedef struct
{
	char c;
	float weight;
	char *code;
}CharMapNode,*CharMap;


/*************************类定义****************************/

class Huffman  
{
private:
	void select(int n, int &s1, int &s2);
	HuffTree huffTree;	//哈弗曼树
	CharMap chars;		//字符表
	int n;				//字符数
	string text;		//原文
	string code;		//编码
public:	

	void InputCharsWeight();
	void CountCharsWeight();
	void Decode();
	void ReadTextFromFile(char *filename);
	void ReadCodeFromFile(char *filename);
	void SaveTextToFile(char *filename);
	void SaveCodeToFile(char *filename);
	void PrintCode();
	void MakeCharMap();
	void PrintText();	
	void PrintCharCode();
	void PrintCharWeight();
	void Encode();
	int FileSize(char* path);
	
	Huffman();
	virtual ~Huffman();

};

#endif // !defined(AFX_HUFFMAN_H__08A1863A_6641_4FE9_9596_5EEBE76B53F7__INCLUDED_)

// Huffman.cpp: implementation of the Huffman class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Huffman.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Huffman::Huffman()
{
	huffTree = NULL;
	chars = NULL;
	n = 0;
}

Huffman::~Huffman()
{

}

//??text?????��?????????
void Huffman::Encode()
{
	code = "";
	for (string::size_type i = 0; i != text.size(); ++i)
	{
		for (int j = 1; j <= n; ++j)
			if (chars[j].c == text[i])
				code += chars[j].code;
	}
}

//?????????
void Huffman::PrintCharWeight()
{
	CharMapNode cc ;
	for(int p = 0 ; p<= n; p++){
		for(int j = 0 ;j<= n-1; j++){
			if(chars[j].weight>chars[j+1].weight){
				cc = chars[j];
				chars[j] = chars[j+1];
				chars[j+1] = cc;
			}
		}
	}

	cout << " |???|??????|??|" << endl;
	for (int i = 1; i <= n; ++i)
	{
		switch (chars[i].c)
		{
		case '\t':
			cout <<"  |"<< "\\t"<<"|";
			break;
		case '\n':
			cout <<"  |"<< "\\n"<<"|";
			break;
		default:
			cout << "  |" << chars[i].c <<"|";
			break;
		}
			cout << "????|" << chars[i].weight <<"|" << endl;
	}
}

//??????????
void Huffman::PrintCharCode()
{
	cout << " |???|??????|??????????|" << endl;
	for (int i = 1; i <= n; ++i)
	{
		switch (chars[i].c)
		{
		case '\t':
			cout <<"  | "<< "\\t"<<" |";
			break;
		case '\n':
			cout <<"  | "<< "\\n"<<" |";
			break;
		default:
			cout << "  | " << chars[i].c <<" |";
			break;
		}
			cout << "??????????| " << chars[i].code <<" |" << endl;
	}
}

//??????��????
void Huffman::select(int n, int &s1, int &s2)
{
	s1 = s2 = 0;
	for (int i = 1; i <= n; ++i)
	{
		// ?��?????????????
		if (huffTree[i].parent != 0)
			continue;
		if (s1 == 0)
			s1 = i;
		else if (s2 == 0)
		{
			//s1???????��???????s1?????s1??s2????
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

//???????
void Huffman::PrintCode()
{
	AfxMessageBox(code.c_str());
}

//??????
void Huffman::PrintText()
{
	AfxMessageBox(text.c_str());
}

//???????????????????-?????
void Huffman::MakeCharMap()
{
	if (n <= 0 || chars == NULL)
		return;
	if (n == 1)
	{
		chars[1].code = new char[2];
		chars[1].code[0] = '0';
		chars[1].code[1] = '\0';
		return;
	}
	int m = 2 * n - 1;
	huffTree = new HuffTreeNode[m+1];
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
		int s1,s2;
		select(i - 1, s1, s2);
		huffTree[s1].parent = huffTree[s2].parent = i;
		huffTree[i].lchild = s1;
		huffTree[i].rchild = s2;
		huffTree[i].weight = huffTree[s1].weight + huffTree[s2].weight;
	}

	char *cd = new char[n];
	cd[n-1] = '\0';
	for(i = 1; i <= n; ++i)
	{
		int start = n - 1;
		int c,f;
		for (c = i, f = huffTree[i].parent; f != 0; c = f, f = huffTree[f].parent)
		{
			if (huffTree[f].lchild == c)
				cd[--start] = '0';
			else
				cd[--start] = '1';
		}
		chars[i].code = new char[n - start];
		strcpy(chars[i].code,&cd[start]);
	}
	delete[] cd;
}


//????????????
void Huffman::ReadTextFromFile(char *filename)
{
	ifstream infile(filename);
	if(!infile)
	{
		AfxMessageBox("??????????");
		return;
	}
	text = "";
	char c;
	while(infile.get(c))
	{
		text += c;
	}
}


//????????????
void Huffman::SaveCodeToFile(char *filename)
{
	int i, j;
	FILE *f = fopen(filename, "wb");
	if (f == NULL)
	{
		AfxMessageBox("??????????");
		return;
	}

	fwrite(&n, sizeof(int), 1, f);
	for (i = 1; i <= n; ++i)
	{
		unsigned char ch = (unsigned char)chars[i].c;
		int codeLen = (chars[i].code == NULL) ? 0 : (int)strlen(chars[i].code);
		fwrite(&ch, sizeof(unsigned char), 1, f);
		fwrite(&codeLen, sizeof(int), 1, f);
		if (codeLen > 0)
			fwrite(chars[i].code, sizeof(char), codeLen, f);
	}

	int length = (int)code.length();
	fwrite(&length, sizeof(int), 1, f);

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
		fwrite(&outByte, sizeof(unsigned char), 1, f);
	}

	fclose(f);
}


//????????????
void Huffman::ReadCodeFromFile(char *filename)
{
	int i, j;
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		AfxMessageBox("??????????");
		code = "";
		n = 0;
		return;
	}

	if (fread(&n, sizeof(int), 1, f) != 1 || n <= 0 || n > 256)
	{
		fclose(f);
		code = "";
		n = 0;
		return;
	}

	chars = new CharMapNode[n + 1];
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
		if (fread(&ch, sizeof(unsigned char), 1, f) != 1 ||
			fread(&codeLen, sizeof(int), 1, f) != 1 ||
			codeLen < 0 || codeLen > 1024)
		{
			fclose(f);
			code = "";
			n = 0;
			return;
		}

		chars[i].c = (char)ch;
		chars[i].code = new char[codeLen + 1];
		if (codeLen > 0)
		{
			if (fread(chars[i].code, sizeof(char), codeLen, f) != (size_t)codeLen)
			{
				fclose(f);
				code = "";
				n = 0;
				return;
			}
		}
		chars[i].code[codeLen] = '\0';
	}

	int length = 0;
	if (fread(&length, sizeof(int), 1, f) != 1 || length < 0 || length > (1 << 30))
	{
		fclose(f);
		code = "";
		n = 0;
		return;
	}

	string str = "";
	str.reserve(length);
	int times = (length + 7) / 8;
	for (i = 0; i < times; ++i)
	{
		unsigned char inByte = 0;
		if (fread(&inByte, sizeof(unsigned char), 1, f) != 1)
			break;
		for (j = 0; j < 8 && (int)str.length() < length; ++j)
		{
			if (((inByte >> (7 - j)) & 1) != 0)
				str += '1';
			else
				str += '0';
		}
	}

	fclose(f);
	code = str;
}


//??0-1????????
void Huffman::Decode()
{
	text = "";
	if (n <= 0 || chars == NULL || code.empty())
		return;

	string::size_type i = 0;
	while (i < code.size())
	{
		bool matched = false;
		for (int j = 1; j <= n; ++j)
		{
			int codeLen = (chars[j].code == NULL) ? 0 : (int)strlen(chars[j].code);
			if (codeLen <= 0)
				continue;
			if (i + codeLen <= code.size() &&
				code.compare(i, codeLen, chars[j].code) == 0)
			{
				text += chars[j].c;
				i += codeLen;
				matched = true;
				break;
			}
		}
		if (!matched)
			break;
	}
}


//???????��????????
void Huffman::CountCharsWeight()
{
	if (text.empty())
		return;
	if (chars != NULL)
		delete[] chars;
	int i = 0;
	n = 0;
	chars = new CharMapNode[2];
	chars[0].c = 0;
	chars[0].weight = 0;
	chars[0].code = NULL;
	chars[1].code = NULL;
	chars[1].c = text[i];
	chars[1].weight = 1;
	++n;
	int total = 1;
	for (i = 1; i != text.size(); ++i)
	{
		int j;
		for (j = 1; j <= n; ++j)
		{
			if (text[i] == chars[j].c)
			{
				total++;
				++chars[j].weight;
				break;
			}
		}
		if (j > n)
		{
			total++;
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

	for(int p = 0 ; p < n+1 ; p++){
		chars[p].weight = chars [p].weight / total;
	}
}


//???????????????
void Huffman::SaveTextToFile(char *filename)
{
	ofstream outfile(filename);
	if (!outfile)
	{
		AfxMessageBox("?????????????");
		return;
	}
	outfile << text;
}

//????????��
int Huffman::FileSize(char* path){
	ifstream in(path);
	//?????????????????��??
    in.seekg(0, ios::end);
	//??????????��??
    int i = in.tellg(); 
	//??????
    in.close();
	return i;
}




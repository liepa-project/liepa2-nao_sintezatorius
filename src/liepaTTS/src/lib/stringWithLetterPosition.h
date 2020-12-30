/**
 * Zr. licenzijos faila LICENSE source medzio virsuje. (LICENSE file at the top of the source tree.)
 *
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raðtija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file StringWithLetterPosition.h
 * 
 * @author dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

#pragma once

#include <string>
using namespace std;

class stringWithLetterPosition
{
public:
	stringWithLetterPosition(char* initialBuffer, int* pLetPos, int letPosBufferMaxSize);
	~stringWithLetterPosition(void);
	string buffer;
	int letPosMaxBuffer;
	int* letPos;
	char at(const int p) const;
	int length() const;
	void append(const char* str);
	int find(const string& str) const;
	int find(const string& str, const int p) const;
	void replace(int p, int l, string str);
	void replace(int p, int l, int s, char c);
	void insert(int p, int l, char c);
	void insert(int p, char* str);
	void erase(int p, int l);
	string substr(const int p, const int l) const;
	int find_first_of(const string& str, const int p) const;
	int rfind(const char c, const int p) const;
	void set_at(const int p, const char c);
	const char* c_str() const;
private:
	void replaceLetPos(int * letPos, int pra, int ilgo, int ilgn);
};

#pragma once

#include "global.h"
#include <string>


// could changed for unicode
typedef unsigned char Char;

bool isWhiteSpaceOrLF(const Char c);
bool isNameCharacter(const Char Value);
bool isDigitCharacter(const Char Value);

bool parseLineFeed(const Char*& p);

// tab, space, LF/CR
void parseWhiteSpaceOrLF(const Char* &p);

void parseWhiteSpaceNoLF(const Char* &p);

bool parseStartsWith(const Char* &p, const char* Token);

// @return true if a return was found
bool parseToEndOfLine(const Char* &p);

// @param extraEndCharacter e.g. ','
// @return true if Out has content
bool parseLine(const Char* &p, std::string &Out, const Char extraEndCharacter = 0);

// uses C++ (variable) name definition
// @return true if Out has content
bool parseName(const Char* &p, std::string &Out);

// outValue is not changed if return is false
bool parseInt64(const Char*& p, int64 &outValue);
// outValue is not changed if return is false
bool parseInt(const Char*& p, int& outValue);
// @param str if 0 fail is used, no whitespace handling before, string parsing ends with end of integer
int64 stringToInt64(const char* str, int64 fail = -1);

// using Using KMP Algorithm (Efficient), case sensitive
const char* strstrOptimized(const char* X, const char* Y, int m, int n);
// using Using KMP Algorithm (Efficient), case insensitive
const char* stristrOptimized(const char* X, const char* Y, int m, int n);


// parse date in format %m/%d/%Y %H:%M:%S
// @return success
//bool parseDate(const Char*& p, tm &out);

template <int TSize>
struct SPushStringA
{
	// constructor
	SPushStringA() : End(dat)
	{
		*dat = 0;
	}

	// WhiteSpace are all characters <=' '
	void trimWhiteSpaceFromRight()
	{
		--End;

		while (End >= dat)
		{
			if (*End > ' ')
			{
				break;
			}

			--End;
		}

		++End;
		*End = 0;
	}

	size_t size() const
	{
		return End - dat;
	}

	char &operator[](const size_t index)
	{
		assert(index < size());
		return dat[index];
	}

	const char *c_str() const
	{
		return dat;
	}

	// @return success
	bool push(const char c)
	{
		assert(c != 0);

		if (End >= dat + TSize - 1)
		{
			assert(0);					// intended?
			return false;
		}

		*End++ = c;
		*End = 0;

		return true;
	}

	bool empty() const
	{
		return End == dat;
	}

	void pop()
	{
		assert(!empty());

		--End;
		*End = 0;
	}

	void clear()
	{
		End = dat;
		*dat = 0;
	}

private: // -----------------------------------------

	char			dat[TSize];
	// points to /0 in dat[], is always 0
	char *			End;
};


// useful for parsing HLSL to extract #include
SPushStringA<MAX_PATH> parsePath(const uint8* &p);

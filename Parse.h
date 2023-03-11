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

void parseLine(const Char* &p, std::string &Out);

// uses C++ (variable) name definition
// @return success
bool parseName(const Char* &p, std::string &Out);


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

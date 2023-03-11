//#include "stdafx.h"
#include "global.h"
#include "Parse.h"

void parseWhiteSpaceOrLF(const Char* &p)
{
	while(*p != 0 && *p <= ' ')
	{
		++p;
	}
}

void parseWhiteSpaceNoLF(const Char* &p)
{
	for(;;++p)
	{
		Char c = *p;

		if (c == 0)
		{
			break;
		}
		
		if (!(c == ' ' || c == '\t'))
		{
			break;
		}
	}
}

bool parseStartsWith(const Char* &_p, const char* Token)
{
	const Char* p = _p;
	const Char* t = (const Char*)Token;

	while(*t)
	{
		if(*p != *t)
		{
			return false;
		}
		++p;
		++t;
	}

	_p = p;
	return true;
}

// @return true if a return was found
bool parseToEndOfLine(const Char* &p)
{
	while (*p)
	{
		if (*p == 13)		// CR
		{
			++p;

			if (*p == 10)	// CR+LF
				++p;

			return true;
		}
		if (*p == 10)		// LF
		{
			++p;
			return true;
		}
		++p;
	}

	return false;
}

void parseLine(const Char* &p, std::string &Out)
{
	Out.clear();

	// can be optimized, does a lot of resize
	while(*p)
	{
		if(*p == 13)		// CR
		{
			++p;

			if (*p == 10)	// CR+LF
				++p;

			break;
		}
		if(*p == 10)		// LF
		{
			++p;
			break;
		}

		Out += *p++;
	}
}

// without Numbers
bool isNameCharacter(Char Value)
{
	return (Value >= 'a' && Value <= 'z') || (Value >= 'A' && Value <= 'Z') || Value == '_';
}

bool IsDigitCharacter(Char Value)
{
	return Value >= '0' && Value <= '9';
}

bool parseName(const Char* &p, std::string &Out)
{
	bool Ret = false;

	Out.clear();

	// can be optimized, does a lot of resize

	if (isNameCharacter(*p))
	{
		Out += *p++;
		Ret = true;
	}

	while(isNameCharacter(*p ) || IsDigitCharacter(*p))
	{
		Out += *p++;
	}

	return Ret;
}

SPushStringA<MAX_PATH> parsePath(const Char* &p)
{
	SPushStringA<MAX_PATH> Ret;

	for (;;)
	{
		Char c = *p++;

		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c == '\\' ||
			c == '/' ||
			c == ' ' ||
			c == '@' ||
			c == '.' ||
			c == '_')
		{
			Ret.push(c);
		}
		else
		{
			break;
		}
	}
	--p;

	return Ret;
}

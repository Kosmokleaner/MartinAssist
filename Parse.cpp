//#include "stdafx.h"
#include "global.h"
#include "Parse.h"

bool isWhiteSpaceOrLF(const Char c) {
	// todo: refine
	return c != 0 && c <= ' ';
}


void parseWhiteSpaceOrLF(const Char* &p)
{
	while(isWhiteSpaceOrLF(*p))
		++p;
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

bool parseLineFeed(const Char*& p) 
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
	return false;
}

void parseLine(const Char* &p, std::string &Out, bool endOnCommaAsWell)
{
	Out.clear();

	// can be optimized, does a lot of resize
	while(*p)
	{
		if(endOnCommaAsWell && *p == ',') 
		{
			++p;
			break;
		}

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

// without digits
bool isNameCharacter(const Char Value)
{
	return (Value >= 'a' && Value <= 'z') || (Value >= 'A' && Value <= 'Z') || Value == '_';
}

bool isDigitCharacter(const Char Value)
{
	return Value >= '0' && Value <= '9';
}

bool parseInt(const Char*& p, int& outValue) 
{
	int64 value64;
	bool ok = parseInt64(p, value64);
	if(ok)
		outValue = (int)value64;
	return ok;
}

bool parseInt64(const Char*& p, int64 &outValue) 
{
	const Char* Backup = p;
	bool bNegate = false;

	if (*p == '-')
	{
		bNegate = true;
		++p;
	}

	if (*p < '0' || *p > '9')
	{
		p = Backup;
		return false;
	}

	outValue = 0;

	while (*p >= '0' && *p <= '9')
	{
		outValue = outValue * 10 + (*p - '0');

		++p;
	}

	if (bNegate)
	{
		outValue = -outValue;
	}

	return true;
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

	while(isNameCharacter(*p ) || isDigitCharacter(*p))
	{
		Out += *p++;
	}

	return Ret;
}

/*
bool parseDate(const Char*& p, tm& out) 
{
	const Char* backup = p;

	int mo, d, y, h, mi, s;

	// %m/%d/%Y %H:%M:%S
	parseWhiteSpaceNoLF(p);
	if(	!parseInt(p, mo) || 
		!parseStartsWith(p, "/") ||
		!parseInt(p, d) ||
		!parseStartsWith(p, "/") ||
		!parseInt(p, y) ||
		!parseStartsWith(p, " ") ||
		!parseInt(p, h) ||
		!parseStartsWith(p, ":") ||
		!parseInt(p, mi) ||
		!parseStartsWith(p, ":") ||
		!parseInt(p, s)
		)
	{
		p = backup;
		return false;
	}
	
	out = { 0 };

	out.tm_year = y - 1900;
	out.tm_mon = mo - 1;
	out.tm_mday = d;
	out.tm_hour = h - 1;
	out.tm_min = mi;
	out.tm_sec = s;

	return true;
}
*/

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

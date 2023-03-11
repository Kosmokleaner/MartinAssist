#include "CppParser.h"

// call parseWhiteSpace before
bool parseCpp(CppParser& parser, const Char*& p)
{
    if (parsePreprocessorLine(parser, p))
        return true;

    if (parseComment(parser, p))
        return true;

    if (parseCppToken(parser, p))
        return true;

    return false;
}


bool parseComment(CppParser& parser, const Char*& p)
{
    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have

    // C++ comment, one line
    if (parseStartsWith(p, "//"))
    {
        parseWhiteSpaceNoLF(p);

        parseLine(p, parser.temp);
        parser.sink->onCommentLine(parser.temp.c_str());
        parseWhiteSpaceOrLF(p);
        return true;
    }
    // C comment, can be multi line
    if (parseStartsWith(p, "/*"))
    {
        const Char* start = p;
        const Char* lastGood = p;
        // parse until file end (not clean input) or when C comment ends
        for(;*p; ++p) {
            lastGood = p;
            if(parseLineFeed(p))
            {
                parser.temp = std::string((const char*)start, lastGood - start);
                parser.sink->onCommentLine(parser.temp.c_str());
                start = p;
                continue;
            }

            if (parseStartsWith(p, "*/"))
                break;
        }
        
        parser.temp = std::string((const char*)start, lastGood - start);
        parser.sink->onCommentLine(parser.temp.c_str());
        parseWhiteSpaceOrLF(p);
        return true;
    }
    
    return false;
}

bool parseCppToken(CppParser& parser, const Char*& p)
{
    if(parseName(p, parser.temp))
    {
        parseWhiteSpaceOrLF(p);
        return true;
    }

    // C++ keywords / operators, can be optimized
    if (parseStartsWith(p, "+") ||
        parseStartsWith(p, "-") ||
        parseStartsWith(p, "*") ||
        parseStartsWith(p, "/") ||
        parseStartsWith(p, "+") ||
        parseStartsWith(p, "-") ||
        parseStartsWith(p, "*") ||
        parseStartsWith(p, "<") ||
        parseStartsWith(p, ">") ||
        parseStartsWith(p, "(") ||
        parseStartsWith(p, ")") ||
        parseStartsWith(p, "{") ||
        parseStartsWith(p, "}") ||
        parseStartsWith(p, "!") ||
        parseStartsWith(p, "~") ||
        parseStartsWith(p, "^") ||
        parseStartsWith(p, "?") ||
        parseStartsWith(p, "%") ||
        parseStartsWith(p, "&") ||
        parseStartsWith(p, "|") ||
        parseStartsWith(p, ":") ||
        parseStartsWith(p, ",") ||
        parseStartsWith(p, ";") ||
        parseStartsWith(p, ".") ||
        parseStartsWith(p, "\\") ||
        parseStartsWith(p, "\"") ||
        parseStartsWith(p, "'") ||
        parseStartsWith(p, "=")
        )
    {
        parseWhiteSpaceOrLF(p);
        return true;
    }

    return false;
}


bool parsePreprocessorLine(CppParser& parser, const Char*& p)
{
    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have

    if (!parseStartsWith(p, "#"))
        return false;

    parseWhiteSpaceNoLF(p);

    if (parseStartsWith(p, "include")) {
        parseWhiteSpaceNoLF(p);

        if (parseStartsWith(p, "<")) {
            parseWhiteSpaceNoLF(p);

            SPushStringA<MAX_PATH> path = parsePath(p);

            if (parseStartsWith(p, ">")) {
                parser.sink->onInclude(path.c_str(), false);
                parseWhiteSpaceOrLF(p);
                return true;
            }
        }

        if (parseStartsWith(p, "\"")) {
            parseWhiteSpaceNoLF(p);

            SPushStringA<MAX_PATH> path = parsePath(p);

            if (parseStartsWith(p, "\"")) {
                parser.sink->onInclude(path.c_str(), true);
                parseWhiteSpaceOrLF(p);
                return true;
            }
        }
    }

    return false;
}
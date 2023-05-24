#include "CppParser.h"

// call parseWhiteSpace before
bool parseCpp(CppParser& parser, const Char*& p)
{
    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have

    if (parsePreprocessorLine(parser, p))
        return true;

    if (parseComment(parser, p))
        return true;

    if (parseCppToken(parser, p))
        return true;

    // parse error
//    assert(0);

    // can happen e.g. non C++ token in string or non compilable code
    ++p;
    parseWhiteSpaceOrLF(p);
    // e.g. 
    const char* challenge = "ô";

    return true;
}


// @return success
bool parseFile(CppParser& parser, const Char*& p) {
    // https://en.wikipedia.org/wiki/Byte_order_mark

    // utf-8
    if (p[0] == 0xef && p[1] == 0xbb && p[2] == 0xbf) {
        // we don't support unicode yet
        return false;
    }
    // utf-16
    if (p[0] == 0xfe && p[1] == 0xff) {
        // we don't support unicode yet
        return false;
    }
    // utf-16
    if (p[0] == 0xff && p[1] == 0xfe) {
        // we don't support unicode yet
        return false;
    }

    parseWhiteSpaceOrLF(p);

    while (*p)
    {
        assert(!isWhiteSpaceOrLF(*p));

        bool ok = parseCpp(parser, p);
        // check *p what the parse wasn't able to consume, todo: error message
        assert(ok);

        assert(!isWhiteSpaceOrLF(*p)); // last function should have called parseWhiteSpace

        parseWhiteSpaceOrLF(p);
        assert(!isWhiteSpaceOrLF(*p));
    }

    return true;
}

bool parseComment(CppParser& parser, const Char*& p)
{
    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have

    // C++ comment, one line
    if (parseStartsWith(p, "//"))
    {
        parseWhiteSpaceNoLF(p);

        parseLine(p, parser.temp, ',');
        parser.sink->onCommentLine((const Char *)parser.temp.c_str());
        parseWhiteSpaceOrLF(p);
        return true;
    }
    // C comment, can be multi line
    if (parseStartsWith(p, "/*"))
    {
        const Char* start = p;
        const Char* lastGood = p;
        // parse until file end (not clean input) or when C comment ends
        for(;*p;) {
            lastGood = p;
            if(parseLineFeed(p))
            {
                parser.temp = std::string((const char*)start, lastGood - start);
                parser.sink->onCommentLine((const Char*)parser.temp.c_str());
                start = lastGood = p;
            }
            else if (parseStartsWith(p, "*/")) 
            {
                break;
            } 
            else ++p;
        }
        
        if(lastGood != start)
        {
            parser.temp = std::string((const char*)start, lastGood - start);
            parser.sink->onCommentLine((const Char*)parser.temp.c_str());
        }
        parseWhiteSpaceOrLF(p);
        return true;
    }
    
    return false;
}

bool parseNameOrNumber(const Char*& p)
{
    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have

    bool ret = false;

    while (isNameCharacter(*p) || isDigitCharacter(*p))
    {
        ++p;
        ret = true;
    }

    parseWhiteSpaceOrLF(p);

    return ret;
}

bool parseCppToken(CppParser& parser, const Char*& p)
{
    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have

    if(parseNameOrNumber(p))
        return true;

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
        parseStartsWith(p, "[") ||
        parseStartsWith(p, "]") ||
        parseStartsWith(p, "{") ||
        parseStartsWith(p, "}") ||
        parseStartsWith(p, "!") ||
        parseStartsWith(p, "~") ||
        parseStartsWith(p, "`") || // is this C++?
        parseStartsWith(p, "@") || // is this C++?
        parseStartsWith(p, "$") || // is this C++?
        parseStartsWith(p, "^") ||
        parseStartsWith(p, "?") ||
        parseStartsWith(p, "%") ||
        parseStartsWith(p, "&") ||
        parseStartsWith(p, "#") ||
        parseStartsWith(p, "|") ||
        parseStartsWith(p, ":") ||
        parseStartsWith(p, ",") ||
        parseStartsWith(p, ";") ||
        parseStartsWith(p, ".") ||
        parseStartsWith(p, "\\") ||
        parseStartsWith(p, "\"") ||
        parseStartsWith(p, "'") ||
        parseStartsWith(p, "="))
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

    parseWhiteSpaceOrLF(p);

    // from MAX_PATH
    const uint32_t maxPath = 260;

    // outside scope for easier debugging
    SPushStringA<maxPath> path;

    if (parseStartsWith(p, "include")) {
        parseWhiteSpaceOrLF(p);

        if (parseStartsWith(p, "<")) {
            parseWhiteSpaceOrLF(p);

            SPushStringA<maxPath> path = parsePath(p);

            if (parseStartsWith(p, ">")) {
                parser.sink->onInclude(path.c_str(), false);
                parseWhiteSpaceOrLF(p);
                return true;
            }
        }

        if (parseStartsWith(p, "\"")) {
            parseWhiteSpaceOrLF(p);

            path = parsePath(p);

            if (parseStartsWith(p, "\"")) {
                parser.sink->onInclude(path.c_str(), true);
                parseWhiteSpaceOrLF(p);
                return true;
            }
        }
    }

    assert(!isWhiteSpaceOrLF(*p)); // call parseWhiteSpace before or last function should have
    return false;
}
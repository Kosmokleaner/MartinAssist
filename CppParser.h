#pragma once

#include "Parse.h"


struct ICppParserSink
{
    virtual ~ICppParserSink() {}

    // @param path must not be 0 
    virtual void onInclude(const char* path, bool local) = 0;
    // @param line must not be 0 
    virtual void onCommentLine(const char* line) = 0;
};

class CppParser {
public:
    CppParser() {}

    ICppParserSink* sink = {};
    std::string temp;
};


// @return true if progress was made
bool parseComment(CppParser& parser, const Char*& p);
// @return true if progress was made
bool parsePreprocessorLine(CppParser& parser, const Char*& p);
// @return true if progress was made
bool parseCppToken(CppParser& parser, const Char*& p);
// call parseWhiteSpace before
bool parseCpp(CppParser& parser, const Char*& p);
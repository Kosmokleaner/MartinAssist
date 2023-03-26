#pragma once
#include "CppParser.h"


class LiterateProcessor : public ICppParserSink
{
public:

    // interface ICppParserSink

    virtual void onInclude(const char* path, bool local);
    virtual void onCommentLine(const char* line);

    //

    // @param inputFile must not be 0
    void addComponents(const wchar_t* inputFile);
    // @param outputFile must not be 0
    void build(const wchar_t* outputFile);
};


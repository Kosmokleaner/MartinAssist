#pragma once
#include <map>
#include <string>
#include "CppParser.h"

struct LiterateTreeNode {
    const Char* start = 0;
    const Char* end = 0;
};


class LiterateProcessor : public ICppParserSink
{
    // [label] = node, root label is ""
    std::multimap<std::string, LiterateTreeNode> nameToNode;
    // was started with #begin
    std::string currentLabel;

public:

    // interface ICppParserSink

    virtual void onInclude(const char* path, bool local);
    virtual void onCommentLine(const Char* line);

    //

    LiterateProcessor();

    // @param inputFile must not be 0
    void addComponents(const wchar_t* inputFile);
    // @param outputFile must not be 0
    void build(const wchar_t* outputFile);
};


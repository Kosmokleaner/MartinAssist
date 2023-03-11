#include <iostream>
#include "ASCIIFile.h"
#include "Parse.h"

struct ICppParserSink
{
    virtual ~ICppParserSink() {}

    virtual void onInclude(const char* path, bool local) = 0;
};

class CppParser {
public:
    CppParser() {}

    ICppParserSink* sink = {};
};


// call parseWhiteSpace before
void parsePreprocessorLine(CppParser& parser, const Char* &p)
{
    if (!parseStartsWith(p, "#"))
        return;

    parseWhiteSpaceNoLF(p);

    if (parseStartsWith(p, "include")) {
        parseWhiteSpaceNoLF(p);

        if (parseStartsWith(p, "<")) {
            parseWhiteSpaceNoLF(p);

            SPushStringA<MAX_PATH> path = parsePath(p);

            if (parseStartsWith(p, ">")) {
                parser.sink->onInclude(path.c_str(), false);
                return;
            }
        }

        if (parseStartsWith(p, "\"")) {
            parseWhiteSpaceNoLF(p);

            SPushStringA<MAX_PATH> path = parsePath(p);

            if (parseStartsWith(p, "\"")) {
                parser.sink->onInclude(path.c_str(), true);
                return;
            }
        }
    }
}

struct CppParserSink : public ICppParserSink
{
    virtual void onInclude(const char* path, bool local) {
        printf("%s include found: <%s>\n", local ? "local" : "global", path);
    }
};

int main()
{
    CASCIIFile file;

    file.IO_LoadASCIIFile("MartinAssist.cpp");

    const Char *p = (Char*)file.GetDataPtr();
    
    CppParser parser;
    CppParserSink sink;
    parser.sink = &sink;

    while(*p) 
    {
        parseWhiteSpaceOrLF(p);

        parsePreprocessorLine(parser, p);
        parseToEndOfLine(p);
    }

} 

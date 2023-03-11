#include <iostream>
#include "ASCIIFile.h"
#include "CppParser.h"  // this is only include needed to get the cpp parser

/* C style comments */
struct CppParserSink : public ICppParserSink
{
    virtual void onInclude(const char* path, bool local) {
        printf("%s include: '%s'\n", local ? "local" : "global", path);
    }
    virtual void onCommentLine(const char* line) {
        printf("comment line: '%s'\n", line);
    }
};

/* This is the main function
   ** just a few parser confusing constructs / *** *  * / \/ * 
   this is the third line */
/* fourth line */
int main()
{
    CASCIIFile file;

    file.IO_LoadASCIIFile("MartinAssist.cpp");

    const Char *p = (Char*)file.GetDataPtr();
    
    CppParser parser;
    CppParserSink sink;
    parser.sink = &sink;

    parseWhiteSpaceOrLF(p);

    while(*p) 
    {
        assert(!isWhiteSpaceOrLF(*p));

        bool ok = parseCpp(parser, p);
        // check *p what the parse wasn't able to consume, todo: error message
        assert(ok);

        assert(!isWhiteSpaceOrLF(*p)); // last function should have called parseWhiteSpace

        parseWhiteSpaceOrLF(p);
        assert(!isWhiteSpaceOrLF(*p));
    }

} 

#include <iostream>
#include "ASCIIFile.h"
#include "Parse.h"

// call parseWhiteSpace before
void parsePreprocessorLine(const Char* &p)
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
                printf("global include found: <%s>\n", path.c_str());
                return;
            }
        }

        if (parseStartsWith(p, "\"")) {
            parseWhiteSpaceNoLF(p);

            SPushStringA<MAX_PATH> path = parsePath(p);

            if (parseStartsWith(p, "\"")) {
                printf("local include found: \"%s\"\n", path.c_str());
                return;
            }
        }
    }

}

int main()
{
    CASCIIFile file;

    file.IO_LoadASCIIFile("MartinAssist.cpp");

    const Char *p = (Char*)file.GetDataPtr();
    
    while(*p) 
    {
        parseWhiteSpaceOrLF(p);

        parsePreprocessorLine(p);
        parseToEndOfLine(p);
    }

} 

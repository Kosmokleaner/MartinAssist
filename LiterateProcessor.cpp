#include "LiterateProcessor.h"
#include "ASCIIFile.h"
#include "Parse.h"
#include "FileSystem.h"

void LiterateProcessor::addComponents(const wchar_t* inputFile) {
    assert(inputFile);

    CASCIIFile fileHnd;

    // todo: wasteful allocations
    fileHnd.IO_LoadASCIIFile(to_string(inputFile).c_str());
    const Char* pStart = (Char*)fileHnd.GetDataPtr();
    const Char* p = pStart;
}

void LiterateProcessor::build(const wchar_t* outputFile) {
    CASCIIFile fileHnd;
    
    char *dat = (char*)malloc(1);
    dat[0] = 0;
    fileHnd.CoverThisData(dat, 1);

    // todo: wasteful allocations
    fileHnd.IO_SaveASCIIFile(to_string(outputFile).c_str());
}

void LiterateProcessor::onInclude(const char* path, bool local) {
    printf("%s include: '%s'\n", local ? "local" : "global", path);
}

void LiterateProcessor::onCommentLine(const char* line) {
    printf("comment line: '%s'\n", line);
}


#include "LiterateProcessor.h"
#include "ASCIIFile.h"
#include "Parse.h"
#include "FileSystem.h"

void LiterateProcessor::addComponents(const wchar_t* inputFile) {
    assert(inputFile);

    // could be optimized
    std::wstring ext = FilePath(inputFile).GetExtension();
    // todo: need case insensitive test
    if (ext != L"cpp" && ext != L"h" && ext != L"hlsl") {
        // todo: better error handling
        return;
    }

    CASCIIFile fileHnd;

    // todo: wasteful allocations
    fileHnd.IO_LoadASCIIFile(to_string(inputFile).c_str());
    const Char* pStart = (Char*)fileHnd.GetDataPtr();
    const Char* p = pStart;

    CppParser parser;
    parser.sink = this;

    parseFile(parser, p);
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
}

void LiterateProcessor::onCommentLine(const Char* line) {
    const Char* p = line;

    parseWhiteSpaceOrLF(p);

    if (!parseStartsWith(p, "#"))
        return;

    parseWhiteSpaceOrLF(p);

    if (parseStartsWith(p, "begin") && isWhiteSpaceOrLF(*p)) {
        parseWhiteSpaceOrLF(p);
        printf("begin %s\n", p);
        return;
    }

    if (parseStartsWith(p, "end") && isWhiteSpaceOrLF(*p)) {
        parseWhiteSpaceOrLF(p);
        printf("end %s\n", p);
        return;
    }

    if (parseStartsWith(p, "anchor") && isWhiteSpaceOrLF(*p)) {
        parseWhiteSpaceOrLF(p);
        printf("anchor %s\n", p);
        return;
    }
}


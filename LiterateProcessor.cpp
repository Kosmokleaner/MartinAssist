#include "LiterateProcessor.h"
#include "ASCIIFile.h"
#include "Parse.h"
#include "FileSystem.h"
#include "LiterateProcessor.h"

LiterateProcessor::LiterateProcessor() {
    // root node
    nameToNode.insert(std::pair<std::string, LiterateTreeNode>("", LiterateTreeNode()));
}

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

    // for debugging
    printf("inputFile: '%s'\n", to_string(inputFile).c_str());

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
        printf("  begin %s\n", p);
        // todo: wasteful memory alloc
        std::string label = (const char*)p;
        // todo: better error handling
//        assert(nameToNode.find(label) == nameToNode.end());
        LiterateTreeNode node;
        nameToNode.insert(std::pair<std::string, LiterateTreeNode>(label, node));
        currentLabel = label;
        return;
    }

    if (parseStartsWith(p, "end") && isWhiteSpaceOrLF(*p)) {
        parseWhiteSpaceOrLF(p);
        printf("  end %s\n", p);
        // todo: wasteful memory alloc
        std::string label = (const char*)p;
        if(currentLabel != label) {
            // todo: better error handling
            printf("  error: #end %s is not matching with last #start %s\n", p, currentLabel.c_str());
            assert(0);
            return;
        }
        // todo: better error handling
        assert(nameToNode.find(label) != nameToNode.end());
        currentLabel.clear();
        return;
    }

    if (parseStartsWith(p, "anchor") && isWhiteSpaceOrLF(*p)) {
        parseWhiteSpaceOrLF(p);
        printf("  anchor %s\n", p);
        // todo: wasteful memory alloc
        std::string label = (const char*)p;
        // todo: better error handling
//        assert(nameToNode.find(label) == nameToNode.end());
//        LiterateTreeNode node;
//        nameToNode.insert(std::pair<std::string, LiterateTreeNode>(label, node));
        return;
    }
}


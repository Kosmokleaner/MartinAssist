#include <iostream>
#include "ASCIIFile.h"
#include "CppParser.h"  // this is only include needed to get the cpp parser
#include "FileSystem.h"

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

void parseFile(CppParser& parser, const Char* &p) {
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
}

struct DirectoryTraverse : public IDirectoryTraverse {

    CppParser parser;
    uint32 fileCount = 0;

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory) {
        return true;
    }

    virtual void OnFile(const FilePath& path, const wchar_t* file) {
        // todo: static to avoid heap allocations, prevents multithreading use
        static FilePath local; local.path.clear();

        local = path;
        local.Normalize();
        local.Append(file);

        // could be optimized
        std::wstring ext = local.GetExtension();
        if(ext != L"cpp" && ext != L"h")
            return;

        CASCIIFile fileHnd;

        fileHnd.IO_LoadASCIIFile(to_string(local.path.c_str()).c_str());
        const Char* pStart = (Char*)fileHnd.GetDataPtr();
        const Char* p = pStart;

        // https://en.wikipedia.org/wiki/Byte_order_mark

        // utf-8
        if(fileHnd.GetDataSize() >= 3 && p[0] == 0xef && p[1] == 0xbb && p[2] == 0xbf) {
            // we don't support unicode yet
            return;
        }
        // utf-16
        if (fileHnd.GetDataSize() >= 2 && p[0] == 0xfe && p[1] == 0xff) {
            // we don't support unicode yet
            return;
        }
        // utf-16
        if (fileHnd.GetDataSize() >= 2 && p[0] == 0xff && p[1] == 0xfe) {
            // we don't support unicode yet
            return;
        }

        parseFile(parser, p);
        ++fileCount;
    }
};

/* This is the main function
   ** just a few parser confusing constructs / *** *  * / \/ * 
   this is the third line */
/* fourth line */
int main()
{
    CppParserSink cppSink;

    DirectoryTraverse traverse;

    traverse.parser.sink = &cppSink;

//    directoryTraverse(traverse, L"C:\\P4Depot\\QuickGame Wiggle");
    directoryTraverse(traverse, L"C:\\P4Depot");

    printf("FileCount: %d\n", traverse.fileCount);

//    parser.sink = &sink;
    //    file.IO_LoadASCIIFile("MartinAssist.cpp");
//    file.IO_LoadASCIIFile("C:\\P4Depot\\QuickGame Wiggle\\GameLevel.cpp"); // 60KB wokrs
//    file.IO_LoadASCIIFile("C:\\P4Depot\\QuickGame Wiggle\\mathlib.h"); // 52 KB
//    file.IO_LoadASCIIFile("C:\\P4Depot\\QuickGame Wiggle\\WorldUnit.h"); // 50 KB
//   parseFile(parser, (Char*)file.GetDataPtr());   

    printf("\n\n\n");
}

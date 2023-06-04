#include "ASCIIFile.h"
#include "LiterateProcessor.h"
#include "CppParser.h"  // this is only include needed to get the cpp parser
#include "FileSystem.h"
#include "Timer.h"
#include "CppParserTest.h"
#include <assert.h>


/* C style comments */
struct CppParserSink : public ICppParserSink
{
    virtual void onInclude(const char* path, bool local) {
        printf("%s include: '%s'\n", local ? "local" : "global", path);
    }
    virtual void onCommentLine(const Char* line) {
        printf("comment line: '%s'\n", line);
    }
};

struct CppDirectoryTraverse : public IDirectoryTraverse {

    CppParser parser;
    uint32 fileEntryCount = 0;
    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;

    CppDirectoryTraverse() {
        startTime = g_Timer.GetAbsoluteTime();
    }

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData) {
        // to prevent compiler warning
        filePath;directory;findData;


        return true;
    }

    virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData) {
        // to prevent compiler warning
        findData;

        // todo: static to avoid heap allocations, prevents multithreading use
        static FilePath local; local.path.clear();

        local = path;
        local.Normalize();
        local.Append(file);

        // could be optimized
        std::wstring ext = local.GetExtension();
        // todo: need case insensitive test
        if (ext != L"cpp" && ext != L"h" && ext != L"hlsl")
            return;

        CASCIIFile fileHnd;

        fileHnd.IO_LoadASCIIFile(to_string(local.path.c_str()).c_str());
        const Char* pStart = (Char*)fileHnd.GetDataPtr();
        const Char* p = pStart;

        if (parseFile(parser, p)) {
            size += fileHnd.GetDataSize();
            ++fileEntryCount;
        }
    }
};

void check(bool a) 
{
    assert(a);
    if(!a)
        __debugbreak();
}

/* This is the main function
   ** just a few parser confusing constructs / *** *  * / \/ *
   this is the third line */
   /* fourth line */
void CppParserTest() 
{
    {
        int32 line = -1, col = -1;

        // todo: test in release as well
        const Char *file = (const Char*)
            "// start\n"                // 0
            "float x;\n"                // 9
            "\t\tAfter 2 Tabs\n"        // 18
            "a special return\r"        // 33
            "a different return\r\n"    // 50
            "EOF"                       // 70
            ;                           // 73

        // start
        check(computeLocationInFile(file, file + 0, line, col));
        check(line == 1 && col == 1);
        // 2 characters in
        check(computeLocationInFile(file, file + 2, line, col));
        check(line == 1 && col == 3);
        // end of line 1
        check(computeLocationInFile(file, file + 8, line, col));
        check(line == 1 && col == 9);
        // line 2
        check(computeLocationInFile(file, file + 9, line, col));
        check(line == 2 && col == 1);
        // line 3
        check(computeLocationInFile(file, file + 18, line, col));
        check(line == 3 && col == 1);
        // after one tab
        check(computeLocationInFile(file, file + 19, line, col));
        check(line == 3 && col == 5);
        // after two tabs
        check(computeLocationInFile(file, file + 20, line, col));
        check(line == 3 && col == 9);
        // line 4
        check(computeLocationInFile(file, file + 33, line, col));
        check(line == 4 && col == 1);
        // line 5 a special return
        check(computeLocationInFile(file, file + 50, line, col));
        check(line == 5 && col == 1);
        // line 6 a different return
        check(computeLocationInFile(file, file + 70, line, col));
        check(line == 6 && col == 1);
        // line 7
        check(computeLocationInFile(file, file + 73, line, col));
        check(line == 6 && col == 4);
        // End of file
        check(!computeLocationInFile(file, file + 74, line, col));
        check(line == 0 && col == 0);
    }


    LiterateProcessor literateProcessor;
    // add all components in this file and apply to existing tree
    literateProcessor.addComponents(L"example0.cpp");
    literateProcessor.build(L"LiterateOutput.cpp");

/*
    CppParserSink cppSink;

    DirectoryTraverse traverse;

    traverse.parser.sink = &cppSink;

    //    directoryTraverse(traverse, L"C:\\P4Depot\\QuickGame Wiggle");
    directoryTraverse(traverse, L"C:\\P4Depot");
    //    directoryTraverse(traverse, L"D:"); // 147984 files, 1.4GB, 1035 sec debug with printf, 127sec Release without printf 11.04MB/sec
    //    directoryTraverse(traverse, L""); // test source files of this application

        // in seconds
    double time = g_Timer.GetAbsoluteTime() - traverse.startTime;

    printf("\n");

    printf("FileCount: %d\n", traverse.fileCount);
    printf("Size: %.2f MB\n", traverse.size / (1024.0f * 1024.0f));
    printf("Time: %.2f sec\n", time);
    printf("MB/sec: %.2f MB\n", traverse.size / time / (1024.0f * 1024.0f));
*/
    // 3055 files 58MB
    // Debug with printf 3.37MB/s
    // Release with printf 4.32MB/s
    // Release without printf 19.67 MB/s

    // single file: 
//    CppParser sink;
//    parser.sink = &sink;
//    file.IO_LoadASCIIFile("MartinAssist.cpp");
//    file.IO_LoadASCIIFile("C:\\P4Depot\\QuickGame Wiggle\\GameLevel.cpp"); // 60KB wokrs
//    file.IO_LoadASCIIFile("C:\\P4Depot\\QuickGame Wiggle\\mathlib.h"); // 52 KB
//    file.IO_LoadASCIIFile("C:\\P4Depot\\QuickGame Wiggle\\WorldUnit.h"); // 50 KB
//   parseFile(parser, (Char*)file.GetDataPtr());   
}

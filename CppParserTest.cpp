#include "ASCIIFile.h"
#include "LiterateProcessor.h"
#include "CppParser.h"  // this is only include needed to get the cpp parser
#include "FileSystem.h"
#include "Timer.h"
#include "CppParserTest.h"


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

struct DirectoryTraverse : public IDirectoryTraverse {

    CppParser parser;
    uint32 fileCount = 0;
    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;

    DirectoryTraverse() {
        startTime = g_Timer.GetAbsoluteTime();
    }

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
        // todo: need case insensitive test
        if (ext != L"cpp" && ext != L"h" && ext != L"hlsl")
            return;

        CASCIIFile fileHnd;

        fileHnd.IO_LoadASCIIFile(to_string(local.path.c_str()).c_str());
        const Char* pStart = (Char*)fileHnd.GetDataPtr();
        const Char* p = pStart;

        if (parseFile(parser, p)) {
            size += fileHnd.GetDataSize();
            ++fileCount;
        }
    }
};

void CppParserTest() 
{
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




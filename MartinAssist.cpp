#include <iostream>
#include "CppParserTest.h"
#include "EveryHere.h"
#include "Gui.h"

// todo: move
#include "Timer.h"
struct DirectoryTraverseTest : public IDirectoryTraverse {

    uint32 fileEntryCount = 0;
    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;

    DirectoryTraverseTest() {
        startTime = g_Timer.GetAbsoluteTime();
    }

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData) {
        // to prevent compiler warning
        filePath; directory; findData;
        return true;
    }

    virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData, float progress) 
    {
        // to prevent compiler warning
        path; file; findData; progress;

        ++fileEntryCount;
    }
};


// depth first (memory efficient)
// @param filePath e.g. L"c:\\temp" L"relative/Path" L""
// @param pattern e.g. L"*.cpp" L"*.*", must not be 0
void directoryTraverseMT(IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern = L"*.*")
{
    // to prevent compiler warning
    sink; filePath; pattern;

}



int main()
{
//    CppParserTest();

    // experiments:

    // OpenGL3 ImGui 
    Gui gui;
    gui.test();

    // drive / folder iterator to dump directory in a file
//    EveryHere everyHere;
    // load input .csv and write into test.csv to verfiy load/save works
//   everyHere.loadCSV(L"Volume{720f86b8-373a-4fe4-ae1b-ef58cb9dd578}.csv");
    // Generate one or more .csv files
//    everyHere.gatherData();

    printf("\n\n\n");
}

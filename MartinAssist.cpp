#include <iostream>
#include "CppParserTest.h"

#include <map>
#include "ASCIIFile.h"
#include "FileSystem.h"
#include "Timer.h"

struct FileKey
{
    // without path (:, / and \)
    std::wstring fileName;
    bool operator<(const FileKey& b) const {
        return fileName < b.fileName;
    }

    // properties making this file unique
    __time64_t time_write = -1;  // modified
    //
    _fsize_t size = 0;
};

struct FileValue
{
//todo    device id;
    std::wstring path;
    __time64_t time_create = -1;    // -1 for FAT file systems
    __time64_t time_access = -1;    // -1 for FAT file systems
};

struct DirectoryTraverse : public IDirectoryTraverse {

    std::map<FileKey, FileValue> files;

    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;

    DirectoryTraverse() {
        startTime = g_Timer.GetAbsoluteTime();
    }
    ~DirectoryTraverse() 
    {
        for(auto it : files) 
        {
            struct tm tm;
            _localtime64_s(&tm, &it.first.time_write);
            char timeStr[80];
            strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %H:%M:%S", &tm);

            printf("%s %8d %s | %s\n",
                timeStr,
                it.first.size,
                to_string(it.first.fileName.c_str()).c_str(),
                to_string(it.second.path.c_str()).c_str()
            );
        }
    }

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory) {
        return true;
    }

    virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData) {
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

        FileKey el;
        el.fileName = file;
        el.size = findData.size;
        el.time_write = findData.time_write;
        FileValue val;
        val.path = path.path;
        val.time_access = findData.time_access;
        val.time_create = findData.time_create;
        files.insert(std::pair<FileKey, FileValue>(el, val));
    }
};

int main()
{
//    CppParserTest();

    DirectoryTraverse traverse;

    directoryTraverse(traverse, L"C:\\P4Depot\\QuickGame Wiggle");
//    directoryTraverse(traverse, L"C:\\P4Depot");


    printf("\n\n\n");
}

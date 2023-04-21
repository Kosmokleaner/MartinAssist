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
        if(time_write < b.time_write)
            return true;
        if (time_write > b.time_write)
            return false;

        if (size < b.size)
            return true;
        if (size > b.size)
            return false;

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

    std::multimap<FileKey, FileValue> files;

    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;
    uint64 fileCount = 0;
    uint64 uniqueCount = 0;

    DirectoryTraverse() {
        startTime = g_Timer.GetAbsoluteTime();
    }
    ~DirectoryTraverse() 
    {
        save("driveData.csv");
        print();
    }

    // @param fileName e.g. "test.csv"
    void save(const char *fileName)
    {
        CASCIIFile file;
        std::string fileData;
        // to avoid reallocations
        fileData.reserve(10 * 1024 * 1024);

        for (auto it : files)
        {
            size_t count = files.count(it.first);
            // show only duplicates
            if (count == 1)
                continue;

            struct tm tm;
            _localtime64_s(&tm, &it.first.time_write);
            char dateTimeStr[80];
            // https://support.echo360.com/hc/en-us/articles/360035034372-Formatting-Dates-for-CSV-Imports#:~:text=Some%20of%20the%20CSV%20imports,program%20like%20Microsoft%C2%AE%20Excel.
            strftime(dateTimeStr, sizeof(dateTimeStr), "%Y/%m/%d %H:%M:%S", &tm);

            char str[MAX_PATH + 100];
            sprintf_s(str, sizeof(str), "%s,%d,%s,%s\n",
                dateTimeStr,
                it.first.size,
                to_string(it.first.fileName.c_str()).c_str(),
                to_string(it.second.path.c_str()).c_str()
            );
            fileData += str;
        }
//        fileData += "\n";
//        logState();
//        fileData += "\n";
        // does not include 0 termination
        size_t len = fileData.length();
        // wasteful
        const char* cpy = (const char*)malloc(len + 1);
        memcpy((void*)cpy, fileData.c_str(), len + 1);
        file.CoverThisData(cpy, len);
        file.IO_SaveASCIIFile(fileName);
    }

    void print() 
    {
        for (auto it : files)
        {
            size_t count = files.count(it.first);
            // show only duplicates
            if (count == 1)
                continue;

            struct tm tm;
            _localtime64_s(&tm, &it.first.time_write);
            char timeStr[80];
            strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %H:%M:%S", &tm);

            printf("#%d %s %8d %s | %s\n",
                (int)count,
                timeStr,
                it.first.size,
                to_string(it.first.fileName.c_str()).c_str(),
                to_string(it.second.path.c_str()).c_str()
            );
        }
        printf("\n");
        logState();
        printf("\n");
    }

    void logState()
    {
        printf("%lld files %lld dups  %.2f sec\n", 
            fileCount, 
            fileCount - uniqueCount,
            g_Timer.GetAbsoluteTime() - startTime);
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

        if(files.find(el) == files.end())
            ++uniqueCount;

        files.insert(std::pair<FileKey, FileValue>(el, val));
        ++fileCount;

        if((fileCount % 100) == 0)
            logState();
    }
};



struct DriveTraverse : public IDriveTraverse {

    virtual void OnDrive(const FilePath& filePath, const wchar_t* deviceName, const wchar_t* internalName, const wchar_t* volumeName) {
        wprintf(L"filePath: %s\n", filePath.path.c_str());
        wprintf(L"deviceName: %s\n", deviceName);
        wprintf(L"internalName: %s\n", internalName);
        wprintf(L"volumeName: %s\n\n", volumeName);

    }
};




int main()
{
    CppParserTest();

//    DriveTraverse drives;
//    driveTraverse(drives);

//    DirectoryTraverse traverse;
//    directoryTraverse(traverse, L"C:\\P4Depot\\QuickGame Wiggle");
//    directoryTraverse(traverse, L"C:\\P4Depot");    // ~3000 files
//    directoryTraverse(traverse, L"C:");    // 199891 files  100 sec 78 sec time release/debug

    printf("\n\n\n");
}

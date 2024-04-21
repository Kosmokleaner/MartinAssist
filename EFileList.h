#pragma once
#include "PooledString.h"
#include "types.h"

// see https://www.voidtools.com/support/everything/file_lists/#:~:text=Everything%20File%20List%20(EFU)%20are,%2C%20sizes%2C%20dates%20and%20attributes

struct FileKey
{
    // without path (:, / and \)
    // UTF8 as IMGUI can print it faster, less conversions during loading and less memory
    PooledString fileName;

    bool operator<(const FileKey& b) const {
        // first by size to find large ones first
        if (size > b.size)
            return true;
        if (size < b.size)
            return false;

        // then by file name
//        int c = wcscmp(fileName.c_str(), b.fileName.c_str());
        int c = strcmp(fileName.c_str(), b.fileName.c_str());
        if (c < 0)
            return true;
        else if (c > 0)
            return false;

        // then by write time to find same files, not as good as hash but
        // if not written on same second or copied with a bad tool it should be ok
        if (time_write < b.time_write)
            return true;
        if (time_write > b.time_write)
            return false;

        return false;
    }

    // properties making this file unique
    __time64_t time_write = -1;  // modified
    // file size in bytes
    int64 size = 0;
};

struct FileValue
{
    // -1 is used for root, otherwise fileEntryIndex
    int64 parent = -1;
    // -1 for FAT file systems
    __time64_t time_create = -1;
    // -1 for FAT file systems
//    __time64_t time_access = -1;
    PooledString path;
    // -1 means not part of a drive yet
    int driveId = -1;
};

struct FileEntry
{
    FileKey key;
    FileValue value;
};

class EFileList
{
    StringPool stringPool;

    std::vector<FileEntry> entries;
public:

    // 1.7ms for 3.2MB file on Ryzen in Release
    // @param must not be 0 
    // @return success
    bool load(const wchar_t* fileName);

    static void test();
};


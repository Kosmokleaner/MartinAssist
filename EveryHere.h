#pragma once
#include "global.h"
#include <map>
#include <vector>
#include <string>
#include "FileSystem.h"
#include "SelectionRange.h"


// ever increasing integer in quotes, it's actually a string
#define SERIALIZE_VERSION "3"

struct FileKey
{
    // without path (:, / and \)
    // UTF8 as IMGUI can print it faster, less conversions during loading and less memory
    std::string fileName;

    bool operator<(const FileKey& b) const {
        // first by size to find large ones first
        if (sizeOrFolder > b.sizeOrFolder)
            return true;
        if (sizeOrFolder < b.sizeOrFolder)
            return false;

        // then by file name
//        int c = wcscmp(fileName.c_str(), b.fileName.c_str());
        int c = strcmp(fileName.c_str(), b.fileName.c_str());
        if(c < 0)
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
    // file size in bytes, -1 for folder
    int64 sizeOrFolder = 0;
};

struct FileValue
{
    // -1 is used for root
    int64 parent = -1;
    __time64_t time_create = -1;    // -1 for FAT file systems
    __time64_t time_access = -1;    // -1 for FAT file systems
    int deviceId = -1;
};

struct FileEntry
{
    FileKey key;
    FileValue value;
};

struct DeviceData {
    std::vector<FileEntry> entries;
    // index in EveryHere.deviceData
    int deviceId = -1;
    // e.g. L"Volume{41122dbf-6011-11ed-1232-04d4121124bd}"
    std::wstring cleanName;
    // e.g. L"First Drive"
    std::wstring volumeName;
    // e.g. L"C:\"
    std::wstring drivePath;
    // call computeStats() to update
    uint64 statsSize = 0;
    // call computeStats() to update, number of directories
    uint64 statsDirs = 0;

    DeviceData();

    void sort();

    // @param fileName e.g. L"test.csv"
    // @param drivePath may be 0, e.g. L"C:\"
    // @param volumeName may be 0
    // @param cleanName may be 0, e.g. L"First Drive"
    void save(const wchar_t* fileName, const wchar_t* drivePath = 0, const wchar_t* volumeName = 0);

    // update statsSize, statsDirs
    void computeStats();

    void verify();
};

struct ViewEntry
{
    // index in deviceData[]
    int32 deviceId = -1;
    // index in deviceData[deviceId].entries
    int64 fileEntryId = -1;
};


struct EveryHere
{
    // [deviceId] = DeviceData
    std::vector<DeviceData> deviceData;

    // built by buildView();
    std::vector<ViewEntry> view;
    int64 viewSumSize = 0;

    void gatherData();
    // @param internalName must not be null, e.g. L"Volume{41122dbf-6011-11ed-1232-04d4121124bd}"
    // @return success
    bool loadCSV(const wchar_t* internalName);

    void buildView(const char* filter, int64 minSize, SelectionRange& deviceSelectionRange);

    void freeData();
};

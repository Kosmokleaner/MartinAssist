#pragma once
#include <map>
#include <string>


struct FileKey
{
    // without path (:, / and \)
    std::wstring fileName;
    bool operator<(const FileKey& b) const {
        if (time_write < b.time_write)
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
    // file size in bytes
    _fsize_t size = 0;
};

struct FileValue
{
    //todo    device id;
    std::wstring path;
    __time64_t time_create = -1;    // -1 for FAT file systems
    __time64_t time_access = -1;    // -1 for FAT file systems
};

struct DeviceData {
    std::multimap<FileKey, FileValue> files;

    // @param fileName e.g. L"test.csv"
    void save(const wchar_t* fileName);

    void printUniques();
};


struct EveryHere
{
    // [deviceName] = 
    std::map<std::wstring, DeviceData> devices;
//public:
    void GatherData();
};

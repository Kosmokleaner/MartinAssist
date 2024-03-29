#pragma once
#include "global.h"
#include <map>
#include <set>
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
    std::string path;
    int driveId = -1;
};

struct FileEntry
{
    FileKey key;
    FileValue value;
};

enum FileColumnID
{
    FCID_Name,
    FCID_Size,
    FCID_Redundancy,
    FCID_DriveId,
    FCID_Path,
};

// need to match DriveColumnNames
enum DriveColumnID
{
    DCID_VolumeName,
    DCID_UniqueName,
    DCID_Path,
    DCID_DriveId,
    DCID_Size,
    DCID_Files,
    DCID_Directories,
    DCID_Computer,
    DCID_User,
    DCID_Date,
    DCID_totalSpace,
    DCID_type,
    DCID_serial,
    DCID_selectedFiles,
    //
    DCID_MAX,
};
// need to match DriveColumnID
inline const char* getDriveColumnName(DriveColumnID e)
{
    const char* names[] = {
        "VolumeName",
        "UniqueName",
        "Path",
        "DriveId",
        "Size",
        "Files",
        "Directories",
        "Computer",
        "User",
        "Date",
        "totalSpace",
        "type",
        "serial",
        "selectedFiles",
    };
    assert(sizeof(names) / sizeof(names[0]) == DCID_MAX);
    return names[e];
};

struct DriveData {
    DriveInfo driveInfo;

    std::vector<FileEntry> entries;
    // index in EveryHere.driveData
    int driveId = -1;
    // generated with generateKeyName() without .csv
    std::string csvName;
    // e.g. "First Drive"
    std::string volumeName;
    // e.g. "C:", no '\' in the end, use setDrivePath() to set
    std::string drivePath;
    // see GetDriveType() https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdrivetypea 
    uint32 driveType = 0;
    // see https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationw
    uint32 driveFlags = 0;
    // e.g. "RYZEN"
    std::string computerName;
    // e.g. "Hans"
    std::string userName;
    // when this data was gathered, saved
    std::string dateGatheredString;
    // computed from dateGatheredString, not saved
    int64 dateGatheredValue = 0;
    // in bytes
    uint64 freeSpace = 0;
    // in bytes
    uint64 totalSpace = 0;
    // is constantly updated
    uint64 selectedKeys = 0;

    bool markedForDelete = false;
    bool isLocalDrive = false;

    // call computeStats() to update
    uint64 statsSize = 0;
    // call computeStats() to update, number of directories
    uint64 statsDirs = 0;

    DriveData();

    // uses driveInfo
    // fast, done on startup for all local drives
    void gatherInfo();
    // setup driveInfo before
    // slow, gather all info and update .csv file
    void rebuild();

    void sort();

    void saveCSV();

    // not reentrant, don't use with multithreading
    // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
    const char* generatePath(int64 fileEntryIndex) const;

    // update statsSize, statsDirs
    void computeStats();

    void verify();

    // @param value must not be 0
    void setDrivePath(const char* value);
};

struct ViewEntry
{
    // index in driveData[]
    int32 driveId = -1;
    // index in driveData[driveId].entries
    int64 fileEntryId = -1;
};

struct ImGuiTableSortSpecs;

struct EveryHere
{
    // [driveId] = driveData
    std::vector<DriveData> driveData;

    // [line_no] = index into driveData[]
    std::vector<uint32> driveView;
    // [line_no] = ViewEntry, built by buildView()
    std::vector<ViewEntry> fileView;
    // [key] = driveId bits
    std::map<FileKey, std::vector<bool> > uniqueFiles;
    //
    int64 viewSumSize = 0;


    EveryHere();
        
    void gatherData();
    // @param internalName must not be null, e.g. L"Volume{41122dbf-6011-11ed-1232-04d4121124bd}"
    // @return success
    bool loadCSV(const wchar_t* internalName);

    void buildDriveView(ImGuiTableSortSpecs* sorts_specs);
    // @param driveSelectionRange into driveView[]
    void buildFileView(const char* filter, int64 minSize, int redundancyFilter, SelectionRange& driveSelectionRange, ImGuiTableSortSpecs* sorts_specs, bool folders);

    void removeDrive(const char* cleanName);

    // @return 0 if not found
    DriveData* findDrive(const char* cleanName);

    void updateLocalDriveState();

    void freeData();

    void buildUniqueFiles();

    std::vector<bool>& getRedundancy(const FileKey& fileKey);

    void addRedundancy(const FileKey& fileKey, uint32 driveId, int delta);

    uint32 findRedundancy(const FileKey& fileKey) const;
};

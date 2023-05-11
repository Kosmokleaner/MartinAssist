#include "EveryHere.h"
#include <time.h>   // _localtime64_s()
#include <vector>
#include <algorithm>
#include "Timer.h"
#include "ASCIIFile.h"
#include "FileSystem.h"
#include "Parse.h"

void dateToCString(__time64_t t, char outTimeStr[80])
{
    struct tm tm;
    errno_t err =  _localtime64_s(&tm, &t);
    assert(!err);
    // https://support.echo360.com/hc/en-us/articles/360035034372-Formatting-Dates-for-CSV-Imports#:~:text=Some%20of%20the%20CSV%20imports,program%20like%20Microsoft%C2%AE%20Excelf
    strftime(outTimeStr, 80, "%m/%d/%Y %H:%M:%S", &tm);
}

struct FolderTree
{
    // 1 based as 0 is used for root 
    uint64 fileEntry1BasedIndex = (uint64)0;

    // [folderName] = child pointer
    std::map<std::wstring, FolderTree*> children;

    ~FolderTree() {
        for (auto& it : children)
            delete it.second;
    }

    // @param must not be 0, e.g. L"temp/folder/sub", no drives e.g. "D:\temp", cannot start with '/' or '\\'
    // @return this object or a child
    FolderTree& findRecursive(const wchar_t* path)
    {
        assert(path);
        const wchar_t* p = path; 

        FolderTree* ret = this;

        while(*p) 
        {
            while(*p && *p != '/' && *p != '\\')
            {
                assert(*p != ':');
                ++p;
            }
            // can be optimized
            ret = ret->findLocal(std::wstring(path, p - path));
            assert(ret);

            if(*p == '/' || *p == '\\')
                ++p;
            path = p;
        }
        return *ret;
    }

    FolderTree& makeChild(const wchar_t* folderName) 
    {
        FolderTree* ret = new FolderTree;

        children.insert(std::pair<std::wstring, FolderTree*>(folderName, ret));
        return *ret;
    }

private:
    FolderTree* findLocal(const std::wstring& name) {
        auto it = children.find(name);
        if(it != children.end())
        {
            assert(it->second->fileEntry1BasedIndex);
            return it->second;
        }
        return nullptr;
    }
};


struct DirectoryTraverse : public IDirectoryTraverse {

    // e.g. L"C:"
    const wchar_t* path = nullptr;
    DeviceData& deviceData;
    FolderTree folderRoot;

    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;
    double nextPrintTime = 0.0f;
    // ever increasing during traversal
    int64 fileEntryCount = 0;
    int deviceId = -1;

    DirectoryTraverse(DeviceData& inDeviceData, int inDeviceId, const wchar_t* inPath)
        : deviceData(inDeviceData), path(inPath), deviceId(inDeviceId)
    {
        assert(inPath);
    }

    void OnStart() 
    {
        startTime = g_Timer.GetAbsoluteTime();
        nextPrintTime = startTime + 1.0;
    }

    void OnEnd()
    {
        logState();
        wprintf(L"\n");

        deviceData.verify();
    }

    void logState()
    {
        // a few spaces in the end to overwrite the line that was there before
        wprintf(L"%s %lld files  %.0f sec           \r",
            path,
            fileEntryCount,
            g_Timer.GetAbsoluteTime() - startTime);
    }

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData) {
        // we don't care what is in the recycling bin, seen this fully uppercase and with upper and lower case
        if (_wcsicmp(directory, L"$RECYCLE.BIN") == 0)
        {
            return false;
        }

        FolderTree& whereToInsert = folderRoot.findRecursive(filePath.getPathWithoutDrive());
        FolderTree& newFolder = whereToInsert.makeChild(directory);
        newFolder.fileEntry1BasedIndex = fileEntryCount + 1;

        FileEntry entry;
        entry.key.fileName = to_string(directory);
//        assert(entry.key.fileName.find(L',') == -1);
        // <0 for folder 
        entry.key.sizeOrFolder = -1;
        entry.key.time_write = findData.time_write;
        entry.value.parent1BasedIndex = whereToInsert.fileEntry1BasedIndex;
        entry.value.time_access = findData.time_access;
        entry.value.time_create = findData.time_create;
        entry.value.deviceId = deviceId;

        deviceData.entries.push_back(entry);
        ++fileEntryCount;

        return true;
    }

    virtual void OnFile(const FilePath& filePath, const wchar_t* file, const _wfinddata_t& findData) {
        // todo: static to avoid heap allocations, prevents multithreading use
        static FilePath local; local.path.clear();

        local = filePath;
        local.Normalize();
        local.Append(file);

        FileEntry entry;

        entry.key.fileName = to_string(file);
//        assert(entry.key.fileName.find(L',') == -1);
        entry.key.sizeOrFolder = findData.size;
        assert(entry.key.sizeOrFolder >= 0);
        entry.key.time_write = findData.time_write;
        entry.value.parent1BasedIndex = folderRoot.findRecursive(filePath.getPathWithoutDrive()).fileEntry1BasedIndex;
        entry.value.time_access = findData.time_access;
        entry.value.time_create = findData.time_create;
        entry.value.deviceId = deviceId;

        deviceData.entries.push_back(entry);
        ++fileEntryCount;

        double nowTime = g_Timer.GetAbsoluteTime();
        if(nowTime > nextPrintTime) 
        {
            // update status often but not too often, constant is in seconds
            nextPrintTime = nowTime + 0.25;
            logState();
        }
    }
};

// from https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
void replace_all(
    std::wstring& s,
    std::wstring const& toReplace,
    std::wstring const& replaceWith
) {
    std::wstring buf;
    std::size_t pos = 0;
    std::size_t prevPos;

    // Reserves rough estimate of final size of string.
    buf.reserve(s.size());

    while (true) {
        prevPos = pos;
        pos = s.find(toReplace, pos);
        if (pos == std::string::npos)
            break;
        buf.append(s, prevPos, pos - prevPos);
        buf += replaceWith;
        pos += toReplace.size();
    }

    buf.append(s, prevPos, s.size() - prevPos);
    s.swap(buf);
}

struct DriveTraverse : public IDriveTraverse {
    EveryHere &everyHere;

    DriveTraverse(EveryHere& inEveryHere) : everyHere(inEveryHere)
    {
    }

    virtual void OnDrive(const FilePath& inDrivePath, const wchar_t* deviceName, const wchar_t* internalName, const wchar_t* volumeName) {
        std::wstring drivePath = inDrivePath.path;

        if (!drivePath.empty() && drivePath.back() == '\\')
            drivePath.pop_back();

        // e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\"
        std::wstring cleanName = internalName;
        // e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\"
        replace_all(cleanName, L"\\", L"");
        replace_all(cleanName, L"/", L"");
        replace_all(cleanName, L"?", L"");

        // hack
//        if(drivePath[0] != 'E')
//            return;

        // filePath e.g. L"C:\"
        wprintf(L"\ndrivePath: %s\n", drivePath.c_str());
        // deviceName e.g. L"\Device\HarddiskVolume4"
        wprintf(L"deviceName: %s\n", deviceName);
        // e.g. L"Volume{41122dbf-6011-11ed-1232-04d4121124bd}"
        wprintf(L"cleanName: %s\n", cleanName.c_str());
        // e.g. L"First Drive"
        wprintf(L"volumeName: %s\n\n", volumeName);

        int deviceId = (int)everyHere.deviceData.size();
        everyHere.deviceData.push_back(DeviceData());
        DeviceData& deviceData = everyHere.deviceData.back();
        deviceData.cleanName = cleanName;
        deviceData.deviceId = deviceId;
        deviceData.volumeName = volumeName;
        deviceData.drivePath = drivePath;

        DirectoryTraverse traverse(deviceData, deviceId, drivePath.c_str());

        directoryTraverse(traverse, drivePath.c_str());

        if(traverse.fileEntryCount)
            deviceData.save((cleanName + std::wstring(L".csv")).c_str(), drivePath.c_str(), volumeName);
    }
};


DeviceData::DeviceData() 
{
    // to avoid some reallocations
    entries.reserve(10 * 1024 * 1024);
}

void DeviceData::sort()
{
    verify();

    size_t size = entries.size();

    // 0 based
    std::vector<size_t> sortedToIndex;
    sortedToIndex.resize(size);

    for(size_t i = 0; i < size; ++i) 
        sortedToIndex[i] = i;

    struct CustomLess
    {
        std::vector<FileEntry> &r;

        bool operator()(size_t a, size_t b) const { return r[a].key < r[b].key; }
    };

    CustomLess customLess = {entries};

    std::sort(sortedToIndex.begin(), sortedToIndex.end(), customLess);

    // 0 based
    std::vector<size_t> indexToSorted;
    indexToSorted.resize(size);
    for (size_t i = 0; i < size; ++i)
        indexToSorted[sortedToIndex[i]] = i;

    std::vector<FileEntry> newFiles;
    newFiles.resize(size);

    // for faster array acces especially in debug
    const size_t* indexToSortedPtr = indexToSorted.data();
    for (size_t i = 0; i < size; ++i)
    {
        FileEntry& dst = newFiles[i];
        std::swap(dst, entries[sortedToIndex[i]]);
        // remap parentFileEntryIndex
        if(dst.value.parent1BasedIndex)
            dst.value.parent1BasedIndex = indexToSortedPtr[dst.value.parent1BasedIndex - 1] + 1;
    }
 
    std::swap(newFiles, entries);

    verify();
}

void DeviceData::save(const wchar_t* fileName, const wchar_t* drivePath, const wchar_t* volumeName)
{
    sort();

    CASCIIFile file;
    std::string fileData;
    // to avoid reallocations
    fileData.reserve(10 * 1024 * 1024);

    if (drivePath)
    {
        fileData += "# drivePath=";
        fileData += to_string(drivePath);
        fileData += "\n";
    }
    if(volumeName)
    {
        fileData += "# volumeName=";
        fileData += to_string(volumeName);
        fileData += "\n";
    }
    if(!cleanName.empty())
    {
        fileData += "# cleanName=";
        fileData += to_string(cleanName);
        fileData += "\n";
    }
    // 2: order: size, fileName, write, path, creat, access
    fileData += "# version=2\n";
    fileData += "# size_t size (<0 for folder), fileName, __time64_t write, # parentEntryId(1 based, 0:root), __time64_t create, __time64_t access\n";
    // start marker
    fileData += "#\n";

    for (const auto& it : entries)
    {
        char str[MAX_PATH + 100];
        sprintf_s(str, sizeof(str), "%lld,\"%s\",%llu,#%llu,%llu,%llu\n",
            // key
            it.key.sizeOrFolder,
            it.key.fileName.c_str(),
            it.key.time_write,
            // value
            it.value.parent1BasedIndex,
            it.value.time_create,
            it.value.time_access);
        fileData += str;
    }
    // does not include 0 termination
    size_t len = fileData.length();
    // wasteful
    const char* cpy = (const char*)malloc(len + 1);
    memcpy((void*)cpy, fileData.c_str(), len + 1);
    file.CoverThisData(cpy, len);
    file.IO_SaveASCIIFile(fileName);
}

void EveryHere::gatherData() 
{
    DriveTraverse drives(*this);
    driveTraverse(drives);
}

void EveryHere::buildView(const char* filter, int64 minSize)
{
    assert(filter);

    view.clear();
    viewSumSize = 0;
    int filterLen = (int)strlen(filter);

    for(const auto& itD : deviceData)
    {
        uint64 id = 0;
        for (const auto& itE : itD.entries)
        {
            ViewEntry entry;
            entry.deviceId = itD.deviceId;
            entry.fileEntryId = id++;

            const FileEntry& fileEntry = itD.entries[entry.fileEntryId];

            if(fileEntry.key.sizeOrFolder < minSize)
                continue;

            // todo: optimize
            if(stristrOptimized(fileEntry.key.fileName.c_str(), filter, (int)fileEntry.key.fileName.size(), filterLen) == 0)
                continue;

            // exclude folders
            if(fileEntry.key.sizeOrFolder < 0)
                continue;

            view.push_back(entry);
            viewSumSize += fileEntry.key.sizeOrFolder;
        }
    }

    struct CustomLess
    {
        std::vector<DeviceData> & deviceData;

        bool operator()(const ViewEntry& a, const ViewEntry& b) const 
        {
            // todo: optimize vector [] in debug
            FileEntry& A = deviceData[a.deviceId].entries[a.fileEntryId];
            FileEntry& B = deviceData[b.deviceId].entries[b.fileEntryId];
            return A.key < B.key;
        }
    };

    CustomLess customLess = { deviceData };

    std::sort(view.begin(), view.end(), customLess);
}

bool EveryHere::loadCSV(const wchar_t* internalName)
{
    assert(internalName);

    CASCIIFile file;
    if(!file.IO_LoadASCIIFile(internalName))
        return false;

    const Char* p = (const Char *)file.GetDataPtr();

    std::string line;

    int deviceId = (int)deviceData.size();
    deviceData.push_back(DeviceData());
    DeviceData& data = deviceData.back();
    data.cleanName = internalName;
    data.deviceId = deviceId;

    bool error = false;

    std::string sPath;
    std::string keyName;
    std::string valueName;

    while(*p)
    {
        // example line:
        // D:/oldCode/Work/Playfield_MipMapper/TGA.h,902,145635462,145635462,145635462
        parseWhiteSpaceOrLF(p);

        if (parseStartsWith(p, "#"))
        {
            const Char *backup = p;
            // e.g.
            // # drivePath = E:
            // # volumeName = RyzenE
            // # cleanName = Volume{ ca72ef4c - 0000 - 0000 - 0000 - 100000000000 }
            // # version = 2
            parseWhiteSpaceOrLF(p);
            if(parseName(p, keyName))
            {
                parseWhiteSpaceOrLF(p);
                if (parseStartsWith(p, "="))
                {
                    parseWhiteSpaceOrLF(p);
                    if (parseLine(p, valueName))
                    {
                        if (keyName == "drivePath")
                            data.drivePath = to_wstring(valueName);
                        if (keyName == "volumeName")
                            data.volumeName = to_wstring(valueName);
                        if (keyName == "cleanName")
                            data.cleanName = to_wstring(valueName);
                        if (keyName == "version" && valueName != "2")
                        {
                            error = true;
                            break;
                        }
                    }
                }
            }
            p = backup;
            parseToEndOfLine(p);
            continue;
        }

        data.entries.push_back(FileEntry());
        FileEntry& entry = data.entries.back();

        if (!parseInt64(p, entry.key.sizeOrFolder) ||
            !parseStartsWith(p, ",") ||
            !parseStartsWith(p, "\""))
        {
            error = true;
            assert(0);
            break;
        }

        parseLine(p, entry.key.fileName, '\"');

        if (!parseStartsWith(p, ",") ||
            !parseInt64(p, entry.key.time_write) ||
            !parseStartsWith(p, ","))
        {
            error = true;
            assert(0);
            break;
        }

        if (!parseStartsWith(p, "#") ||
            !parseInt64(p, (int64&)entry.value.parent1BasedIndex) ||
            !parseStartsWith(p, ",") ||
            !parseInt64(p, entry.value.time_create) ||
            !parseStartsWith(p, ",") ||
            !parseInt64(p, entry.value.time_access))
        {
            error = true;
            assert(0);
            break;
        }

        entry.value.deviceId = deviceId;

        parseLineFeed(p);
    }

    data.verify();
    return !error;
}

void EveryHere::freeData() 
{
    view.clear();
    deviceData.clear();
}

void DeviceData::verify() 
{
    for (const auto& it : entries)
    {
//        assert(it.key.fileName.find(',') == -1);
        assert(it.key.fileName.find('\"') == -1);
        if (it.value.parent1BasedIndex)
        {
            assert(it.value.parent1BasedIndex < entries.size() + 1);
        }
    }
}



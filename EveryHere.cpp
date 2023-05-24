#include "EveryHere.h"
#include <time.h>   // _localtime64_s()
#include <windows.h>
#include <vector>
#include "imgui.h"
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
    // -1 is used for root 
    int64 fileEntry = -1;

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
            assert(it->second->fileEntry >= 0);
            return it->second;
        }
        return nullptr;
    }
};


struct EveryHereDirectory : public IDirectoryTraverse {

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

    EveryHereDirectory(DeviceData& inDeviceData, int inDeviceId, const wchar_t* inPath)
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
        newFolder.fileEntry = fileEntryCount;

        FileEntry entry;
        entry.key.fileName = to_string(directory);
//        assert(entry.key.fileName.find(L',') == -1);
        // <0 for folder 
        entry.key.sizeOrFolder = -1;
        entry.key.time_write = findData.time_write;
        entry.value.parent = whereToInsert.fileEntry;
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
        entry.value.parent = folderRoot.findRecursive(filePath.getPathWithoutDrive()).fileEntry;
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

struct DriveGatherTraverse : public IDriveTraverse {
    EveryHere &everyHere;

    DriveGatherTraverse(EveryHere& inEveryHere) : everyHere(inEveryHere)
    {
    }

    virtual void OnDrive(const FilePath& inDrivePath, const wchar_t* deviceName, const wchar_t* internalName, const wchar_t* volumeName, uint32 driveFlags, uint32 serialNumber) {
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

        everyHere.removeDevice(to_string(cleanName).c_str());

        int deviceId = (int)everyHere.deviceData.size();
        everyHere.deviceData.push_back(DeviceData());
        DeviceData& deviceData = everyHere.deviceData.back();
        deviceData.cleanName = to_string(cleanName);
        deviceData.deviceId = deviceId;
        deviceData.volumeName = to_string(volumeName);
        deviceData.drivePath = to_string(drivePath);

        // freeSpace, totalSpace
        {
            DWORD SectorsPerCluster = 0;
            DWORD BytesPerSector = 0;
            DWORD NumberOfFreeClusters = 0;
            DWORD TotalNumberOfClusters = 0;
            GetDiskFreeSpace(drivePath.c_str(), &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
            
            uint64 clusterSize = SectorsPerCluster * (uint64)BytesPerSector;
            deviceData.freeSpace = clusterSize * NumberOfFreeClusters;
            deviceData.totalSpace = clusterSize * TotalNumberOfClusters;
        }

        deviceData.driveType = GetDriveType(drivePath.c_str());
        deviceData.driveFlags = driveFlags;
        deviceData.serialNumber = serialNumber;

        // https://stackoverflow.com/questions/76022257/getdrivetype-detects-google-drive-as-drive-fixed-how-to-exclude-them
        

        {
            char name[256];
            DWORD size = sizeof(name);
            if (GetComputerNameA(name, &size))
            {
                // e.g. "RYZEN"
                deviceData.computerName = name;
            }
        }

        {
            char name[256];
            DWORD size = sizeof(name);
            if (GetUserNameA(name, &size))
            {
                // e.g. "Hans"
                deviceData.userName = name;
            }
        }

        {
            // https://zetcode.com/gui/winapi/datetime/
            SYSTEMTIME st = { 0 };

            GetLocalTime(&st);

            char str[1024];
            sprintf_s(str, sizeof(str) / sizeof(*str), "%02d/%02d/%04d %02d:%02d:%02d\n",
                st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

            deviceData.date = str;
        }

        EveryHereDirectory traverse(deviceData, deviceId, drivePath.c_str());

        directoryTraverse(traverse, drivePath.c_str());

        if(traverse.fileEntryCount)
            deviceData.save();
    }
};


struct LocalDriveStateTraverse : public IDriveTraverse {
    EveryHere& everyHere;

    LocalDriveStateTraverse(EveryHere& inEveryHere) : everyHere(inEveryHere)
    {
        // later we update the ones we find local with true
        for (auto& drive : everyHere.deviceData)
            drive.isLocalDrive = false;
    }

    virtual void OnDrive(const FilePath& inDrivePath, const wchar_t* deviceName, const wchar_t* internalName, const wchar_t* volumeName, uint32 driveFlags, uint32 serialNumber) {
        // e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\"
        std::wstring cleanName = internalName;
        // e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\"
        replace_all(cleanName, L"\\", L"");
        replace_all(cleanName, L"/", L"");
        replace_all(cleanName, L"?", L"");

        if(DeviceData * drive = everyHere.findDrive(to_string(cleanName).c_str()))
        {
            // before we them all with false so this is updating only the local ones
            drive->isLocalDrive = true;
        }
    }
};

void EveryHere::updateLocalDriveState()
{
    LocalDriveStateTraverse drives(*this);
    driveTraverse(drives);
}


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
        if(dst.value.parent != -1)
            dst.value.parent = indexToSortedPtr[dst.value.parent];
    }
 
    std::swap(newFiles, entries);

    verify();
}

void DeviceData::computeStats() 
{
    statsSize = 0;
    statsDirs = 0;

    for (const auto& el : entries)
    {
        if(el.key.sizeOrFolder >= 0)
            statsSize += el.key.sizeOrFolder;
        else
            ++statsDirs;
    }
}

void DeviceData::save()
{
    char str[MAX_PATH + 100];

    sort();

    CASCIIFile file;
    std::string fileData;
    // to avoid reallocations
    fileData.reserve(10 * 1024 * 1024);

#define HEADER_STRING(name) \
    if (!name.empty()) \
    {\
        fileData += "# " #name "="; \
        fileData += name; \
        fileData += "\n"; \
    }

    HEADER_STRING(drivePath)
    HEADER_STRING(volumeName)
    HEADER_STRING(cleanName)
    HEADER_STRING(computerName)
    HEADER_STRING(userName)
    HEADER_STRING(date)

#undef HEADER_STRING

    sprintf_s(str, sizeof(str), "# freeSpace=%llu\n", freeSpace);
    fileData += str;
    sprintf_s(str, sizeof(str), "# totalSpace=%llu\n", totalSpace);
    fileData += str;
    sprintf_s(str, sizeof(str), "# type=%u\n", driveType);
    fileData += str;
    sprintf_s(str, sizeof(str), "# flags=%u\n", driveFlags);
    fileData += str;
    sprintf_s(str, sizeof(str), "# serialNumber=%u\n", serialNumber);
    fileData += str;

    // 2: order: size, fileName, write, path, creat, access
    fileData += "# version=" SERIALIZE_VERSION "\n";
    fileData += "# size_t size (<0 for folder), fileName, __time64_t write, # parentEntryId(-1:root), __time64_t create, __time64_t access\n";
    // start marker
    fileData += "#\n";

    for (const auto& it : entries)
    {
        sprintf_s(str, sizeof(str), "%lld,\"%s\",%llu,#%lld,%llu,%llu\n",
            // key
            it.key.sizeOrFolder,
            it.key.fileName.c_str(),
            it.key.time_write,
            // value
            it.value.parent,
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

    std::string fileName = (cleanName + std::string(".csv")).c_str();

    file.IO_SaveASCIIFile(fileName.c_str());
}

void EveryHere::gatherData() 
{
    DriveGatherTraverse drives(*this);
    driveTraverse(drives);

    for (auto& itD : deviceData)
    {
        itD.computeStats();
    }

    updateLocalDriveState();
}

DeviceData* EveryHere::findDrive(const char* cleanName) 
{
    for (auto& itD : deviceData)
        if (itD.cleanName == cleanName)
            return &itD;

    return nullptr;
}

void EveryHere::removeDevice(const char* cleanName)
{
    uint32 i = 0;
    for (auto& itD : deviceData)
    {
        if(itD.cleanName == cleanName)
        {
            deviceData.erase(deviceData.begin() + i);
            return;
        }
        ++i;
    }
}

void EveryHere::buildView(const char* filter, int64 minSize, SelectionRange& deviceSelectionRange, ImGuiTableSortSpecs* sorts_specs)
{
    assert(filter);

    view.clear();
    viewSumSize = 0;
    int filterLen = (int)strlen(filter);

    for(auto& itD : deviceData)
    {
        uint64 id = 0;

        if (!deviceSelectionRange.isSelected(itD.deviceId))
            continue;

        view.reserve(itD.entries.size());

        for (const auto& itE : itD.entries)
        {
            ViewEntry entry;
            entry.deviceId = itD.deviceId;
            entry.fileEntryId = id++;

            FileEntry& fileEntry = itD.entries[entry.fileEntryId];

            // should happen only in the beginnng
            if(fileEntry.value.parent >= 0 && fileEntry.value.path.empty())
                fileEntry.value.path = itD.generatePath(fileEntry.value.parent);

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
        ImGuiTableSortSpecs* sorts_specs = {};

        bool operator()(const ViewEntry& a, const ViewEntry& b) const 
        {
            // todo: optimize vector [] in debug
            FileEntry& A = deviceData[a.deviceId].entries[a.fileEntryId];
            FileEntry& B = deviceData[b.deviceId].entries[b.fileEntryId];

            int count = sorts_specs ? sorts_specs->SpecsCount : 0;

            for (int n = 0; n < count; n++)
            {
                // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
                // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
                const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
                int delta = 0;
                switch (sort_spec->ColumnUserID)
                {
                    case FCID_Name:        delta = strcmp(A.key.fileName.c_str(), B.key.fileName.c_str()); break;
                    case FCID_Path:        delta = strcmp(A.value.path.c_str(), B.value.path.c_str()); break;
                    case FCID_Size:        delta = (int)(A.key.sizeOrFolder - B.key.sizeOrFolder); break;
                    case FCID_DeviceId:    delta = a.deviceId - b.deviceId; break;
                    default: IM_ASSERT(0); break;
                }
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
            }

//            if(sorts_specs->SpecsCount == FCID_Name)
//                return A.key.fileName < B.key.fileName;
//            if (sort == FCID_Path)
//                return A.key < B.key;
//            if (sort == FCID_DeviceId)
//                return a.deviceId < b.deviceId;
//            if (sort == FCID_Size)
//            return A.key < B.key;

            // qsort() is instable so always return a way to differenciate items.
            // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
            return &a < &b;
        }
    };

    CustomLess customLess = { deviceData, sorts_specs };

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
    data.cleanName = to_string(internalName);
    data.deviceId = deviceId;

    bool error = false;

    std::string sPath;
    std::string keyName;
    std::string valueName;

    while(*p)
    {
        // example line:
        // 8192,"BOOTSECT.BAK",1612980750,#-1,1558230967,1612980750

        parseWhiteSpaceOrLF(p);

        if (parseStartsWith(p, "#"))
        {
            const Char *backup = p;
            // e.g.
            // # drivePath = E:
            // # volumeName = RyzenE
            // # cleanName = Volume{ca72ef4c-0000-0000-0000-100000000000}
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
                            data.drivePath = valueName;
                        if (keyName == "volumeName")
                            data.volumeName = valueName;
                        if (keyName == "cleanName")
                            data.cleanName = valueName;
                        if (keyName == "computerName")
                            data.computerName = valueName;
                        if (keyName == "userName")
                            data.userName = valueName;
                        if (keyName == "date")
                            data.date = valueName;
                        if (keyName == "freeSpace")
                            data.freeSpace = stringToInt64(valueName.c_str());
                        if (keyName == "totalSpace")
                            data.totalSpace = stringToInt64(valueName.c_str());
                        if (keyName == "type")
                            data.driveType = (uint32)stringToInt64(valueName.c_str());
                        if (keyName == "flags")
                            data.driveFlags = (uint32)stringToInt64(valueName.c_str());
                        if (keyName == "serialNumber")
                            data.serialNumber = (uint32)stringToInt64(valueName.c_str());
                        if (keyName == "version" && valueName != SERIALIZE_VERSION)
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
            !parseInt64(p, entry.value.parent) ||
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
    data.computeStats();

    data.verify();
    return !error;
}

// not reentrant, don't use with multithreading
// https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
const char* DeviceData::generatePath(int64 fileEntryIndex) const
{
    static char pathBuffer[32 * 1024];

    char* writer = pathBuffer + sizeof(pathBuffer) - 1;
    *writer = 0;
    while (fileEntryIndex >= 0)
    {
        const FileEntry& here = entries[fileEntryIndex];
        assert(here.key.sizeOrFolder == -1);
        fileEntryIndex = here.value.parent;
        size_t len = here.key.fileName.size();
        if (*writer) *--writer = '/';
        writer -= len;
        memcpy(writer, here.key.fileName.c_str(), len);
    }
    return writer;
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
        if (it.value.parent >= 0)
        {
            assert(it.value.parent < (int64)entries.size());
        }
    }
}



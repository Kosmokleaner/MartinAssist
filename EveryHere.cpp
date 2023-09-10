#include "EveryHere.h"
#include <time.h>   // _localtime64_s()
#include <windows.h>
#include <vector>
#include "imgui.h"
#include "Timer.h"
#include "ASCIIFile.h"
#include "FileSystem.h"
#include "Parse.h"

void dateToCString(__time64_t t, char outTimeStr[80])
{
    struct tm tm;
    errno_t err =  _localtime64_s(&tm, &t);
    assert(!err);
    if(err)
    {
        *outTimeStr = 0;
        return;
    }    
    // https://support.echo360.com/hc/en-us/articles/360035034372-Formatting-Dates-for-CSV-Imports#:~:text=Some%20of%20the%20CSV%20imports,program%20like%20Microsoft%C2%AE%20Excelf
    strftime(outTimeStr, 80, "%m/%d/%Y %H:%M:%S", &tm);
}

// @param inTime date in this form: "%02d/%02d/%04d %02d:%02d:%02d"
int64 timeStringToValue(const char* inTime)
{
    assert(inTime);

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    SYSTEMTIME a = {};

    if (*inTime)
    {
        int cnt = sscanf_s(inTime, "%02d/%02d/%04d %02d:%02d:%02d", &day, &month, &year, &hour, &minute, &second);
        assert(cnt == 6);
        if (cnt != 6)
            return 0;

        a.wYear = (WORD)year;
        a.wMonth = (WORD)month;
        a.wDay = (WORD)day;
        a.wHour = (WORD)hour;
        a.wMinute = (WORD)minute;
        a.wSecond = (WORD)second;
    }

    FILETIME v_ftime;
    BOOL ok = SystemTimeToFileTime(&a, &v_ftime);
    assert(ok);
    if(!ok)
        return 0;
    ULARGE_INTEGER v_ui;
    v_ui.LowPart = v_ftime.dwLowDateTime;
    v_ui.HighPart = v_ftime.dwHighDateTime;
    return v_ui.QuadPart;
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


struct EveryHereDirectory : public IDirectoryTraverse
{
    // e.g. L"C:"
    const wchar_t* path = nullptr;
    DriveData& driveData;
    FolderTree folderRoot;

    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;
    double nextPrintTime = 0.0f;
    // ever increasing during traversal
    int64 fileEntryCount = 0;
    int driveId = -1;
    // 0..100
    int percent = 0;

    EveryHereDirectory(DriveData& inDriveData, int inDriveId, const wchar_t* inPath)
        : driveData(inDriveData), path(inPath), driveId(inDriveId)
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

        driveData.verify();
    }

    void logState()
    {
        // a few spaces in the end to overwrite the line that was there before
        wprintf(L"%s %lld files  %.0f sec    %d%%       \r",
            path,
            fileEntryCount,
            g_Timer.GetAbsoluteTime() - startTime,
            percent);
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
        entry.key.fileName = driveData.stringPool.push(to_string(directory).c_str());
//        assert(entry.key.fileName.find(L',') == -1);
        // <0 for folder 
        entry.key.sizeOrFolder = -1;
        entry.key.time_write = findData.time_write;
        entry.value.parent = whereToInsert.fileEntry;
        entry.value.time_access = findData.time_access;
        entry.value.time_create = findData.time_create;
        entry.value.driveId = driveId;

        driveData.entries.push_back(entry);
        ++fileEntryCount;

        return true;
    }

    virtual void OnFile(const FilePath& filePath, const wchar_t* file, const _wfinddata_t& findData, float progress) 
    {
        percent = (int)(100.0f * progress);

        // todo: static to avoid heap allocations, prevents multithreading use
        static FilePath local; local.path.clear();

        local = filePath;
        local.Normalize();
        local.Append(file);

        FileEntry entry;

        entry.key.fileName = driveData.stringPool.push(to_string(file).c_str());
//        assert(entry.key.fileName.find(L',') == -1);
        entry.key.sizeOrFolder = findData.size;
        assert(entry.key.sizeOrFolder >= 0);
        entry.key.time_write = findData.time_write;
        entry.value.parent = folderRoot.findRecursive(filePath.getPathWithoutDrive()).fileEntry;
        entry.value.time_access = findData.time_access;
        entry.value.time_create = findData.time_create;
        entry.value.driveId = driveId;

        driveData.entries.push_back(entry);
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


struct DriveGatherTraverse : public IDriveTraverse {
    EveryHere &everyHere;

    DriveGatherTraverse(EveryHere& inEveryHere) : everyHere(inEveryHere)
    {
    }

    virtual void OnDrive(const DriveInfo& driveInfo) 
    {
        std::wstring csvName = driveInfo.generateKeyName();
        everyHere.removeDrive(to_string(csvName).c_str());

        int deviceId = (int)everyHere.driveData.size();

        DriveData& data = *(new DriveData());
        everyHere.driveData.push_back(&data);
        data.driveInfo = driveInfo;
        data.driveId = deviceId;
        data.rebuild();
    }
};

struct LocalDriveStateTraverse : public IDriveTraverse 
{
    EveryHere& everyHere;

    LocalDriveStateTraverse(EveryHere& inEveryHere) : everyHere(inEveryHere)
    {
        // later we update the ones we find local with true
        for (auto& drive : everyHere.driveData)
            drive->isLocalDrive = false;
    }

    virtual void OnDrive(const DriveInfo& driveInfo) 
    {
        std::wstring csvName = driveInfo.generateKeyName();

        // todo: google drive is changing it's internalName so we cannot key by this
        printf("LocalDriveStateTraverse '%s' '%s' '%s' %u\n", to_string(driveInfo.drivePath.path).c_str(),  to_string(csvName).c_str(), to_string(driveInfo.volumeName).c_str(), driveInfo.serialNumber);

        DriveData* drive = everyHere.findDrive(to_string(csvName).c_str());

        if(!drive)
        {
            DriveData& data = *(new DriveData());
            everyHere.driveData.push_back(&data);
            data.driveId = (int)(everyHere.driveData.size() - 1);
            data.driveInfo = driveInfo;
            data.gatherInfo();
        }
    }
};

void EveryHere::updateLocalDriveState()
{
    LocalDriveStateTraverse drives(*this);
    driveTraverse(drives);
}


DriveData::DriveData() 
{
    // to avoid some reallocations
    entries.reserve(10 * 1024 * 1024);
}


void DriveData::rebuild()
{
    // set this before
    assert(driveId >= 0);

    gatherInfo();

    // cleaned up e.g. L"C:"
    std::wstring wDrivePath = to_wstring(drivePath);

    // filePath e.g. L"C:"
    printf("\ndrivePath: %s\n", drivePath.c_str());
    // deviceName e.g. L"\Device\HarddiskVolume4"
    wprintf(L"deviceName: %s\n", driveInfo.deviceName.c_str());
    // e.g. L"Volume{41122dbf-6011-11ed-1232-04d4121124bd}"
    printf("csvName: %s\n", csvName.c_str());
    // e.g. L"First Drive"
    wprintf(L"volumeName: %s\n\n", driveInfo.volumeName.c_str());

    driveFlags = driveInfo.driveFlags;

    // https://stackoverflow.com/questions/76022257/getdrivetype-detects-google-drive-as-drive-fixed-how-to-exclude-them



    {
        // https://zetcode.com/gui/winapi/datetime/
        SYSTEMTIME st = { 0 };

        GetLocalTime(&st);

        char str[1024];
        sprintf_s(str, sizeof(str) / sizeof(*str), "%02d/%02d/%04d %02d:%02d:%02d\n",
            st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

        dateGatheredString = str;
        dateGatheredValue = timeStringToValue(str);
    }

    EveryHereDirectory traverse(*this, driveId, wDrivePath.c_str());

//    directoryTraverse(traverse, wDrivePath.c_str());

    uint64 totalExpectedFileSize = totalSpace - freeSpace;

    directoryTraverse2(traverse, wDrivePath.c_str(), totalExpectedFileSize);

//    if (traverse.fileEntryCount)
    saveCSV();
}


void DriveData::gatherInfo()
{
    csvName = to_string(driveInfo.generateKeyName());
    
    volumeName = to_string(driveInfo.volumeName);
    setDrivePath(to_string(driveInfo.drivePath.path).c_str());
    // before we them all with false so this is updating only the local ones
    isLocalDrive = true;

    // freeSpace, totalSpace
    {
        DWORD SectorsPerCluster = 0;
        DWORD BytesPerSector = 0;
        DWORD NumberOfFreeClusters = 0;
        DWORD TotalNumberOfClusters = 0;
        GetDiskFreeSpace(to_wstring(drivePath).c_str(), &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

        uint64 clusterSize = SectorsPerCluster * (uint64)BytesPerSector;
        freeSpace = clusterSize * NumberOfFreeClusters;
        totalSpace = clusterSize * TotalNumberOfClusters;
    }

    driveType = GetDriveType(to_wstring(drivePath).c_str());

    {
        char name[256];
        DWORD size = sizeof(name);
        if (GetComputerNameA(name, &size))
        {
            // e.g. "RYZEN"
            computerName = name;
        }
    }

    {
        char name[256];
        DWORD size = sizeof(name);
        if (GetUserNameA(name, &size))
        {
            // e.g. "Hans"
            userName = name;
        }
    }
}


void DriveData::sort()
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

void DriveData::computeStats() 
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

// from https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
void replace_all(
    std::string& s,
    std::string const& toReplace,
    std::string const& replaceWith
) {
    std::string buf;
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


void DriveData::saveCSV()
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
    HEADER_STRING(csvName)
    HEADER_STRING(computerName)
    HEADER_STRING(userName)
    HEADER_STRING(dateGatheredString)

#undef HEADER_STRING

    sprintf_s(str, sizeof(str), "# freeSpace=%llu\n", freeSpace);
    fileData += str;
    sprintf_s(str, sizeof(str), "# totalSpace=%llu\n", totalSpace);
    fileData += str;
    sprintf_s(str, sizeof(str), "# type=%u\n", driveType);
    fileData += str;
    sprintf_s(str, sizeof(str), "# flags=%u\n", driveFlags);
    fileData += str;
    sprintf_s(str, sizeof(str), "# serialNumber=%u\n", driveInfo.serialNumber);
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

//    std::string fileName = (csvName + std::string(".csv")).c_str();
    std::string fileName = (csvName + volumeName + std::string(".csv")).c_str();

    replace_all(fileName, " ", "_");
    replace_all(fileName, "\\", "");
    replace_all(fileName, "/", "");
    replace_all(fileName, "?", "");

    file.IO_SaveASCIIFile(fileName.c_str());
}

EveryHere::EveryHere()
{
    PooledStringUnitTest();

/*    // some unit tests
    int64 a = timeStringToValue("23/05/2023 19:40:40");
    int64 b = timeStringToValue("30/05/2023 01:47:36");
    int64 c = timeStringToValue("30/05/2023 01:48:31");
    int64 d = timeStringToValue("30/05/2023 01:49:57");
    assert(a < b);
    assert(b < c);
    assert(c < d);
*/
    updateLocalDriveState();
}

EveryHere::~EveryHere()
{
    freeData();
}

void EveryHere::gatherData() 
{
    DriveGatherTraverse drives(*this);
    driveTraverse(drives);

    for (auto& itD : driveData)
        itD->computeStats();

    updateLocalDriveState();
    buildUniqueFiles();
}

DriveData* EveryHere::findDrive(const char* cleanName) 
{
    for (auto& itD : driveData)
        if (itD->csvName == cleanName)
            return itD;

    return nullptr;
}

void EveryHere::removeDrive(const char* csvName)
{
    uint32 i = 0;
    for (auto& itD : driveData)
    {
        if(itD->csvName == csvName)
        {
            driveData.erase(driveData.begin() + i);
            return;
        }
        ++i;
    }
}

void EveryHere::buildFileView(const char* filter, int64 minSize, int redundancyFilter, SelectionRange& driveSelectionRange, ImGuiTableSortSpecs* sorts_specs, bool folders)
{
    assert(minSize >= 0);
    assert(filter);

    fileView.clear();
    viewSumSize = 0;
    int filterLen = (int)strlen(filter);

    for (uint32 lineNoDrive = 0; lineNoDrive < driveView.size(); ++lineNoDrive)
    {
        if (!driveSelectionRange.isSelected(lineNoDrive))
            continue;

        DriveData& itD = *driveData[driveView[lineNoDrive]];

        fileView.reserve(itD.entries.size());

        uint64 id = 0;
        for (auto& fileEntry : itD.entries)
        {
            ViewEntry entry;
            entry.driveId = itD.driveId;
            entry.fileEntryId = id++;

//            FileEntry& fileEntry = itD.entries[entry.fileEntryId];

            // should happen only in the beginnng
            if(fileEntry.value.parent >= 0 && fileEntry.value.path.empty())
                fileEntry.value.path = itD.generatePath(fileEntry.value.parent);

            if(folders)
            {
                if (fileEntry.key.sizeOrFolder >= 0)
                    continue;
            }
            else
            {
                if(fileEntry.key.sizeOrFolder < minSize)
                    continue;
            }

            if(redundancyFilter)
            {
                int redundancy = (int)findRedundancy(fileEntry.key);
                switch(redundancyFilter)
                {
                    case 1: if(redundancy >= 2) continue;
                        break;
                    case 2: if (redundancy >= 3) continue;
                        break;
                    case 3: if (redundancy != 3) continue;
                        break;
                    case 4: if (redundancy <= 3) continue;
                        break;
                    case 5: if (redundancy <= 4) continue;
                        break;
                    default:
                        assert(0);
                }
            }

            // todo: optimize
            if(stristrOptimized(fileEntry.key.fileName.c_str(), filter, (int)fileEntry.key.fileName.size(), filterLen) == 0)
                continue;

            fileView.push_back(entry);
            viewSumSize += fileEntry.key.sizeOrFolder;
        }
    }

    struct CustomLessFile
    {
        const EveryHere& everyHere;
        ImGuiTableSortSpecs* sorts_specs = {};

        bool operator()(const ViewEntry& a, const ViewEntry& b) const 
        {
            // todo: optimize vector [] in debug
            const FileEntry& A = everyHere.driveData[a.driveId]->entries[a.fileEntryId];
            const FileEntry& B = everyHere.driveData[b.driveId]->entries[b.fileEntryId];

            int count = sorts_specs ? sorts_specs->SpecsCount : 0;

            for (int n = 0; n < count; n++)
            {
                // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
                // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
                const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
                int64 delta = 0;
                switch (sort_spec->ColumnUserID)
                {
                    case FCID_Name:        delta = strcmp(A.key.fileName.c_str(), B.key.fileName.c_str()); break;
                    case FCID_Size:        delta = A.key.sizeOrFolder - B.key.sizeOrFolder; break;
                    case FCID_Redundancy:  delta = everyHere.findRedundancy(A.key) - everyHere.findRedundancy(B.key); break;
                    case FCID_DriveId:    delta = a.driveId - b.driveId; break;
                    case FCID_Path:        delta = strcmp(A.value.path.c_str(), B.value.path.c_str()); break;
                    default: IM_ASSERT(0); break;
                }
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
            }

            // qsort() is instable so always return a way to differenciate items.
            // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
            return &A < &B;
        }
    };

    CustomLessFile customLess = { *this, sorts_specs };

    std::sort(fileView.begin(), fileView.end(), customLess);
}

std::vector<bool>& EveryHere::getRedundancy(const FileKey& fileKey)
{
    auto it = uniqueFiles.find(fileKey);
    if (it == uniqueFiles.end())
    {
        uniqueFiles.insert(std::pair<FileKey, std::vector<bool> >(fileKey, std::vector<bool>()));
        it = uniqueFiles.find(fileKey);
    }

    return it->second;
}

void EveryHere::addRedundancy(const FileKey& fileKey, uint32 driveId, int delta)
{
    assert(delta == -1 || delta == 1);

    std::vector<bool>& set = getRedundancy(fileKey);
    if(set.size() <= driveId)
        set.resize(driveId + 1, 0);

    if(delta > 0)
        set[driveId] = true;
    else
        set[driveId] = false;
}

uint32 EveryHere::findRedundancy(const FileKey& fileKey) const
{
    const auto& it = uniqueFiles.find(fileKey);
    if(it == uniqueFiles.end())
        return 0;

    const std::vector<bool>& set = it->second;
    uint32 ret = 0;
    for (auto itBool = set.begin(), end = set.end(); itBool != end; ++itBool)
    {
        if(*itBool)
            ++ret;
    }
    return ret;
}

void EveryHere::buildDriveView(ImGuiTableSortSpecs* sorts_specs)
{
    uint32 count = (uint32)driveView.size();
    driveView.resize(driveData.size());

    for(uint32 i = 0; i < count; ++i)
        driveView[i] = i;

    struct CustomLessDrive
    {
        std::vector<DriveData*>& driveData;
        ImGuiTableSortSpecs* sorts_specs = {};

        bool operator()(const uint32 a, const uint32 b) const
        {
            // todo: optimize vector [] in debug
            DriveData& A = *driveData[a];
            DriveData& B = *driveData[b];

            int count = sorts_specs ? sorts_specs->SpecsCount : 0;

            for (int n = 0; n < count; n++)
            {
                // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
                // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
                const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
                int64 delta = 0;
                switch (sort_spec->ColumnUserID)
                {
                    case DCID_VolumeName:   delta = strcmp(A.volumeName.c_str(), B.volumeName.c_str()); break;
                    case DCID_UniqueName:   delta = strcmp(A.csvName.c_str(), B.csvName.c_str()); break;
                    case DCID_Path:   delta = strcmp(A.drivePath.c_str(), B.drivePath.c_str()); break;
                    case DCID_DriveId:   delta = A.driveId - B.driveId; break;
                    case DCID_Size:   delta = A.statsSize - B.statsSize; break;
                    case DCID_Files:   delta = A.entries.size() - B.entries.size(); break;
                    case DCID_Directories:   delta = A.statsDirs - B.statsDirs; break;
                    case DCID_Computer:   delta = strcmp(A.computerName.c_str(), B.computerName.c_str()); break;
                    case DCID_User:   delta = strcmp(A.userName.c_str(), B.userName.c_str()); break;
                    case DCID_Date:   delta = A.dateGatheredValue - B.dateGatheredValue; break;
                    case DCID_totalSpace:   delta = A.totalSpace - B.totalSpace; break;
                    case DCID_type:   delta = (int64)A.driveType - (int64)B.driveType; break;
                    case DCID_serial:   delta = (int64)A.driveInfo.serialNumber - (int64)B.driveInfo.serialNumber; break;
                    case DCID_selectedFiles:   delta = A.selectedKeys - B.selectedKeys; break;
                    default: IM_ASSERT(0); break;
                }
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
            }

            // qsort() is instable so always return a way to differenciate items.
            // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
            return &A < &B;
        }
    };

    CustomLessDrive customLess = { driveData, sorts_specs };

    std::sort(driveView.begin(), driveView.end(), customLess);
}

bool EveryHere::loadCSV(const wchar_t* internalName)
{
    assert(internalName);


    CASCIIFile file;

    {
        CScopedCPUTimerLog scope("loadFile");

        if(!file.IO_LoadASCIIFile(internalName))
            return false;
    }

    const Char* p = (const Char *)file.GetDataPtr();

    std::string line;

    int deviceId = (int)driveData.size();
    DriveData& data = *(new DriveData());
    driveData.push_back(&data);
    data.csvName = to_string(internalName);
    data.driveId = deviceId;

    bool error = false;

    std::string sPath;
    std::string keyName;
    std::string valueName;
    std::string cleanName;

    keyName.reserve(1000);
    valueName.reserve(1000);
    cleanName.reserve(1000);

    {
        CScopedCPUTimerLog scope("parse");

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
                                data.setDrivePath(valueName.c_str());
                            if (keyName == "volumeName")
                                data.volumeName = valueName;
                            if (keyName == "cleanName")
                                cleanName = valueName;
                            if (keyName == "computerName")
                                data.computerName = valueName;
                            if (keyName == "userName")
                                data.userName = valueName;
                            if (keyName == "dateGatheredString")
                            {
                                data.dateGatheredString = valueName;
                                data.dateGatheredValue = timeStringToValue(valueName.c_str());
                            }
                            if (keyName == "freeSpace")
                                data.freeSpace = stringToInt64(valueName.c_str());
                            if (keyName == "totalSpace")
                                data.totalSpace = stringToInt64(valueName.c_str());
                            if (keyName == "type")
                                data.driveType = (uint32)stringToInt64(valueName.c_str());
                            if (keyName == "flags")
                                data.driveFlags = (uint32)stringToInt64(valueName.c_str());
                            if (keyName == "serialNumber")
                                data.driveInfo.serialNumber = (uint32)stringToInt64(valueName.c_str());
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
            
            // faster than data.entries.push_back(FileEntry()); 4400ms -> 2400ms
            data.entries.resize(data.entries.size() + 1);

            FileEntry& entry = data.entries.back();

            if (!parseInt64(p, entry.key.sizeOrFolder) ||
                !parseStartsWith(p, ",") ||
                !parseStartsWith(p, "\""))
            {
                error = true;
                assert(0);
                break;
            }

            parseLine(p, data.stringPool, entry.key.fileName, '\"');

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

            entry.value.driveId = deviceId;

            parseLineFeed(p);
        }
    }

    DriveInfo driveInfo;
    driveInfo.drivePath = FilePath(to_wstring(data.drivePath).c_str());
//    driveInfo.deviceName = data.;
//   driveInfo.internalName = data.;
    driveInfo.volumeName = to_wstring(data.volumeName).c_str();
    driveInfo.driveFlags = data.driveFlags;

    data.csvName = to_string(driveInfo.generateKeyName()).c_str();

    {
        CScopedCPUTimerLog scope("computeStats");
        data.computeStats();
    }

    {
        CScopedCPUTimerLog scope("verify");
        data.verify();
    }
    return !error;
}

// https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
PooledString DriveData::generatePath(int64 fileEntryIndex)
{
    char pathBuffer[32 * 1024];

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

    return stringPool.push(pathBuffer);
}

void EveryHere::buildUniqueFiles()
{
    CScopedCPUTimerLog scope("buildUniqueFiles");

    uniqueFiles.clear();

    uint32 driveId = 0;
    for (auto& drive : driveData)
    {
        for(auto& entry : drive->entries)
        {
            addRedundancy(entry.key, driveId, 1);
        }
        ++driveId;
    }
}

void EveryHere::freeData() 
{
    for (auto& itD : driveData)
        delete itD;
    driveData.clear();

    fileView.clear();
    driveData.clear();
}

void DriveData::setDrivePath(const char * inDrivePath)
{
    drivePath = inDrivePath;

    if (!drivePath.empty() && drivePath.back() == '\\')
        drivePath.pop_back();
}

void DriveData::verify() 
{
    for (const auto& it : entries)
    {
//        assert(it.key.fileName.find(',') == -1);
//        assert(it.key.fileName.find('\"') == -1);
        if (it.value.parent >= 0)
        {
            assert(it.value.parent < (int64)entries.size());
        }
    }
}



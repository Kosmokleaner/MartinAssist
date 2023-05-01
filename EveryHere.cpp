#include "EveryHere.h"
#include <time.h>   // _localtime64_s()
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


struct DirectoryTraverse : public IDirectoryTraverse {

    // e.g. L"C:"
    const wchar_t* path;
    DeviceData& deviceData;

    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;
    double nextPrintTime = 0.0f;
    uint64 fileCount = 0;
    uint64 uniqueCount = 0;

    DirectoryTraverse(DeviceData& inDeviceData, const wchar_t* inPath)
        : deviceData(inDeviceData), path(inPath)
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
    }

    void logState()
    {
        wprintf(L"%s %lld files %lld dups  %.0f sec           \r",
            path,
            fileCount,
            fileCount - uniqueCount,
            g_Timer.GetAbsoluteTime() - startTime);
    }

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory) {
        std::string s = to_string(filePath.path);
        const Char*p = (const Char *)s.c_str();

        if (*p >='A' && *p <= 'Z')
        {
            ++p;
            if (parseStartsWith(p, ":/$RECYCLE.BIN"))
            {
                if(*p == 0)
                    return false;
            }
            if (parseStartsWith(p, ":/$Recycle.Bin"))
            {
                if (*p == 0)
                    return false;
            }
        }

        return true;
    }

    virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData) {
        // todo: static to avoid heap allocations, prevents multithreading use
        static FilePath local; local.path.clear();

        local = path;
        local.Normalize();
        local.Append(file);

        FileKey el;
        el.fileName = file;
        el.size = findData.size;
        el.time_write = findData.time_write;
        FileValue val;
        val.path = path.path;
        val.time_access = findData.time_access;
        val.time_create = findData.time_create;

/*
        // test0
        {
            struct tm tm;
            errno_t err = _localtime64_s(&tm, &el.time_write);
            assert(!err);
            __time64_t c = _mktime64(&tm);     //todo this changes t
            assert(c == el.time_write);
        }

        // test1
        {
            char str[80];
            dateToCString(el.time_write, str);
            const Char* p = (const Char*)str;
            tm t0;
            parseDate(p, t0);
            tm t1 = t0;
            // The time function returns the number of seconds elapsed since midnight (00:00:00), January 1, 1970,
            __time64_t c = _mktime64(&t1);     //todo this changes t
            char str2[80];
            dateToCString(c, str2);
            assert(findData.time_write == c);
        }
*/
        if (deviceData.files.find(el) == deviceData.files.end())
            ++uniqueCount;

        deviceData.files.insert(std::pair<FileKey, FileValue>(el, val));
        ++fileCount;

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

        // filePath e.g. L"C:\"
        wprintf(L"\ndrivePath: %s\n", drivePath.c_str());
        // deviceName e.g. L"\Device\HarddiskVolume4"
        wprintf(L"deviceName: %s\n", deviceName);
        // e.g. L"Volume{41122dbf-6011-11ed-1232-04d4121124bd}"
        wprintf(L"cleanName: %s\n", cleanName.c_str());
        // e.g. L"First Drive"
        wprintf(L"volumeName: %s\n\n", volumeName);

        everyHere.devices.insert(std::pair<std::wstring, DeviceData>(cleanName, DeviceData()));
        DeviceData & deviceData = everyHere.devices[cleanName];

        DirectoryTraverse traverse(deviceData, drivePath.c_str());

        directoryTraverse(traverse, drivePath.c_str());

        if(traverse.fileCount)
            deviceData.save((cleanName + std::wstring(L".csv")).c_str(), drivePath.c_str(), volumeName, cleanName.c_str());
    }
};



void DeviceData::save(const wchar_t* fileName, const wchar_t* drivePath, const wchar_t* volumeName, const wchar_t* cleanName)
{
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
    if(cleanName)
    {
        fileData += "# cleanName=";
        fileData += to_string(cleanName);
        fileData += "\n";
    }
    // 2: order: size, fileName, write, path, creat, access
    fileData += "# version=2\n";
    fileData += "# size_t size, fileName, __time64_t write, path, __time64_t create, __time64_t access\n";
    // start marker
    fileData += "#\n";

    for (auto it : files)
    {
        char str[MAX_PATH + 100];
        sprintf_s(str, sizeof(str), "%llu,%s,%llu,%s,%llu,%llu\n",
            // key
            it.first.size,
            to_string(it.first.fileName.c_str()).c_str(),
            it.first.time_write,
            // value
            to_string(it.second.path.c_str()).c_str(),
            it.second.time_create,
            it.second.time_access);
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

void DeviceData::printUniques()
{
    for (auto it : files)
    {
        size_t count = files.count(it.first);
        // show only uniques
        if (count == 1)
            continue;

        char timeStr[80];
        dateToCString(it.first.time_write, timeStr);

        printf("#%d %s %8llu %s/%s\n",
            (int)count,
            timeStr,
            it.first.size,
            to_string(it.second.path.c_str()).c_str(),
            to_string(it.first.fileName.c_str()).c_str());
    }
    printf("\n");
}

void EveryHere::gatherData() 
{
    DriveTraverse drives(*this);
    driveTraverse(drives);
}

void EveryHere::loadCSV(const wchar_t* internalName)
{
    assert(internalName);

    CASCIIFile file;
    if(!file.IO_LoadASCIIFile(internalName))
        return;

    const Char* p = (const Char *)file.GetDataPtr();

    std::string line;


    devices.insert(std::pair<std::wstring, DeviceData>(internalName, DeviceData()));
    DeviceData& deviceData = devices[internalName];

    bool error = false;

    std::string sFileName;
    std::string sPath;

    while(*p)
    {
        // example line:
        // D:/oldCode/Work/Playfield_MipMapper/TGA.h,902,145635462,145635462,145635462
        parseWhiteSpaceOrLF(p);

        if (parseStartsWith(p, "#"))
        {
            parseToEndOfLine(p);
            continue;
        }
 
        int64 size = 0;

        FileKey el;
        el.fileName = to_wstring(sFileName);

        if (!parseInt64(p, size) ||
            !parseStartsWith(p, ","))
        {
            error = true;
            break;
        }

        parseLine(p, sFileName, true);

        if (!parseInt64(p, el.time_write) ||
            !parseStartsWith(p, ","))
        {
            error = true;
            break;
        }

        parseLine(p, sPath, true);

        FileValue val;
        val.path = to_wstring(sPath);

        if (!parseInt64(p, size) ||
            !parseStartsWith(p, ",") ||
            !parseInt64(p, el.time_write) ||
            !parseStartsWith(p, ",") ||
            !parseInt64(p, val.time_create) ||
            !parseStartsWith(p, ",") ||
            !parseInt64(p, val.time_access))
        {
            error = true;
            break;
        }

        el.size = size;

        parseLineFeed(p);

//        if (deviceData.files.find(el) == deviceData.files.end())
//            ++uniqueCount;

        deviceData.files.insert(std::pair<FileKey, FileValue>(el, val));
    }

    deviceData.save(std::wstring(L"test.csv").c_str());
}



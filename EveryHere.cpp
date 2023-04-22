#include "EveryHere.h"
#include <time.h>   // _localtime64_s()
#include "Timer.h"
#include "ASCIIFile.h"
#include "FileSystem.h"



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
    }

    void logState()
    {
        wprintf(L"%s %lld files %lld dups  %.0f sec\n",
            path,
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

        if (deviceData.files.find(el) == deviceData.files.end())
            ++uniqueCount;

        deviceData.files.insert(std::pair<FileKey, FileValue>(el, val));
        ++fileCount;

        double nowTime = g_Timer.GetAbsoluteTime();
        if(nowTime > nextPrintTime) 
        {
            // every second
            nextPrintTime = nowTime + 1.0;
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

    virtual void OnDrive(const FilePath& filePath, const wchar_t* deviceName, const wchar_t* internalName, const wchar_t* volumeName) {
        std::wstring drive = filePath.path;

        if (!drive.empty() && drive.back() == '\\')
            drive.pop_back();

        std::wstring cleanName = internalName;
        // e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\"
        replace_all(cleanName, L"\\", L"");
        replace_all(cleanName, L"/", L"");
        replace_all(cleanName, L"?", L"");

        wprintf(L"filePath: %s\n", drive.c_str());
        wprintf(L"deviceName: %s\n", deviceName);
        wprintf(L"cleanName: %s\n", cleanName.c_str());
        wprintf(L"volumeName: %s\n\n", volumeName);

        everyHere.devices.insert(std::pair<std::wstring, DeviceData>(internalName, DeviceData()));
        DeviceData & deviceData = everyHere.devices[internalName];

        DirectoryTraverse traverse(deviceData, drive.c_str());

        directoryTraverse(traverse, drive.c_str());

        if(traverse.fileCount)
            deviceData.save((cleanName + std::wstring(L".csv")).c_str());
    }
};



void DeviceData::save(const wchar_t* fileName)
{
    CASCIIFile file;
    std::string fileData;
    // to avoid reallocations
    fileData.reserve(10 * 1024 * 1024);

    for (auto it : files)
    {
        //            size_t count = files.count(it.first);
        //            // show only duplicates
        //            if (count == 1)
        //                continue;

        struct tm tm;
        _localtime64_s(&tm, &it.first.time_write);
        char dateTimeStr[80];
        // https://support.echo360.com/hc/en-us/articles/360035034372-Formatting-Dates-for-CSV-Imports#:~:text=Some%20of%20the%20CSV%20imports,program%20like%20Microsoft%C2%AE%20Excel.
        strftime(dateTimeStr, sizeof(dateTimeStr), "%Y/%m/%d %H:%M:%S", &tm);

        char str[MAX_PATH + 100];
        sprintf_s(str, sizeof(str), "%s,%d,%s/%s\n",
            dateTimeStr,
            it.first.size,
            to_string(it.second.path.c_str()).c_str(),
            to_string(it.first.fileName.c_str()).c_str());
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

        struct tm tm;
        _localtime64_s(&tm, &it.first.time_write);
        char timeStr[80];
        strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %H:%M:%S", &tm);

        printf("#%d %s %8d %s/%s\n",
            (int)count,
            timeStr,
            it.first.size,
            to_string(it.second.path.c_str()).c_str(),
            to_string(it.first.fileName.c_str()).c_str());
    }
    printf("\n");
}

void EveryHere::GatherData() 
{
    DriveTraverse drives(*this);
    driveTraverse(drives);

}


#pragma once
#include <vector>
#include <mutex>
#include <memory> // std::shared_ptr<>
#include "FileSystem.h"
#include "EFileList.h"
#include "SelectionRange.h"

struct DriveInfo2 : public IFileLoadSink
{
    // e.g. "C:\"
    std::string drivePath;
    // e.g. "\Device\HarddiskVolume4", must not be 0
    std::string deviceName;
    // e.g. "\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\", must not be 0
    std::string internalName;
    // e.g. "First Drive", must not be 0
    std::string volumeName;
    // e.g. "MainPC"
    std::string computerName;
    // e.g. "Hans"
    std::string userName;
    // e.g. "MyPC.efu"
    std::string efuFileName;
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationw
    int64 driveFlags = 0;
    //
    int64 serialNumber = 0;
    // -1 means unknown
    int64 freeSpace = -1;
    // -1 means unknown
    int64 totalSpace = -1;
    // use dateToCString()
    __time64_t date = {};

    // 
    bool localDrive = false;
    //
    bool newestEntry = false;

    void load();


    // protecting fileListEx
    std::mutex mutex;
    // protected by mutex
    // data from efuFileName, maybe be 0
    std::shared_ptr<FileList> fileListEx;

    size_t getFileListSize();

private:

    // interface IFileLoadSink

    virtual void onIncomingFiles(const FileList& incomingFileList);
};

class WindowDrives
{
public:
    void gui();

    bool showWindow = true;

    // elements must not be 0
    std::vector<std::shared_ptr<DriveInfo2> > drives;

private:
    // into everyHere.driveView[]
    SelectionRange driveSelectionRange;
    //
    double whenToRebuildView = -1;

    void rescan();
    void popup();
    void openDrive();
};


#pragma once
#include <vector>
#include <memory> // std::shared_ptr<>
#include "FileSystem.h"
#include "EFileList.h"
#include "SelectionRange.h"

struct DriveInfo2
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

    // data from efuFileName, maybe be 0
    std::shared_ptr<EFileList> fileList;

    void load();
};

class WindowDrives
{
public:
    void gui();

    bool showWindow = true;

private:
    std::vector<DriveInfo2> drives;
    // into everyHere.driveView[]
    SelectionRange driveSelectionRange;
    //
    double whenToRebuildView = -1;

    void rescan();
    void popup();
    void openDrive();
};


#pragma once
#include <vector>
#include "FileSystem.h"
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
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationw
    uint32 driveFlags = 0;
    //
    uint32 serialNumber = 0;
    // -1 means unknown
    int64 freeSpace = -1;
    // -1 means unknown
    int64 totalSpace = -1;
};

class WindowDrives
{
public:
    void gui(bool& show);


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


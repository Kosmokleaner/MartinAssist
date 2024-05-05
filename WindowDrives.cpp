#include "WindowDrives.h"
#include "ImGui/imgui.h"
#include "FileSystem.h"
#include "ASCIIFile.h"
#include "Timer.h"
#include "Gui.h"
#include <time.h>
#include <windows.h>  // GetDiskFreeSpaceW()
#undef min
#undef max

#pragma warning(disable: 4996) // open and close depreacted
#pragma warning(disable: 4100) // unreferenced formal parameter

#define SERIALIZE_VERSION "2"

#include <io.h>															// open
#include <fcntl.h>														// O_RDONLY

#include <time.h>   // _localtime64_s()

void dateToCString(__time64_t t, char outTimeStr[80])
{
    struct tm tm;
    errno_t err = _localtime64_s(&tm, &t);
    assert(!err);
    if (err)
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
    if (!ok)
        return 0;
    ULARGE_INTEGER v_ui;
    v_ui.LowPart = v_ftime.dwLowDateTime;
    v_ui.HighPart = v_ftime.dwHighDateTime;
    return v_ui.QuadPart;
}


bool IO_FileExists(const char* Name)
{
    assert(Name);
    int handle;

    handle = open(Name, O_RDONLY);
    if (handle == -1)
        return false;

    close(handle);

    return true;
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

// managed all .EFU files in the EFUs directory
class EFUManager
{
public:

    void generateNewFileName(DriveInfo2& drive, std::string &out)
    {
        std::string volumeName = drive.volumeName;

        replace_all(volumeName, " ", "_");
        replace_all(volumeName, "\\", "");
        replace_all(volumeName, "/", "");
        replace_all(volumeName, "?", "");

        static int counter = 0;
        for(;;)
        {
            ++counter;
            char filename[256];
            sprintf_s(filename, 256, EFU_FOLDER "\\%s_%d", volumeName.c_str(), counter);

            if(!IO_FileExists(filename))
            {
                out = filename;
                return;
            }
        }
    }
};

void DriveInfo2::load()
{
    fileList = std::make_shared<EFileList>();
    fileList->load(::to_wstring(efuFileName).c_str());
}

void build(DriveInfo2 & drive)
{
    std::string cmdLine;
    std::string fileName;

    EFUManager manager;

    manager.generateNewFileName(drive, fileName);

    cmdLine = std::string("-export-efu \"") + fileName.c_str() + ".efu\" \"" + drive.drivePath + "\"";

    // todo: this only works in everything is started

    const char* command = "Everything\\es.exe";

    printf("%s %s\n", command, cmdLine.c_str());

//    HINSTANCE inst = 
    ShellExecuteA(0, 0, command, cmdLine.c_str(), 0, SW_HIDE);

    CASCIIFile file;
    std::string fileData;
    // to avoid reallocations
    fileData.reserve(10 * 1024 * 1024);
    char str[1024];

#define EL_STRING(NAME) \
    if (!drive.NAME.empty()) \
    {\
        fileData += "# " #NAME "="; \
        fileData += drive.NAME; \
        fileData += "\n"; \
    }
#define EL_INT64(NAME) \
    sprintf_s(str, sizeof(str), "# " #NAME "=%llu\n", drive.NAME); \
    fileData += str;

    EL_STRING(drivePath);
    EL_STRING(deviceName);
    EL_STRING(internalName);
    EL_STRING(volumeName);
    EL_STRING(computerName);
    EL_STRING(userName);
    EL_STRING(efuFileName);
    EL_INT64(freeSpace);
    EL_INT64(totalSpace);
// EL_INT64(driveType);
    EL_INT64(driveFlags);
    EL_INT64(serialNumber);
    EL_INT64(date);

#undef EL_STRING
#undef EL_INT64

    fileData += "# version=" SERIALIZE_VERSION "\n";
    fileData += "#\n";

    // does not include 0 termination
    size_t len = fileData.length();
    // wasteful
    const char* cpy = (const char*)malloc(len + 1);
    memcpy((void*)cpy, fileData.c_str(), len + 1);
    file.CoverThisData(cpy, len);

    file.IO_SaveASCIIFile((fileName + ".txt").c_str());
}

void WindowDrives::gui(bool& show)
{
    if (!show)
        return;

    static bool first = true;
    if (first)
    {
        first = false;
        whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.1f;
    }

    if (whenToRebuildView != -1 && g_Timer.GetAbsoluteTime() > whenToRebuildView)
    {
        rescan();
    }


    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Drives", &show, ImGuiWindowFlags_NoCollapse);

    // 0:Local Drives, 1: Historical Data
    int driveTabId = 0;

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("DriveLocality", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Local Drives"))
        {
            driveTabId = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Latest EFUs"))
        {
            driveTabId = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("All EFUs"))
        {
            driveTabId = 2;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
/*
    const char* refreshIcon = "\xef\x83\xa2";

//    ImGui::SameLine();
//    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(refreshIcon).x - 2 * ImGui::GetStyle().ItemSpacing.x);

    if (ImGui::Button("\xef\x83\xa2")) // Refresh icon
    {
        drives.clear();
        driveSelectionRange = {};
        whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.1f;

//        everyHere.gatherData();
//        setViewDirty();
    }
    if (BeginTooltipPaused())
    {
        ImGui::TextUnformatted("Rescan");
        EndTooltip();
    }
*/
    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

    {
        for (int line_no = 0; line_no < drives.size(); line_no++)
        {
            DriveInfo2& drive = drives[line_no];

            if(driveTabId == 0 && !drive.localDrive)
                continue;
            if (driveTabId != 0 && drive.localDrive)
                continue;

            if (driveTabId == 1 && !drive.newestEntry)
                continue;

            double gb = 1024 * 1024 * 1024;
            double free = drive.freeSpace / gb;
            double total = drive.totalSpace / gb;

            float fraction = total ? (float)(free / total) : 1.0f;
            char overlay_buf[32];
            sprintf_s(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5f, 0.5f, 1.0f, 1));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1));
            float oldY = ImGui::GetCursorPosY();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY());
            ImGui::ProgressBar(fraction, ImVec2(ImGui::GetFontSize() * 3, ImGui::GetFontSize()), overlay_buf);

            ImGui::PopStyleColor(3);
    //        if (BeginTooltipPaused())
    //        {
    //            ImGui::Text("%llu files", drive.progressFileCount);
    //            EndTooltip();
    //        }
            ImGui::SameLine();
            ImGui::SetCursorPosY(oldY - ImGui::GetStyle().FramePadding.y);

            char item[1024];
            // hard drive symbol
            sprintf_s(item, sizeof(item) / sizeof(*item), "\xef\x87\x80  %s  %s", drive.drivePath.c_str(), drive.volumeName.c_str());

            bool selected = driveSelectionRange.isSelected(line_no);
            ImGuiSelectable(item, &selected);

            if (ImGui::IsItemClicked(0))
            {
                driveSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);

                drive.load();
                g_gui.files.set(drive);

    //            setViewDirty();
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (!driveSelectionRange.isSelected(line_no))
                {
                    driveSelectionRange.reset();
                    driveSelectionRange.onClick(line_no, false, false);
                }

                popup();

                ImGui::EndPopup();
            }
            if(ImGui::IsItemFocused() && ImGui::IsMouseDoubleClicked(0))
            {
                driveSelectionRange.reset();
                driveSelectionRange.onClick(line_no, false, false);
                openDrive();
            }
            if (BeginTooltipPaused())
            {    
                ImGui::Text("drivePath: '%s'", drive.drivePath.c_str());
                ImGui::Text("deviceName: '%s'", drive.deviceName.c_str());
                ImGui::Text("internalName: '%s'", drive.internalName.c_str());
                ImGui::Text("volumeName: '%s'", drive.volumeName.c_str());
                ImGui::Text("computerName: '%s'", drive.computerName.c_str());
                ImGui::Text("efuFileName: '%s'", drive.efuFileName.c_str());
                ImGui::Text("userName: '%s'", drive.userName.c_str());
                ImGui::Text("driveFlags: %x", drive.driveFlags);
                ImGui::Text("serialNumber: %x", drive.serialNumber);
                ImGui::Text("newestEntry: %d", drive.newestEntry);
                {
                    double value;
                    const char *unit = computeReadableSize(drive.freeSpace, value);
                    ImGui::Text("freeSpace: ");
                    ImGui::SameLine();
                    ImGui::Text(unit, value);
                }
                {
                    double value;
                    const char* unit = computeReadableSize(drive.totalSpace, value);
                    ImGui::Text("totalSpace: ");
                    ImGui::SameLine();
                    ImGui::Text(unit, value);
                }
                if(drive.date)
                {
                    char date[80];
                    dateToCString(drive.date, date);
                    ImGui::Text("date: %s", date);
                }
                if(drive.fileList)
                    ImGui::Text("fileCount: %llu", (uint64)(drive.fileList->entries.size()));
                else
                    ImGui::Text("fileCount: ? (load with left click)");
                //            bool supportsRemoteStorage = drive.driveFlags & 0x100;

                EndTooltip();
            }
            if(drive.date && !drive.localDrive)
            {
                char date[80];
                dateToCString(drive.date, date);

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 1, 1, 0.5f), " %s", date);
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1,1,1,0.5f), " %.1f / %.1f GB", free, total);
        }
    }

    if (ImGui::BeginPopupContextWindow())
    {
        popup();

        ImGui::EndPopup();
    }
    ImGui::EndChild();

    ImGui::End();
}

void WindowDrives::openDrive()
{
    driveSelectionRange.foreach([&](int64 index) {
        auto& drive = drives[index];
        FilePath filePath(to_wstring(drive.drivePath).c_str());
        ShellExecuteA(0, 0, to_string(filePath.extractPath()).c_str(), 0, 0, SW_SHOW);
        });

}

void WindowDrives::popup()
{
    if (driveSelectionRange.count() == 1)
    {
        if (ImGui::MenuItem("Open drive (in Explorer)"))
        {
            openDrive();
        }
        ImGui::Separator();
    }

    // can be optimized
    bool rescan = false;

    if (driveSelectionRange.count() >= 1)
    {
        if (ImGui::MenuItem("Build EFU for selected drive(s)"))
        {
            driveSelectionRange.foreach([&](int64 index) {
                DriveInfo2& ref = drives[index];
                build(ref);
                });
            rescan = true;
        }
    }
    if (ImGui::MenuItem("Build EFU for each local drive"))
    {
        for (auto& ref : drives)
            build(ref);

        rescan = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Rescan") || rescan)
    {
        drives.clear();
        driveSelectionRange = {};
        whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.1f;
    }
}

class DriveScan : public IDriveTraverse
{
    std::vector<DriveInfo2>& drives;
    std::string computerName;
    std::string userName;
    __time64_t date = {};
public:
    DriveScan(std::vector<DriveInfo2> &inDrives)
        : drives(inDrives)
    {
        char name[256];
        DWORD size = sizeof(name);
        if (GetComputerNameA(name, &size))
        {
            // e.g. "RYZEN"
            computerName = name;
        }
        if (GetUserNameA(name, &size))
        {
            // e.g. "Hans"
            userName = name;
        }
        date = _time64(0);
    }
    virtual void OnDrive(const DriveInfo& driveInfo)
    {
        DriveInfo2 el;

        el.drivePath = to_string(driveInfo.drivePath.path);
        el.deviceName = to_string(driveInfo.deviceName);
        el.internalName = to_string(driveInfo.internalName);
        el.volumeName = to_string(driveInfo.volumeName);
        el.driveFlags = driveInfo.driveFlags;
        el.serialNumber = driveInfo.serialNumber;
        el.localDrive = true;
        el.computerName = computerName;
        el.userName = userName;
        el.date = date;
        DWORD lpSectorsPerCluster;
        DWORD lpBytesPerSector;
        DWORD lpNumberOfFreeClusters;
        DWORD lpTotalNumberOfClusters;
        if(GetDiskFreeSpaceA(el.drivePath.c_str(), &lpSectorsPerCluster, &lpBytesPerSector, &lpNumberOfFreeClusters, &lpTotalNumberOfClusters))
        {
            el.freeSpace = (uint64)lpSectorsPerCluster * lpBytesPerSector * lpNumberOfFreeClusters;
            el.totalSpace = (uint64)lpSectorsPerCluster * lpBytesPerSector * lpTotalNumberOfClusters;
        }

        drives.push_back(el);
    }
};

class FolderScan : public IDirectoryTraverse
{
    std::vector<DriveInfo2>& drives;
public:
    FolderScan(std::vector<DriveInfo2>& inDrives)
        : drives(inDrives)
    {
    }

    // @param filePath without directory e.g. L"D:\temp"
    // @param directory name e.g. L"sub"
    // @return true to recurse into the folder
    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData)
    {
        // no sub directories for now
        return false;
    }
    // @param path without file
    // @param file with extension
    // @param progress 0:unknown or just started .. 1:done
    virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData, float progress)
    {
        DriveInfo2 el;

        CASCIIFile txtFile;

        FilePath fullPath = path;
        fullPath.Append(file);
        if (!txtFile.IO_LoadASCIIFile(fullPath.path.c_str()))
            return;

        fullPath.RemoveExtension();
        fullPath.path += (L".efu");
        el.efuFileName = to_string(fullPath.path);

        const Char* p = (const Char*)txtFile.GetDataPtr();

        std::string keyName;
        std::string valueName;
        bool error = false;

        while (*p)
        {
            parseWhiteSpaceOrLF(p);

            const Char* backup = p;

            if (parseStartsWith(p, "#"))
            {
                // e.g.
                // # drivePath = E:
                // # volumeName = RyzenE
                // # cleanName = Volume{ca72ef4c-0000-0000-0000-100000000000}
                // # version = 2
                parseWhiteSpaceOrLF(p);
                if (parseName(p, keyName))
                {
                    parseWhiteSpaceOrLF(p);
                    if (parseStartsWith(p, "="))
                    {
                        parseWhiteSpaceOrLF(p);
                        if (parseLine(p, valueName))
                        {
#define EL_STRING(NAME) if (keyName == #NAME) el.NAME = valueName
#define EL_INT64(NAME) if (keyName == #NAME) el.NAME = stringToInt64(valueName.c_str())

                            EL_STRING(drivePath);
                            EL_STRING(deviceName);
                            EL_STRING(internalName);
                            EL_STRING(volumeName);
                            EL_STRING(computerName);
                            EL_STRING(userName);
                            EL_STRING(efuFileName);
                            EL_INT64(freeSpace);
                            EL_INT64(totalSpace);
                            // EL_INT64(driveType);
                            EL_INT64(driveFlags);
                            EL_INT64(serialNumber);
                            EL_INT64(date);

#undef EL_STRING
#undef EL_INT64
                            if (keyName == "version" && valueName != SERIALIZE_VERSION)
                            {
                                error = true;
                                break;
                            }
                        }
                    }
                }
            }
            p = backup;
            parseToEndOfLine(p);
            continue;
        }

        if (!error)
        {
            drives.push_back(el);
        }
    }
};

void WindowDrives::rescan()
{
    drives.clear();

    // local drives
    {
       DriveScan scan(drives);
        driveTraverse(scan);
    }

    // all EFUs
    {
        FolderScan scan(drives);
        directoryTraverse(scan, L"EFUs", L"*.txt");
    }

    struct CustomLessFile
    {
        bool operator()(const DriveInfo2& A, const DriveInfo2& B) const
        {
            int64 delta;

            delta = strcmp(A.computerName.c_str(), B.computerName.c_str());
            if (delta != 0) return delta < 0;
            delta = strcmp(A.drivePath.c_str(), B.drivePath.c_str());
            if(delta != 0) return delta < 0;
            delta = strcmp(A.deviceName.c_str(), B.deviceName.c_str());
            if (delta != 0) return delta < 0;
            delta = strcmp(A.internalName.c_str(), B.internalName.c_str());
            if (delta != 0) return delta < 0;
            delta = A.date - B.date;
            if (delta != 0) return delta < 0;

            // qsort() is instable so always return a way to differenciate items.
            // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
            return &A < &B;
        }
    };

    CustomLessFile customLess = { };

    std::sort(drives.begin(), drives.end(), customLess);

    // set newestEntry
    {
        std::string internalName;
        bool first = true;
        for(auto& el : drives)
        {
            if(el.internalName != internalName)
                first = true;

            el.newestEntry = first;
            first = false;
            internalName = el.internalName;
        }
    }

    whenToRebuildView = -1;
}
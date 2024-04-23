#include "WindowDrives.h"
#include "ImGui/imgui.h"
#include "FileSystem.h"
#include "Gui.h"
#include <windows.h>  // GetDiskFreeSpaceW()
#undef min
#undef max

void WindowDrives::gui(bool& show)
{
    static bool first = true;
    if (first)
    {
        first = false;
        rescan();
    }


    if (!show)
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Drives", &show, ImGuiWindowFlags_NoCollapse);

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0)); // Tighten spacing
//    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0)); // Tighten spacing
    for (int line_no = 0; line_no < drives.size(); line_no++)
    {
        DriveInfo2& drive = drives[line_no];

        double gb = 1024 * 1024 * 1024;
        double free = drive.freeSpace / gb;
        double total = drive.totalSpace / gb;

        float fraction = total ? (float)(free / total) : 1.0f;
        char overlay_buf[32];
        sprintf_s(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5f, 0.5f, 1.0f, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
        ImGui::ProgressBar(fraction, ImVec2(ImGui::GetFontSize() * 3, ImGui::GetFontSize()), overlay_buf);
        ImGui::PopStyleColor(3);
//        if (BeginTooltipPaused())
//        {
//            ImGui::Text("%llu files", drive.progressFileCount);
//            EndTooltip();
//        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y);

        char item[1024];
        // hard drive symbol
        sprintf_s(item, sizeof(item) / sizeof(*item), "\xef\x87\x80  '%s'", drive.volumeName.c_str());

        bool selected = driveSelectionRange.isSelected(line_no);
        ImGui::Selectable(item, selected);


        if (ImGui::IsItemClicked(0))
        {
            driveSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
//            setViewDirty();
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (!driveSelectionRange.isSelected(line_no))
            {
                driveSelectionRange.reset();
                driveSelectionRange.onClick(line_no, false, false);
            }

            if (driveSelectionRange.count() == 1)
            {
                if (ImGui::MenuItem("Scan"))
                {
//todo                    drive.rebuild();
                }
//                if (ImGui::MenuItem("Open path (in Explorer)"))
//                {
//                    FilePath filePath(to_wstring(drive.csvName + ".csv").c_str());
//
//                    ShellExecuteA(0, 0, to_string(filePath.extractPath()).c_str(), 0, 0, SW_SHOW);
 //               }
            }
            ImGui::EndPopup();
        }
        if(ImGui::IsMouseDoubleClicked(0))
        {
            // todo
        }
        if (BeginTooltipPaused())
        {
            ImGui::Text("deviceName: '%s'", drive.deviceName.c_str());
            ImGui::Text("internalName: '%s'", drive.internalName.c_str());
            ImGui::Text("volumeName: '%s'", drive.volumeName.c_str());
            ImGui::Text("driveFlags: %x", drive.driveFlags);
            ImGui::Text("serialNumber: %x", drive.serialNumber);
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
            //            bool supportsRemoteStorage = drive.driveFlags & 0x100;

            EndTooltip();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1,1,1,0.5f), " %.0f / %.0f GB", free, total);
    }
    ImGui::PopStyleVar(1);
    ImGui::EndChild();

    ImGui::End();
}

class DriveScan : public IDriveTraverse
{
    std::vector<DriveInfo2>& drives;
public:
    DriveScan(std::vector<DriveInfo2> &inDrives)
        : drives(inDrives)
    {
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

void WindowDrives::rescan()
{
    DriveScan driveScan(drives);

    driveTraverse(driveScan);
}
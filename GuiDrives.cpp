#include "ImGui/imgui.h"
#include "Gui.h"
#include "ImGui/imgui_stdlib.h"
#include "FileSystem.h"
#include "ImGui/imgui_internal.h"
#include <shlobj.h> // DeleteFile()
#include <thread>


class LoadCVSFiles : public IDirectoryTraverse
{
    EveryHere& everyHere;
public:
    LoadCVSFiles(EveryHere& inEveryHere)
        : everyHere(inEveryHere)
    {
        everyHere.freeData();
    }

    void OnEnd()
    {
        everyHere.updateLocalDriveState();
    }

    bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData)
    {
        filePath;directory;findData;
        return false;
    }
    void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData, float progress)
    {
        findData; progress;

        FilePath combined = path;
        combined.Append(file);

        OutputDebugStringA("loadCSV '");
        OutputDebugStringW(file);
        OutputDebugStringA("'\n");

        everyHere.loadCSV(combined.path.c_str());
    }
};

void asyncDriveBuild(EveryHere& everyHere, std::mutex& everyHere_mutex, DriveData& formerDrive)
{
    formerDrive.progressPercent = 0.0f;
    formerDrive.progressFileCount = 0;

    const DriveInfo &info = formerDrive.driveInfo;

    IProgressThreadCallback* progressThreadCallback = &formerDrive;

//    auto ret = 
    new std::thread([&everyHere, &everyHere_mutex, info, progressThreadCallback]
    {
        DriveData *tempDrive = new DriveData;
        tempDrive->driveInfo = info;

        tempDrive->rebuild(progressThreadCallback);

        {
            std::unique_lock<std::mutex> lock(everyHere_mutex);

            const uint32 driveId = (uint32)everyHere.driveData.size();

            // adjust driveId on all entries
            for(auto& entry : tempDrive->entries)
                entry.value.driveId = driveId;

            everyHere.driveData.push_back(tempDrive);
        }
    });
    // todo: free ret ?
}

void printRightAlignedSize(uint64 bytes)
{
    if(!bytes)
        return;

    double printSize = 0;
    const char* printUnit = computeReadableSize(bytes, printSize);

    // right aligned
    char str[256];
    sprintf_s(str, sizeof(str), printUnit, printSize);
    // https://stackoverflow.com/questions/58044749/how-to-right-align-text-in-imgui-columns
    float posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(str).x - ImGui::GetScrollX());
    if (posX > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(posX);
    ImGui::TextUnformatted(str);
}

void Gui::guiDrives(bool &show)
{
    if(!show)
        return;

    static bool localDrivesColumns[DCID_MAX] = { false };
    localDrivesColumns[DCID_VolumeName] = true;
    localDrivesColumns[DCID_Path] = true;
    localDrivesColumns[DCID_Computer] = true;
    localDrivesColumns[DCID_User] = true;
    localDrivesColumns[DCID_freeSpace] = true;
    localDrivesColumns[DCID_totalSpace] = true;
    localDrivesColumns[DCID_type] = true;
    localDrivesColumns[DCID_serial] = true;
    localDrivesColumns[DCID_Actions] = true;

    static bool historicalDataColumns[DCID_MAX] = { false };
    historicalDataColumns[DCID_VolumeName] = true;
    historicalDataColumns[DCID_Path] = true;
    historicalDataColumns[DCID_Computer] = true;
    historicalDataColumns[DCID_User] = true;
    historicalDataColumns[DCID_freeSpace] = true;
    historicalDataColumns[DCID_totalSpace] = true;
    historicalDataColumns[DCID_type] = true;
    historicalDataColumns[DCID_serial] = true;
    historicalDataColumns[DCID_Size] = true;
    historicalDataColumns[DCID_Files] = true;
    historicalDataColumns[DCID_Directories] = true;
    historicalDataColumns[DCID_Date] = true;
    historicalDataColumns[DCID_Actions] = true;
//    historicalDataColumns[DCID_selectedFiles] = true;

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 100), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(1050, 580), ImGuiCond_FirstUseEver);

    ImGui::Begin("EveryHere Drives", &show, ImGuiWindowFlags_NoCollapse);

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
        if (ImGui::BeginTabItem("Historical Data"))
        {
            driveTabId = 1;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    bool* columns = (bool*)((driveTabId == 1) ? &historicalDataColumns : &localDrivesColumns);

    if (ImGui::Button("build"))
    {
        everyHere.gatherData();
        setViewDirty();
    }

    ImGui::SameLine();


    if (ImGui::Button("load"))
    {
        LoadCVSFiles loadCVSFiles(everyHere);
        directoryTraverse(loadCVSFiles, FilePath(), L"*.csv");
        setViewDirty();
        everyHere.buildUniqueFiles();
    }

    {
        ImGuiTableFlags flags =
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_Resizable |
//            ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable |
            ImGuiTableFlags_Sortable |
            ImGuiTableFlags_SortMulti;

        uint32 numberOfColumns = 0;
        for (uint32 column = 0; column < DCID_MAX; ++column)
        {
            if(columns[column])
                ++numberOfColumns;
        }
        assert(numberOfColumns);

        if (ImGui::BeginTable((driveTabId == 1) ? "HistoricalData" : "LocalDrives", numberOfColumns, flags))
        {
            std::string line;

            pushTableStyle3();
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

            const char* s0 = getDriveColumnName(DCID_freeSpace);
            const char* s1 = getDriveColumnName(DCID_totalSpace);

            for(uint32 column = 0; column < DCID_MAX; ++column)
            {
                if (columns[column])
                    ImGui::TableSetupColumn(getDriveColumnName((DriveColumnID)column), ImGuiTableColumnFlags_WidthFixed, 0.0f, column);
            }
/*
            ImGui::TableSetupColumn("VolumeName", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_VolumeName);
            //                    ImGui::TableSetupColumn("UniqueName", ImGuiTableColumnFlags_None, 0.0f, DCID_UniqueName);
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_Path);
            //                    ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_None, 0.0f, DCID_DeviceId);
            if (driveTabId == 1) // date makes no sense for local drives
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_Size);
            if (driveTabId == 1) // date makes no sense for local drives
                ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_Files);
            if (driveTabId == 1) // date makes no sense for local drives
                ImGui::TableSetupColumn("Directories", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0.0f, DCID_Directories);
            ImGui::TableSetupColumn("Computer", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_Computer);
            ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0.0f, DCID_User);
            if (driveTabId == 1) // date makes no sense for local drives
                ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_Date);
            ImGui::TableSetupColumn("totalSpace", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_totalSpace);
            ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0.0f, DCID_type);
            ImGui::TableSetupColumn("serial", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0.0f, DCID_serial);
            if (driveTabId == 1) // date makes no sense for local drives
                ImGui::TableSetupColumn("selected Files", ImGuiTableColumnFlags_WidthFixed, 0.0f, DCID_selectedFiles);
*/
            ImGui::TableHeadersRow();

            everyHere.buildDriveView(ImGui::TableGetSortSpecs());

            int line_no = 0;
            for (auto it = everyHere.driveView.begin(), end = everyHere.driveView.end(); it != end; ++it, ++line_no)
            {
                DriveData& drive = *everyHere.driveData[*it];

//started unify                if (driveTabId == 0 && !drive.isLocalDrive)
//                    continue;
//                if (driveTabId == 1 && drive.isLocalDrive)
//                    continue;

                ImGui::TableNextRow();

                ImGui::PushID(line_no);

                ImGui::PushStyleColor(ImGuiCol_Text, drive.isLocalDrive ? ImVec4(0.5f, 0.5f, 1.0f, 1) : ImVec4(0.8f, 0.8f, 0.8f, 1));

                int columnId = 0;

                if (columns[DCID_VolumeName])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                    bool selected = driveSelectionRange.isSelected(line_no);
                    ImGui::Selectable(drive.volumeName.c_str(), &selected, selectable_flags);
                    if (ImGui::IsItemClicked(0))
                    {
                        driveSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                        setViewDirty();
                    }
                    if (BeginTooltipPaused())
                    {
                        ImGui::Text("UniqueName: %s.csv", drive.csvName.c_str());
                        ImGui::Text("DriveId: %d", drive.driveId);
                        EndTooltip();
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
                            if (ImGui::MenuItem("Delete .csv File"))
                            {
                                DeleteFileA((drive.csvName + ".csv").c_str());
                                drive.markedForDelete = true;
                            }
                            if (ImGui::MenuItem("[Re]build .csv"))
                            {
                                drive.rebuild();
                            }
                            if (ImGui::MenuItem("Open path (in Explorer)"))
                            {
                                FilePath filePath(to_wstring(drive.csvName + ".csv").c_str());

                                ShellExecuteA(0, 0, to_string(filePath.extractPath()).c_str(), 0, 0, SW_SHOW);
                            }
                        }
                        ImGui::EndPopup();
                    }
                }

                if (columns[DCID_Path])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::TextUnformatted(drive.drivePath.c_str());
                }

                if (columns[DCID_Size])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    {
                        double printSize = 0;
                        const char* printUnit = computeReadableSize(drive.statsSize, printSize);
                        ImGui::Text(printUnit, printSize);
                    }
                }

                if (columns[DCID_Files])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::Text("%llu", (uint64)drive.entries.size());
                }

                if (columns[DCID_Directories])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::Text("%llu", drive.statsDirs);
                }

                if (columns[DCID_Computer])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    if (drive.isLocalDrive)
                        ImGui::TextUnformatted("\xef\x80\x95"); // Home house
                    else
                        ImGui::TextUnformatted("\xef\x83\x82"); // Cloud
                    ImGui::SameLine();
                    ImGui::TextUnformatted(drive.computerName.c_str());
                }


                if (columns[DCID_User])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::TextUnformatted(drive.userName.c_str());
                }

                if (columns[DCID_Date])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::TextUnformatted(drive.dateGatheredString.c_str());
                }

                if (columns[DCID_freeSpace])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    printRightAlignedSize(drive.freeSpace);
                }

                if (columns[DCID_totalSpace])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    printRightAlignedSize(drive.totalSpace);
                }

                if (columns[DCID_type])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    bool supportsRemoteStorage = drive.driveFlags & 0x100;
                    ImGui::Text("%d", supportsRemoteStorage ? -(int)(drive.driveType) : drive.driveType);
                }

                if (columns[DCID_serial])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::Text("%u", drive.driveInfo.serialNumber);
                }

                if (columns[DCID_selectedFiles])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::Text("%llu", drive.selectedKeys);
                }

                if (columns[DCID_Actions])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    if(ImGui::SmallButton("\xef\x83\xa2"))    // Refresh
                        asyncDriveBuild(everyHere, everyHere_mutex, drive);
//doesn't work                    TooltipPaused("Refresh / Rebuild");

                    ImGui::SameLine();
                    if(drive.progressPercent >= 0)
                    {
                        float fraction = drive.progressPercent * 0.01f;
                        char overlay_buf[32];
                        ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.1f %%", fraction * 100 + 0.01f);
                        ImGui::ProgressBar(fraction, ImVec2(0.0f, ImGui::GetFontSize()), overlay_buf);
                        if(BeginTooltipPaused())
                        {
                            ImGui::Text("%llu files", drive.progressFileCount);
                            EndTooltip();
                        }
                    }
                }
                

                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::PopStyleColor(3);

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

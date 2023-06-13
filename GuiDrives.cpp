#include "ImGui/imgui.h"
#include "Gui.h"
#include "ImGui/imgui_stdlib.h"
#include "FileSystem.h"
#include "ImGui/imgui_internal.h"
#include <shlobj.h> // DeleteFile()



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
    void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData)
    {
        findData;

        FilePath combined = path;
        combined.Append(file);

        everyHere.loadCSV(combined.path.c_str());
    }
};



void Gui::guiDrives()
{
    static bool localDrivesColumns[DCID_MAX] = { false };
    localDrivesColumns[DCID_VolumeName] = true;
    localDrivesColumns[DCID_Path] = true;
    localDrivesColumns[DCID_Computer] = true;
    localDrivesColumns[DCID_User] = true;
    localDrivesColumns[DCID_totalSpace] = true;
    localDrivesColumns[DCID_type] = true;
    localDrivesColumns[DCID_serial] = true;

    static bool historicalDataColumns[DCID_MAX] = { false };
    historicalDataColumns[DCID_VolumeName] = true;
    historicalDataColumns[DCID_Path] = true;
    historicalDataColumns[DCID_Computer] = true;
    historicalDataColumns[DCID_User] = true;
    historicalDataColumns[DCID_totalSpace] = true;
    historicalDataColumns[DCID_type] = true;
    historicalDataColumns[DCID_serial] = true;
    historicalDataColumns[DCID_Size] = true;
    historicalDataColumns[DCID_Files] = true;
    historicalDataColumns[DCID_Directories] = true;
    historicalDataColumns[DCID_Date] = true;
    historicalDataColumns[DCID_selectedFiles] = true;

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 100), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("EveryHere Drives", 0, ImGuiWindowFlags_NoCollapse);

    // 0:Local Drives, 1: Historical Data
    int driveTabId = 0;

    bool *columns = (bool*)((driveTabId == 1) ? &historicalDataColumns : &localDrivesColumns);

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

    if (ImGui::Button("build"))
    {
        everyHere.gatherData();
        setViewDirty();
    }

    ImGui::SameLine();

    --triggerLoadOnStartup;
    if (triggerLoadOnStartup < -1)
        triggerLoadOnStartup = -1;

#ifdef _DEBUG
    if (triggerLoadOnStartup == 0)
    {
        everyHere.freeData();
        char str[80];
        for (int i = 0; i < 5; ++i)
        {
            str[0] = 'a' + (rand() % 26);
            str[1] = 'a' + (rand() % 26);
            str[2] = 0;
            everyHere.deviceData.push_back(DeviceData());
            DeviceData& d = everyHere.deviceData.back();
            d.csvName = str;
            d.volumeName = str;
            str[0] = 'A' + (rand() % 26);
            str[1] = ':';
            str[2] = 0;
            d.setDrivePath(str);
            d.deviceId = i;
        }

        for (int i = 0; i < 200; ++i)
        {
            int deviceId = rand() % 5;
            DeviceData& r = everyHere.deviceData[deviceId];
            r.entries.push_back(FileEntry());
            FileEntry& e = r.entries.back();
            str[0] = 'a' + (rand() % 26);
            str[1] = 'a' + (rand() % 26);
            str[2] = 0;
            e.key.fileName = str;
            e.key.sizeOrFolder = rand() * 1000;
            e.key.time_write = rand();
            e.value.parent = -1;
            e.value.deviceId = deviceId;
        }
        everyHere.updateLocalDriveState();
        setViewDirty();
        everyHere.buildUniqueFiles();
        triggerLoadOnStartup = -1;
    }
#endif

    if (ImGui::Button("load") || triggerLoadOnStartup == 0)
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
            ImGuiTableFlags_Reorderable |
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
                DeviceData& drive = everyHere.deviceData[*it];

                if (driveTabId == 0 && !drive.isLocalDrive)
                    continue;

                ImGui::TableNextRow();

                ImGui::PushID(line_no);

                ImGui::PushStyleColor(ImGuiCol_Text, drive.isLocalDrive ? ImVec4(0.5f, 0.5f, 1.0f, 1) : ImVec4(0.8f, 0.8f, 0.8f, 1));

                int columnId = 0;

                if (columns[DCID_VolumeName])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                    bool selected = deviceSelectionRange.isSelected(line_no);
                    ImGui::Selectable(drive.volumeName.c_str(), &selected, selectable_flags);
                    if (ImGui::IsItemClicked(0))
                    {
                        deviceSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                        setViewDirty();
                    }
                    if (BeginTooltip())
                    {
                        ImGui::Text("UniqueName: %s.csv", drive.csvName.c_str());
                        ImGui::Text("DeviceId: %d", drive.deviceId);
                        EndTooltip();
                    }

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (!deviceSelectionRange.isSelected(line_no))
                        {
                            deviceSelectionRange.reset();
                            deviceSelectionRange.onClick(line_no, false, false);
                        }

                        if (deviceSelectionRange.count() == 1)
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

                if (columns[DCID_totalSpace])
                {
                    ImGui::TableSetColumnIndex(columnId++);
                    if (drive.totalSpace)
                    {
                        double printSize = 0;
                        const char* printUnit = computeReadableSize(drive.totalSpace, printSize);
                        ImGui::Text(printUnit, printSize);
                    }
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

                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::PopStyleColor(3);

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

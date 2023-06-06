#include <windows.h> // HICON
#include "ImGui/imgui.h"
#include "Gui.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/imgui_internal.h"
#include "FileSystem.h"
#include "Timer.h"





void Gui::guiFiles()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("EveryHere Files", 0, ImGuiWindowFlags_NoCollapse);

    // todo: filter button
    if (ImGui::InputText("filter", &filter))
    {
        setViewDirty();
    }

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    // 0: files, 1:folders
    int filesTabId = 0;
    static int oldFilesTabId = 0;
    if (ImGui::BeginTabBar("FilesOrDir", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Files"))
        {
            filesTabId = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Folders"))
        {
            filesTabId = 1;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (oldFilesTabId != filesTabId)
    {
        oldFilesTabId = filesTabId;
        setViewDirty();
    }

    if (filesTabId == 0)
    {
        {
            int step = 1;
            const char* fmt[] = { "", "1 KB", "1 MB", "10 MB", "100 MB", "1 GB" };
            minLogSize = ImClamp(minLogSize, 0, 5);
            ImGui::SetNextItemWidth(150);
            if (ImGui::InputScalar("minimum File Size  ", ImGuiDataType_S32, &minLogSize, &step, &step, fmt[ImClamp(minLogSize, 0, 5)]))
                setViewDirty();
        }
        ImGui::SameLine();
        {
            const char* fmt = " \000" "<2 no\000" "<3 minor\000" "=3 enough\000" ">3 too much\000" ">4 way too much\000" "\000";
            ImGui::SetNextItemWidth(150);
            if (ImGui::Combo("Redundancy  ", &redundancyFilter, fmt))
                setViewDirty();
        }
    }

    {
        // todo: 0
//                DeviceData& data = everyHere.deviceData[0];

//                const int lineHeight = (int)ImGui::GetFontSize();
//                int height = (int)data.files.size() * lineHeight;

        ImGuiTableFlags flags = ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable |
            ImGuiTableFlags_Sortable |
            ImGuiTableFlags_SortMulti;

        // safe space for info line
        ImVec2 outerSize = ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight() - ImGui::GetStyle().ItemSpacing.y);
        uint32 numberOfColumns = 3;
        if (filesTabId == 0)
            numberOfColumns += 2;

        if (ImGui::BeginTable("table_scrolly", numberOfColumns, flags, outerSize))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Name);
            if (filesTabId == 0)
            {
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Size);
                ImGui::TableSetupColumn("Redundancy", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Redundancy);
            }
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Path);
            ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_DeviceId);
            //                    ImGui::TableSetupColumn("Date Modified", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Date Accessed", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Date Created", ImGuiTableColumnFlags_None);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
                if (sorts_specs->SpecsDirty)
                {
                    whenToRebuildView = 0;
                    sorts_specs->SpecsDirty = false;
                }

            if (whenToRebuildView != -1 && g_Timer.GetAbsoluteTime() > whenToRebuildView)
            {
                fileSortCriteria = ImGui::TableGetSortSpecs();
                // do this before changes to the fileView
                fileSelectionRange.reset();
                int64 minSize[] = { 0, 1024, 1024 * 1024, 10 * 1024 * 1024, 100 * 1024 * 1024, 1024 * 1024 * 1024 };
                everyHere.buildFileView(filter.c_str(), minSize[ImClamp(minLogSize, 0, 5)], redundancyFilter, deviceSelectionRange, fileSortCriteria, filesTabId == 1);
                whenToRebuildView = -1;
                fileSortCriteria = {};
            }

            ImGuiListClipper clipper;
            clipper.Begin((int)everyHere.fileView.size());
            std::string line;
            pushTableStyle3();
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    if (line_no >= everyHere.fileView.size())
                        break;

                    ViewEntry& viewEntry = everyHere.fileView[line_no];
                    const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                    const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];

                    line = entry.key.fileName;

                    ImGui::TableNextRow();

                    ImGui::PushID(line_no);

                    int columnId = 0;

                    ImGui::TableSetColumnIndex(columnId++);
                    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                    bool selected = fileSelectionRange.isSelected(line_no);
                    ImGui::Selectable(line.c_str(), &selected, selectable_flags);
                    if (ImGui::IsItemClicked(0))
                    {
                        fileSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                    }
                    if (BeginTooltip())
                    {
                        // todo: path concat needs to be improved
                        ImGui::Text("FilePath: %s%s/%s", deviceData.drivePath.c_str(), entry.value.path.c_str(), line.c_str());
                        ImGui::Text("Size: %llu Bytes", entry.key.sizeOrFolder);
                        EndTooltip();
                    }
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (!fileSelectionRange.isSelected(line_no))
                        {
                            fileSelectionRange.reset();
                            fileSelectionRange.onClick(line_no, false, false);
                        }

                        if (fileSelectionRange.count() == 1)
                        {
                            if (ImGui::MenuItem("Open (with default program)"))
                            {
                                //                                        const char* path = deviceData.generatePath(entry.value.parent);
                                const char* path = entry.value.path.c_str();
                                std::string fullPath = deviceData.drivePath + "/" + path + "/" + entry.key.fileName;
                                ShellExecuteA(0, 0, fullPath.c_str(), 0, 0, SW_SHOW);
                            }
                            if (ImGui::MenuItem("Open path (in Explorer)"))
                            {
                                //                                        const char* path = deviceData.generatePath(entry.value.parent);
                                const char* path = entry.value.path.c_str();
                                std::string fullPath = deviceData.drivePath + "/" + path;
                                ShellExecuteA(0, 0, fullPath.c_str(), 0, 0, SW_SHOW);
                            }
                        }
                        if (ImGui::MenuItem("Copy selection as path (to clipboard)"))
                        {
                            ImGui::LogToClipboard();

                            fileSelectionRange.foreach([&](int64 line_no) {
                                ViewEntry& viewEntry = everyHere.fileView[line_no];
                                const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                                const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];
                                if (entry.key.sizeOrFolder >= 0)
                                {
                                    //                                            const char* path = deviceData.generatePath(entry.value.parent);
                                    const char* path = entry.value.path.c_str();
                                    if (*path)
                                        ImGui::LogText("%s/%s/%s\n", deviceData.drivePath.c_str(), path, entry.key.fileName.c_str());
                                    else
                                        ImGui::LogText("%s/%s\n", deviceData.drivePath.c_str(), entry.key.fileName.c_str());
                                }
                                });

                            ImGui::LogFinish();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();

                    if (filesTabId == 0)
                    {
                        ImGui::TableSetColumnIndex(columnId++);
                        double printSize = 0;
                        const char* printUnit = computeReadableSize(entry.key.sizeOrFolder, printSize);
                        ImGui::Text(printUnit, printSize);

                        ImGui::TableSetColumnIndex(columnId++);
                        ImGui::Text("%d", everyHere.findRedundancy(entry.key));
                    }

                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::TextUnformatted(entry.value.path.c_str());

                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::Text("%d", entry.value.deviceId);
                }
            }
            clipper.End();
            ImGui::PopStyleColor(3);

            ImGui::EndTable();
        }

        if (fileSelectionRange.empty())
        {
            ImGui::Text("Files: %lld", (int64)everyHere.fileView.size());
            ImGui::SameLine();
            ImGui::Text("Size: ");
            ImGui::SameLine();

            double printSize = 0;
            const char* printUnit = computeReadableSize(everyHere.viewSumSize, printSize);
            ImGui::Text(printUnit, printSize);
        }
        else
        {
            ImGui::Text("Selected: %lld", (int64)fileSelectionRange.count());
            ImGui::SameLine();

            // Can be optimized when using drives instead but then we need selectedFileSize there as well
            uint64 selectedSize = 0;
            fileSelectionRange.foreach([&](int64 line_no) {
                ViewEntry& viewEntry = everyHere.fileView[line_no];
                const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];
                if (entry.key.sizeOrFolder >= 0)
                    selectedSize += entry.key.sizeOrFolder;
                });

            double printSize = 0;
            const char* printUnit = computeReadableSize(selectedSize, printSize);
            ImGui::Text("Size: %llu %s", printSize, printUnit);
        }
    }

    ImGui::End();
}

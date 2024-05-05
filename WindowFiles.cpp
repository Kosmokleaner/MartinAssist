#include <windows.h> // HICON
#undef max
#undef min

#include "ImGui/imgui.h"
#include "Gui.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/imgui_internal.h"
#include "FileSystem.h"
#include "Timer.h"
#include "WindowFiles.h"

#pragma warning( disable : 4100 ) // unreferenced formal parameter

void WindowFiles::fileLineUI(int32 line_no, const FileEntry& entry, std::string& line)
{


    if (ImGui::IsItemClicked(0))
    {
        fileSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
    }
//    if (BeginTooltipPaused())
//    {
//        // todo: path concat needs to be improved
//        ImGui::Text("FilePath: %s%s/%s", deviceData.drivePath.c_str(), entry.value.path.c_str(), line.c_str());
//        ImGui::Text("Size: %llu Bytes", entry.key.sizeOrFolder);
//        EndTooltip();
//    }
    if (ImGui::BeginPopupContextItem())
    {
        if (!fileSelectionRange.isSelected(line_no))
        {
            fileSelectionRange.reset();
            fileSelectionRange.onClick(line_no, false, false);
        }

/*        if (fileSelectionRange.count() == 1)
        {
            if (ImGui::MenuItem("Open (with default program)"))
            {
                //                                        const char* path = deviceData.generatePath(entry.value.parent);
                const char* path = entry.value.path.c_str();
                std::string fullPath = deviceData.drivePath + "/" + path + "/" + entry.key.fileName.c_str();
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
                ViewEntry& viewEntry = fileView[line_no];
//                const DriveData& deviceData = *everyHere.driveData[viewEntry.driveId];
                const FileEntry& entry = l.entries[viewEntry.fileEntryId];
                if (entry.key.size >= 0)
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
 */
       ImGui::EndPopup();
    }
}



void WindowFiles::buildFileView(const char* filter, int64 minSize, int redundancyFilter, SelectionRange& driveSelectionRange, bool folder)
{
    assert(minSize >= 0);
    assert(filter);

    fileView.clear();

    if(!fileList)
        return;

    EFileList& l = *fileList;

 //   viewSumSize = 0;
    int filterLen = (int)strlen(filter);

//    for (uint32 lineNoDrive = 0; lineNoDrive < driveView.size(); ++lineNoDrive)
//    {
//        if (!driveSelectionRange.isSelected(lineNoDrive))
//            continue;

//        DriveData& itD = *driveData[driveView[lineNoDrive]];
    {


        fileView.reserve(l.entries.size());

        uint64 id = 0;
        for (auto& fileEntry : l.entries)
        {
            ViewEntry entry;
//            entry.driveId = itD.driveId;
            entry.fileEntryId = id++;

            //            FileEntry& fileEntry = itD.entries[entry.fileEntryId];

                        // should happen only in the beginnng
//            if (fileEntry.value.parent >= 0 && fileEntry.value.path.empty())
//                fileEntry.value.path = itD.generatePath(fileEntry.value.parent);
/*
            if (folder)
            {
                if (fileEntry.key.isFile())
                    continue;   // hide files
            }
            else
            {
                if (fileEntry.key.size < minSize)
                    continue;   // hide folders and files that are filtered out
            }
*/
/*
            if (redundancyFilter)
            {
                int redundancy = (int)findRedundancy(fileEntry.key);
                switch (redundancyFilter)
                {
                case 1: if (redundancy >= 2) continue;
                    break;
                case 2: if (redundancy >= 3) continue;
                    break;
                case 3: if (redundancy != 3) continue;
                    break;
                case 4: if (redundancy <= 3) continue;
                    break;
                case 5: if (redundancy <= 4) continue;
                    break;
                default:
                    assert(0);
                }
            }
*/
            // todo: optimize
            if (stristrOptimized(fileEntry.key.fileName.c_str(), filter, (int)fileEntry.key.fileName.size(), filterLen) == 0)
                continue;

            fileView.push_back(entry);
//            viewSumSize += fileEntry.key.sizeOrFolder;
        }
    }

/*    struct CustomLessFile
    {
//        const EveryHere& everyHere;
        ImGuiTableSortSpecs* sorts_specs = {};

        bool operator()(const ViewEntry& a, const ViewEntry& b) const
        {
            // todo: optimize vector [] in debug
            const FileEntry& A = everyHere.driveData[a.driveId]->entries[a.fileEntryId];
            const FileEntry& B = everyHere.driveData[b.driveId]->entries[b.fileEntryId];

            int count = sorts_specs ? sorts_specs->SpecsCount : 0;

            for (int n = 0; n < count; n++)
            {
                // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
                // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
                const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
                int64 delta = 0;
                switch (sort_spec->ColumnUserID)
                {
                case FCID_Name:        delta = strcmp(A.key.fileName.c_str(), B.key.fileName.c_str()); break;
                case FCID_Size:        delta = A.key.sizeOrFolder - B.key.sizeOrFolder; break;
                case FCID_Redundancy:  delta = everyHere.findRedundancy(A.key) - everyHere.findRedundancy(B.key); break;
                case FCID_DriveId:    delta = a.driveId - b.driveId; break;
                case FCID_Path:        delta = strcmp(A.value.path.c_str(), B.value.path.c_str()); break;
                default: IM_ASSERT(0); break;
                }
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
            }

            // qsort() is instable so always return a way to differenciate items.
            // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
            return &A < &B;
        }
    };

    CustomLessFile customLess = { *this, sorts_specs };

    std::sort(fileView.begin(), fileView.end(), customLess);
*/

    // todo: only needed for TreeView
 //   buildFileViewChildLists();
}

void WindowFiles::set(DriveInfo2& driveInfo)
{
    // call load before
    assert(driveInfo.fileList);

    fileList = driveInfo.fileList;

    SelectionRange driveSelectionRange;
    buildFileView("", 0, 0, driveSelectionRange, false);
}

void WindowFiles::gui()
{
    if(!showWindow)
        return;

    // mayb be 0
    EFileList* l = fileList.get();

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 200, main_viewport->WorkPos.y + 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Files", &showWindow, ImGuiWindowFlags_NoCollapse);

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

    std::string line;

    if(l)
    if (ImGui::BeginTable("table_scrolly", numberOfColumns, flags, outerSize))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Name);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Size);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Path);

        ImGui::TableHeadersRow();

        pushTableStyle3();

        // list view
        ImGuiListClipper clipper;
        clipper.Begin((int)fileView.size());
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                if (line_no >= fileView.size())
                    break;

                ViewEntry& viewEntry = fileView[line_no];
//                const DriveData& deviceData = *everyHere.driveData[viewEntry.driveId];
                const FileEntry& entry = l->entries[viewEntry.fileEntryId];

                // todo: optimize
                line = entry.key.fileName.c_str();

                ImGui::TableNextRow();

                ImGui::PushID(line_no);

                int columnId = 0;

                ImGui::TableSetColumnIndex(columnId++);
                ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
//                bool selected = fileSelectionRange.isSelected(line_no);
                bool selected = false; // todo

                ImGuiSelectable(line.c_str(), &selected, selectable_flags);

                fileLineUI(line_no, entry, line);

                ImGui::PopID();

                // Size
                ImGui::TableSetColumnIndex(columnId++);
                if (entry.key.size >= 0)
                {
                    double printSize = 0;
                    const char* printUnit = computeReadableSize(entry.key.size, printSize);
                    ImGui::Text(printUnit, printSize);
                }

                ImGui::TableSetColumnIndex(columnId++);
                ImGui::TextUnformatted(entry.value.path.c_str());

//                ImGui::TableSetColumnIndex(columnId++);
//                ImGui::Text("%d", entry.value.driveId);
            }
        }
        clipper.End();

        ImGui::PopStyleColor(3);

        ImGui::EndTable();
    }

    // todo: filter button
//    if (ImGui::InputText("filter", &filter))
//    {
//        setViewDirty();
//    }

/*    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

//    EFilesMode filesTabId = eFM_Files;
    static int oldFilesTabId = eFM_Files;
    if (ImGui::BeginTabBar("FilesOrDir", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Files"))
        {
//            filesTabId = eFM_Files;
            ImGui::EndTabItem();
        }
//        if (ImGui::BeginTabItem("Folders"))
//        {
//            filesTabId = eFM_Folders;
//            ImGui::EndTabItem();
//        }
//        if (ImGui::BeginTabItem("TreeView"))
//        {
//            filesTabId = eFM_TreeView;
//            ImGui::EndTabItem();
//        }
        ImGui::EndTabBar();
    }
    if (oldFilesTabId != filesTabId)
    {
        oldFilesTabId = filesTabId;
        setViewDirty();
    }
*/
//    if (filesTabId == eFM_Files)
/*
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

     
            if (filesTabId == eFM_Files)
            {
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Size);
                ImGui::TableSetupColumn("Redundancy", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Redundancy);
            }
            if (filesTabId != eFM_TreeView)
            {
                ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Path);
            }
            ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_DriveId);
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
                everyHere.buildFileView(filter.c_str(), minSize[ImClamp(minLogSize, 0, 5)], redundancyFilter, driveSelectionRange, fileSortCriteria, filesTabId);
                whenToRebuildView = -1;
                fileSortCriteria = {};
            }

            pushTableStyle3();
            std::string line;

            if(filesTabId == eFM_TreeView)
            {
                for (uint32 lineNoDrive = 0; lineNoDrive < everyHere.driveView.size(); ++lineNoDrive)
                {
                    if (!driveSelectionRange.isSelected(lineNoDrive))
                        continue;

                    if(everyHere.rootFileId.isValid())
                        treeNodeUI(everyHere.rootFileId, line);
                }
            }
            else
            {
                // list view
                ImGuiListClipper clipper;
                clipper.Begin((int)everyHere.fileView.size());
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        if (line_no >= everyHere.fileView.size())
                            break;

                        ViewEntry& viewEntry = everyHere.fileView[line_no];
                        const DriveData& deviceData = *everyHere.driveData[viewEntry.driveId];
                        const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];

                        // todo: optimize
                        line = entry.key.fileName.c_str();

                        ImGui::TableNextRow();

                        ImGui::PushID(line_no);

                        int columnId = 0;

                        ImGui::TableSetColumnIndex(columnId++);
                        ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                        bool selected = fileSelectionRange.isSelected(line_no);

                        ImGui::Selectable(line.c_str(), &selected, selectable_flags);

                        fileLineUI(line_no, deviceData, entry, line);

                        ImGui::PopID();

                        if (filesTabId == eFM_Files)
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
                        ImGui::Text("%d", entry.value.driveId);
                    }
                }
                clipper.End();
            }
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
                const DriveData& deviceData = *everyHere.driveData[viewEntry.driveId];
                const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];
                if (entry.key.sizeOrFolder >= 0)
                    selectedSize += entry.key.sizeOrFolder;
                });

            double printSize = 0;
            const char* printUnit = computeReadableSize(selectedSize, printSize);
            ImGui::Text("Size: %llu %s", printSize, printUnit);
        }
    }
*/
    ImGui::End();
}

//void WindowFiles::setViewDirty()
//{
//    // constant is in seconds
//    whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.25f;
//}

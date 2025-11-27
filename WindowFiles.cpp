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

void WindowFiles::fileLineUI(const std::vector<std::shared_ptr<DriveInfo2> >& drives, int32 line_no, const FileEntry& entry, const std::string& line)
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

        if (fileSelectionRange.count() == 1)
        {
            if (ImGui::MenuItem("Open (with default program)"))
            {
                std::string fullPath = entry.value.path.c_str();

                fullPath += "/";
                fullPath += entry.key.fileName.c_str();
                ShellExecuteA(0, 0, fullPath.c_str(), 0, 0, SW_SHOW);
            }
            if (ImGui::MenuItem("Open path (in Explorer)"))
            {
                ShellExecuteA(0, 0, entry.value.path.c_str(), 0, 0, SW_SHOW);
            }
        }

        if (ImGui::MenuItem("Copy selection as path (to clipboard)"))
        {
            ImGui::LogToClipboard();

            fileSelectionRange.foreach([&](int64 line_no) {
                ViewEntry& viewEntry = fileViewEx[line_no];

                DriveInfo2& drive = *drives[viewEntry.driveId];

                std::unique_lock<std::mutex> lock(drive.mutex);
                std::shared_ptr<FileList>& fileList = drive.fileListEx;

                const FileEntry& entry = fileList->entries[viewEntry.fileEntryId];
                if (entry.key.size >= 0)
                {
                    ImGui::LogText("%s/%s\n", entry.value.path.c_str(), entry.key.fileName.c_str());
                }
                });

            ImGui::LogFinish();
        }
       ImGui::EndPopup();
    }
}



void WindowFiles::buildFileView(const char* inFilter, int64 minSize, int inRedundancyFilter, const SelectionRange& driveSelectionRange, ImGuiTableSortSpecs* sorts_specs)
{
    std::unique_lock<std::mutex> viewLock(mutex);

    assert(minSize >= 0);
    assert(inFilter);

 //   viewSumSize = 0;
    int filterLen = (int)strlen(inFilter);

    for (uint32 lineNoDrive = 0; lineNoDrive < g_gui.windowDrives.drives.size(); ++lineNoDrive)
    {
        if (!driveSelectionRange.isSelected(lineNoDrive))
            continue;

        DriveInfo2& itD = *g_gui.windowDrives.drives[lineNoDrive];

        std::unique_lock<std::mutex> driveLock(itD.mutex);

        // convervative but simple
        fileViewEx.reserve(fileViewEx.size() + itD.fileListEx->entries.size());

        uint64 id = 0;
        for (auto& fileEntry : itD.fileListEx->entries)
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
            }
*/
            assert(fileEntry.key.size >= 0);
            if (fileEntry.key.size < minSize)
                continue;   // hide folders and files that are filtered out

            if (redundancyFilter)
            {
                uint32 redundancy = fileEntry.value.redundancy;

#if TRACK_REDUNDANCY == 1
                if(!redundancy)
                    redundancy = fileEntry.value.redundancy = g_gui.redundancy.computeRedundancy(fileEntry.key);
#endif

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

            // todo: optimize
            if (stristrOptimized(fileEntry.key.fileName.c_str(), inFilter, (int)fileEntry.key.fileName.size(), filterLen) == 0)
                continue;

            fileViewEx.push_back(entry);
//            viewSumSize += fileEntry.key.sizeOrFolder;
        }
    }

/*
    struct CustomLessFile
    {
        const FileList& ref;
        ImGuiTableSortSpecs* sorts_specs = {};

        bool operator()(const ViewEntry& a, const ViewEntry& b) const
        {

            // todo: optimize vector [] in debug
//            const FileEntry& A = everyHere.driveData[a.driveId]->entries[a.fileEntryId];
//            const FileEntry& B = everyHere.driveData[b.driveId]->entries[b.fileEntryId];

            const FileEntry& A = ref.entries[a.fileEntryId];
            const FileEntry& B = ref.entries[b.fileEntryId];
   

            int count = sorts_specs ? sorts_specs->SpecsCount : 0;

            for (int n = 0; n < count; n++)
            {
                // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
                // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
                const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
                int64 delta = 0;

                assert(A.value.redundancy);
                assert(B.value.redundancy);

                switch (sort_spec->ColumnUserID)
                {
                case FCID_Name:        delta = strcmp(A.key.fileName.c_str(), B.key.fileName.c_str()); break;
                case FCID_Size:        delta = (int64)A.key.size - (int64)B.key.size; break;
//                case FCID_Redundancy:  delta = (int64)g_gui.redundancy.computeRedundancy(A.key) - (int64)g_gui.redundancy.computeRedundancy(B.key); break;
                case FCID_Redundancy:  delta = (int64)A.value.redundancy - (int64)B.value.redundancy; break;
                case FCID_DriveId:    delta = (int64)a.driveId - (int64)b.driveId; break;
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

    CustomLessFile customLessFile = { fileListEx, sorts_specs };

    std::sort(fileViewEx.begin(), fileViewEx.end(), customLessFile);
*/

    // todo: only needed for TreeView
 //   buildFileViewChildLists();
}

void WindowFiles::clear()
{
    std::unique_lock<std::mutex> lock(mutex);

    fileViewEx.clear();
}

void WindowFiles::rebuild(const SelectionRange& driveSelectionRange)
{
    CScopedCPUTimerLog log("WindowFiles::set buildFileView");

    buildFileView("", 0, 0, driveSelectionRange, 0);
}

void WindowFiles::gui(const std::vector<std::shared_ptr<DriveInfo2> >& drives)
{
    if(!showWindow)
        return


    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 200, main_viewport->WorkPos.y + 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Files", &showWindow, ImGuiWindowFlags_NoCollapse);

    { 
        if (ImGuiSearch("filter", &filter))
        {
            setViewDirty();
        }
    }

    {
//        ImGui::Text("min Size:");
//        ImGui::SameLine();
//        int step = 1;
        const char* fmt[] = { "any size", "> 1KB", "> 1MB", "> 10MB", "> 100MB", "> 1GB" };
        minLogSize = ImClamp(minLogSize, 0, 5);
        ImGui::SetNextItemWidth(150);
//        if (ImGui::InputScalar("minimum File Size  ", ImGuiDataType_S32, &minLogSize, &step, &step, fmt[ImClamp(minLogSize, 0, 5)]))
        if (ImGui::SliderInt("##min Size", &minLogSize, 0, 5, fmt[ImClamp(minLogSize, 0, 5)]))
            setViewDirty();
        if (BeginTooltipPaused())
        {
            ImGui::Text("Minimum file size. Sorting many backups of small files doesn't matter so best to focus on large files");
            EndTooltip();
        }
    }
    ImGui::SameLine();
    {
//        ImGui::Text("Redundancy:");
//        ImGui::SameLine();
        const char* fmt[] = { "any dup", "<2 dup", "<3 dup", "=3 dup", ">3 dup", ">4 dup" };
//        const char* fmt = " \000" "<2 no\000" "<3 minor\000" "=3 enough\000" ">3 too much\000" ">4 way too much\000" "\000";
        redundancyFilter = ImClamp(redundancyFilter, 0, 5);
        ImGui::SetNextItemWidth(150);
//        if (ImGui::Combo("Redundancy  ", &redundancyFilter, fmt))
        if (ImGui::SliderInt("##Redundancy", &redundancyFilter, 0, 5, fmt[ImClamp(redundancyFilter, 0, 5)]))
            setViewDirty();
        if (BeginTooltipPaused())
        {
            ImGui::Text("Minimum number of duplicates (redundancy). It'a a tradeoff between safety and storage cost.");
            ImGui::Text("Suggested: <2:no, <3:minor, =3:enough, >3 too much, >4 way too much");
            EndTooltip();
        }
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollX |
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

    std::string line;

    uint32 numberOfColumns = 6;

    if (ImGui::BeginTable("table_scrolly", numberOfColumns, flags, outerSize))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Name);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Size);
        ImGui::TableSetupColumn("dup", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Redundancy);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed, 0.0f, FCID_Path);
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
            SelectionRange driveSelectionRange; // todo
            buildFileView(filter.c_str(), minSize[ImClamp(minLogSize, 0, 5)], redundancyFilter, driveSelectionRange, fileSortCriteria);
            whenToRebuildView = -1;
            fileSortCriteria = {};
        }

        pushTableStyle3();

        {
            std::unique_lock<std::mutex> lock(mutex);

            // list view
            ImGuiListClipper clipper;
            clipper.Begin((int)fileViewEx.size());
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    if (line_no >= (int)fileViewEx.size())
                        break;

// todo
                    std::shared_ptr<FileList> l;

                    ViewEntry& viewEntry = fileViewEx[line_no];
//                    const DriveData& deviceData = *everyHere.driveData[viewEntry.driveId];
                    const FileEntry& entry = l->entries[viewEntry.fileEntryId];

                    // todo: optimize
                    line = entry.key.fileName.c_str();

                    ImGui::TableNextRow();

                    ImGui::PushID(line_no);

                    int columnId = 0;

                    ImGui::TableSetColumnIndex(columnId++);
                    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                    bool selected = fileSelectionRange.isSelected(line_no);

                    ImGui::Selectable(line.c_str(), &selected, selectable_flags);

                    fileLineUI(drives, line_no, entry, line);

                    ImGui::PopID();

                    {
                        ImGui::TableSetColumnIndex(columnId++);
                        double printSize = 0;
                        const char* printUnit = computeReadableSize(entry.key.size, printSize);
                        ImGui::Text(printUnit, printSize);
                    }

                    ImGui::TableSetColumnIndex(columnId++);
                    ImGui::Text("%d", g_gui.redundancy.computeRedundancy(entry.key));

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
        ImGui::Text("Files: %lld", (int64)fileViewEx.size());
        ImGui::SameLine();
//todo        ImGui::Text("Size: ");
//        ImGui::SameLine();

//        double printSize = 0;
//        const char* printUnit = computeReadableSize(everyHere.viewSumSize, printSize);
//        ImGui::Text(printUnit, printSize);
    }
    else
    {
        ImGui::Text("Selected: %lld", (int64)fileSelectionRange.count());
  /*      ImGui::SameLine();

        // Can be optimized when using drives instead but then we need selectedFileSize there as well
        uint64 selectedSize = 0;
        fileSelectionRange.foreach([&](int64 line_no) {
            ViewEntry& viewEntry = fileView[line_no];
//            const DriveData& deviceData = *everyHere.driveData[viewEntry.driveId];
            const FileEntry& entry = l->entries[viewEntry.fileEntryId];
            if (entry.key.size >= 0)
                selectedSize += entry.key.size;
            });

        double printSize = 0;
        const char* printUnit = computeReadableSize(selectedSize, printSize);
        ImGui::Text("Size: %llu %s", printSize, printUnit);
*/
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

void WindowFiles::setViewDirty()
{
    // constant is in seconds
    whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.25f;
}

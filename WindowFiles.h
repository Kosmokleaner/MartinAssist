#pragma once
#include <memory> // std::shared_ptr<>
#include "EFileList.h"
#include "SelectionRange.h"
#include "WindowDrives.h"
#include "ImGui/imgui.h"
#include <mutex>

struct DriveInfo2;

class WindowFiles
{
public:

    // protecting fileViewEx
    std::mutex mutex;

    // [line_no] = ViewEntry, built by buildView()
    std::vector<ViewEntry> fileViewEx;

    void clear();

    void rebuild(const SelectionRange& driveSelectionRange);

    void gui(const std::vector<std::shared_ptr<DriveInfo2> >& drives);

    bool showWindow = true;

private:
    //
    std::string filter;
    // 2: 1MB
    int32 minLogSize = 2;
    //
    int32 redundancyFilter = 0;
    //
    double whenToRebuildView = -1;
//
    ImGuiTableSortSpecs* fileSortCriteria = {};

    // into everyHere.fileView[]
//    SelectionRangeWithDriveUpdate fileSelectionRange;
    SelectionRange fileSelectionRange;

    void setViewDirty();

//    void treeNodeUI(FileViewId id, std::string& line);

    //
    void fileLineUI(const std::vector<std::shared_ptr<DriveInfo2> >& drives, int32 line_no, const FileEntry& entry, const std::string& line);
    //
    void buildFileView(const char* filter, int64 minSize, int redundancyFilter, const SelectionRange& driveSelectionRange, ImGuiTableSortSpecs* sorts_specs);
};

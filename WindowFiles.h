#pragma once
#include <memory> // std::shared_ptr<>
#include "EFileList.h"
#include "SelectionRange.h"

struct DriveInfo2;

class WindowFiles
{
    // todo: refactor
//    EFileList l;
    std::shared_ptr<EFileList> fileList;
public:

    // [line_no] = ViewEntry, built by buildView()
    std::vector<ViewEntry> fileView;

    void set(DriveInfo2& driveInfo);

    void gui(bool& show);

private:
    //
//    double whenToRebuildView = -1;

    // into everyHere.fileView[]
//    SelectionRangeWithDriveUpdate fileSelectionRange;
    SelectionRange fileSelectionRange;

//    void setViewDirty();

//    void treeNodeUI(FileViewId id, std::string& line);

    void fileLineUI(int32 line_no, const FileEntry& entry, std::string& line);
    void buildFileView(const char* filter, int64 minSize, int redundancyFilter, SelectionRange& driveSelectionRange, bool folder);
};

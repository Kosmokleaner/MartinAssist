#pragma once
#include "EFileList.h"
#include "SelectionRange.h"

class WindowFiles
{
public:

    // todo: refactor
    EFileList l;

    // [line_no] = ViewEntry, built by buildView()
    std::vector<ViewEntry> fileView;


    void guiFiles(bool& show);

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

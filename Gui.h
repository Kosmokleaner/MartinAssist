#pragma once
#include "EveryHere.h"
#include "SelectionRange.h"
#include <mutex>

class Gui
{
public:
    Gui();

    int test();
    
    void guiDrives(bool &show);
    void guiFiles(bool &show);

    EveryHere everyHere;
    std::mutex everyHere_mutex;

    void setViewDirty();

private:
    //
    std::string filter;
    //
    double whenToRebuildView = -1;
    // 2: 1MB
    int32 minLogSize = 2;
    //
    int32 redundancyFilter = 0;
    // into everyHere.driveView[]
    SelectionRange driveSelectionRange;
    // into everyHere.fileView[]
    SelectionRangeWithDriveUpdate fileSelectionRange;

    ImGuiTableSortSpecs* fileSortCriteria = {};
};


void pushTableStyle3();

// @param must not be 0 
void TooltipPaused(const char* text);

// e.g.
// if(BeginTooltipPaused())
// {
//   ..
//   EndTooltip();
// }
bool BeginTooltipPaused();
// yellow tooltip background
void BeginTooltip();
// to end BeginTooltip()
void EndTooltip();


// @return printUnit e.g. "%.3f GB" or "%.3f MB" 
const char* computeReadableSize(uint64 inputSize, double& outPrintSize);



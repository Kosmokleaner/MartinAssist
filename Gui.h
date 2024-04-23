#pragma once

#include "WindowFiles.h"
#include "WindowDrives.h"

class Gui
{
public:

    int test();
    
private:

    WindowFiles files;
    WindowDrives drives;
};


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

void pushTableStyle3();
#pragma once

class Gui
{
public:
    Gui();

    int test();
    
private:
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



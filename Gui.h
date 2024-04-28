#pragma once

#include "WindowFiles.h"
#include "WindowDrives.h"
#include "imgui.h"

#define EFU_FOLDER "EFUs"

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

// same as ImGui::Selectable()
bool ImGuiSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = ImVec2(0, 0));


// @return printUnit e.g. "%.3f GB" or "%.3f MB" 
const char* computeReadableSize(uint64 inputSize, double& outPrintSize);

void pushTableStyle3();

// RAII: https://en.cppreference.com/w/cpp/language/raii
// use like this:
// {
//   ImStyleColor_RAII col;
//   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
//   ImGui::Text("Test");
// }
// or
// {
//   ImStyleColor_RAII col(2);
//   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
//   ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(1,0,0,1));
//   ImGui::Text("Test");
// }
struct ImStyleColor_RAII
{
    uint32 count = 0;

    ImStyleColor_RAII(const uint32 inCount = 1)
        : count(inCount)
    {
    }

    ~ImStyleColor_RAII()
    {
        ImGui::PopStyleColor(count);
    }
};
// similar to ImStyleColor_RAII
struct ImStyleVar_RAII
{
    uint32 count = 0;

    ImStyleVar_RAII(const uint32 inCount = 1)
        : count(inCount)
    {
    }

    ~ImStyleVar_RAII()
    {
        ImGui::PopStyleVar(count);
    }
};

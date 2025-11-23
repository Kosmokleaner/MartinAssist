#include "GranularSynth.h"
#include "ImGui/imgui.h"


void GranularSynth::gui()
{
    if (!showWindow)
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 300, main_viewport->WorkPos.y + 220), ImGuiCond_FirstUseEver);
    ImGui::Begin("GranularSynth", &showWindow, ImGuiWindowFlags_NoCollapse);
    ImGui::End();
}

#include "WindowDrives.h"
#include "ImGui/imgui.h"

void WindowDrives::gui(bool& show)
{
    static bool first = true;
    if (first)
    {
        first = false;
    }


    if (!show)
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Drives", &show, ImGuiWindowFlags_NoCollapse);


    ImGui::End();
}

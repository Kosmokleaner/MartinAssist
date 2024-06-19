#include <windows.h> // HICON
#undef max
#undef min

#include "Gui.h"


// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "FileSystem.h"
#include "Timer.h"
#include "resource.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <ImGui/GLFW/glfw3.h> // Will drag system OpenGL headers

#include <vector>
#include <algorithm> // std::max()


#include <shlobj.h> // _wfinddata_t

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#pragma comment(lib, "ImGui/GLFW/glfw3.lib")        // 64bit
#pragma comment(lib, "OpenGL32.lib")                // 64bit

Gui g_gui;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
unsigned int RGBSwizzle(unsigned int c) {
    return (c >> 16) | (c & 0xff00) | ((c & 0xff) << 16);
}

void openEFUsFolder()
{
    ShellExecuteA(0, 0, EFU_FOLDER, 0, 0, SW_SHOW);
}

// copied from ImGui, the optional endMarker adds a rectangle to the triangle arrow indicating a stop
// useful to scroll to beginning or end
bool ArrowButton2(const char* str_id, ImGuiDir dir, bool smallButton, bool endMarker)
{
    if (smallButton)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    ImGuiButtonFlags flags = ImGuiButtonFlags_None;
    float sz = ImGui::GetFrameHeight();
    ImVec2 size(sz, sz);

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
    const float default_size = ImGui::GetFrameHeight();
    ImGui::ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);

    if (window->SkipItems || !ImGui::ItemAdd(bb, id))
    {
        if (smallButton)
            ImGui::PopStyleVar();

        return false;
    }

    if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
        flags |= ImGuiButtonFlags_Repeat;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
    ImVec2 pos = bb.Min;
    pos.x += ImMax(0.0f, (size.x - g.FontSize) * 0.5f);
    pos.y += ImMax(0.0f, (size.y - g.FontSize) * 0.5f);
    pos.x = roundf(pos.x);
    pos.y = roundf(pos.y);
    ImGui::RenderArrow(window->DrawList, pos, text_col, dir);
    if (endMarker)
    {
        const float h = roundf(g.FontSize / 8);
        const float w = roundf(g.FontSize - 2 * h);
        if(dir == ImGuiDir_Up || dir == ImGuiDir_Down)
        {
            pos.x += h;
            pos.y += h;
            if (dir == ImGuiDir_Down)
                pos.y += g.FontSize - 3 * h;
            window->DrawList->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), text_col, 0.0f);
        }
        else
        {
            pos.x += h;
            pos.y += h;
            if (dir == ImGuiDir_Right)
                pos.x += g.FontSize - 3 * h;
            window->DrawList->AddRectFilled(pos, ImVec2(pos.x + h, pos.y + w), text_col, 0.0f);
        }
    }

    if(smallButton)
        ImGui::PopStyleVar();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
    return pressed;
}

void showTestWindow(bool& show)
{
    if(!show)
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 300), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(500, 680), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Test", &show))
        return;

    // test ArrowButton2(), todo: move
    for (int i = 0; i < 2; ++i)
    {
        bool smallButton = i;
        ArrowButton2("1", ImGuiDir_Left, smallButton, false);
        TooltipPaused("Left");
        ImGui::SameLine();
        ArrowButton2("2", ImGuiDir_Right, smallButton, false);
        TooltipPaused("Right");
        ImGui::SameLine();
        ArrowButton2("3", ImGuiDir_Up, smallButton, false);
        ImGui::SameLine();
        ArrowButton2("4", ImGuiDir_Down, smallButton, false);
        ImGui::SameLine();
        ArrowButton2("1b", ImGuiDir_Left, smallButton, true);
        ImGui::SameLine();
        ArrowButton2("2b", ImGuiDir_Right, smallButton, true);
        ImGui::SameLine();
        ArrowButton2("3b", ImGuiDir_Up, smallButton, true);
        ImGui::SameLine();
        ArrowButton2("4b", ImGuiDir_Down, smallButton, true);
    }

    ImGui::End();
}

void showIconsWindow(ImFont *font, bool &show)
{
    if(!show)
        return;

    int32 IconsPerLine = 8;

    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 300), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(500, 680), ImGuiCond_FirstUseEver);

    if(!ImGui::Begin("Icons", &show))
        return;

    static std::string characterToShow;
    static std::string literalToShow;

    float fontWidth = 0;
    {
        for (unsigned int base = 0; base <= IM_UNICODE_CODEPOINT_MAX; base += 256)
        {
            // optimization
            if (!(base & 4095) && font->IsGlyphRangeUnused(base, base + 4095))
            {
                base += 4096 - 256;
                continue;
            }
            for (unsigned int n = 0; n < 256; n++)
            {
                const ImFontGlyph* glyph = font->FindGlyphNoFallback((ImWchar)(base + n));
                if (glyph)
                {
                    fontWidth = std::max(fontWidth, glyph->X1 - glyph->X0);
                }
            }
        }
    }

    float iconHeight;
    {
        ImGui::PushFont(font);
        ImGui::Text("%s", characterToShow.c_str());
        iconHeight = ImGui::GetTextLineHeight();
        ImGui::PopFont();
        ImGui::Text("%s", literalToShow.c_str());
        ImGui::Separator();
    }

    const float padding = 2;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImVec2 itemSpacing = ImGui::GetStyle().ItemSpacing;

    const ImU32 glyph_col = ImGui::GetColorU32(ImGuiCol_Text);
    const float cell_size = fontWidth; //font->FontSize * 1;

    if (ImGui::BeginChild("ScrollReg", ImVec2((cell_size + itemSpacing.x) * IconsPerLine + ImGui::GetStyle().ScrollbarSize, 0), false))
    {
        int32 printedCharId = 0;
        for (unsigned int base = 0; base <= IM_UNICODE_CODEPOINT_MAX; base += 256)
        {
            // Skip ahead if a large bunch of glyphs are not present in the font (test in chunks of 4k)
            // This is only a small optimization to reduce the number of iterations when IM_UNICODE_MAX_CODEPOINT
            // is large // (if ImWchar==ImWchar32 we will do at least about 272 queries here)
            if (!(base & 4095) && font->IsGlyphRangeUnused(base, base + 4095))
            {
                base += 4096 - 256;
                continue;
            }

            for (unsigned int n = 0; n < 256; n++)
            {
                const ImFontGlyph* glyph = font->FindGlyphNoFallback((ImWchar)(base + n));
                if (glyph && glyph->Visible)
                {
                    ImGui::SetCursorPosX((printedCharId % IconsPerLine) * (cell_size + itemSpacing.x));

                    // static to avoid memory allocations
                    static std::wstring wstr;
                    wstr.clear();
                    wstr.push_back((TCHAR)(base + n));
                    ImGui::PushID(n);
                    // inefficient (memory allocation) but simple
                    ImGui::PushFont(font);
                    ImGui::Button(to_string(wstr).c_str(), ImVec2(cell_size, 0));
                    ImGui::PopFont();
                    ImGui::PopID();

                    printedCharId++;
                    if (printedCharId % IconsPerLine)
                        ImGui::SameLine();

                    if(ImGui::IsItemActive())
                    {
                        literalToShow.clear();
                        // e.g. "\xef\x80\x81" = U+f001
                        //std::wstring wstr;
                        //wstr.push_back((TCHAR)(base + n));
                        characterToShow = to_string(wstr);
                        const char* p = characterToShow.c_str();
                        literalToShow += "\"";
                        while(*p)
                        {
                            char str[8];
                            sprintf_s(str, sizeof(str), "\\x%hhx", *p++);
                            literalToShow += str;
                        }
                        literalToShow += "\"";

                        ImGui::LogToClipboard();
                        ImGui::LogText("%s", literalToShow.c_str());
                        ImGui::LogFinish();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        BeginTooltip();
                        ImGui::Text("Codepoint: U+%04X", base + n);
                        EndTooltip();
                    }

                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::PopStyleVar(2);
    ImGui::SameLine();
    ImGui::TextUnformatted("cursor up/down/space or click on a character to copy it into the clipboard");

    ImGui::End();
}


int Gui::test()
{
    {
        Redundancy r;
        StringPool pool;
        FileKey k0;
        k0.fileName = pool.push("testA");
        k0.size = 10;
        FileKey k1;
        k1.fileName = pool.push("testA");
        k1.size = 10;

        r.addRedundancy(k0, 0, 123);
        r.addRedundancy(k1, 1, 24);
        uint32 cnt = r.computeRedundancy(k0);
        cnt;
        assert(cnt == 2);
    }




    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "MartinAssist UI", NULL, NULL);
    if (window == NULL)
        return 1;

    glfwMaximizeWindow(window);

    // https://stackoverflow.com/questions/7375003/how-to-convert-hicon-to-hbitmap-in-vc
#if _WIN32
    {
//      HICON hIcon = (HICON)LoadImage(GetModuleHandle(0), L"icon.ico", IMAGE_ICON, 0, 0, 0);
        HICON hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, 0, 0, 0);
        //        assert(hIcon);

        if(hIcon)
        {
            HBITMAP hBITMAPcopy;
            ICONINFOEX IconInfo;
            BITMAP BM_32_bit_color;

            memset((void*)&IconInfo, 0, sizeof(ICONINFOEX));
            IconInfo.cbSize = sizeof(ICONINFOEX);
            GetIconInfoEx(hIcon, &IconInfo);

            hBITMAPcopy = (HBITMAP)CopyImage(IconInfo.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            GetObject(hBITMAPcopy, sizeof(BITMAP), &BM_32_bit_color);
            //Now: BM_32_bit_color.bmBits pointing to BGRA data.(.bmWidth * .bmHeight * (.bmBitsPixel/8))

            //    BITMAP BM_1_bit_mask;
            //HBITMAP IconInfo.hbmMask is 1bit per pxl
            // From HBITMAP to BITMAP for mask
        //    hBITMAPcopy = (HBITMAP)CopyImage(IconInfo.hbmMask, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        //    GetObject(hBITMAPcopy, sizeof(BITMAP), &BM_1_bit_mask);
            //Now: BM_1_bit_mask.bmBits pointing to mask data (.bmWidth * .bmHeight Bits!)

            assert(BM_32_bit_color.bmBitsPixel == 32);

            GLFWimage images[1];
            images[0].width = BM_32_bit_color.bmWidth;
            images[0].height = BM_32_bit_color.bmHeight;

            std::vector<int> mem;
            mem.resize(images[0].width * images[0].height);
            int* src = (int*)BM_32_bit_color.bmBits;
            for (int y = images[0].height - 1; y >= 0; --y) {
                for (int x = 0; x < images[0].width; ++x) {
                    // seems glfwSetWindowIcon() doesn't support alpha
                    mem[y * images[0].width + x] = RGBSwizzle(*src++);
                }
            }
            images[0].pixels = (unsigned char*)mem.data();
            glfwSetWindowIcon(window, 1, images);
        }
    }
#endif


    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // theme
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        // no transparent windows
        colors[ImGuiCol_WindowBg].w = 1.0f;
        colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        // not selected tab more distinct
        colors[ImGuiCol_Tab].w = 0.5f;
        // this affects Selectable(), was (0.260f, 0.590f, 0.980f, 0.8f)
        colors[ImGuiCol_Header] = ImVec4(0.260f, 0.590f, 0.980f, 0.8f);
        // this affects Selectable(), make hovered less strong and no color, was (0.260f, 0.590f, 0.980f, 0.800f)
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.4f, 0.4f, 0.400f);

        // height of a vertical scrollbar grab handle
        ImGui::GetStyle().GrabMinSize = 25;
        // width of a vertical scrollbar grab handle
        ImGui::GetStyle().ScrollbarSize = 25;
        ImGui::GetStyle().ScrollbarRounding = 4;
        // docs say >1 is dangerous
        ImGui::GetStyle().WindowBorderSize = 2;
        ImGui::GetStyle().WindowPadding = ImVec2(4, 4);
        // this way the vertical gaps between progress bars on drive window has better tooltip behavior
        ImGui::GetStyle().TouchExtraPadding = ImVec2(2, 2);
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);

    // https://stackoverflow.com/questions/2414828/get-path-to-my-documents
    char fonts[MAX_PATH];
    HRESULT result = SHGetFolderPathA(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, fonts);

    ImFont* fontAwesome = nullptr;
    ImFont* fontAwesomeLarge = nullptr;
    if(result == S_OK)
    {
        io.Fonts->AddFontFromFileTTF((std::string(fonts) +  "\\Arial.ttf").c_str(), 21.0f);

        // https://pixtur.github.io/mkdocs-for-imgui/site/FONTS
        // https://github.com/beakerbrowser/beakerbrowser.com/blob/master/fonts/fontawesome-webfont.ttf
        static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // Will not be copied by AddFont* so keep in scope.
        ImFontConfig config;
        config.MergeMode = true;
        fontAwesome = io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f, &config, icons_ranges);
        config.MergeMode = false;
        fontAwesomeLarge = io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f * 2, &config, icons_ranges);

        io.Fonts->Build();
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool quitApp = false;

    // todo: serialize
    bool showImGuiDemoWindow = false;
    bool showIcons = false;
    bool showTest = false;

    // Main loop
    while (!glfwWindowShouldClose(window) && !quitApp)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (showImGuiDemoWindow)
            ImGui::ShowDemoWindow(&showImGuiDemoWindow);

        showTestWindow(showTest);

        showIconsWindow(fontAwesomeLarge, showIcons);

        windowDrives.gui();
        windowFiles.gui(windowDrives.drives);
        showIconsWindow(fontAwesomeLarge, showIcons);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Drives Window", 0, &windowDrives.showWindow);
                ImGui::MenuItem("Files Window", 0, &windowFiles.showWindow);
                ImGui::Separator();
                if (ImGui::MenuItem("Open EFUs folder"))
                    openEFUsFolder();
                ImGui::Separator();
                ImGui::MenuItem("ImGui Demo", 0, &showImGuiDemoWindow);
                ImGui::MenuItem("Icons", 0, &showIcons);
//                ImGui::MenuItem("Test", 0, &showTest);
                ImGui::Separator();
                if (ImGui::MenuItem("Quit"))
                {
                    quitApp = true;
                }

                ImGui::EndMenu();
            }
            ImGui::TextColored(ImVec4(1, 1, 1, 0.3f), "    %4.1f ms  %.1f FPS", 1000.0f / io.Framerate, io.Framerate);

            ImGui::EndMainMenuBar();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void TooltipPaused(const char* text)
{
    assert(text);
    if(BeginTooltipPaused())
    {
        ImGui::TextUnformatted(text);
        EndTooltip();
    }
}

void pushTableStyle3()
{
    // top to bottom priority, the colors do not blend
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.961f, 0.514f, 0.000f, 0.600f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.961f, 0.514f, 0.000f, 0.600f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.961f, 0.514f, 0.000f, 0.400f));
}

bool BeginTooltipPaused()
{
    bool ret = ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1.0f;

    if (ret)
    {
        BeginTooltip();
    }

    return ret;
}

// looks like Text but can do tooltip and you can test if clicked / activated
bool ColoredTextButton(ImVec4 color, const char* label)
{
//    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    bool ret = ImGui::SmallButton(label);
    ImGui::PopStyleColor(4);

    return ret;
}

void BeginTooltip()
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.9f, 0.9f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::BeginTooltip();
}

void EndTooltip()
{
    ImGui::EndTooltip();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

bool ImGuiSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    ImGuiStyle& style = ImGui::GetStyle();
 
    bool isSelected = p_selected ? *p_selected : false;

    ImStyleColor_RAII active;
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_Header]);

    ImStyleColor_RAII hover(isSelected);
    if(isSelected)
    {
        ImVec4 sel = style.Colors[ImGuiCol_Header];

        sel.w = 1.0f;

        // hover on selected items should combine up
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sel);
    }

    bool ret = ImGui::Selectable(label, p_selected, flags, size_arg);

    return ret;
}

// @return printUnit e.g. "%.3f GB" or "%.3f MB" 
const char* computeReadableSize(uint64 inputSize, double& outPrintSize)
{
    outPrintSize = (double)inputSize;

    if (inputSize >= 1024 * 1024 * 1024)
    {
        outPrintSize /= (double)(1024 * 1024 * 1024);
        return ((inputSize / 1024 / 1024) % 1024) ? "%.3f GB" : "%.0f GB";
    }
    else if (inputSize >= 1024 * 1024)
    {
        outPrintSize /= (double)(1024 * 1024);
        return ((inputSize / 1024) % 1024) ? "%.3f MB" : "%.0f MB";
    }
    else if (inputSize >= 1024)
    {
        outPrintSize /= (double)1024;
        return (inputSize % 1024) ? "%.3f KB" : "%.0f KB";
    }

    return "%.0f B";
}

bool ImGuiSearch(const char* label, std::string* value, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    assert(value);

    const char* lensIcon = "\xef\x80\x82";

    ImGuiStyle& style = ImGui::GetStyle();

    float iconWidth = ImGui::CalcTextSize(lensIcon).x + ImGui::GetTextLineHeight();
    float leftDecoWithoutIcon = ImGui::CalcTextSize(lensIcon).x + ImGui::GetTextLineHeight();
    float rightDeco = ImGui::CalcTextSize(label).x + style.ItemSpacing.x;

    // label should not to close to the border
    rightDeco += style.ItemSpacing.x;

    // width of the usable TextInput part
    float buttonWidth = ImGui::GetContentRegionAvail().x - leftDecoWithoutIcon - ImGui::GetTextLineHeight() - rightDeco;

    float x = ImGui::GetCursorPosX();

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    {
        ImVec2 size(buttonWidth + iconWidth + ImGui::GetTextLineHeight(), ImGui::GetFontSize() + 2 * style.FramePadding.y);

        const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
        const ImU32 col = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImGui::RenderFrame(bb.Min, bb.Max, col, true, 100.0f);
    }

    bool ret;
    {
        ImStyleColor_RAII seeThrough;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));

        ImGui::SameLine();
        ImGui::SetCursorPosX(x + leftDecoWithoutIcon);
        ImGui::PushItemWidth(buttonWidth);
        ImGui::PushID(label);
        ret = ImGui::InputText("##", value, flags, callback, user_data);
        ImGui::PopID();
        ImGui::PopItemWidth();
    }

    // icon
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + ImGui::GetFontSize() * 0.5f);
    ImGui::TextUnformatted(lensIcon);

    // label
    ImGui::SameLine();
    ImGui::SetCursorPosX(x + buttonWidth + ImGui::CalcTextSize(lensIcon).x + 2 * ImGui::GetTextLineHeight() + style.ItemSpacing.x);
    ImGui::TextUnformatted(label);

    return ret;
}

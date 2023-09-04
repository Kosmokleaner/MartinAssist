#include <windows.h> // HICON
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

#include <shlobj.h> // _wfinddata_t

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#pragma comment(lib, "ImGui/GLFW/glfw3.lib")        // 64bit
#pragma comment(lib, "OpenGL32.lib")                // 64bit


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
unsigned int RGBSwizzle(unsigned int c) {
    return (c >> 16) | (c & 0xff00) | ((c & 0xff) << 16);
}

void Gui::setViewDirty() 
{    
    // constant is in seconds
    whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.25f;
}

Gui::Gui()
    : fileSelectionRange(everyHere)
{
}

void showIconsWindow(ImFont *font, bool &show)
{
    if(!ImGui::Begin("Font", &show))
        return;

    static std::string characterToShow;
    static std::string literalToShow;

    // todo: not perfect, see G+
    float fontWidth = 0;
    for(int32 i = 0, count = font->IndexAdvanceX.Size; i < count; ++i)
    {
        fontWidth = std::max(fontWidth, font->IndexAdvanceX[i]);
    }

    const ImU32 glyph_col = ImGui::GetColorU32(ImGuiCol_Text);
    const float cell_size = fontWidth; //font->FontSize * 1;
    const float cell_spacing = ImGui::GetStyle().ItemSpacing.y;

    if (ImGui::BeginChild("ScrollReg", ImVec2((cell_size + cell_spacing) * 16 + cell_spacing * 4, 0), true))
//    if(ImGui::BeginChild("ScrollReg", ImVec2((cell_size + cell_spacing) * 16 + cell_spacing * 4, 500), true))
    {
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

            // Draw a 16x16 grid of glyphs
            int32 printedCharId = 0;
            for (unsigned int n = 0; n < 256; n++)
            {
                const ImFontGlyph* glyph = font->FindGlyphNoFallback((ImWchar)(base + n));
                if (glyph)
                {
                    ImGui::SetCursorPosX((printedCharId % 16) * (cell_size + cell_spacing));

                    // static to avoid memory allocations
                    static std::wstring wstr;
                    wstr.clear();
                    wstr.push_back((TCHAR)(base + n));
                    ImGui::PushID(n);
                    // inefficient (memory allocations) but simple
                    ImGui::PushFont(font);
                    ImGui::Button(to_string(wstr).c_str(), ImVec2(cell_size, 0));
                    ImGui::PopFont();
                    ImGui::PopID();

                    printedCharId++;
                    if (printedCharId % 16)
                        ImGui::SameLine();

                    if(ImGui::IsItemHovered())
                    {
                        BeginTooltip();
                        ImGui::Text("Codepoint: U+%04X", base + n);
                        EndTooltip();

                        if(ImGui::IsMouseClicked(0))
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
                    }
                }
            }
        }
        ImGui::EndChild();
    }
    if(!characterToShow.empty())
    {
        ImGui::SameLine();
        ImGui::PushFont(font);
        ImGui::Text("%s", characterToShow.c_str());
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%s", literalToShow.c_str());
    }

    ImGui::End();
}


int Gui::test()
{
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

        ImGui::GetStyle().GrabMinSize = 20;
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
        ImFont* font = io.Fonts->AddFontFromFileTTF((std::string(fonts) +  "\\Arial.ttf").c_str(), 21.0f);
        font;

        // https://pixtur.github.io/mkdocs-for-imgui/site/FONTS
        // https://github.com/beakerbrowser/beakerbrowser.com/blob/master/fonts/fontawesome-webfont.ttf
        static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // Will not be copied by AddFont* so keep in scope.
        ImFontConfig config;
        config.MergeMode = true;
//        fontAwesome = io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f, &config, icons_ranges);
        fontAwesome = io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f, &config, icons_ranges);
        config.MergeMode = false;
        fontAwesomeLarge = io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 18.0f * 2, &config, icons_ranges);
        io.Fonts->Build();

        //IM_ASSERT(font != NULL);
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool quitApp = false;

    // todo: serialize
    bool showDrives = true;
    bool showFiles = false;
    bool show_demo_window = false;
    bool showIcons = true;

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

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        showIconsWindow(fontAwesomeLarge, showIcons);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("EveryHere Drives", 0, &showDrives);
                ImGui::MenuItem("EveryHere Files", 0, &showFiles);
                ImGui::Separator();
                ImGui::MenuItem("ImGui Demo", 0, &show_demo_window);
                ImGui::MenuItem("Icons", 0, &showIcons);
                ImGui::Separator();
                if (ImGui::MenuItem("Quit"))
                {
                    quitApp = true;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        guiDrives(showDrives);
        

        for (auto it = everyHere.driveData.begin(); it != everyHere.driveData.end(); ++it)
        {
            if(it->markedForDelete)
            {
               everyHere.driveData.erase(it);
               it = everyHere.driveData.begin();
               driveSelectionRange.reset();
               setViewDirty();
            }
        }

        guiFiles(showFiles);


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

void BeginTooltip()
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.9f, 0.9f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
    ImGui::BeginTooltip();
}

void EndTooltip()
{
    ImGui::EndTooltip();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
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

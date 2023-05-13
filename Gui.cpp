#include <windows.h> // HICON
#include "Gui.h"


// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "FileSystem.h"
#include "Timer.h"

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

class LoadCVSFiles : public IDirectoryTraverse
{
    EveryHere& everyHere;
public:
    LoadCVSFiles(EveryHere& inEveryHere)
        : everyHere(inEveryHere)
    {
        everyHere.freeData();
    }

    bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData)
    {
        return false;
    }
    void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData)
    {
        FilePath combined = path;
        combined.Append(file);

        everyHere.loadCSV(combined.path.c_str());
    }
};


unsigned int RGBSwizzle(unsigned int c) {
    return (c >> 16) | (c & 0xff00) | ((c & 0xff) << 16);
}

void Gui::setViewDirty() 
{
    // costant is in seconds
    whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.25f;
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


    // https://stackoverflow.com/questions/7375003/how-to-convert-hicon-to-hbitmap-in-vc
#if _WIN32
    {
        HICON hIcon = (HICON)LoadImage(GetModuleHandle(0), L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

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
#endif


    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        // no transparent windows
        colors[ImGuiCol_WindowBg].w = 1.0f;
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
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
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

        // Devices
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(320, 100), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("EveryHere Devices");

            if (ImGui::Button("build"))
            {
                everyHere.gatherData();
                setViewDirty();
            }

            ImGui::SameLine();

            if (ImGui::Button("load"))
            {
                LoadCVSFiles loadCVSFiles(everyHere);
                directoryTraverse(loadCVSFiles, FilePath(), L"*.csv");
                setViewDirty();
            }

            {
                ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
                // number of columns: 4
                if (ImGui::BeginTable("table_scrolly", 4, flags))
                {
                    std::string line;
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.961f, 0.514f, 0.000f, 0.400f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.961f, 0.514f, 0.000f, 0.600f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.99f, 0.99f, 0.99f, 0.60f));

                    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                    ImGui::TableSetupColumn("VolumeName", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("UniqueName", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_None);
                    ImGui::TableHeadersRow();

                    int line_no = 0;
                    for (auto it = everyHere.deviceData.begin(), end = everyHere.deviceData.end(); it != end; ++it, ++line_no)
                    {
                        ImGui::TableNextRow();

                        ImGui::PushID(line_no);

                        ImGui::TableSetColumnIndex(0);
                        ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                        bool selected = deviceSelectionRange.isSelected(line_no);
                        ImGui::Selectable(to_string(it->volumeName).c_str(), &selected, selectable_flags);
                        if (ImGui::IsItemClicked(0))
                        {
                            deviceSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                            setViewDirty();
                        }

                        ImGui::TableSetColumnIndex(1);
                        line = to_string(it->cleanName);
                        ImGui::TextUnformatted(line.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(to_string(it->drivePath).c_str());

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%d", it->deviceId);

                        ImGui::PopID();
                        //                            ImGui::TextUnformatted("2");
                    }
                    ImGui::PopStyleColor(3);

                    ImGui::EndTable();
                }
            }

            ImGui::End();
        }

        // FileEntries
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("EveryHere Files");

            // todo: filter button
            if(ImGui::InputText("filter", &filter))
            {
                setViewDirty();
            }

            {
                int step=1;
                const char *fmt[] = {"", "1 KB", "1 MB", "10 MB", "100 MB", "1 GB"};
                minLogSize = ImClamp(minLogSize, 0, 5);
                ImGui::SetNextItemWidth(150);
                if(ImGui::InputScalar(" minimum Size", ImGuiDataType_S32, &minLogSize, &step, &step, fmt[ImClamp(minLogSize, 0, 5)]))
                    setViewDirty();
            }

            if(whenToRebuildView != -1 && g_Timer.GetAbsoluteTime() > whenToRebuildView)
            {
                int64 minSize[] = { 0, 1024, 1024 * 1024, 10 * 1024 * 1024, 100 * 1024 * 1024, 1024 * 1024 * 1024 };
                everyHere.buildView(filter.c_str(), minSize[ImClamp(minLogSize, 0, 5)], deviceSelectionRange);
                whenToRebuildView = -1;
            }


            {
                // todo: 0
//                DeviceData& data = everyHere.deviceData[0];

//                const int lineHeight = (int)ImGui::GetFontSize();
//                int height = (int)data.files.size() * lineHeight;

                ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
                static SelectionRange selectionRange;

                // safe space for info line
                ImVec2 outerSize = ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight() - ImGui::GetStyle().ItemSpacing.y);
                // number of columns: 4
                if (ImGui::BeginTable("table_scrolly", 4, flags, outerSize))
                {
                    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_None);
                    //                    ImGui::TableSetupColumn("Date Modified", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Date Accessed", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Date Created", ImGuiTableColumnFlags_None);
                    ImGui::TableHeadersRow();

                    ImGuiListClipper clipper;
                    clipper.Begin((int)everyHere.view.size());
                    std::string line;
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.961f, 0.514f, 0.000f, 0.400f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.961f, 0.514f, 0.000f, 0.600f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.99f, 0.99f, 0.99f, 0.60f));
                    while (clipper.Step())
                    {
                        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                        {
                            if(line_no >= everyHere.view.size())
                                break;

                            ViewEntry& viewEntry = everyHere.view[line_no];
                            const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                            const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];

                            line = entry.key.fileName;

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::PushID(line_no);
                            ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                            bool selected = selectionRange.isSelected(line_no);
                            ImGui::Selectable(line.c_str(), &selected, selectable_flags); 
                            if (ImGui::IsItemClicked(0))
                            {
                                selectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                            }
                            ImGui::PopID();
                            //                            ImGui::TextUnformatted(line.c_str());
                            ImGui::TableSetColumnIndex(1);

                            {
                                int64 index = entry.value.parent;
                                int i = 0;
                                while(index >= 0) 
                                { 
                                    ++i; if(i>15)break;
                                    const FileEntry& here = deviceData.entries[index];
                                    assert(here.key.sizeOrFolder == -1);
                                    ImGui::Text("/%s", here.key.fileName.c_str());
                                    index = here.value.parent;
                                    if(index >= 0)
                                        ImGui::SameLine(0, 0);
                                }
                            }
                            ImGui::TableSetColumnIndex(2);
                            if(entry.key.sizeOrFolder >= 1024 * 1024 * 1024)
                                ImGui::Text("%.3f GB", entry.key.sizeOrFolder / (1024.0f * 1024.0f * 1024.0f));
                            else if (entry.key.sizeOrFolder >= 1024 * 1024)
                                ImGui::Text("%.0f MB", entry.key.sizeOrFolder / (1024.0f * 1024.0f));
                            else if (entry.key.sizeOrFolder >= 1024)
                                ImGui::Text("%.0f KB", entry.key.sizeOrFolder / 1024.0f);
                            else
                                ImGui::Text("%lld B", entry.key.sizeOrFolder);

                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%d", entry.value.deviceId);
                            //                            ImGui::TextUnformatted("2");
                        }
                    }
                    clipper.End();
                    ImGui::PopStyleColor(3);

                    ImGui::EndTable();
                }
                ImGui::Text("Files: %lld", (int64)everyHere.view.size());
                ImGui::SameLine();        
                if (everyHere.viewSumSize > 1024 * 1024 * 1024)
                    ImGui::Text("Size: %lld GB", everyHere.viewSumSize / (1024 * 1024 * 1024));
                else if (everyHere.viewSumSize > 1024 * 1024)
                    ImGui::Text("Size: %lld MB", everyHere.viewSumSize / (1024 * 1024));
                else if (everyHere.viewSumSize >= 1024)
                    ImGui::Text("Size: %lld KB", everyHere.viewSumSize / 1024);
                else
                    ImGui::Text("Size: %lld Bytes", everyHere.viewSumSize);
            }

            ImGui::End();
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

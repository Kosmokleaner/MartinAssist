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

    void OnEnd() 
    {
        everyHere.updateLocalDriveState();
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
    // constant is in seconds
    whenToRebuildView = g_Timer.GetAbsoluteTime() + 0.25f;
}

// @return printUnit e.g. "GB" or "MB" 
const char* computeReadableSize(uint64 inputSize, uint64 &outPrintSize) 
{
    outPrintSize = inputSize;

    if (inputSize >= 1024 * 1024 * 1024)
    {
        outPrintSize /= (1024 * 1024 * 1024);
        return "GB";
    }
    else if (inputSize >= 1024 * 1024)
    {
        outPrintSize /= (1024 * 1024);
        return "MB";
    }
    else if (inputSize >= 1024)
    {
        outPrintSize /= 1024;
        return "KB";
    }

    return "B";
}


void pushTableStyle3() 
{
    // top to bottom priority, the colors do not blend
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.961f, 0.514f, 0.000f, 0.600f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.961f, 0.514f, 0.000f, 0.600f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.961f, 0.514f, 0.000f, 0.400f));
}

Gui::Gui()
    : fileSelectionRange(everyHere)
{
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

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        // no transparent windows
        colors[ImGuiCol_WindowBg].w = 1.0f;

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

            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            // 0: all drives, 1:local drives
            int tabId = 0;
            if (ImGui::BeginTabBar("DriveLocality", tab_bar_flags))
            {
                if (ImGui::BeginTabItem("All Drives"))
                {
                    tabId = 0;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Local Drives"))
                {
                    tabId = 1;
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            if (ImGui::Button("build"))
            {
                everyHere.gatherData();
                setViewDirty();
            }

            ImGui::SameLine();

            --triggerLoadOnStartup;
            if (triggerLoadOnStartup < -1)
                triggerLoadOnStartup = -1;

#ifdef _DEBUG
            if(triggerLoadOnStartup == 0) 
            {
                everyHere.freeData();
                char str[80];
                for(int i = 0; i < 5; ++i)
                {
                    str[0] = 'a' + (rand() % 26);
                    str[1] = 'a' + (rand() % 26);
                    str[2] = 0;
                    everyHere.deviceData.push_back(DeviceData());
                    DeviceData &d = everyHere.deviceData.back();
                    d.cleanName = str;
                    d.volumeName = str;
                    str[0] = 'A' + (rand() % 26);
                    str[1] = ':';
                    str[2] = 0;
                    d.drivePath = str;
                    d.deviceId = i;
                }

                for(int i = 0; i < 200; ++i)
                {
                    int deviceId = rand() % 5;
                    DeviceData& r = everyHere.deviceData[deviceId];
                    r.entries.push_back(FileEntry());
                    FileEntry &e = r.entries.back();
                    str[0] = 'a' + (rand() % 26);
                    str[1] = 'a' + (rand() % 26);
                    str[2] = 0;
                    e.key.fileName = str;
                    e.key.sizeOrFolder = rand() * 1000;
                    e.key.time_write = rand();
                    e.value.parent = -1;
                    e.value.deviceId = deviceId;
                }
                setViewDirty();
                everyHere.buildUniqueFiles();
                triggerLoadOnStartup = -1;
            }
#endif

            if (ImGui::Button("load") || triggerLoadOnStartup == 0)
            {
                LoadCVSFiles loadCVSFiles(everyHere);
                directoryTraverse(loadCVSFiles, FilePath(), L"*.csv");
                setViewDirty();
                everyHere.buildUniqueFiles();
            }

            {
                ImGuiTableFlags flags = 
                    ImGuiTableFlags_Borders |
                    ImGuiTableFlags_ScrollY |
                    ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_Reorderable |
                    ImGuiTableFlags_Hideable |
                    ImGuiTableFlags_Sortable |
                    ImGuiTableFlags_SortMulti;

                const uint32 numberOfColumns = 14;
                if (ImGui::BeginTable("table_scrolly", numberOfColumns, flags))
                {
                    std::string line;

                    pushTableStyle3();
                    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                    ImGui::TableSetupColumn("VolumeName", ImGuiTableColumnFlags_None, 0.0f, DCID_VolumeName);
                    ImGui::TableSetupColumn("UniqueName", ImGuiTableColumnFlags_None, 0.0f, DCID_UniqueName);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_None, 0.0f, DCID_Path);
                    ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_None, 0.0f, DCID_DeviceId);
                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None, 0.0f, DCID_Size);
                    ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_None, 0.0f, DCID_Files);
                    ImGui::TableSetupColumn("Directories", ImGuiTableColumnFlags_None, 0.0f, DCID_Directories);
                    ImGui::TableSetupColumn("Computer", ImGuiTableColumnFlags_None, 0.0f, DCID_Computer);
                    ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_None, 0.0f, DCID_User);
                    ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_None, 0.0f, DCID_Date);
                    ImGui::TableSetupColumn("totalSpace", ImGuiTableColumnFlags_None, 0.0f, DCID_totalSpace);
                    ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_None, 0.0f, DCID_type);
                    ImGui::TableSetupColumn("serial", ImGuiTableColumnFlags_None, 0.0f, DCID_serial);
                    ImGui::TableSetupColumn("selected Files", ImGuiTableColumnFlags_None, 0.0f, DCID_selectedFiles);
                    ImGui::TableHeadersRow();

                    everyHere.buildDriveView(ImGui::TableGetSortSpecs());

                    int line_no = 0;
                    for (auto it = everyHere.driveView.begin(), end = everyHere.driveView.end(); it != end; ++it, ++line_no)
                    {
                        DeviceData& drive = everyHere.deviceData[*it];

                        if(tabId == 1 && !drive.isLocalDrive)
                            continue;

                        ImGui::TableNextRow();

                        ImGui::PushID(line_no);

                        ImGui::PushStyleColor(ImGuiCol_Text, drive.isLocalDrive ? ImVec4(0.5f, 0.5f, 1.0f,1) : ImVec4(0.8f, 0.8f, 0.8f, 1));

                        ImGui::TableSetColumnIndex(0);
                        ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                        bool selected = deviceSelectionRange.isSelected(line_no);
                        ImGui::Selectable(drive.volumeName.c_str(), &selected, selectable_flags);
                        if (ImGui::IsItemClicked(0))
                        {
                            deviceSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                            setViewDirty();
                        }
                        if (ImGui::BeginPopupContextItem())
                        {
                            if (!deviceSelectionRange.isSelected(line_no))
                            {
                                deviceSelectionRange.reset();
                                deviceSelectionRange.onClick(line_no, false, false);
                            }

                            if (deviceSelectionRange.count() == 1)
                            {
                                if (ImGui::MenuItem("Delete File"))
                                {
                                    DeleteFileA((drive.csvName + ".csv").c_str());
                                    drive.markedForDelete = true;
                                }
                                if (ImGui::MenuItem("Open path (in Explorer)"))
                                {
                                    FilePath filePath(to_wstring(drive.csvName + ".csv").c_str());

                                    ShellExecuteA(0, 0, to_string(filePath.extractPath()).c_str(), 0, 0, SW_SHOW);
                                }
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::TableSetColumnIndex(1);
                        line = drive.csvName;
                        ImGui::TextUnformatted(line.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(drive.drivePath.c_str());

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%d", drive.deviceId);

                        ImGui::TableSetColumnIndex(4);
                        {
                            uint64 printSize = 0;
                            const char* printUnit = computeReadableSize(drive.statsSize, printSize);
                            ImGui::Text("%llu %s", printSize, printUnit);
                        }

                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%llu", (uint64)drive.entries.size());

                        ImGui::TableSetColumnIndex(6);
                        ImGui::Text("%llu", drive.statsDirs);

                        ImGui::TableSetColumnIndex(7);
                        ImGui::TextUnformatted(drive.computerName.c_str());

                        ImGui::TableSetColumnIndex(8);
                        ImGui::TextUnformatted(drive.userName.c_str());

                        ImGui::TableSetColumnIndex(9);
                        ImGui::TextUnformatted(drive.date.c_str());

                        ImGui::TableSetColumnIndex(10);
                        if(drive.totalSpace)
                        {
                            uint64 printSize = 0;
                            const char* printUnit = computeReadableSize(drive.totalSpace, printSize);
                            ImGui::Text("%llu %s", printSize, printUnit);
                        }
                        ImGui::TableSetColumnIndex(11);
                        bool supportsRemoteStorage = drive.driveFlags & 0x100;
                        ImGui::Text("%d", supportsRemoteStorage ? -(int)(drive.driveType) : drive.driveType);

                        ImGui::TableSetColumnIndex(12);
                        ImGui::Text("%u", drive.serialNumber);

                        ImGui::TableSetColumnIndex(13);
                        ImGui::Text("%llu", drive.selectedKeys);

                        ImGui::PopStyleColor();
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor(3);

                    ImGui::EndTable();
                }
            }

            ImGui::End();
        }

        for (auto it = everyHere.deviceData.begin(); it != everyHere.deviceData.end(); ++it)
        {
            if(it->markedForDelete)
            {
               everyHere.deviceData.erase(it);
               it = everyHere.deviceData.begin();
               deviceSelectionRange.reset();
               setViewDirty();
            }
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
                if(ImGui::InputScalar("minimum File Size  ", ImGuiDataType_S32, &minLogSize, &step, &step, fmt[ImClamp(minLogSize, 0, 5)]))
                    setViewDirty();
            }
            ImGui::SameLine();
            {
                const char* fmt = " \000" "<2 no\000" "<3 minor\000" "=3 enough\000" ">3 too much\000" ">4 way too much\000" "\000";
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo("Redundancy  ", &redundancyFilter, fmt))
                    setViewDirty();
            }

            {
                // todo: 0
//                DeviceData& data = everyHere.deviceData[0];

//                const int lineHeight = (int)ImGui::GetFontSize();
//                int height = (int)data.files.size() * lineHeight;

                ImGuiTableFlags flags = ImGuiTableFlags_Borders |
                    ImGuiTableFlags_ScrollY |
                    ImGuiTableFlags_BordersOuter | 
                    ImGuiTableFlags_BordersV | 
                    ImGuiTableFlags_Resizable | 
                    ImGuiTableFlags_Reorderable | 
                    ImGuiTableFlags_Hideable | 
                    ImGuiTableFlags_Sortable | 
                    ImGuiTableFlags_SortMulti;

                // safe space for info line
                ImVec2 outerSize = ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight() - ImGui::GetStyle().ItemSpacing.y);
                const uint32 numberOfColumns = 5;
                if (ImGui::BeginTable("table_scrolly", numberOfColumns, flags, outerSize))
                {
                    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.0f, FCID_Name);
                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None, 0.0f, FCID_Size);
                    ImGui::TableSetupColumn("Redundancy", ImGuiTableColumnFlags_None, 0.0f, FCID_Redundancy);
                    ImGui::TableSetupColumn("DeviceId", ImGuiTableColumnFlags_None, 0.0f, FCID_DeviceId);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_None, 0.0f, FCID_Path);
                    //                    ImGui::TableSetupColumn("Date Modified", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Date Accessed", ImGuiTableColumnFlags_None);
//                    ImGui::TableSetupColumn("Date Created", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
                    ImGui::TableHeadersRow();

                    if(ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
                        if(sorts_specs->SpecsDirty) 
                        {
                            whenToRebuildView = 0;
                            sorts_specs->SpecsDirty = false;
                        }

                    if (whenToRebuildView != -1 && g_Timer.GetAbsoluteTime() > whenToRebuildView)
                    {
                        fileSortCriteria = ImGui::TableGetSortSpecs();
                        // do this before changes to the fileView
                        fileSelectionRange.reset();
                        int64 minSize[] = { 0, 1024, 1024 * 1024, 10 * 1024 * 1024, 100 * 1024 * 1024, 1024 * 1024 * 1024 };
                        everyHere.buildFileView(filter.c_str(), minSize[ImClamp(minLogSize, 0, 5)], redundancyFilter, deviceSelectionRange, fileSortCriteria);
                        whenToRebuildView = -1;
                        fileSortCriteria = {};
                    }

                    ImGuiListClipper clipper;
                    clipper.Begin((int)everyHere.fileView.size());
                    std::string line;
                    pushTableStyle3();
                    while (clipper.Step())
                    {
                        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                        {
                            if(line_no >= everyHere.fileView.size())
                                break;

                            ViewEntry& viewEntry = everyHere.fileView[line_no];
                            const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                            const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];

                            line = entry.key.fileName;

                            ImGui::TableNextRow();

                            ImGui::PushID(line_no);

                            ImGui::TableSetColumnIndex(0);
                            ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                            bool selected = fileSelectionRange.isSelected(line_no);
                            ImGui::Selectable(line.c_str(), &selected, selectable_flags); 
                            if (ImGui::IsItemClicked(0))
                            {
                                fileSelectionRange.onClick(line_no, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
                            }
                            if (ImGui::BeginPopupContextItem())
                            {
                                if(!fileSelectionRange.isSelected(line_no))
                                {
                                    fileSelectionRange.reset();
                                    fileSelectionRange.onClick(line_no, false, false);
                                }

                                if(fileSelectionRange.count() == 1)
                                {
                                    if (ImGui::MenuItem("Open (with default program)"))
                                    {
//                                        const char* path = deviceData.generatePath(entry.value.parent);
                                        const char* path = entry.value.path.c_str();
                                        std::string fullPath = deviceData.drivePath + "/" + path + "/" + entry.key.fileName;
                                        ShellExecuteA(0, 0, fullPath.c_str(), 0, 0, SW_SHOW);
                                    }
                                    if (ImGui::MenuItem("Open path (in Explorer)"))
                                    {
//                                        const char* path = deviceData.generatePath(entry.value.parent);
                                        const char* path = entry.value.path.c_str();
                                        std::string fullPath = deviceData.drivePath + "/" + path;
                                        ShellExecuteA(0, 0, fullPath.c_str(), 0, 0, SW_SHOW);
                                    }
                                }
                                if (ImGui::MenuItem("Copy selection as path (to clipboard)")) 
                                {
                                    ImGui::LogToClipboard();

                                    fileSelectionRange.foreach([&](int64 line_no) {
                                        ViewEntry& viewEntry = everyHere.fileView[line_no];
                                        const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                                        const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];
                                        if (entry.key.sizeOrFolder >= 0) 
                                        {
//                                            const char* path = deviceData.generatePath(entry.value.parent);
                                            const char* path = entry.value.path.c_str();
                                            if(*path)
                                                ImGui::LogText("%s/%s/%s\n", deviceData.drivePath.c_str(), path, entry.key.fileName.c_str());
                                            else
                                                ImGui::LogText("%s/%s\n", deviceData.drivePath.c_str(), entry.key.fileName.c_str());
                                        }
                                        });

                                    ImGui::LogFinish();
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::PopID();

                            ImGui::TableSetColumnIndex(1);
                            uint64 printSize = 0;
                            const char* printUnit = computeReadableSize(entry.key.sizeOrFolder, printSize);
                            ImGui::Text("%llu %s", printSize, printUnit);

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%d", everyHere.findRedundancy(entry.key));

                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%d", entry.value.deviceId);

                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextUnformatted(entry.value.path.c_str());
                        }
                    }
                    clipper.End();
                    ImGui::PopStyleColor(3);

                    ImGui::EndTable();
                }

                if(fileSelectionRange.empty())
                {
                    ImGui::Text("Files: %lld", (int64)everyHere.fileView.size());
                    ImGui::SameLine();

                    uint64 printSize = 0;
                    const char* printUnit = computeReadableSize(everyHere.viewSumSize, printSize);
                    ImGui::Text("Size: %llu %s", printSize, printUnit);
                }
                else 
                {
                    ImGui::Text("Selected: %lld", (int64)fileSelectionRange.count());
                    ImGui::SameLine();

                    // Can be optimized when using drives instead but then we need selectedFileSize there as well
                    uint64 selectedSize = 0;
                    fileSelectionRange.foreach([&](int64 line_no) {
                        ViewEntry& viewEntry = everyHere.fileView[line_no];
                        const DeviceData& deviceData = everyHere.deviceData[viewEntry.deviceId];
                        const FileEntry& entry = deviceData.entries[viewEntry.fileEntryId];
                        if(entry.key.sizeOrFolder >= 0)
                            selectedSize += entry.key.sizeOrFolder;
                    });

                    uint64 printSize = 0;
                    const char* printUnit = computeReadableSize(selectedSize, printSize);
                    ImGui::Text("Size: %llu %s", printSize, printUnit);
                }
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

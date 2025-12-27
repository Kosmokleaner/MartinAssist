# works on ubunutu 64bit
# cmake -DCMAKE_BUILD_TYPE=Release ..
# ../MartinAssist
#
# cmake -DCMAKE_BUILD_TYPE=Debug ..
# gdb ../MartinAssist
#   run
#   backtrace
#   quit

cmake_minimum_required(VERSION 3.12) # Use an appropriate version
project(MartinAssistProject CXX C)

# Global C++ standard settings:
#   - Use C++ 17
#   - Require C++ 17 support from compiler
#   - Dissalow compiler-specific extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

add_executable(MartinAssist
	AllFilesInFolder.cpp
	ASCIIFile.cpp
	audioSample.cpp
	CppParser.cpp
	CppParserTest.cpp
	EFileList.cpp
	ERedundancy.cpp
#	example0.cpp
	FileIODialog.cpp
	FileSystem.cpp
	GranularSynth.cpp
	GranularSynthUI.cpp
	Gui.cpp
#	LiterateOutput.cpp
	LiterateProcessor.cpp
	MartinAssist.cpp
	Parse.cpp
	PooledString.cpp
	SelectionRange.cpp
	Timer.cpp
	WindowDrives.cpp
	WindowFiles.cpp
#	Everything/cli.c
	external/miniAudio/miniaudio.cpp
    ImGui/imgui.cpp
    ImGui/imgui_draw.cpp
    ImGui/imgui_demo.cpp
    ImGui/imgui_tables.cpp
    ImGui/misc/cpp/imgui_stdlib.cpp
    ImGui/imgui_widgets.cpp
    ImGui/backends/imgui_impl_glfw.cpp
    ImGui/backends/imgui_impl_opengl3.cpp
)

find_package(OpenGL REQUIRED)
# sudo apt install libglfw3-dev libgl1-mesa-dev
find_package(glfw3 CONFIG REQUIRED)

#target_include_directories(MartinAssist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(MartinAssist PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
)

target_link_libraries(MartinAssist PRIVATE
    glfw
    OpenGL::GL
)

#target_link_libraries(MartinAssist PRIVATE YourLibraryTargetName)


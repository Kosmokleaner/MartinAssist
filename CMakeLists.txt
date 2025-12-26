cmake_minimum_required(VERSION 3.12) # Use an appropriate version
project(MartinAssistProject CXX C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
    ImGui/imgui_tables.cpp
    ImGui/imgui_widgets.cpp
    ImGui/backends/imgui_impl_glfw.cpp
    ImGui/backends/imgui_impl_opengl3.cpp
)

#target_include_directories(MartinAssist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(MartinAssist PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
)
#target_link_libraries(MartinAssist PRIVATE YourLibraryTargetName)


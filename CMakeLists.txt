cmake_minimum_required(VERSION 3.12) # Use an appropriate version
project(MartinAssistProject CXX)

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
)

#target_include_directories(MartinAssist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(MartinAssist PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
)
target_link_libraries(MartinAssist PRIVATE YourLibraryTargetName)


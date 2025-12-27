#pragma once

#include <string>

class CFileIODialog
{
public:
    // @param szExtension e.g. ".wav"
    // @param filter e.g. "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0"
    // @return success
	bool FileDialogLoad(const char *szDialogTitle, const char *szExtension, const char *szFilter, std::string &rInOutFilename);
    // @param szExtension e.g. ".wav"
    // @param filter e.g. "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0"
    // @return success
    bool FileDialogSave(const char *szDialogTitle, const char *szExtension, const char *szFilter, std::string &rInOutFilename);
};

#include "types.h"
#include "FileIODialog.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #include <commdlg.h>																		// OPENFILENAME, GetOpenFileName
#else
    #include <nfd.h>
#endif

#include <assert.h>



//! e.g. "c:\test/filename.ext" -> "filename.ext"
inline void RemovePath(std::string& v)
{
    char* p = v.data(), * s = 0, * d = v.data();

    while (*p != 0)
    {
        if (*p == '/' || *p == '\\')
            s = p;

        ++p;
    }

    if (!s)
        return;

    ++s;

    while (*s != 0)
        *d++ = *s++;

    *d = 0;
}

//! e.g. "c:\test/filename.ext" -> "c:\test"
inline void RemoveFilename(std::string& v)
{
    char* p = v.data(), * s = 0;

    while (*p != 0)
    {
        if (*p == '/' || *p == '\\')
            s = p;

        ++p;
    }

    if (!s)
        return;

    *s = 0;
}




bool CFileIODialog::FileDialogLoad( const char *szDialogTitle, const char *szExtension, const char *szFilter, std::string &rInOutFilename)
{
#ifdef _WIN32
	char back[MAX_PATH];
	if(GetCurrentDirectoryA(MAX_PATH,back)==0)
	{
//			Error.Add("GetCurrentDirectory failed");
		return 0;
	}


    OPENFILENAMEA ofn;

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
//		ofn.hwndOwner=m_Renderer.GetWindowHWND();
	ofn.hwndOwner=NULL;
	ofn.hInstance=(HINSTANCE)::GetModuleHandle(0);
	ofn.lpstrFilter=szFilter;
	ofn.lpstrCustomFilter = (LPSTR)NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;
//	ofn.lpstrInitialDir=back;
//		if(filestr[0]==0)ofn.lpstrInitialDir=back;
	ofn.lpstrTitle=szDialogTitle;

//	if(bShowPreview)
//		ofn.lpfnHook = MyHookProc;

	ofn.lpstrFileTitle=0;
	ofn.nMaxFileTitle=0;

//    TCHAR szFile[MAX_PATH] = TEXT(""); // Buffer for file name
    rInOutFilename.reserve(MAX_PATH);
	ofn.lpstrFile = rInOutFilename.data();
//    ofn.lpstrFile = szFile;
	ofn.nMaxFile=MAX_PATH;

	// the following is needed for the win32 function
	for(char *p=&rInOutFilename[0];*p!=0;++p)
		if(*p=='/')
			*p='\\';

	// Vista requires the path and the file split nicely otherwise the folder in the dialog is set wrong
	{
		std::string sPath = rInOutFilename;
		RemoveFilename(sPath);
		ofn.lpstrInitialDir=sPath.c_str();
		RemovePath(rInOutFilename);
	}

	//  ofn.lpTemplateName = MAKEINTRESOURCE(IDD_CUSTOM);			// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnwui/html/msdn_cmndlg32.asp

	ofn.lCustData=0;
	ofn.lpstrDefExt=szExtension;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_ENABLEHOOK | OFN_EXPLORER;	// | OFN_ENABLETEMPLATE;

//	ofn.Flags |= OFN_FILEMUSTEXIST;

	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;

	bool bOk = GetOpenFileNameA(&ofn)!=0;
    // set std::string size
    rInOutFilename = rInOutFilename.data();

	// this is to have consistent path structures
	for(char *p=&rInOutFilename[0];*p!=0;++p)
		if(*p=='\\')
			*p='/';

	SetCurrentDirectoryA(back);
	return bOk;
#else

    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath );
        
    if ( result == NFD_OKAY ) {
        puts("Success!");
        puts(outPath);
        free(outPath);
    }
    else if ( result == NFD_CANCEL ) {
        puts("User pressed cancel.");
    }
    else {
        printf("Error: %s\n", NFD_GetError() );
    }

    return false;
#endif
}



bool CFileIODialog::FileDialogSave( const char *szDialogTitle, const char *szExtension, const char *szFilter, std::string &rInOutFilename )
{
#ifdef _WIN32
	char back[MAX_PATH];
	if(GetCurrentDirectoryA(MAX_PATH,back)==0)
		return false;

    OPENFILENAMEA ofn;

	memset(&ofn,0,sizeof(OPENFILENAMEA));
	ofn.lStructSize=sizeof(OPENFILENAMEA);
//		ofn.hwndOwner=m_Renderer.GetWindowHWND();
	ofn.hwndOwner=NULL;
	ofn.hInstance=(HINSTANCE)::GetModuleHandle(0);
	ofn.lpstrFilter=szFilter;
	ofn.lpstrCustomFilter = (LPSTR)NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;
	ofn.lpstrInitialDir=NULL;
//		if(filestr[0]==0)ofn.lpstrInitialDir=back;
	ofn.lpstrTitle=szDialogTitle;

	ofn.lpstrFileTitle=0;
	ofn.nMaxFileTitle=0;

	ofn.lpstrFile = rInOutFilename.data();
	ofn.nMaxFile=sizeof(rInOutFilename);

	// the following is needed for the win32 function
	for(char *p=&rInOutFilename[0];*p!=0;++p)
		if(*p=='/')
			*p='\\';

	ofn.lCustData=0;

	if(szExtension)
		assert(szExtension[0]!='.');	// MSDN: This string can be any length, but only the first three characters are appended. The string should not contain a period (.).

	ofn.lpstrDefExt=szExtension;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLESIZING;

	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;

	bool bOk = GetSaveFileNameA(&ofn)!=0;

	// this is to have consisent path structures
	for(char *p=&rInOutFilename[0];*p!=0;++p)
		if(*p=='\\')
			*p='/';

	SetCurrentDirectoryA(back);
	return bOk;
#else
    assert(0);
	return false;
#endif
}


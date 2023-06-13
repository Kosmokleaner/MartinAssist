#pragma once
#include <string>

// we use wchar_t instead of TCHAR, see here: https://social.msdn.microsoft.com/Forums/sqlserver/en-US/c7ea5d5b-453e-4326-9b97-0a133d3fef36/what-exactly-is-the-difference-between-tchar-and-wchart?forum=vcgeneral

// platform: windows, should work on other OS
// no copyright issues, written from scratch by Martin Mittring
// intended to be released as public domain without any legal restrictions

// FilePath supports "/" and "\\" slashes, relative and absolute path, should not end with a slash
class FilePath {
    static const wchar_t extensionChar = '.';
public:
    // public so you can access easily
    std::wstring path;

    // default constructor 
    FilePath() {}

    // constructor e.g. FilePath(L"c:\\temp\\a.ext")
    // @param in must not be 0
    FilePath(const wchar_t* in);

    // e.g. L"c:\temp\test/filename.ext"
    // @return L"filename.ext"
    const wchar_t* getFileName() const;

    // e.g. L"c:\temp\test/filename"
    // @return L"c:\temp\test"
    std::wstring extractPath() const;

    const wchar_t* getPathWithoutDrive() const;

    // constructor e.g. FilePath(pathString)
    FilePath(std::wstring& in) : path(in) { }

    // inserts forward slash if needed and appends
    // @param rhs filename, can be with relative path or extension, must not be 0
    void Append(const wchar_t* rhs);

    // assumes wstring is null terminated
    // @return points to 0 terminated extension (after last .) or end of name if no extension was found, never 0, do not access after path changes
    const wchar_t* GetExtension() const;

    // removed extension including '.'
    // @return true if something changed, can be use like this: while(path.RemoveExtension());
    bool RemoveExtension();
  
    // chaneg all \ to /
    void Normalize();

    // valid if doesn't end with slash
    // todo: should c:\ be valid? currently it's not
    bool IsValid() const;

    static void Test();
};

struct DriveInfo
{
    // e.g. L"C:\"
    FilePath drivePath;
    // e.g. L"\Device\HarddiskVolume4", must not be 0
    std::wstring deviceName;
    // e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\", must not be 0
    std::wstring internalName;
    // e.g. L"First Drive", must not be 0
    std::wstring volumeName;
    // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationw
    uint32 driveFlags = 0;
    //
    uint32 serialNumber = 0;

    std::wstring generateKeyName() const;
};

// implement for DriveTraverse()
struct IDriveTraverse {
    virtual void OnStart() {}
    virtual void OnDrive(const DriveInfo& driveInfo) = 0;
    virtual void OnEnd() {}
};


// implement for directoryTraverse()
struct IDirectoryTraverse {
  virtual void OnStart() {}
  // @param filePath without directory e.g. L"D:\temp"
  // @param directory name e.g. L"sub"
  // @return true to recurse into the folder
  virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData) = 0;
  // @param path without file
  // @param file with extension
  virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData) = 0;
  virtual void OnEnd() {}
};

// @param filePath e.g. L"c:\\temp" L"relative/Path" L""
// @param pattern e.g. L"*.cpp" L"*.*", must not be 0
void directoryTraverse(IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern = L"*.*");

// in error case sink might not even get OnStart() call
void driveTraverse(IDriveTraverse& sink);

std::string to_string(std::wstring wstr);

std::wstring to_wstring(std::string str);
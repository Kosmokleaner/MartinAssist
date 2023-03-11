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

// implement for DirectoryTraverse()
struct IDirectoryTraverse {
  virtual void OnStart() {}
  // @param path without directoy
  // @return true to recurse into the folder
  virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory) = 0;
  // @param path without file
  // @param file with extension
  virtual void OnFile(const FilePath& path, const wchar_t* file) = 0;
  virtual void OnEnd() {}
};

// @param filePath e.g. L"c:\\temp" L"relative/Path" L"", must not be 0
// @param pattern e.g. L"*.cpp" L"*.*", must not be 0
void directoryTraverse(IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern = L"*.*");


std::string to_string(std::wstring wstr);

std::wstring to_wstring(std::string str);
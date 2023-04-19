//#include "StdAfx.h"
#include "global.h"
#include "FileSystem.h"
#include <io.h>	// _A_SUBDIR, _findclose()


FilePath::FilePath(const wchar_t* in) {
	assert(in);

	if (in)
		path = in;
}

static bool IsAnySlash(const wchar_t c) {
	return c == '\\' || c== '/';
}

const wchar_t* FilePath::GetExtension() const {
	assert(IsValid());

	const wchar_t* p = &path[0];
	const wchar_t* ret = nullptr;

	while(*p)	{
		if(IsAnySlash(*p))
			ret = nullptr;
		else if (*p == extensionChar)
			ret = p + 1;

		++p;
	}

	if (!ret)
		ret = p;

	return ret;
}

bool FilePath::RemoveExtension() {
	if(path.empty())
		return false;

	const wchar_t* extension = GetExtension();

	// remove '.' as well
	if(extension > &path.front() && extension[-1] == extensionChar)
		--extension;

	size_t extensionLen = &path.back() + 1 - extension;

	if(extensionLen == 0)
		return false;
	
	path.resize(path.size() - extensionLen);
	return true;
}

void FilePath::Normalize() {
	assert(IsValid());

	wchar_t* p = &path[0];

	while(*p) {
		if(*p == '\\')
			*p = '/';
		++p;
	}
}

void FilePath::Append(const wchar_t* rhs) {
	assert(rhs);
	assert(IsValid());

	// to prevent crash with bad input
	if (!rhs)
		return;

	if(!path.empty())
		path += (wchar_t)'/';

	path += rhs;
}

bool FilePath::IsValid() const {
	if (path.empty())
		return true;
	
	return !IsAnySlash(path.back());
}
// ---------------------------------------------------------------------------

void directoryTraverse(IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern) {
	assert(pattern);
	sink.OnStart();

	// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions?view=vs-2019
	struct _wfinddata_t c_file;
	intptr_t hFile = 0;

	FilePath pathWithPattern = filePath;
	pathWithPattern.Append(pattern);

	if ((hFile = _wfindfirst(pathWithPattern.path.c_str(), &c_file)) == -1L)
		return;

	do
	{
		if (wcscmp(c_file.name, L".") == 0 || wcscmp(c_file.name, L"..") == 0)
			continue;

		if (c_file.attrib & _A_SUBDIR) {
			FilePath pathWithDirectory = filePath;
			pathWithDirectory.Append(c_file.name);

			if (sink.OnDirectory(pathWithDirectory, c_file.name)) {
				// recursion
				directoryTraverse(sink, pathWithDirectory, pattern);
			}
		} else {
			sink.OnFile(filePath, c_file.name, c_file);
		}
	} while (_wfindnext(hFile, &c_file) == 0);

	_findclose(hFile);

	sink.OnEnd();
}

void FilePath::Test() {
	// GetExtension()

	{
		std::wstring a = FilePath(L"c:\\temp\\a.ext").GetExtension();
		assert(a == L"ext");
	}
	{
		std::wstring a = FilePath(L"a.ext").GetExtension();
		assert(a == L"ext");
	}
	{
		std::wstring a = FilePath(L"c:\\aa.ss\\a.ext").GetExtension();
		assert(a == L"ext");
	}
	{
		std::wstring a = FilePath(L"c:\\aa.ss\\a").GetExtension();
		assert(a == L"");
	}
	{
		std::wstring a = FilePath(L"file name a").GetExtension();
		assert(a == L"");
	}
	{
		std::wstring a = FilePath(L"./aa.ss/a").GetExtension();
		assert(a == L"");
	}
	{
		std::wstring a = FilePath(L"").GetExtension();
		assert(a == L"");
	}
	{
		FilePath path;
		std::wstring a = path.GetExtension();
		assert(a == L"");
	}

	// IsValid()

	{
		FilePath A(L"c:\\aa.ss\\a.ext");
		assert(A.IsValid());
	}
	{
		FilePath A(L"");
		assert(A.IsValid());
	}
	{
		FilePath A(L"/");
		assert(!A.IsValid());
	}
	{
		FilePath A(L"\\");
		assert(!A.IsValid());
	}
	{
		FilePath A(L"\\a");
		assert(A.IsValid());
	}
	{
		FilePath A(L"c:");
		assert(A.IsValid());
	}
	{
		FilePath A(L"c:\\");
		assert(!A.IsValid());
	}
	{
		FilePath A(L"c:/");
		assert(!A.IsValid());
	}

	// Append()

	{
		FilePath A(L"");
		A.Append(L"");
		assert(A.path == L"");
	}
	{
		FilePath A(L"");
		A.Append(L"file");
		assert(A.path == L"file");
	}
	{
		FilePath A(L"c:");
		A.Append(L"file");
		assert(A.path == L"c:/file");
	}
	{
		FilePath A(L"path/dir");
		A.Append(L"file");
		assert(A.path == L"path/dir/file");
	}
	{
		FilePath A(L"");
		A.Append(L"file");
		assert(A.path == L"file");
	}


	// Normalize()

	{
		FilePath A(L"c:\\aa.ss\\a.ext");
		A.Normalize();
		assert(A.path == L"c:/aa.ss/a.ext");
	}
	{
		FilePath A(L"D:/aa\\a.ext");
		A.Normalize();
		assert(A.path == L"D:/aa/a.ext");
	}
	{
		FilePath A(L"");
		A.Normalize();
		assert(A.path == L"");
	}
	{
		FilePath A;
		A.Normalize();
		assert(A.path == L"");
	}

	// RemoveExtension()

	{
		FilePath A(L"c:/aa.ss\\a");
		A.RemoveExtension();
		assert(A.path == L"c:/aa.ss\\a");
	}
	{
		FilePath A(L"D:/aa.ss\\a.ext");
		A.RemoveExtension();
		assert(A.path == L"D:/aa.ss\\a");
	}
	{
		FilePath A(L"D:/aa\\b.");
		A.RemoveExtension();
		assert(A.path == L"D:/aa\\b");
	}
	{
		FilePath A(L"");
		A.RemoveExtension();
		assert(A.path == L"");
	}
	{
		FilePath A;
		A.RemoveExtension();
		assert(A.path == L"");
	}
}

// todo: move
//https://riptutorial.com/cplusplus/example/4190/conversion-to-std--wstring
#include <codecvt>
#include <locale>

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr) {
	return strconverter.to_bytes(wstr);
}

std::wstring to_wstring(std::string str) {
	return strconverter.from_bytes(str);
}

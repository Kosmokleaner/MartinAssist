//#include "StdAfx.h"
#include "global.h"
#include "FileSystem.h"
#include "Timer.h"
#include <io.h>	// _A_SUBDIR, _findclose()
#include <windows.h>    // GetVolumePathNamesForVolumeNameW()
#include <algorithm>
#include <list>

#pragma warning( disable : 4996 )

FilePath::FilePath(const wchar_t* in) {
	assert(in);

	if (in)
		path = in;
}

static bool IsAnySlash(const wchar_t c) {
	return c == '\\' || c == '/';
}

const wchar_t* FilePath::getFileName() const
{
	const wchar_t* start = path.c_str();
	const wchar_t* ret = start;
	
	for(const wchar_t* p = start; *p; ++p) {
		if(IsAnySlash(*p) || *p == ':')
			ret = p + 1;
	}

	return ret;
}

std::wstring FilePath::extractPath() const
{
	const wchar_t* fileName = getFileName();

	const wchar_t* last = fileName;

	if(path.c_str() != last && IsAnySlash(last[-1]))
		--last;

	std::wstring ret(path.c_str(), last - path.c_str());

	return ret;
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


const wchar_t* FilePath::getPathWithoutDrive() const
{
	const wchar_t* ret = path.c_str();
	const wchar_t* p = ret;
	while (*p != 0 && *p != ':' && *p != '/' && *p != '\\')
		++p;
	if (*p == ':') 
	{
		++p;
		if (*p == '/' || *p == '\\')
			++p;

		ret = p;
	}
	return ret;
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

// depth first
static void _directoryTraverse(IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern) {
	assert(pattern);

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

			if (sink.OnDirectory(filePath, c_file.name, c_file)) {
				FilePath pathWithDirectory = filePath;
				pathWithDirectory.Append(c_file.name);
				// recursion
				_directoryTraverse(sink, pathWithDirectory, pattern);
			}
		}
		else {
			sink.OnFile(filePath, c_file.name, c_file, 0.0f);
		}
	} while (_wfindnext(hFile, &c_file) == 0);

	_findclose(hFile);
}

std::wstring DriveInfo::generateKeyName() const
{
	// Google drive will change it's internalName all the time so no good as drive key name
	// e.g. L"\\?\Volume{41122dbf-6011-11ed-1232-04d4121124bd}\"
//	std::wstring cleanName = internalName;

	// renaming Google drive would create a new entry
//	std::wstring cleanName = std::to_wstring(serialNumber) + L"_" + volumeName;

	// not very human friendly but works to key the drive
	std::wstring cleanName = std::to_wstring(serialNumber);

	return cleanName;
}

void directoryTraverse(IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern) {
	assert(pattern);
	sink.OnStart();

	_directoryTraverse(sink, filePath, pattern);

	sink.OnEnd();
}

void directoryTraverse2(IDirectoryTraverse& sink, const FilePath& inFilePath, uint64 totalExpectedFileSize, const wchar_t* pattern) 
{
	// 59sec C:
	CScopedCPUTimerLog timer("directoryTraverse2");

	assert(pattern);
	sink.OnStart();

	std::list<FilePath> workItems;
	workItems.push_back(inFilePath);

	float doneUnits = 0;
	float totalUnits = 1;

	uint64 totalFileSize = 0;

	double invTotalExpectedFileSize = 1.0 / totalExpectedFileSize;

	while (!workItems.empty())
	{
		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions?view=vs-2019
		struct _wfinddata_t c_file;
		intptr_t hFile = 0;

		const FilePath& filePath = workItems.front();
		FilePath pathWithPattern = filePath;
		pathWithPattern.Append(pattern);

		if ((hFile = _wfindfirst(pathWithPattern.path.c_str(), &c_file)) != -1L)
		{
			do
			{
				if (wcscmp(c_file.name, L".") == 0 || wcscmp(c_file.name, L"..") == 0)
					continue;

				if (c_file.attrib & _A_SUBDIR) 
				{
					if (sink.OnDirectory(filePath, c_file.name, c_file)) {
						FilePath pathWithDirectory = filePath;

						pathWithDirectory.Append(c_file.name);

						static int stepper = 0; ++stepper;
						if((stepper % 1000) == 0)
						{
							OutputDebugStringW(pathWithDirectory.path.c_str());
							OutputDebugStringA("\n");
						}

						workItems.push_back(std::move(pathWithDirectory));
						++totalUnits;
					}
				}
				else {
					float progress = doneUnits / totalUnits;	// this prediction can be improved with some non linear curve assuming an average drive depth, maybe based on former depth

					totalFileSize += c_file.size;

					if(totalExpectedFileSize)
						progress = (float)(totalFileSize * invTotalExpectedFileSize);

					if(progress > 1)
						progress = 1;

					sink.OnFile(filePath, c_file.name, c_file, progress);
				}
			} while (_wfindnext(hFile, &c_file) == 0);

			_findclose(hFile);
		}

		++doneUnits;
		workItems.pop_front();
	}
	sink.OnEnd();
}

// ---------------------------------------------------------------------------

// from https://learn.microsoft.com/en-us/windows/win32/fileio/displaying-volume-paths?redirectedfrom=MSDN
static std::wstring getVolumePaths(__in PWCHAR InternalName)
{
	DWORD  CharCount = MAX_PATH + 1;
	PWCHAR Names = NULL;
	BOOL   Success = FALSE;

	std::wstring ret;

	for (;;)
	{
		//
		//  Allocate a buffer to hold the paths.
		Names = (PWCHAR) new BYTE[CharCount * sizeof(WCHAR)];

		if (!Names)
			return ret;

		//
		//  Obtain all of the paths
		//  for this volume.
		Success = GetVolumePathNamesForVolumeNameW(
			InternalName, Names, CharCount, &CharCount
		);

		if (Success)
			break;

		if (GetLastError() != ERROR_MORE_DATA)
			break;

		//
		//  Try again with the
		//  new suggested size.
		delete[] Names;
		Names = NULL;
	}

	if (Success)
	{
		ret = Names;
	}

	if (Names)
	{
		delete[] Names;
		Names = NULL;
	}

	return ret;
}


void driveTraverse(IDriveTraverse& sink) 
{
	DWORD  CharCount = 0;
	WCHAR  deviceName[MAX_PATH] = L"";
	DWORD  Error = ERROR_SUCCESS;
	HANDLE FindHandle = INVALID_HANDLE_VALUE;
	size_t Index = 0;
	BOOL   Success = FALSE;
	WCHAR  internalName[MAX_PATH] = L"";

	FindHandle = FindFirstVolumeW(internalName, ARRAYSIZE(internalName));

	if (FindHandle == INVALID_HANDLE_VALUE)
	{
//		Error = GetLastError();
		return;
	}

	sink.OnStart();

	for (;;)
	{
		//
		//  Skip the \\?\ prefix and remove the trailing backslash.
		Index = wcslen(internalName) - 1;

		if (internalName[0] != L'\\' ||
			internalName[1] != L'\\' ||
			internalName[2] != L'?' ||
			internalName[3] != L'\\' ||
			internalName[Index] != L'\\')
		{
			Error = ERROR_BAD_PATHNAME;
//			wprintf(L"FindFirstVolumeW/FindNextVolumeW returned a bad path: %s\n", internalName);
			break;
		}

		//  QueryDosDeviceW does not allow a trailing backslash,
		//  so temporarily remove it.
		internalName[Index] = L'\0';

		CharCount = QueryDosDeviceW(&internalName[4], deviceName, ARRAYSIZE(deviceName));

		internalName[Index] = L'\\';

		if (CharCount == 0)
		{
//			Error = GetLastError();
			break;
		}

		if (internalName[0]) {
			std::wstring path = getVolumePaths(internalName);
			if (!path.empty())
			{
				WCHAR volumeName[MAX_PATH];//MAX_PATH is the size of the char array.
				DWORD serialNumber = (DWORD)-1;
				// https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationw
				// FILE_SUPPORTS_REMOTE_STORAGE to detect google drive
				DWORD driveFlags = (DWORD)-1;
				if (GetVolumeInformation(path.c_str(), volumeName, sizeof(volumeName),
					&serialNumber,
					NULL,
					&driveFlags,
					NULL,
					0))
				{
					DriveInfo driveInfo;
					driveInfo.drivePath = path;
					driveInfo.deviceName = deviceName;
					driveInfo.internalName = internalName;
					driveInfo.volumeName = volumeName;
					driveInfo.driveFlags = driveFlags;
					driveInfo.serialNumber = serialNumber;

					sink.OnDrive(driveInfo);
				}
			}
		}

		Success = FindNextVolumeW(FindHandle, internalName, ARRAYSIZE(internalName));

		if (!Success)
		{
			Error = GetLastError();

			if (Error != ERROR_NO_MORE_FILES)
			{
//				wprintf(L"FindNextVolumeW failed with error code %d\n", Error);
				break;
			}

			Error = ERROR_SUCCESS;
			break;
		}
	}

	FindVolumeClose(FindHandle);
	FindHandle = INVALID_HANDLE_VALUE;

	sink.OnEnd();
}

void FilePath::Test() 
{
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

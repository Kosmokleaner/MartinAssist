#include "stdafx.h"
#include "AllFilesInFolder.h"
#include <algorithm>
#include <string>


static bool IsDigit(char c) {
  return c >= '0' && c <= '9';
}

// @return -1 if not part of a randomizer group
static int GetRandomizerGroupId(const char* file, std::string& outName) {
  const char* lastUnderscore = nullptr;

  for (const char* p = file; *p; ++p) {
    if (*p == '_')
      lastUnderscore = p;
  }

  // some _ in the name
  if (!lastUnderscore)
    return -1;

  // needs to start with digit
  if (!IsDigit(lastUnderscore[1]))
    return -1;

  uint32 ret = 0;

  const char* p = lastUnderscore + 1;

  for (; *p; ++p) {
    // file extension starts
    if (*p == '.')
      break;

    // only digits or extension
    if (!IsDigit(*p))
      return -1;

    ret = ret * 10 + (*p - '0');
  }

  // e.g. "" or ".wav"
  const char* extension = p;

  outName = std::string(file, lastUnderscore + 1) + "%d" + extension;

  return ret;
}


AllFilesInFolder::AllFilesInFolder(const char* inPath, const char* inEndsWith, bool inRecursive, bool inDetectRandomizer)
  : path(inPath)
  , endsWith(inEndsWith)
  , recursive(inRecursive)
  , detectRandomizer(inDetectRandomizer) {
  check(inPath);
}

int AllFilesInFolder::FindIndex(const std::string& in) const {
  if (in.size() <= path.size() + 1)
    return -1;

  if (in[path.size()] != '/')
    return -1;

  const char* search = in.c_str() + path.size() + 1;

  int ret = 0;
  for (auto it = files.begin(), end = files.end(); it != end; ++it, ++ret) {
    const std::string& sStr = *it;

    if (strcmp(sStr.c_str(), search) == 0)
      return ret;
  }
  return -1;
}

// to sort 1,2,3,4,5,6,7,8,9,10,11... in that order
struct comp {
  bool operator()(const std::string& x, const std::string& y) const {

    // could be improved e.g. sort by name and then trailing number
//    if (strlen(x.c_str()) < strlen(y.c_str()))
//      return true;

    return strcmp(x.c_str(), y.c_str()) < 0;
  }
};

void AllFilesInFolder::Update() {
  if (isValid)
    return;

  isValid = true;
  files.clear();
  directoryTraverse(*this, FilePath(to_wstring(path)), to_wstring(std::string(endsWith)).c_str());
  std::sort(files.begin(), files.end(), comp());
}

void AllFilesInFolder::FreeData() {
  isValid = false;
  files.clear();
}

bool AllFilesInFolder::IsValid() const { return isValid; }

std::string AllFilesInFolder::GetPath() const { return path; }

bool AllFilesInFolder::OnDirectory(const FilePath& filePath, const char* directory) {
  return recursive;
}

void AllFilesInFolder::OnFile(const FilePath& inPath, const char* file) {
  assert(file);

  if (std::string(file) == "donotdelete.txt")
    return;

  int randomizerId = -1;
  std::string randomizerName;

  if (detectRandomizer)
    randomizerId = GetRandomizerGroupId(file, randomizerName);

  if (randomizerId > 0)
    return;

  const char* concatFile = (randomizerId == 0) ? randomizerName.c_str() : file;

  char* localPath = (char*)inPath.path.c_str();
  // drop input path and trailing slash
  localPath += path.size() + 1;

  check(*concatFile);
  check(*concatFile != '/');

  if (*localPath) {
    check(*localPath != '/');
    files.push_back(std::string(localPath) + "/" + concatFile);
  }
  else
    files.push_back(concatFile);
}

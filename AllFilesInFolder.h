#pragma once

#include <vector>
#include "FileSystem.h"


class AllFilesInFolder : public IDirectoryTraverse {
  bool isValid = false;
  std::string path;
  bool recursive = false;
  bool detectRandomizer = false;
  // e.g. "" or ".wav"
  const char* endsWith = 0;

public:
  std::vector<std::string> files;

  // @param inPath e.g. g_levelsFolder, must not be 0
  AllFilesInFolder(const char* inPath, const char* inEndsWith, bool inRecursive, bool inDetectRandomizer = false);

  // O(n)
  // @param in including path, case sensitive
  // @return -1 if not found, otherwise index into files[]
  int FindIndex(const std::string& in) const;

  // does nothing if data is already there
  void Update();

  void FreeData();

  bool IsValid() const;

  std::string GetPath() const;

  // IDirectoryTraverse -------------------------------

  virtual bool OnDirectory(const FilePath& filePath, const char* directory);

  virtual void OnFile(const FilePath& inPath, const char* file);
};

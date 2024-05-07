#pragma once
#include "types.h"
#include "EFileList.h"
#include <unordered_map>

class Redundancy
{
public:
    struct DriveFileRef
    {
        // index into WindowDrives::drives[]
        uint32 driveId;
        // index into DriveInfo2::fileList[]
        uint64 fileId;
    };

    struct KeyHash
    {
        // hash
        std::size_t operator()(const FileKey& k) const
        {
            return std::hash<std::string_view>()(k.fileName.c_str())
                ^ (std::hash<uint64>()(k.time_write) << 1)
                ^ (std::hash<uint64>()(k.size) << 2);
        }
    };
    struct KeyEqual
    {
        bool operator()(const FileKey& a, const FileKey& b) const
        {
            return a.size == b.size && strcmp(a.fileName.c_str(), b.fileName.c_str()) == 0;
//            return strcmp(a.fileName.c_str(), b.fileName.c_str()) == 0;
        }
    };


    std::unordered_multimap<FileKey, DriveFileRef, KeyHash, KeyEqual> keyToRef;
    
    void addRedundancy(const FileKey& fileKey, uint32 driveId, uint64 fileId);

    uint32 findRedundancy(const FileKey& fileKey) const;
    

};


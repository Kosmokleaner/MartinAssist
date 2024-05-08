#include "ERedundancy.h"

void Redundancy::addRedundancy(const FileKey& fileKey, uint32 driveId, uint64 fileId)
{
    DriveFileRef ref = { driveId, fileId };
    keyToRef.insert(std::pair<FileKey, DriveFileRef>(fileKey, ref));
}

uint32 Redundancy::computeRedundancy(const FileKey& fileKey) const
{
    return (uint32)keyToRef.count(fileKey);
}

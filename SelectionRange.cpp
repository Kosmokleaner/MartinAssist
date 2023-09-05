#include "SelectionRange.h"
#include "EveryHere.h"

void SelectionRangeWithDriveUpdate::addRedundancy(int64 line_no, int32 delta)
{
    const ViewEntry& viewEntry = everyHere.fileView[line_no];
    const FileKey& key = everyHere.driveData[viewEntry.driveId]->entries[viewEntry.fileEntryId].key;
    std::vector<bool>& set = everyHere.getRedundancy(key);

    uint32 id = 0; 
    for(auto it = set.begin(), end = set.end(); it != end; ++it, ++id)
    {
        if(*it)
            everyHere.driveData[id]->selectedKeys += delta;
    }
}

void SelectionRangeWithDriveUpdate::reset()
{
    foreach([&](int64 line_no) {
        addRedundancy(line_no, -1);
    });
    SelectionRange::reset();
}

void SelectionRangeWithDriveUpdate::onClick(int64 x, bool shift, bool ctrl)
{
    // can be optimized
    foreach([&](int64 line_no) {
        addRedundancy(line_no, -1);
    });

    SelectionRange::onClick(x, shift, ctrl);

    foreach([&](int64 line_no) {
        addRedundancy(line_no, 1);
    });
}

void SelectionRangeWithDriveUpdate::toggle(int64 x)
{
    addRedundancy(x, isSelected(x) ? -1 : 1);

    SelectionRange::toggle(x);
}

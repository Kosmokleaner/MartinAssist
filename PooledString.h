#pragma once
#include <vector>
#include "Parse.h"

class PooledString
{
    // 0 for empty string
    class StringPool* pool = 0;
    // only used if pool != 0
    size_t startInPool = 0;
public:

    const char* c_str() const;
    bool empty() { return !pool; }

    size_t size() const { return pool ? strlen(c_str()) : 0; }

    // useful other than for testing?
//    bool operator==(PooledString& rhs) const { return pool == rhs.pool && startInPool == rhs.startInPool; }
//    bool operator!=(PooledString& rhs) const { return !(*this == rhs); }

    friend StringPool;
};

// ever growing, single large buffer, could be made a list or ringbuffer of StringPool objects
class StringPool
{
    // 0 terminated
    std::vector<char> mem;
    struct MergeContext
    {
        size_t sizeBeforeMerge = 0;
    };

public:
    StringPool();
    ~StringPool();

    // @param in 0 means empty string
    PooledString push(const char* in);
    // @param 0 termination not expected
    PooledString push(const char* in, size_t inSize);

    // call once to merg in another pool
    MergeContext merge(const StringPool& in);
    // then call this once per entry to remap
    PooledString mergeIn(const MergeContext mergeContext, const PooledString& in);
    //
    void clear();

    friend PooledString;
};

bool parseLine(const Char*& p, StringPool& pool, PooledString& Out, const Char extraEndCharacter);

void PooledStringUnitTest();
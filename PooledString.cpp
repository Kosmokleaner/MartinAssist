#include "PooledString.h"
#include <assert.h>

const char* PooledString::c_str() const
{
    return pool ? &pool->mem[startInPool] : ""; 
}

StringPool::StringPool()
{
    // to avoid reallocations
    mem.reserve(10 * 1024 * 1024);
}

StringPool::~StringPool()
{
//    __debugbreak();
}

PooledString StringPool::push(const char* in)
{
    PooledString ret;

    if(in && *in)
    {
        const size_t addSize = strlen(in) + 1;

        ret.pool = this;
        ret.startInPool = mem.size();

        mem.resize(ret.startInPool + addSize);

        memcpy(&mem[ret.startInPool], in, addSize);
    }

    return ret;
}

void PooledStringUnitTest()
{
    static StringPool pool;

    PooledString a0;
    PooledString a1 = pool.push(nullptr);
    PooledString a2 = pool.push("");

    assert(a0.empty());
    assert(a1.empty());
    assert(a2.empty());
    assert(strcmp(a0.c_str(), "") == 0);
    assert(strcmp(a1.c_str(), "") == 0);
    assert(strcmp(a2.c_str(), "") == 0);

    PooledString b0 = pool.push("Hello");
    PooledString b1 = pool.push("This");
    PooledString b2 = pool.push("Is");
    PooledString b3 = pool.push("A");
    PooledString b4 = pool.push("Test");

    PooledString a3 = pool.push("test");
    assert(strcmp(a3.c_str(), "test") == 0);
    assert(!a3.empty());
    
    assert(strcmp(b0.c_str(), "Hello") == 0);
    assert(strcmp(b1.c_str(), "This") == 0);
    assert(strcmp(b2.c_str(), "Is") == 0);
    assert(strcmp(b3.c_str(), "A") == 0);
    assert(strcmp(b4.c_str(), "Test") == 0);
}

StringPool::MergeContext StringPool::merge(const StringPool& in)
{
    MergeContext ret;

    ret.sizeBeforeMerge = mem.size();

    const size_t oldSize = mem.size();
    const size_t inSize = in.mem.size();
    mem.resize(mem.size() + inSize);
    memcpy(&mem[oldSize], in.mem.data(), inSize);

    return ret;
}

PooledString StringPool::mergeIn(const StringPool::MergeContext mergeContext, const PooledString& in)
{
    PooledString ret;
        
    ret.pool = this;
    ret.startInPool = mergeContext.sizeBeforeMerge + in.startInPool;

    return ret;
}

PooledString StringPool::push(const char* in, size_t inSize)
{
    PooledString ret;

    if (in && *in)
    {
        const size_t addSize = inSize + 1;

        ret.pool = this;
        ret.startInPool = mem.size();

        mem.resize(ret.startInPool + addSize);

        memcpy(&mem[ret.startInPool], in, inSize);
        // 0 termination is not expected from in
        mem[ret.startInPool + inSize] = 0;
    }

    return ret;
}

bool parseLine(const Char*& p, StringPool& pool, PooledString& Out, const Char extraEndCharacter)
{
    const Char* start = p;
    const Char* end = p;

    // can be optimized, does a lot of resize
    while (*p)
    {
        if (*p == extraEndCharacter)
        {
            end = p++;
            break;
        }

        if (*p == 13)		// CR
        {
            end = p++;

            if (*p == 10)	// CR+LF
                ++p;

            break;
        }
        if (*p == 10)		// LF
        {
            end = p++;
            break;
        }

        p++;
    }

    size_t size = end - start;

    // 1083ms in DEBUG
    Out = pool.push((const char*)start, size);

    // 4406ms in DEBUG
//    Out.resize(size);
//    memcpy((void*)Out.data(), (const char*)start, size);

    return size;
}

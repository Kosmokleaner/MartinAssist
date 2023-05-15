#pragma once
#include "types.h"
#include <algorithm>


class SelectionRange
{
private:
    // index
    int64 first = -1;
    // index
    int64 second = -1;
    // indices
    std::vector<int64> exceptions;

public:
    bool empty() const {
        return count() == 0;
/*
        // slower method, todo: move to unit test
        const bool ret = !count();

        if (first == second && exceptions.empty())
        {
            assert(!ret);
            return false;
        }

        assert(ret);
        return true;
*/    }

    size_t count() const
    {
        size_t ret = 0;

        if(first >=0 && second >= 0)
            ret = std::max(first, second) - std::min(first, second) + 1;

        for(auto& el : exceptions)
        {
            if(isRangeSelected(el))
                --ret;
            else
                ++ret;
        }

        return ret;
    }

    // T is function(int64 x)
    template <class T>
    void foreach(T fn) 
    {
        if (first >= 0 && second >= 0)
        {
            int64 start = std::min(first, second);
            int64 stop = std::max(first, second);

            for(int64 x = start; x <= stop; ++x)
                if (isSelected(x))
                    fn(x);
        }

        for (auto& x : exceptions)
        {
            if (!isRangeSelected(x))
                fn(x);
        }
    }

    void reset()
    {
        first = -1;
        second = -1;
        exceptions.clear();
    }

    void onClick(int64 x, bool shift, bool ctrl)
    {
        if (ctrl)
            toggle(x);
        else
        {
            if (first == second && first == x)
            {
                reset();
                return;
            }

            exceptions.clear();

            if (shift)
                second = x;
            else
                first = second = x;
        }
    }

    bool isSelected(int64 x) const
    {
        bool ret = isRangeSelected(x);

        ret ^= std::find(exceptions.begin(), exceptions.end(), x) != exceptions.end();

        return ret;
    }

    void toggle(int64 x)
    {
        auto it = std::find(exceptions.begin(), exceptions.end(), x);

        if (it == exceptions.end())
            exceptions.push_back(x);
        else
            exceptions.erase(it);
    }

private:
    bool isRangeSelected(int64 x) const
    {
        bool ret = false;

        // range selection
        if (first == x)
            ret = true;
        if (second != -1)
            ret = std::min(first, second) <= x && x <= std::max(first, second);

        return ret;
    }
};


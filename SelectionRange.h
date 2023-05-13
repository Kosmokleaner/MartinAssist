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
        if (first == second && exceptions.empty())
            return false;

        return true;
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
            exceptions.clear();

            if (shift)
                second = x;
            else
                first = second = x;
        }
    }

    bool isSelected(int64 x) const
    {
        bool ret = false;

        // range selection
        if (first == x)
            ret = true;
        if (second != -1)
            ret = std::min(first, second) <= x && x <= std::max(first, second);

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
};


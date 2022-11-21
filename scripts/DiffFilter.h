#pragma once
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// #include "xLog.h"

namespace SignalsProcessing
{
    template<int N>
    struct DiffFilter
    {
        static constexpr int32_t Size{N};
        int32_t buf[N];
        int point{0};
        bool flg{true};

        DiffFilter()
        {
            // xLog() << MY_FUNC;
            Reset();
        }

        inline void Reset()
        {
            flg = true;
        }

        inline void Set(int32_t x)
        {
            for(int i = 0; i < N; i++) buf[i] = x;
            flg = false;
        }
        //-----------------------------------------------------------------------------
        template<bool PHF>
        inline int32_t Exe(int32_t x)
        {
            if(flg) Set(x);
            int32_t last = buf[point];
            buf[point] = x;
            point++; point %= N;
            if (PHF) return last - x; else return x - last;
        }
    };
}
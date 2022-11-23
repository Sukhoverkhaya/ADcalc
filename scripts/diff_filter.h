struct DiffFilter{ // Дифференциальный фильтр из вебдивайса, с изменениями по примеру Виктора из микробокса
    
    int32_t N;
    int point{0};
    bool flg{true};
    int32_t*buf = nullptr;

    DiffFilter(size_t _delta = 2)
    : N(_delta)
    {
        buf = new int32_t[N];
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
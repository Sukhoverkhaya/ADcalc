#pragma once

struct DiffFilter{ // Дифференциальный фильтр из вебдивайса, с изменениями по примеру Виктора из микробокса
    
    int32_t N;               // длина буфера (в элементах)
    int point{0};            // указатель на переписываемую ячейку буфера
    bool flg{true};          // флаг разрешения записи в буфер
    int32_t* buf = nullptr;  // указатель на первый элемент буфера
    bool PHF;                // полярность?? сигнала
    int delay;               // задержка фильтра

    DiffFilter(size_t _delta = 2, bool _phf = true)
    : N(_delta), PHF(_phf)
    {
        buf = new int32_t[N];
        delay = N/2;
        flg = true;
    }

    inline void Set(int32_t x)
    {
        for(int i = 0; i < N; i++) buf[i] = x;
        flg = false;
    }
    //-----------------------------------------------------------------------------
    inline int32_t Exe(int32_t x)
    {
        if(flg) Set(x);
        int32_t last = buf[point];
        buf[point] = x;
        point++; point %= N;
        if (PHF) return last - x; else return x - last;
    }

};
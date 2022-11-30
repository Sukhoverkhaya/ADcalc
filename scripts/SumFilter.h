#pragma once // Интегратор из вебдивайса (немного переделан)

struct SumFilter{ // Интегратор из вебдивайса (немного переделан)

    int32_t N;               // длина буфера (в элементах)
    int point{0};            // указатель на переписываемую ячейку буфера
    bool flg{true};          // флаг разрешения записи в буфер
    int32_t* buf = nullptr;  // указатель на первый элемент буфера
    int32_t rgS;
    int delay;               // задержка фильтра

    SumFilter(size_t _delta = 2)
    : N(_delta)
    {
        buf = new int32_t[N];
        delay = N/2;
        flg = true;
        point = 0;
        rgS = 0;
    }

    inline void Set(int32_t x)
    {
        for(int i = 0; i < N; i++) buf[i] = x;
        rgS = x * N;
        flg = false;
    }

    inline int32_t Exe(int32_t x)
    {
        if(flg) Set(x);
        int32_t last = buf[point];
        buf[point] = x;
        point++; point %= N;

        rgS -= last;
        rgS += x;

        return rgS / N;
    }
};
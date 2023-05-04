#pragma once
#include <stdint.h>

enum Modes
{
    WAIT = 0,
    INFL,
    DEFL,
};

#define ISVALID(press, tone) ((press > PrsSet(15) && tone > -1e7) ? true : false)

struct WorkMode
{
    Modes mode;

    int32_t PressMax;
    int32_t timer;
    int32_t Fs;

    bool inflbeg;
    bool deflbeg;
    bool waitbeg;

    WorkMode(int32_t _fs) : Fs(_fs) { mode = WAIT; Reset();};

    inline void Reset()
    {
        PressMax = 0;
        timer = 0;
        inflbeg = false;
        deflbeg = false;
        waitbeg = false;
    };

    inline void ModeControl(int32_t press, int32_t tone) // вызывается для каждой входящей точки сигнала, а не только для событий тонов или пульсаций
    {

        inflbeg = false;
        deflbeg = false;
        waitbeg = false;

        if (mode == INFL){   // поиск максимума треугольника давления с момента начала накачки
            if (press >= PressMax)
            {
                PressMax = press;
                timer = 0;
            }
            else 
            {
                timer++;
            }
        };

        bool isval = ISVALID(press, tone);

        if (isval && mode == WAIT) // переключение с ожидания на накачку
        {
            mode = INFL; 
            inflbeg = true;
        }
        else if (!isval && mode == DEFL) // переключение со спуска на ожидание
        {
            mode = WAIT;
            waitbeg = true;
        }
        else if (isval && mode == INFL && timer > 2*Fs) // переключение с накачки на спуск (если с последнего найденногого максимума давления прошло больше 2 секунд)
        {
            mode = DEFL; 
            deflbeg = true;
        }; 
    };
};

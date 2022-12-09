// на основании матлабовского кода Виктора (функция estimate_AD_tone_v2)
#pragma once

namespace SKV
{
    struct ESTIMATE_AD_TONE
    {

        int32_t Cnt; // счетчик обработанных событий

        int32_t kPres;
        int32_t fs;

        // state
        int32_t maxLvl;
        bool fail;
        int32_t minPresBad;
        int32_t minPsys;
        int32_t Pmin; // минимальное давление начала анализа
        int32_t wait; // 3 сек отсутствия пульсаций 

        // критерии браковки измерения
        int32_t BadTime;
        int32_t MaxBadTime;
        #define at(i) ((i-1) % 3) + 1;
        int32_t* buf;

        int32_t Nb;
        int32_t imax;
        int32_t Nlow;

        int32_t ilast; // индекс последнего значимого и НЕ бракованного пика
        int32_t i3s;   // индекс последнего значимого пика

        int32_t i1; int32_t a1; // два предыдущих хороших пика
        int32_t i2; int32_t a2;

        int32_t step;
        #define StartCondition(pres) pres > Pmin

        bool mode; // true - накачка, false - спуск (задается изве на входе statemachine)

        // peak attributes
        int32_t tonepos;
        int32_t toneamp;
        bool bad;
        int32_t presamp;

        inline void Init(int32_t _kpres, int32_t _fs) // вызывается после создания общекта типа ESTIMATE_AD_TONE для заполнения полей (начало цикла определения границ АД)
        {
            // state
            Cnt = 0;
            kPres = _kpres;
            fs = fs;
            maxLvl = 0;
            fail = true;
            minPresBad = 70*kPres;
            minPsys = 120*kPres;
            Pmin = 40*kPres; 
            wait = 3; 
            buf = new int32_t[0,0,0];
            Nb = 0;
            maxLvl = 0; 
            imax = 1;
            Nlow = 0;
            ilast = 0; 
            i3s = 0; 
            i1 = -1;
            i2 = -1;
            step = 0;
        };

        inline void SetMode(bool _mode) // задание режима: true - накачка, false - спуск
        {
            mode = _mode;
        };

        // inline void SetPoint(int32_t _tonepos, int32_t _toneamp, bool _bad, int32_t _presamp) // обновляем значения, которые подаём для каждой точки (в т.ч. чтобы хранить предыдущую)
        // {
        //     tonepos =  _tonepos;
        //     toneamp = _toneamp;
        //     bad = _bad;
        //     presamp = _presamp;
        // };

        inline void ADStateMachine(int32_t _tonepos, int32_t _toneamp, bool _bad, int32_t _presamp)
        {

            if (step == 0) // первая хорошая пульсация
            {
                if (StartCondition(presamp)) && !bad 
                {
                    Nb = 1;
                    step ++;
                    // i1 = Cnt; 
                    i1 = _tonepos; // сюда пишется не индекс позиции, а сама позиция
                }
            }
            else if (step == 1) // начинаем копить хорошие пульсации
            {
                if (bad) {contue}; // если пришла плохая - ничего не делаем
                buf[at(Cnt)] = toneamp;
                Nb++;
                if(Nb > 1 && (tonepos - i1) > wait*fs){Nb = 1}; // 2 пульсации - 3 секунды

                if (Nb == 3) // есть 3 пульсации - продолжаем
                {
                    maxLvl = (_toneamp)/2
                }
            }
            else if (step == 2) 
            {

            }

            // SetPoint(_tonepos, _toneamp, _bad, _presamp)
            // Cnt++; // пока не исп
        };
        
    };
}
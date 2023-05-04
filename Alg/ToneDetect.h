#pragma once
#include <cstring>
#include "arithm.h"

// namespace SignalsProcessing
// {
    struct Tone  // событие: тон 
    {
        int32_t env;              // амплитуда огибающей
        int32_t  val;            // амплитуда
        int32_t  press;          // давление
        int32_t pos;
    };

    struct ToneEvent : Tone
    {
        int32_t bad; 
        int32_t startMarkPos;

        void Reset()
        {
            env = 0;
            val = 0;
            press = 0;
            bad = 0;
            pos = 0;
            startMarkPos = 0;
        }
    };

    struct SmartRoundBuff /// общий большой буфер
    {
        const int32_t rail;
        Tone* buf;
        int32_t forward;   //rd
        int32_t backward;  //wr

        SmartRoundBuff(int32_t size)
        :  rail(size)
            ,buf( new Tone[size] )
            ,forward(0)
            ,backward(0)
        { }
            
    };	

    struct RBIterator /// работает внутри общего большого буфера
    {
        SmartRoundBuff& buf;
	    int32_t it;

        RBIterator(SmartRoundBuff& _buf, int32_t  _it = 0)
        :	 buf(_buf)
            ,it(_it)
        { }
        
        RBIterator(const RBIterator& rhs)
        : buf(rhs.buf), it(rhs.it)
        { }
        
        void operator--()
        {
            --it;
            if( it < 0 ) it = buf.rail + it; // NOTE(romanm): в теории нужно it %= rail;
        }
        
        void operator++()
        {
            ++it %= buf.rail;
        }
        
        // void Set(int32_t  env, int32_t  tone, int32_t  press)
        void Set(int32_t  env, int32_t  tone, int32_t  press, int32_t pos)
        {
            buf.buf[it].env   = env;
            buf.buf[it].val  = tone;
            buf.buf[it].press = press;
            buf.buf[it].pos = pos;
        }
        
        Tone Get()
        {
            return buf.buf[it];
        }

        bool operator!=(const RBIterator& rhs)
        {
            return it != rhs.it;
        }	
        
        void operator=(const RBIterator& rhs)
        {
            it = rhs.it;
        }
        
        void operator=(int32_t  _it)
        {
            if( _it < 0) it = buf.rail + _it; //NOTE(romanm): в теории нужно it %= rail;
            else         it = _it % buf.rail; 
        }	
    };

    struct ToneDetect : Tone
    {

        ToneDetect(int32_t _fs) 
        :   Fs(_fs), minDist(0.2*_fs), 
            Wmax(0.1*Fs),
            noiselen(0.06*Fs),
            noiseOffset1(0.04*Fs),
            noiseOffset2(0.04*Fs),
            buf(minDist*2+1), 
            current(buf),
            candidPeakPos(buf),
            peakPos(buf)
        { 
            Reset();
        };

        inline void Reset()
        {
            minR = 90;
            LvN = 100;
            err = 0;
            firstflag = false;

            val = 0;
            press = 0;
            cnt = 0;

            W = 0;
        };

        // Детектор
        int32_t Fs;
        int32_t minDist;

        int32_t LvP = ~0; // -Inf
        int32_t mxCnt = 0;

        int32_t minR;
        int32_t LvR; // уровень слежения - разряда
        int32_t kR = 20; // /4

        int32_t LvN;
        int32_t kN = 100; // /4
        int32_t kaN = 2.5*10; // 2.5

        int32_t err;

        int32_t rCnt; // реальный отсчет, на котором был найден предполагаемый пик


        bool firstflag = false; // true сразу после первой итерации метода Exe
        bool peakflag; // true на той итерации, где подтверждён предыдущий пик (с задержкой в mxCnt)

        // Буфер
		SmartRoundBuff buf; // большой общий буфер для области до и после предполагаемого пика
		RBIterator current;
		RBIterator candidPeakPos;
		RBIterator peakPos;


        inline void detect() // детектор
        {
            int32_t tone = current.Get().env;

            if (!firstflag) {LvR = tone; firstflag = true;} // инициализация LvR первым пришедшим тоном

            peakflag = false;

            err = tone - LvN;
            LvN += err/kN;

            if (tone > LvR)   // поиск максимума
            {
                LvR = tone;
                if (LvP < LvR)
                {
                    candidPeakPos = current; // позиция предполагаемого пика в кольцевом буфере
                    rCnt = cnt; // пишем реальную позицию предполагаемого пика
                    LvP = LvR;
                    mxCnt = 0;
                }
            }
            else            // медленный разряд
            {
                LvR -= LvR/kR;
                if (LvR < minR) {LvR = minR;}
            };

            if (mxCnt == minDist)
            {
                if (LvP > LvN * (kaN/10)) // если пик выше по амплитуде, чем шум, и была детекция вниз
                {
                    peakPos = candidPeakPos; // позиция в кольцевом буфере, в которой находимся при подтвержднии предполагаемого пика
                    peakflag = true;
                }

                LvP = 0;
            }

            mxCnt++;
        };

        int32_t Wmax;           // макс полуширина не более
        int32_t noiselen;       // длина участка поиска уровня шума
        int32_t noiseOffset1;   // отступ от границы ДО
        int32_t noiseOffset2;   // отступ от границы ПОСЛЕ

        int32_t Acoef = 3;
        int32_t Lvl = 0;
	
        //
        int32_t W;   // Ширина
        int32_t snr; //Signal\noise ratio

        inline void param() // параметризатор (вызывается, когда peakflag = true;)
        {
            Lvl = (peakPos.Get().env)/Acoef;
            // поиск ширины
            // ДО пика
            int32_t wBefore = 0;
            RBIterator it = peakPos; // присваиваем итератору значение позиции текущего пика в кольцевом буфере
            for(int32_t i = 0; i < Wmax; ++i, --it) // и идем по буферу (т.е. во времени) в обратном направлении
            {	
                if( it.Get().env < Lvl )
                {
                    wBefore = i;
                    break;
                }
            }
            
            // После
            int32_t wAfter = 0;
            it = peakPos;
            for(int32_t i = 0; i < Wmax; ++i, ++it)
            {		
                if( it.Get().env < Lvl )
                {
                    wAfter = i;
                    break;
                }
            }
            
            W = wAfter + wBefore; // ширина

            // поиск уровня шума
        
            //назад
            RBIterator ibeg(buf);
            RBIterator iend(buf);
            ibeg = peakPos.it - wBefore - 1 - noiseOffset1;
            iend = ibeg.it - noiselen;
            
            int32_t MaxBefore = ibeg.Get().env;
            while( ibeg != iend )
            {
                --ibeg;
                if( ibeg.Get().env > MaxBefore ) {MaxBefore = ibeg.Get().env;}
            }
            
            //вперед
            ibeg = peakPos.it + wAfter + 1 + noiseOffset2;
            iend = ibeg.it + noiselen;
            
            int32_t MaxAfter = ibeg.Get().env;
            while( ibeg != iend )
            {
                ++ibeg;
                if( ibeg.Get().env > MaxAfter ) MaxAfter = ibeg.Get().env;
            }
            
            int32_t noise = fnMax(MaxBefore, MaxAfter);
            if (noise == 0) noise = 1;
            snr = (peakPos.Get().env*10 / noise);
        };

        ToneEvent toneEv;

        inline void discard() // отбраковщик
        {
            int32_t bad = 0;

            if (peakPos.Get().press < 30000) {bad += pow2(1);}
            if (snr < 25) {bad += pow2(2);} 
            if (peakPos.Get().env < 300/20) {bad += pow2(3);}

            toneEv.bad = bad;
            toneEv.val = peakPos.Get().env;
            toneEv.pos = rCnt;
            toneEv.press = peakPos.Get().press;
            toneEv.startMarkPos = startMarkPos;
        };

        int32_t cnt; // Реальный текущий отсчет
        int32_t startMarkPos; // Позиция в точках относительно метки старта (старта чего: текущего измерения, накачки/спуска, etc.?)

        inline void Exe(int32_t env, int32_t tone, int32_t press)
        {
            current.Set(env, tone, press, cnt); // добавление нового события в буфер

            detect(); // детекция
            if (peakflag) // если детектирован тон
            {
                param(); // параметризация
                discard(); // отбраковщик
            };

            ++cnt; // инкрементация счетчика точек на входе
            ++current;
            ++startMarkPos;
        };
    };
// }

#pragma once
#include <cstring>

// namespace SignalsProcessing
// {
    struct Tone  // событие: тон 
    {
    public: 

        // int32_t  pos;            // позиция
        // int32_t env; // по идее - аплитуда отфильтрованного сигнала, которая идет в алгоритм детекции, но пока тоны не фильтруем и считаем по val
        int32_t  val;            // амплитуда
        int32_t  press;          // давление
        // int32_t  startMarkPos;   // позиция маркера начала (исп. для САД и ДАД как минимум)

        //                          // для параметризации
        // int32_t ins1;
        // int32_t ins2;
        // int32_t ins3;
        // int32_t ins4;
        // int32_t i1;
        // int32_t i2;
        // int32_t W;
        // int32_t noise;
        // int32_t snr;

        // bool bad;                 // вердикт браковщика (true - плохая, false - хорошая)

        // virtual void Reset()
        // {
        //     bad = false;
        //     pos = 0;
        //     val = 0;
        //     press = 0;
        //     startMarkPos = 0;
        // }
    };

    struct SmartRoundBuff /// общий большой буфер
    {
        const int rail;
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

        RBIterator(SmartRoundBuff& _buf, s32 _it = 0)
        :	 buf(_buf)
            ,it(_it)
        { }
        
        RBIterator(const RBIterator& rhs)
        : buf(rhs.buf)
        {
            this->it = rhs.it;
        }
        
        void operator--()
        {
            --it;
            if( it < 0 ) it = buf.rail + it; // NOTE(romanm): в теории нужно it %= rail;
        }
        
        void operator++()
        {
            ++it %= buf.rail;
        }
        
        // void Set(s32 env, s32 tone, s32 press)
        void Set(s32 tone, s32 press)
        {
            // buf.buf[it].env   = env;
            buf.buf[it].val  = tone;
            buf.buf[it].press = press;
        }
        
        s32& Get()
        {
            return buf.buf[it]
        }
        
        void operator=(const RBIterator& rhs)
        {
            this->it = rhs.it;
        }
        
        void operator=(s32 _it)
        {
            if( _it < 0) it = buf.rail + _it; //NOTE(romanm): в теории нужно it %= rail;
            else         it = _it % buf.rail; 
        }	
    };

    struct ToneDetect : Tone
    {
        inline ToneDetect(int32_t _fs) 
        :   Fs(_fs), minDist(0.2*_fs), 
            Wmax(0.1*Fs),
            noiselen(0.08*Fs),
            noiseOffset1(0.06*Fs),
            noiseOffset2(0.06*Fs),
            buf(Wmax*2 + noiseOffset1 + noiseOffset2 + 3), // by skv: было mindist*2
            current(buf),
            candidPeakPos(buf),
            peakPos(buf)
        { 
            Reset()
        }

        inline Reset()
        {
            minR = 90;
            LvN = 100;
            err = 0;
            firstflag = false;

            val = 0;
            press = 0;
            cnt = 0;
        }

        // Детектор
        int32_t Fs;
        int32_t minDist;

        int32_t LvP = -10000000; // -Inf
        int32_t mxCnt = 0;

        int32_t minR;
        int32_t LvR; // уровень слежения - разряда
        int32_t kR = 20*4; // /4

        int32_t LvN;
        int32_t kN = 100*4; // /4
        int32_t kaN = 3; // 2.5

        int32_t err;

        int32_t rCnt; // реальный отсчет, на котором был найден предполагаемый пик
        int32_t cnt; // Реальный текущий отсчет


        bool firstflag = false; // true сразу после первой итерации метода Exe
        bool peakflag; // true на той итерации, где подтверждён предыдущий пик (с задержкой в mxCnt)

        // Буфер
		SmartRoundBuff buf; // большой общий буфер для области до и после предполагаемого пика
		RBIterator current;
		RBIterator candidPeakPos;
		RBIterator peakPos;

        inline detect(int32_t tone, int32_t press) // детектор
        {
            if (!firstflag) {LvR = tone; firstflag = true;} // инициализация LvR первым пришедшим тоном

            current.Set(tone, press); // добавление нового события в буфер

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
                if (LvP > LvN * kaN) // если пик выше по амплитуде, чем шум, и была детекция вниз
                {
                    peakPos = candidPeakPos; // позиция в кольцевом буфере, в которой находимся при подтвержднии предполагаемого пика
                    peakflag = true;
                }

                LvP = 0;
            }

            mxCnt++;
            cnt++;
        }

        int32_t Wmax;           // макс полуширина не более
        int32_t noiselen;       // длина участка поиска уровня шума
        int32_t noiseOffset1;   // отступ от границы ДО
        int32_t noiseOffset2;   // отступ от границы ПОСЛЕ

        int32_t Acoef = 3;
        int32_t Lvl = 0;
	
        //
        int32_t Wbefore;  
        int32_t Wafter;  
        int32_t W;   // Ширина
        int32_t snr; //Signal\noise ratio
        int32_t startMarkPos;       //Позиция в точках относительно метки старта 

        RBIterator it; // итератор для хождения вперёд-назад относительно пика

        inline param() // параметризатор (вызывается, когда peakflag = true;)
        {
            Lvl = peakPos.Get().val/Acoef;

            // поиск ширины
            // ДО пика
            it = peakPos; // присваиваем итератору значение позиции текущего пика в кольцевом буфере
            for( int32_t i = 0; i < Wmax; ++i, --it) // и идем по буферу (т.е. во времени) в обратном направлении
            {	
                if( it.Get().val < Lvl )
                {
                    wBefore = i;
                    break;
                }
            }
            
            // После
            it = peakPos;
            for(int32_t i = 0; i < Wmax; ++i, ++it)
            {		
                if( it.Get()val. < Lvl )
                {
                    wAfter = i;
                    break;
                }
            }
            
            W = wAfter + wBefore; // ширина

        }

        inline Exe(int32_t tone)
        {
            detect(tone) // детекция
        }
    };
// }
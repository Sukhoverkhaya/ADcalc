#pragma once
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// #include "xLog.h"
#include <cstring>

//#include "Soft/SignalProcessing/Alg/DiffFilter.h"
//#include "Soft/SignalProcessing/Alg/SumFilter.h"
//#include "Soft/SignalProcessing/Alg/AlgorithmEvent.h"

namespace SignalsProcessing
{
    struct ToneOnlyDetect // детектор тонов без привязки к QRS
    {
//        SumFilter<20> mFltSum{};
//        DiffFilter<20> mDiffSum{};

//        AlgorithmEvent&     mQrsEvent;

        inline ToneOnlyDetect()
//        : mQrsEvent{qrs}
        {
            // xLog() << MY_FUNC;
        }
        /*
        #define _kTonSN     3 // коэффициент отношения сигнал/шум
        #define _tTonRefr	300 // ms интервал окресности тона
        #define _tTonHigh	 80 // ms интервал тона
        //_TonOnly TonParse; // пераметры тона Короткова
        int PeakTime;  // счетчик точек между пиками,интервалами поиска
        //int TimeNoDtc;  // интервал наличия тона
        int LvR;        // уровень сигнала
        int LvN;        // уровень шума
        int LvP;        // порог для детекции
        //int NoiseMark;    // метка зашумленного интервала
        */

        int mDetectInterval;  // счетчик точек между пиками,интервалами поиска

        int32_t cntPeak;
        int32_t divNoise;

        int32_t LvR;
//        int32_t LvR2;
        int32_t LvP;
        int32_t LvN;
        int32_t LvN1;
        int32_t LvN2;
        int32_t LvD;
        int32_t LvZ;

        int32_t Tone;
//        int32_t rgFlt;
//        int32_t rgNoise;
//        int32_t Reg;

        static constexpr int32_t sizeNoise{10};
        int32_t mNoise[sizeNoise]; // = {0,0,0,0}; // 25*10 = 250ms
        int32_t posNoise;

        //int TonDtcTimeSar[4]; // буфер усреднения TT интервалов
        //int TonDtcMaxAmpl[3];	// таблица максимумов амплитуд
        const int32_t cAmplTonLow{30 * 256}; // минимальная амплитуда тона


        //=============================================================================
        // Детектор без привязки к детектору QRS
        //-----------------------------------------------------------------------------
//      void TonDtcStart(void) // Начало цикла детекции периода сигнала
//        {
//            LvP = cAmplTonLow / _kTonSN;  // порог для детекции
//            LvN = LvP;                          // реальный уровень шума
//
//            TonDtcRestart(); // Начало цикла детекции периода сигнала
//        }

        inline void Start() // Начало цикла детекции периода сигнала
        {
            mDetectInterval = 0; // счетчик точек между пиками,интервалами поиска

            cntPeak = 0;
            divNoise = 0;
            posNoise = 0;

            LvR = 0;
            //LvR2 = 0;
            LvP = 0;
            LvN = 500;
            LvN1 = 500; //???
            LvN2 = 500;
            //LvN = 500;
            LvD = 1000; //???
            LvZ = 1000; //???

            memset( mNoise, 0, sizeof(mNoise) );
        }

        inline void Stop()
        {
        }
        //-------------------------------------
        inline int32_t Exe(int32_t tone, int32_t fs) // Детектор без привязкой к детектору QRS
        {
            int32_t res{0};

            Tone = tone;
//            rgFlt =  mFltSum.Exe(Tone);

            //  --- обработка пиков ---
            mDetectInterval++;  // счетчик точек между пиками,интервалами поиска
            cntPeak++;  // счет времени между пиками
//            if ( cntPeak > 2000) cntPeak = 2000;
            if ( Tone > LvR ) // следим за сигналом
            { // пик зафиксирован
                LvR = Tone;
//                if ( LvR > 10000) LvR = 10000;    // 4000 mkv
//                if ( LvR <   100) LvR =   100;    // 500 mkV

                if ( LvR > LvP )    // если он больше,чем максимально-зафиксированный
                {
                    LvP = LvR;      // запоминаем как текущий максимальный
                    cntPeak = 0;    // обнулим счетчик времени между локальными максимумами

                    // взять шум до пика -50ms по макс, на интервале 100ms (-6 интервалов с оценкой на 4х)
                    int32_t pos = posNoise - 5; if(pos < 0) pos += sizeNoise;
                    LvN1 = 0;
                    for ( int i = 1; i < 4; i++ )
                    {
                        int32_t z = mNoise[pos];
                        pos++;
                        pos %= sizeNoise;
                        if(LvN1 < z) LvN1 = z;
                    }
                }

            }
            else
            { // нет пика
                int32_t err = Tone - LvR;       // ошибка сигнал-уровень слежения
                LvR += err * 100 / 1000;        // разряд t=0.1cek  вынуждены зафильтровать

//                if ( LvR > 10000) LvR = 10000;    // 4000 mkv
//                if ( LvR <   100) LvR =   100;    // 500 mkV
            }

//            int32_t err = Tone - LvR2;       // ошибка сигнал-уровень слежения
//            LvR2 += err * 5 / 1000;        // разряд t=0.2cek  вынуждены зафильтровать

            // --- обработка шума ---
            //cdL--;                  // делитель шумовых периодов
            divNoise %= 25;
            if ( divNoise++ == 0)           // // 50ms
            {
                posNoise++;                  // переход на след. рег. кольцевого буфера
                posNoise %= sizeNoise;    // в буфере 4 элементов

                mNoise[posNoise] = LvR;         // захват уровня регистра слежения

//                Reg = mNoise[0];
//                for ( int i = 1; i < int(sizeof(mNoise)/sizeof(int32_t)); i++ ) if(Reg > mNoise[i]) Reg = mNoise[i];
            }
            if ( mNoise[posNoise] > LvR ) mNoise[posNoise] = LvR; // выбор наименьшего в интервале 12*8= 96ms
            // --- анализ локального максимума на QRS ---

            if ( cntPeak >= 0.25*fs )                 // от последнего пика прошло 250ms(10
            {

                int32_t err = LvP - LvD;                // вычисление усредненного пикового уровня
                if ( err>=0 )   LvD += err * 100 /1000;     //??? вверх - в четыре раза быстрее 0.20
                    else        LvD += err * 100 /1000;     //  чем вниз 0.05 t=0.2cek
                if ( LvD > 10000) LvD = 10000;    // 4000 mkv
                if ( LvD <  500) LvD =  500;    // 500 mkV
                int32_t p_lvd = LvD / 3;

                // взять шум после пика +75ms по макс, на интервале 100ms ( -10+3 = -7 интервалов с оценкой на 4х)
                int32_t pos = posNoise - 8; if(pos < 0) pos += sizeNoise;
                LvN2 = 0;
                for ( int i = 1; i < 4; i++ )
                {
                    int32_t z = mNoise[pos];
                    pos++;
                    pos %= sizeNoise;
                    if(LvN2 < z) LvN2 = z;
                }

                LvN = LvN1 > LvN2 ? LvN1 : LvN2;

                LvZ = LvN * 5;
                if(LvZ < p_lvd) LvZ = p_lvd;
                //int32_t kf = (150 * 4 - 8 * xLog2(LvD) * 4);      // вычисление коэффициента влияния пикового уровня ( LvD <= 2000 -> xLog2(LvD) <= 11 )
//                LvZ = LvN * 5; // + ( (LvD - LvN) * kf / 1000);        // вычисление уровня захвата пики

                if ( LvP > LvZ )
                { // Детектор сработал
                    res = 1000;
                    // xLog(xCOLOR::GREEN) << "Interval:" << mDetectInterval << "; Peak:" << LvP << "; LvN1:" << LvN1 << "; LvN2:" << LvN2 << "; LvZ:" << LvZ;
                    mDetectInterval = 0;  // счетчик точек между пиками,интервалами поиска
                }

                LvP = 0; // ищем новый пик
            }

           /*
            if( ton > cAmplTonLow)
            {
                if( PeriodTime<=0 ) { TonParse.RegPrsBegin = TrPrsSignal.PrsMsr; }
                    else { TimeNoDtc += (_tTonRefr - PeriodTime); }
                PeriodTime	= _tTonRefr;
            }
            else
            {
                LvN += (ton - LvN) / 100;           // построение огибающей сигнала err=Prs-Lv1; Lv1=Lv1+k*err; это фильтр k = 0.1
                if ( LvN > LvP ) { NoiceMark = 1; } // метка зашумленного интервала
            }
            -----------------------------------
            if( PickTime > 0 )
            { // обратный отсчет
                TonParse.PickTime -= _PlsTick;
                if ( TonParse.PickTime<=0 )
                { // тон принят, анализ валидности
                    TonParse.rgRes = 1;
                    if(TonParse.TimeNoDtc >  _tTonHigh )	TonParse.rgRes = -1;
                    if(TonParse.NoiceMark	!=  0        )	TonParse.rgRes = -2;
                    TrTonExe.Prs = TonParse.RegPrsBegin;
                    TrTonExe.Dtc = TonParse.rgRes;	// флаг выделения пульсации
                    TonDtcRestart(); // Начало цикла детекции периода сигнала
                }
            }
            */
          //-----------------------------------
            return res;
        }
/*
        inline int32_t xLog2(int32_t x)
        {
            int32_t res = 0;
            while(x>0)
            {
                x = (x>>1);
                res++;
            }
            return res;
        }
*/
    };
}
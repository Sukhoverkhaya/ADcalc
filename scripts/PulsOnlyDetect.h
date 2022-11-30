#pragma once
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//XLOG НЕ РАБОТАЕТ ИЗ-ЗА МЬЮТА // #include "xLog.h"
//#include "Soft/SignalProcessing/Alg/AlgorithmEvent.h"

namespace SignalsProcessing
{
    struct PulsOnlyDetect // детектор пульсаций без привязки к QRS
    {
        PulsOnlyDetect()
        {
            //XLOG НЕ РАБОТАЕТ ИЗ-ЗА МЬЮТА // xLog() << MY_FUNC;
            Init();
            Stop();
        }

        int32_t mTch;
//        DiffFilter<40>  mDiff{};
        const int32_t Tick{1};

        int32_t mComp;          // регистр компоратора с гистерезисом
        int32_t ZeroLvTch;      // уровень нуля с учетом скорости спуска
        bool flgBreak;          // игнорирование первой детекции после рестарта или бракованной пульсации
        bool flgEnable;         // состояние алгоритма детекции вкл/выкл

        int32_t rgPrsZero{0};   // регистр значения давления в точке перехода через 0 тахоосциллограммы
        int32_t rgPrsFront{0};  // регистр значения давления в конце фронта
        int32_t rgPrsBegin{0};  // регистр значения давления в начале пульсации
        int32_t rgPrsEnd{0};    // регистр значения давления в конце  пульсации

        int32_t rgZeroTime{0};
        int32_t rgFrontTime{0};
        int32_t rgPeriodTime{0};

//        int32_t rgTchMax{0};    // максимальное значение тахоосцилллограммы - считается на фазе детекции фронта пульсации
//        int32_t rgTchMin{0};    // минимальное значение тахоосцилллограммы - считается на фазе детекции спада пульсации
        //------------------------
        int32_t mParseResult;   // код результата анализа валидности пульсации
        int32_t mPlsSpeed;      // скорость на интервале выделенной пульсации
        int32_t mPlsSpeedLast;  // скорость на предыдущем интервале
        int32_t mPlsSpeedDelta; // абсолютное значение изменения скорости на текущем интервале
        int32_t mPlsSpeedSet;   // установленная скорость изменения давления
        int32_t mPlsTimeRatio;  // скважность времен детектора
        int32_t mPlsAmplitude;  // амплитуда пульсации
        bool    flgPlsPremat;   // код преждевременности
        int32_t mPlsAmplLow;    // скорректированная минимальная амплитуда пульсации
        int32_t mErrTimeFull;   // счетчик временного интервала помехи за все время измерения
        int32_t mErrTimePeriod; // счетчик временного интервала помехи от последней хорошей пульсации

        int32_t mPlsDtcTimeSar[4];   // буфер усреднения PP интервалов
        int32_t mPlsDtcMaxAmpl[3];   // таблица максимумов амплитуд
        //-----------------------------------------------------------------------------
        void Init()
        {
            mPlsAmplLow = 450;  // 0.45mmHg скорректированная минимальная амплитуда пульсации
        }
        //-----------------------------------------------------------------------------
        inline void Start(int32_t prs, int32_t speed) // Начало цикла детекции периода сигнала
        {
            //XLOG НЕ РАБОТАЕТ ИЗ-ЗА МЬЮТА // xLog() << MY_FUNC;
//            mTchLvP = 40;
            mPlsSpeedSet = speed;
            ZeroLvTch = speed * 100 / 1000;

            mPlsAmplLow = 450;

            flgBreak = true;    // игнорирование первой детекции после рестарта или бракованной пульсации
            flgEnable = true;
            Repeat();

            rgPrsZero      = prs;  // регистр значения давления в точке перехода через 0 тахоосциллограммы
            mPlsSpeed      = 0;    // скорость на интервале выделенной пульсации
            mPlsSpeedLast  = 0;    // скорость на предыдущем интервале

            mErrTimeFull   = 0;    // счетчик временного интервала помехи за все время измерения
            mErrTimePeriod = 0;  // счетчик временного интервала помехи от последней хорошей пульсации

            memset( mPlsDtcMaxAmpl, 0, sizeof(mPlsDtcMaxAmpl));   // таблица максимумов амплитуд
            memset( mPlsDtcTimeSar, 0, sizeof(mPlsDtcTimeSar));   // буфер усреднения PP интервалов
        }

        inline void Stop()
        {
            flgEnable = false;
            mComp = 0;
        }

        inline void Repeat(void) // Начало цикла детекции периода сигнала
        {
            mComp = ZeroLvTch; // переключаем уровень компаратора

            rgPeriodTime = rgZeroTime;  // коррекция времени на переход через 0
            rgZeroTime = 0;             // время точки перехода через 0
            rgFrontTime = 0;            // время фронта пульсовой волны
//            rgTchMax = 0;               // максимум тахоосциллограммы
//            rgTchMin = 0;               // максимум тахоосциллограммы
        }
        //-----------------------------------------------------------------------------
        int32_t Exe(int32_t prs,int32_t tch) // Детектор пульсовой волны без привязки к QRS
        {
            int32_t res{0};
            if(!flgEnable) return res;

            mTch = tch; //mDiff.Exe<true>(prs);
            //-----------------------------------
            rgPeriodTime += Tick; // разметка временного интервала анализа в ms
            rgZeroTime   += Tick;
            //-----------------------------------
            //if (LvP < mTch) { LvP = mTch; } // уровень пика тахоосциллограммы
            //OksiPls.LvP -= OksiPls.LvP/200; // уровень пика тахоосциллограммы
            //if (OksiPls.LvP < _OksiPlsTchMin) { OksiPls.LvP = _OksiPlsTchMin; };
            //if (OksiPls.LvP > _OksiPlsTchMax) { OksiPls.LvP = _OksiPlsTchMax; };
            //-----------------------------------
            if(mComp == ZeroLvTch) // <_OksiPlsShift выбор типа полуволны компаратора
            {   // Lv<0 положительный полупериод // фронт пульсации

//                if(rgTchMax < mTch) rgTchMax = mTch;

                if(mTch < ZeroLvTch) // тест на смену полупериода
                {
                    // завершение фронта пульсации
                    // в этой точке завершается фронт и мы переключаем компаратор
                    //int32_t lv = mTchLvP / 2; // переключаем уровень компаратора
                    //if ( lv < 20 ) lv = 20;
                    mComp = ZeroLvTch; //
                    mComp += 5000 * 100 / 1000; //эмпирическая константа, зависит от используемых фильтров сигнала тахо

                    // регистр значения светопроводимости на вершине фронта
                    // точка максимального кровенаполнения сосудов
                    rgPrsFront = prs;
                    rgPrsBegin = rgPrsEnd;

                    rgFrontTime = rgPeriodTime; // фиксация времени фронта пульсовой волны
                }
            }
            else
            {   // Lv>=0 отрицательный полупериод // спад пульсации
                //???mCompLv = OksiPls.LvP / 2;

//                if(rgTchMin > mTch) rgTchMin = mTch;

                if(mTch > mComp) // тест на смену полупериода
                {
                    // Начало цикла детекции периода сигнала(фронт пульсовой волны)
                    // в этой точке начинается фронт и мы перекльчаем компаратор
                    //mComp = 0; // переключаем уровень компаратора

                    // регистр значения светопроводимости в начале пульсации
                    // точка обескровливания, минимального кровенаполнения сосудов
                    rgPrsEnd = rgPrsZero;

                    res = Pars();      // pacчет параметров пульсовой волны
                    if(res <= 3)
                    {
//                        int32_t d = (rgTchMax - mTchLvP) / 10;
//                        if(d > mTchLvP) d = mTchLvP;
//                        mTchLvP += d;
                            res += 1000; // отладка
//                        mSpoCount.Exe(rgRedBegin,rgRedFront, rgIrdBegin,rgIrdFront);    // pacчет R и Spo
//                        Pri1(res);       // результаты прямых измерений на сигнале
                        //XLOG НЕ РАБОТАЕТ ИЗ-ЗА МЬЮТА // xLog() << res << ' ' <<  mPlsSpeed << ';' << mPlsAmplitude << '-' << mPlsAmplLow << ';' << rgPrsBegin << '-' << rgPrsFront << ';' <<  rgPeriodTime << '-'<< rgFrontTime;
                        //xLog() << res << ' ' <<  rgPeriodTime << ' ' << rgFrontTime << ' ' << rgPrsBegin << ' ' << rgPrsFront << ' ' << rgPrsEnd;
                    }
                    else
                    {
//                        Pri2(res);
                        res = -(res + 1000); // отладка
                        //XLOG НЕ РАБОТАЕТ ИЗ-ЗА МЬЮТА // xLog() << res << ' ' << mPlsSpeedSet << ' ' <<  mPlsSpeed << ' ' << mPlsSpeedDelta;
                    }

                    Repeat();   // начинаем новый цикл детекции
                }

                // фиксируем состояние в последней точке перехода через 0 перед фронтом пульсации
                if(mTch < ZeroLvTch)
                {
                    // регистр значения светопроводимости в конце пульсации
                    rgPrsZero = prs;    // регистр значения давления в точке перехода через 0 тахоосциллограммы
                    rgZeroTime = 0;     // время точки перехода через 0
                }
            }

//???            if(rgPeriodTime > 5000) Restart(); // Новая настройка режимов детекции

            return res;
        }
        //-----------------------------------------------------------------------------
        //-----------------------------------------------------------------------------
        enum RegulatorSteps {stNorm = 0};

        int32_t Pars(void)  // pacчет параметров пульсовой волны
        {
            PlsSpeedCnt();      // Расчет скорости спуска
            PlsAmplCnt();       // Расчет амплитуды пульсации
//??            PlsTimeRatioCnt();  // Расчет отношения фаз пульсации
            PlsPrematCnt();     // Выделение преждевременности по пульсациям

            //if( PrsExe.QrsBindFlg ==0 )  // без привязки к QRS
            {
                if      ( flgPlsPremat )                        { mParseResult = 3; }  // преждевременная
                else if ( mPlsAmplitude < mPlsAmplLow )         { mParseResult = 2; }  // ниже порога чувствительности PlsDtcLow
                else if ( mPlsSpeedDelta > 3000 )               { mParseResult = 4; }  // относительная скорость dSp>3mmHg/sec
                else if ( mPlsSpeed > mPlsSpeedSet + 2000 )     { mParseResult = 5; }  // скорость S > Sset + 2mmHg/sec
                else if ( mPlsSpeed < mPlsSpeedSet - 2000 )     { mParseResult = 6; }  // скорость S < Sset - 2mmHg/sec
                else if ( rgPeriodTime < 300 )                  { mParseResult = 7; }  // тест на Time < 0.3 sec
//??                else if ( PlsParse.TimeRatio > 155 * 4 )                    { mParseResult = 8; }  // тест на TimeRatio>    155/256
                else if ( mPlsAmplitude > 7000 )                { mParseResult = 9; }  // тест на PlsDtcAmplPls > 7mmHg
                else                                            { mParseResult = 1; }   // нормальная пульсация для анализа
            }
            /*
            else
            {
                PlsParse.RP = PlsParse.PickTime - PlsParse.QrsTime; // время между детекиями QRS и тона
                if( QrsReg.Prm2 != 0 ) 										{ PlsParse.Result=3; }  // преждевременный
                else if( PlsParse.RP > 180 ) 								{ PlsParse.Result=10; } // не синхр. с QRS
                else if( PlsParse.RP < 0 )									{ PlsParse.Result=11; } // не синхр. с QRS
                else if ( PlsParse.Amplitude < PlsParse.AmplLow )  	       	{ PlsParse.Result=2; }  // ниже порога чувствительности PlsDtcLow
                else if ( abs(PlsParse.SpeedLast - PlsParse.Speed)>PrsSet(4)){ PlsParse.Result=4; } // относительная скорость dSp>3
                else if ( PlsParse.Speed > PrsSet(15) )           			{ PlsParse.Result=5; }  // скорость S<-15
                else if ( PlsParse.Speed < PrsSet(-5) )           			{ PlsParse.Result=6; }  // скорость S>3
                else if ( PlsParse.PeriodTime < 400 )                 		{ PlsParse.Result=7; }  // тест на Time<0.4
                else if ( PlsParse.TimeRatio > 155 )               			{ PlsParse.Result=8; }  // тест на TimeRatio>155/256
                else if ( PlsParse.Amplitude>PrsSet(7) )          			{ PlsParse.Result=9; }  // тест на PlsDtcAmplPls>5mmHg
                else                                         				{ PlsParse.Result=1; }; // нормальная пульсация для анализа
            }
            */
            //-----------------------------------
            PlsMax3();              // расчет максимума третьего порядка
            PlsAmplLowEnd();        // расчет минимального уровня пика для завершения измерения
            PlsTimeIntervalsCnt();  //

            mPlsSpeedLast  = mPlsSpeed; // скорость на предыдущем интервале

            return mParseResult;
        }

        //-----------------------------------------------------------------------------
        inline void PlsSpeedCnt(void) // Расчет скорости спуска
        {
            mPlsSpeed = (rgPrsEnd - rgPrsBegin) * 1000 / rgPeriodTime; // (mmHg* 1000)/sec
            mPlsSpeedDelta = mPlsSpeedLast - mPlsSpeed;
            if(mPlsSpeedDelta < 0) mPlsSpeedDelta = -mPlsSpeedDelta;
        }

        inline void PlsAmplCnt(void) // Расчет амплитуды пульсации
        {
            mPlsAmplitude =  rgPrsFront - rgPrsBegin;           // амплитуда пульсации (mmHg* 0x100)
            mPlsAmplitude -= (mPlsSpeed * rgFrontTime) / 1000;  //коррекция амплитуды пульсовой волны на скорость спуска
        }
        //-------------------------------------
        inline void PlsTimeRatioCnt(void) // Расчет отношения фаз пульсации
        {   // скважность времен детектора - PlsParse.Ratio=(PlsParse.FrontTime*256)/PlsParse.PeriodTime
            mPlsTimeRatio = (rgPrsFront * 1000) / rgPeriodTime;
        }
        //-------------------------------------
        inline void PlsPrematCnt(void) // Выделение преждевременности по пульсациям
        {
            // определяем порог преждевременности 0.7 от среднего
            // Rg*1/3*0.75 = Rg*1/4 (0x40/0x100)
            // сравнениние предыдущего PP интервала с уровнем преждевременности
            // если меньше - код завершения 1 , иначе 0
            int32_t rg = (mPlsDtcTimeSar[3] + mPlsDtcTimeSar[2] + mPlsDtcTimeSar[1]) / 4; // 0.75(Sar)  Sum/3 **
            flgPlsPremat = (mPlsDtcTimeSar[0] < rg); // преждевременные
        }
        //-------------------------------------
        //-------------------------------------
        inline void PlsTimeIntervalsCnt() // для расчета преждевременности по пульсациям
        {
            if ( mParseResult <= 3 ) // нормальная пульсация + ниже порога чувствительности PlsDtcLow + преждевременная
            {
                // запись PlsDtcPerTime в буфер усреднения PlsDtcPerTimeSar
                //    с ограничениями амплитуды не менее 0.4с не более 1.5с
                mPlsDtcTimeSar[3] = mPlsDtcTimeSar[2];
                mPlsDtcTimeSar[2] = mPlsDtcTimeSar[1];
                mPlsDtcTimeSar[1] = mPlsDtcTimeSar[0];
                int32_t rg = rgPeriodTime;  // запись в буфер интервалов
                if (rg < 400) rg= 400;
                if (rg > 1500) rg= 0;
                mPlsDtcTimeSar[0] = rg;

                mErrTimePeriod = 0; // // счетчик временного интервала помехи от последней хорошей пульсации
            }
            else
            {
                mErrTimeFull    += rgPeriodTime; // счетчик временного интервала помехи за все время измерения
                mErrTimePeriod  += rgPeriodTime; // счетчик временного интервала помехи от последней хорошей пульсации
            }
        }
        //-------------------------------------
        //-------------------------------------
        //??? это для PulsOnLineCalculation
        inline void PlsMax3(void)   // расчет максимума третьего порядка
        {
            if ( mParseResult != 1 ) { return; } // только для амплитудных

            if (mPlsAmplitude > mPlsDtcMaxAmpl[0] )
            {
                mPlsDtcMaxAmpl[2] = mPlsDtcMaxAmpl[1];
                mPlsDtcMaxAmpl[1] = mPlsDtcMaxAmpl[0];
                mPlsDtcMaxAmpl[0] = mPlsAmplitude;
                return;
            }
            else if (mPlsAmplitude > mPlsDtcMaxAmpl[1] )
            {
                mPlsDtcMaxAmpl[2] = mPlsDtcMaxAmpl[1];
                mPlsDtcMaxAmpl[1] = mPlsAmplitude;
                return;
            }
            else if (mPlsAmplitude > mPlsDtcMaxAmpl[2] )
            {
                mPlsDtcMaxAmpl[2] = mPlsAmplitude;
                return;
            }
        }
        //-------------------------------------
        inline void PlsAmplLowEnd(void) // расчет минимального уровня пика для завершения измерения
        {
            if ( mParseResult != 1 )        { return; }     // только для чистых
            if ( mPlsDtcMaxAmpl[0] == 0 )   { return; }     // третий максимум не готов
            // minLvP = minLvP + (LvM(3)*0.45 - minLvP) * 0.25;
            // minLvP = max( minLvP, 0.4mmHg);
            mPlsAmplLow += ( mPlsDtcMaxAmpl[0] * 102/0x100 - mPlsAmplLow) * 51 / 0x100;
            if(mPlsAmplLow < 450)   { mPlsAmplLow = 450; } // 0.45mmHg
        }
        //-------------------------------------
    };
}
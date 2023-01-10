#pragma once

#include "arithm.h"

//-----------------------------------------------------------------------------
struct EventPulseElement
{
	int32_t press;
	int32_t pulse;
	int32_t pos;
	
	void Reset()
	{
		pulse = 0;
		pos   = 0;
	}
	
	void Set( int32_t _press, int32_t _pulse, int32_t _pos )
	{
		press = _press;
        pulse = _pulse;
		pos   = _pos;
	}
	
	void operator = (const EventPulseElement& rhs)
	{
		press = rhs.press;
        pulse = rhs.pulse;
		pos   = rhs.pos;		
	}
};

struct PulseEvent
{
	int32_t  bad;
	int32_t  pos;
	int32_t  val;
	int32_t  range;
	int32_t  press;
	int32_t  imax;
	int32_t  startMarkPos;
	
	void Reset()
	{
		bad = 0;
		pos = 0;
		val = 0;
		range = 0;
        press = 0;
		imax  = 0;
		startMarkPos = 0;
	}
};

struct EventPulse
{
	bool bad;
	EventPulseElement i1;
	EventPulseElement i2;
	EventPulseElement imax;
    //	EventPulseElement imin;
    
	int32_t W;
	int32_t Range;
	int32_t n1;
	int32_t duty;
	int32_t kW;
	int32_t speed;
	
	void Reset()
	{
		bad = true;
		i1.Reset();
		i2.Reset();
		imax.Reset();
		//imin.Reset();		
		W = 0;
		Range = 0;
		n1 = 0;
		duty = 0;
		kW = 0;
		speed = 0;
	}
	
};

struct CandidatesPulse
{
	struct MainBack
	{
		EventPulseElement main; //В main хранится итоговое значение
		EventPulseElement back; //В back хранится промежуточное найденное
		
		void Reset()
		{
			main.Reset();
			back.Reset();
		}
	};
	
	MainBack i1;
	MainBack i2;
	MainBack imax;
//	MainBack imin;
	
	void Reset()
	{	
		i1.Reset();
		i2.Reset();
		imax.Reset();
//    imin.Reset();
	}
	
};

struct InnerEvents
{
	bool eventUp;
	bool eventDn;
	bool eventPkDn;
	bool eventMinDist;
	bool eventRdy;
	bool eventMaxCandidate;	

	void Reset()
	{
		eventUp            = false;
		eventDn            = false;
		eventPkDn		   = false;
		eventMinDist	   = false;
		eventRdy           = false; // скороее всего, true при наличии всех элементов детекции
		eventMaxCandidate  = false;	
	}
};

struct PulseDetect
{	
    int32_t Fs;
	int32_t minDist;

    PulseDetect(int32_t _fs, int32_t _minDist)
    : Fs(_fs), minDist(_minDist)
    { Reset(); };

    int32_t LvP;
    int32_t CntP;

    int32_t minR;
    int32_t LvZ; //средний уровень пика
    int32_t LvR; //уровень слежения-разряда
    int32_t invkR; 

    int32_t invkZ;
    bool modeZero;
    bool lookDn;

    int32_t dZ;
    int32_t cnt; // Абсолютный счетчик точек

    int32_t end, last, prev, cur;

    int32_t trends[6]; //Для вывода отладочной инфы

    CandidatesPulse candid;     //Кандитаты в ключевые точки
    InnerEvents e;              //События попадания в точки
    int32_t startMarkPos;       //Позиция в точках относительно метки старта

    inline void Reset()
    {
        LvP = ~0; //-Inf
        CntP = 0;
            
        minR  = 100;
        LvZ   = 1000; //средний уровень пика
        LvR   = 0; //уровень слежения-разряда
        invkR = 50;//1000; 
            
        invkZ = 5;
        modeZero = true;
        lookDn = false;

        cnt = 0;
        dZ  = 0;

        e.Reset();

        end  = 0;
        last = 1; 
        prev = 2;
        cur  = 3;

        candid.Reset();	
    };

    inline void detect(int32_t x) // Детектор
    {
        trends[2] = 0;
	
        if(modeZero)
        {
            if( x > LvZ / 10 )
            {
                e.eventUp = true;
                modeZero = false;
            }
        }
        else
        {
            if( x < LvZ / 10 )
            {
                e.eventDn = true;
                modeZero = true;
                
                if(lookDn)
                {
                    e.eventPkDn = true;
                    lookDn = false;
                }
                
            }
        }
                
        if (x > LvR)		 // поиск максимума
        {
            LvR = x;
            if ( LvP < LvR )
            {
                LvP = LvR;
                lookDn = true;
                e.eventMaxCandidate = true;
                CntP = 0;
            }
        }
        else // медленный разряд
        {
            LvR = LvR - LvR / invkR;
            if (LvR < minR)  LvR = minR;
        } 	


        if ( CntP == minDist )
        {
                e.eventMinDist = true;
                if ( LvP > LvZ && !lookDn) // если пик выше по амплитуде и была детекция вниз
                {
                    e.eventRdy = true;
                }
                
                int32_t err = (LvP / 2 - LvZ);
                if( err > LvZ )  err = LvZ; // мин-макс динамика
                if( err < -LvZ ) err = -LvZ;
                
                if( err > 0 )    dZ = err / invkZ; // разная динамика вверх-вниз
                else             dZ = err * 4 / invkZ;
                
                LvZ = LvZ + dZ;
                trends[2] = LvP;
                LvP = 0; //%сброс
        }
        
    	trends[0] = LvR;
    	trends[1] = LvZ;
        
        CntP++;
        cnt++;
    };

    //Буфер событий детекций
    const static int32_t rail = 4;
    EventPulse eBuf[rail];
    //

    inline void param(int32_t press, int32_t pulse)	// Параметризатор
    {
        if (e.eventUp) {candid.i1.back.Set(press, pulse, cnt);};    // событие перехода через уровень вверх
        if (e.eventPkDn) {candid.i2.main.Set(press, pulse, cnt);};  // событие перехода через уровень вниз, когда кандидат в максимум уже найден
        if (e.eventMaxCandidate) 
        {
            candid.i1.main = candid.i1.back;
            candid.imax.back.Set(press, pulse, cnt);
        };
        if (e.eventRdy)
        {
            // Двигаем буфер
            ++cur %= rail; ++prev %= rail; ++last %= rail; ++end %= rail;
            eBuf[cur].Reset();

            eBuf[cur].i1 = candid.i1.main;
            eBuf[cur].i2 = candid.i2.main;
            candid.imax.main = candid.imax.back;
            eBuf[cur].imax = candid.imax.main;
        };
    };

    int32_t Range = 0;
    int32_t discardArr[9] = {0};
    int32_t dbgcntspeed = 0;
    int32_t dbgcntdspeed = 0;
    int32_t dbgspeed = 0;
    int32_t dbgdspeed = 0;

    PulseEvent pulse;

    inline void discard() // Браковка пульсаций 
    {
        trends[3] = 0;
		
        EventPulse& eNext     = eBuf[cur];
        EventPulse& eCurrent  = eBuf[prev];
        EventPulse& ePrevious = eBuf[last];
        int32_t multi = 1000;
        int32_t kPres = 1000;
        int32_t bad = 0;

        if(e.eventRdy)
        { 
            // Ширина
            eNext.W = eNext.i2.pos - eCurrent.i2.pos;
            
            // Размах
            // Range = [ P(i2) - P(i1) ] - (i2 - i1)/(i1' - i1) * [P(i1') - P(i1)]
            int32_t dpdt = ( eNext.i1.press - eCurrent.i1.press ) / (int32_t)( eNext.i1.pos - eCurrent.i1.pos );
            Range = eCurrent.Range = (eCurrent.i2.press - eCurrent.i1.press) - 
                                            (int32_t)(eCurrent.i2.pos - eCurrent.i1.pos) * dpdt;
            
            // Скорость и ускорение
            dbgspeed = eCurrent.speed = dpdt * Fs;
            int32_t dSpeed = dbgdspeed =  fnAbs(eCurrent.speed - ePrevious.speed);
            
            // Остальные параметры
            eCurrent.n1   = eCurrent.i2.pos - eCurrent.i1.pos; //время фронта:
            eCurrent.duty = eCurrent.n1 * multi / eNext.W; //скважность
            eCurrent.kW   = eCurrent.W * multi / eNext.W; //отношение интервалов до/после:
            
            // Браковка
            if( eCurrent.W  * multi < 250 * Fs || 1500 * Fs < eCurrent.W * multi ) { discardArr[1]++; bad += pow2(0); }
            if ( eCurrent.n1 * multi < 80 * Fs )                                   { discardArr[2]++; bad += pow2(1); }
            if ( eCurrent.Range < kPres / 3 || 5 * kPres < eCurrent.Range )        { discardArr[3]++; bad += pow2(2); } 	
            if ( eCurrent.duty < 120 || 800 < eCurrent.duty )                      { discardArr[4]++; bad += pow2(3); } 	
            if ( eCurrent.kW < 500 && ( eCurrent.W * multi < 500 * Fs ) )          { discardArr[5]++; bad += pow2(4); }
            if ( eCurrent.kW > 2000 && ( eNext.W * multi > 1000 * Fs ) )           { discardArr[6]++; bad += pow2(5); }			
            // if( GET_SERVICE(BpmAlg)->IsAscend() ) //Различается в зависимости от того подъем или спуск
            // {
            //     if ( eCurrent.speed > 10 * kPres || eCurrent.speed < 0 * kPres )     { discardArr[7]++; bad += pow2(6); dbgcntspeed++; } //Не корректна, нужно различать спуск и подъем
            // }                                                                        
            // else                                                                     
            // {                                                                        
            //     if ( eCurrent.speed < -10 * kPres || eCurrent.speed > 0 * kPres )    { discardArr[7]++; bad += pow2(6); dbgcntspeed++; } //Не корректна, нужно различать спуск и подъем
            // }                                                                        
            if ( dSpeed > 4 * kPres )                                              { discardArr[8]++; bad += pow2(7); dbgcntdspeed++; } //Требует уточнения
            
            discardArr[0]++; // Отладка
            
            pulse.bad   = bad;
            pulse.pos   = eCurrent.i2.pos;
            pulse.press = eCurrent.i2.press;
            pulse.range = eCurrent.Range;
            pulse.val   = eCurrent.i2.pulse;
            pulse.imax  = eCurrent.imax.pos;
            pulse.startMarkPos = startMarkPos;
            
            trends[0] = eCurrent.Range;
            trends[1] = eCurrent.W; 
            trends[2] = eCurrent.duty;
        };
    };

    inline void Exe(int32_t tacho, int32_t press, int32_t pulse)
    {
        trends[0] = 0;
        trends[1] = 0;
        trends[2] = 0;
        
        e.Reset();
        
        detect(tacho);          // детекция
        param(press, pulse);    // параметризация (проверка на наличие нужной детекции происходит внутри)
        discard();              // отбраковка

        startMarkPos++; //Инкрементируем позицию от метки старт
    };
};
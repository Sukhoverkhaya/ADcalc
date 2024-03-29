// на основании ControlPulse.h из firmware, но без обертки для железа
#pragma once

#include "StateMachine.h"
#include "PulseDetect.h"
#include "arithm.h"

struct STP // Состояния для автоматов пульсаций
{
	enum States
	{
		STATE_0 = 0,
		STATE_1,
		STATE_2,
		STATE_3,
		STATE_SUCCESS,
		STATE_FAIL,		
	};
};

struct BaseStatePulse;
class ControlPulse : public ST
{	
public:

	bool stopInfl;
	bool stopDefl;
	STP::States currentState;

	// Results	
	PulseEvent inflBeg;
	PulseEvent inflEnd;
	PulseEvent deflBeg;
	PulseEvent deflEnd;

	const int32_t Fs;

	STP::States nextState;

	BaseStatePulse*  stateArrayInfl[6];
	BaseStatePulse*  stateArrayDefl[6];
	BaseStatePulse** currentStateMachine;
	
	static const int32_t wait = 3;
	static const int32_t rail = 100;
	PulseEvent* buf;

	PulseEvent bufInfl[rail];
	PulseEvent bufDefl[rail];
	int32_t savedSzInfl;
	int32_t savedSzDefl;
	bool InflSuccess;
	bool DeflSuccess;

	int32_t sz;
	int32_t cursor;
	int32_t first;

	// Итераторы хороших пульсации
	int32_t ilast;
	int32_t i1;
	int32_t i2;
	int32_t i3s;
	//
	int32_t minLvl = 500;
	int32_t Lvl;
	int32_t maxLvl;
	int32_t Nb;
	int32_t Nbad;
	int32_t maxNbad;
	int32_t maxBadTime;
	int32_t Nlow;
	//
	int32_t Pmax;
	int32_t Pmin;
	
	// Буфер для вычисления медианы
	static const int32_t rangeRail = 5;
	int32_t rangeBuf[rangeRail];
	int32_t rangeCursor;
	//

	ControlPulse(const int32_t _fs)
	: Fs(_fs)
	{
		Pmin    = PrsSet(40);
		Pmax    = 0; //PrsSet(240);
		//
		inflBeg.Reset();
		inflEnd.Reset();
		deflBeg.Reset();
		deflEnd.Reset();	
		//
		stopInfl = false;
		stopDefl = false;
		
		Reset();
	};

	inline void StartInflST()
	{
		Reset();
		On();
		currentStateMachine = stateArrayInfl;
		buf = bufInfl;
		stopInfl = false;
		stopDefl = false;
		savedSzInfl = 0;
		savedSzDefl = 0;
		InflSuccess = false;
		DeflSuccess = false;
		
		inflBeg.Reset();
		inflEnd.Reset();
		deflBeg.Reset();
		deflEnd.Reset();	
	}

	inline void StartDeflST()
	{
		Reset();
		On();
		buf = bufDefl;
	}

	inline void Reset()
	{
		currentState = STP::STATE_0;
		nextState    = STP::STATE_0;
		stateChanged = true;
		
		
		//Буфер событий тонов
		cursor = 0;
		first  = 0;
		sz     = 0;
		
		//	//
		inflBeg.Reset();
		inflEnd.Reset();
		deflBeg.Reset();
		deflEnd.Reset();
		//
		ilast = 0;
		i1    = 0;
		i2    = 0;
		i3s   = 0;
		//
		Lvl     = 0;
		maxLvl  = 0;	
		Nb      = 0;
		Nbad    = 0;
		maxNbad = 5;
		Nlow    = 0;
		maxBadTime = 5 * Fs;
		rangeCursor = 0;	
	}

	inline void ChangeState(STP::States _state)
	{
		nextState   = _state;
		stateChanged = true;
	}
	
	bool IsStateChanged()
	{
		if(stateChanged)
		{
			stateChanged = false;
			return true;
		}
		
		return false;
	}	
	
};	

struct BaseStatePulse  // родительский класс для всех состояний алгоритма
{
public:
    ControlPulse& sm;

    BaseStatePulse(ControlPulse& _sm) : sm(_sm) { }
    
    virtual void Tick()      { };
    virtual void NewPulse(PulseEvent& PulseEvent)  { };
    virtual void Enter()     { };
    virtual void Exit()      { };	
};
//_________________________________________________________________
struct StatePulseInfl0 : BaseStatePulse
{
	StatePulseInfl0(ControlPulse& sm) : BaseStatePulse(sm) {}
	
	inline void NewPulse(PulseEvent& PulseEvent) 
	{
		sm.buf[sm.cursor] = PulseEvent;

		if( PulseEvent.press > sm.Pmin && !sm.buf[sm.cursor].bad)
		{	
			sm.Nb = 1;
			sm.ChangeState(STP::STATE_1);
			sm.i1 = sm.cursor;
		}
		
		++sm.cursor %= sm.rail;
	}
	
};
	
struct StatePulseInfl1 : BaseStatePulse //начинаем копить хорошие пульсации
{
	StatePulseInfl1(ControlPulse& sm) : BaseStatePulse(sm) {}
								
	inline void Enter()
	{
		sm.Nb = 1;
	}
		
	inline void NewPulse(PulseEvent& PulseEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;			
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;

			return;
		}
			
		//Если хорошая
		sm.rangeBuf[sm.rangeCursor] = sm.buf[sm.cursor].range; //!!!sm.rangeBuf[sm.rangeCursor++] = sm.buf[sm.cursor].range;
		++sm.rangeCursor %= sm.rangeRail;
		sm.Nb++;

		if( sm.Nb > 1 &&                  
			(sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos) > sm.wait * sm.Fs) // 2 пульсации - 3 секунды
		{
			sm.Nb = 1;
		}			
		
		if ( sm.Nb == 3 ) // есть 3 пульсации - продолжаем
		{	
			//среднее по двум пульсациям
			sm.Lvl = sm.buf[sm.cursor].val + sm.buf[sm.i1].val;
			sm.Lvl /= 2;
			sm.maxLvl = sm.Lvl;
			
			if(sm.buf[sm.i2].range > sm.Lvl / 2) //*0.5 //первая значимая
			{
				sm.inflBeg  = sm.buf[sm.i2]; //запоминаем пульсацию начала диастолич.
				sm.ChangeState(STP::STATE_2);
				sm.Nbad = 0;
			}
			else
			{
				sm.Nb = 2;
			}
			
			sm.ilast = sm.cursor;			
		}
		
		//итерация указателей на последние 3 хороших
		sm.i2 = sm.i1;
		sm.i1 = sm.cursor;
		
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
	}
	
};	

struct StatePulseInfl2 : BaseStatePulse
{
	StatePulseInfl2(ControlPulse& sm) : BaseStatePulse(sm), k1000(0), maxBadTime(sm.Fs*5), badTime(0), imax(0), timer(0) {}
				
	inline void Enter()
	{
		k1000 = 0;
		badTime = 0;
		imax = 0;	
		timer = 0;
	}		
		
	inline void NewPulse(PulseEvent& PulseEvent) 
	{
		// критерий браковки по времени и по количеству бракованных пульсаций 
    // будет плохо работать, если диастола на накачке захвачена рано!
		
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;
		
		//критерий браковки по времени
		//всю браковку начинаем после NO_BAD_TILL_PRESS мм
		
		if( sm.buf[sm.cursor].press < PrsSet(NO_BAD_TILL_PRESS) ) 
		{
			// NOP..
		}
		else if( sm.buf[sm.cursor].bad )
		{
			//Вычисляем предыдущий элем. отностительно sm.cursor
			int32_t prevCursor;
			if(sm.cursor == 0) prevCursor = sm.rail - 1;
			else               prevCursor = sm.cursor - 1;
			//
			
			badTime += sm.buf[sm.cursor].pos - sm.buf[prevCursor].pos;
			sm.Nbad++;
			if ( sm.Nbad >= sm.maxNbad ) // браковка накачки и переход на спуск
			{
				sm.ChangeState(STP::STATE_FAIL);
				sm.Off();	
			}
		}
		else
		{
			if( badTime > 0 ) 
			{
				//Вычисляем предыдущий элем. отностительно sm.cursor
				int32_t prevCursor;
				if(sm.cursor == 0) prevCursor = sm.rail - 1;
				else               prevCursor = sm.cursor - 1;
				//				
				
				badTime += sm.buf[sm.cursor].pos - sm.buf[prevCursor].pos;
				if( badTime > maxBadTime ) //измерение забраковано по макс интервалу бракованных пиков
				{
					sm.ChangeState(STP::STATE_FAIL);
					sm.Off();						
				}
				badTime = 0;
			}
		}
		
		if(!sm.buf[sm.cursor].bad)
		{
			sm.rangeBuf[sm.rangeCursor] = sm.buf[sm.cursor].range; // заполнение буфера
			++sm.rangeCursor %= sm.rangeRail;
			
			sm.Nb++;
			
			if( sm.Nb >= 5) //пересчет макс уровня
			{
				int32_t med = med5(sm.rangeBuf);
				sm.Lvl = fnMax(med, sm.minLvl);
			}
			else
			{
				sm.Lvl = sm.maxLvl;
			}
			
			if( sm.Lvl >= sm.maxLvl )
			{
				sm.maxLvl       = sm.Lvl;
				imax            = sm.cursor;
				timer           = 0; //Здесь сбрасываем таймер обратного отсчета
			}
			else
			{
				if( sm.Lvl < sm.maxLvl / 2 ) //* 0.5
				{
					sm.Nlow++;
				}
			}
			sm.ilast = sm.cursor;
		}
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;
	}
	
	inline void Tick()
	{
		timer++;
        //if(sm.buf[sm.cursor].press < PrsSet(NO_END_SEARCH_TILL_PRESS) ) return;
		
		// if( (sm.Lvl < sm.maxLvl * 3 / 5 && timer > sm.wait * sm.Fs ) /*|| sm.Nlow > 3*/ )
		if( (timer > sm.wait * sm.Fs ) /*|| sm.Nlow > 3*/ )
		{
			sm.inflEnd = sm.buf[sm.ilast];
			//проверка, что (сист*0.15) > (сист - диаст)
			if( sm.inflEnd.press * 3 / 20 > ( sm.inflEnd.press - sm.inflBeg.press ) )  
			{
				sm.ChangeState(STP::STATE_1);
                sm.Nb = 0;
			}
			else
			{
				sm.ChangeState(STP::STATE_SUCCESS);
			}
		}
	}	
	
	int32_t k1000;
	const int32_t maxBadTime;
	int32_t badTime;
	int32_t imax;
	int32_t timer;
};	

//struct StatePulseInfl3 : BaseStatePulse
//{
//	StatePulseInfl3(ControlPulse& sm) : BaseStatePulse(sm) {}
//		
//	void Enter()
//	{	
//		//МЕТКА
//		_MARK( createHighBoundary(ADType_Pulse, ADDir_INFL); )
//		//МЕТКА
//		//sm.savedSzInfl = sm.sz;
//		sm.InflSuccess = true;
//	}			
//		
//	void Tick()
//	{
//		int32_t currentPress = GET_SERVICE(BpmMediator)->PrsMsr;
//		ControlTone* ct = GET_SERVICE(ControlTone);
//		
//		if( sm.inflEnd.press + PrsSet(20) < currentPress ||
//			  ( ct->InflSuccess && ( sm.inflEnd.press + PrsSet(10) < currentPress || ct->inflEnd.press + PrsSet(20) < currentPress) )
//		)	
//		{
//			sm.ChangeState(STP::STATE_SUCCESS);
//			sm.stopInfl = true;
//		}
//	}
//				
//	void NewPulse(PulseEvent& PulseEvent) 
//	{ 
//		//Добавляем пульсацию в буфер
//		if( sm.cursor >= sm.rail ) return;
//		sm.buf[sm.cursor] = PulseEvent;
//		//Двигаем буфер
//		++sm.cursor %= sm.rail;
//		++sm.sz;		
//	}
//	
//};

struct StatePulseInflSuccess : BaseStatePulse
{
	StatePulseInflSuccess(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	inline void Enter() {sm.InflSuccess = true;}			
		
	inline void Tick(){sm.Off();}
	
};

struct StatePulseInflFail : BaseStatePulse
{
	StatePulseInflFail(ControlPulse& sm) : BaseStatePulse(sm) {}
					
	inline void Tick() {sm.Off();}
	
};

// СПУСК

struct StatePulseDefl0 : BaseStatePulse
{
	StatePulseDefl0(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	inline void NewPulse(PulseEvent& PulseEvent) 
	{
		sm.buf[sm.cursor] = PulseEvent;
		
		if(sm.buf[sm.cursor].press < sm.Pmax - PrsSet(10) && !sm.buf[sm.cursor].bad)
		{			
			sm.Nb = 1;
			sm.ChangeState(STP::STATE_1);
			sm.i1 = sm.cursor;
		}
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;		
	}
};

struct StatePulseDefl1 : BaseStatePulse //начинаем копить хорошие пульсации
{
	StatePulseDefl1(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	inline void NewPulse(PulseEvent& PulseEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;			
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;
			return;
		}
			
		//Если хорошая
		sm.rangeBuf[sm.rangeCursor] = sm.buf[sm.cursor].range; ///!!!sm.rangeBuf[sm.rangeCursor++] = sm.buf[sm.cursor].range;
		++sm.rangeCursor %= sm.rangeRail;
		sm.Nb++;

		if( sm.Nb > 1 &&                  
			(sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos) > sm.wait * sm.Fs) // 2 пульсации - 3 секунды
		{
			sm.Nb = 1;
		}			
		
		if ( sm.Nb == 3 ) //есть 3 пульсации - продолжаем
		{	
			//среднее по двум пульсациям
			sm.Lvl = sm.buf[sm.cursor].val + sm.buf[sm.i1].val;
			// sm.Lvl = sm.buf[sm.cursor].range + sm.buf[sm.i1].range;
			sm.Lvl /= 2;
			sm.maxLvl = sm.Lvl;
			
			if( sm.buf[sm.i2].range > sm.Lvl / 2) //*0.5 //первая значимая
			{
				sm.deflBeg  = sm.buf[sm.i2]; //запоминаем пульсацию начала диастолич.
				sm.ChangeState(STP::STATE_2);
				sm.Nbad = 0;
			}
			else
			{
				sm.Nb = 2;
			}
			
			sm.ilast = sm.cursor;			
		}
		
		//итерация указателей на последние 3 хороших
		sm.i2 = sm.i1;
		sm.i1 = sm.cursor;
		
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
	}
	
};	


struct StatePulseDefl2 : BaseStatePulse
{
	StatePulseDefl2(ControlPulse& sm) : BaseStatePulse(sm), k1000(0), maxBadTime(sm.Fs*5), badTime(0), imax(0), timer(0) {}
		
	inline void Enter()
	{
		k1000 = 0;
		badTime = 0;
		imax = 0;	
		timer = 0;	
	}
				
	inline void NewPulse(PulseEvent& PulseEvent) 
	{
		// критерий браковки по времени и по количеству бракованных пульсаций 
    	// будет плохо работать, если диастола на накачке захвачена рано!
		
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;
		
		//браковка
		if( sm.buf[sm.cursor].bad )
		{
			//Вычисляем предыдущий элем. отностительно sm.cursor
			int32_t prevCursor;
			if(sm.cursor == 0) prevCursor = sm.rail - 1;
			else               prevCursor = sm.cursor - 1;
			//			
			
			badTime += sm.buf[sm.cursor].pos - sm.buf[prevCursor].pos;
			sm.Nbad++;
			if ( sm.Nbad >= sm.maxNbad ) // браковка накачки и переход на спуск
			{
				sm.ChangeState(STP::STATE_FAIL);
				sm.Off();	
			}
		}
		else
		{
			if( badTime > 0 ) 
			{
				//Вычисляем предыдущий элем. отностительно sm.cursor
				int32_t prevCursor;
				if(sm.cursor == 0) prevCursor = sm.rail - 1;
				else               prevCursor = sm.cursor - 1;
				//							
				
				badTime += sm.buf[sm.cursor].pos - sm.buf[prevCursor].pos;
				if( badTime > maxBadTime ) //измерение забраковано по макс интервалу бракованных пиков
				{
					sm.ChangeState(STP::STATE_FAIL);
					sm.Off();						
				}
				badTime = 0;
			}
		}
		
		if(!sm.buf[sm.cursor].bad)
		{
			sm.rangeBuf[sm.rangeCursor] = sm.buf[sm.cursor].range; //заполнение буфера
			++sm.rangeCursor %= sm.rangeRail;
			
			sm.Nb++;
			
			if( sm.Nb >= 5) //пересчет макс уровня
			{
				int32_t med = med5(sm.rangeBuf);
				sm.Lvl = fnMax(med, sm.minLvl);
			}
			else
			{
				sm.Lvl = sm.maxLvl;
			}
			
			if( sm.Lvl >= sm.maxLvl )
			{
				sm.maxLvl       = sm.Lvl;
				imax            = sm.cursor;
				timer           = 0; //Здесь сбрасываем таймер обратного отсчета
			}
			else
			{
				if( sm.Lvl < sm.maxLvl / 2 ) //* 0.5
				{
					sm.Nlow++;
				}
			}
			sm.ilast = sm.cursor;
		}
			
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;
	}
	
		
	inline void Tick()
	{
		timer++;
		if( (sm.Lvl < sm.maxLvl * 3 / 5 && timer > sm.wait * sm.Fs ) /*|| sm.Nlow > 3*/ )
		{
			sm.deflEnd = sm.buf[sm.ilast];
			
			//проверка, что (сист*0.15) > (сист - диаст)
			if( sm.deflBeg.press * 3 / 20 > ( sm.deflBeg.press - sm.deflEnd.press ) )  
			{
				sm.ChangeState(STP::STATE_1);
                sm.Nb = 0;
			}
			else
			{
				sm.ChangeState(STP::STATE_3);
			}
		}
	}		
	
	int32_t k1000;
	const int32_t maxBadTime;
	int32_t badTime;
	int32_t imax;
	int32_t timer;
};	


struct StatePulseDefl3 : BaseStatePulse
{
	StatePulseDefl3(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	inline void Enter() { sm.DeflSuccess = true;}		
		
	inline void Tick()
	{
		if( sm.deflEnd.press - PrsSet(10) > sm.buf[sm.cursor].press ||  PrsSet(30) >sm.buf[sm.cursor].press ) //?
		{
			sm.ChangeState(STP::STATE_SUCCESS);
			sm.stopDefl = true;
		}
	}
				
	inline void NewPulse(PulseEvent& PulseEvent) 
	{ 
		//Добавляем пульсацию в буфер
		if( sm.cursor >= sm.rail ) return;
		sm.buf[sm.cursor] = PulseEvent;
		//Двигаем буфер
		++sm.cursor %= sm.rail;	
	}
	
};

struct StatePulseDeflSuccess : BaseStatePulse
{
	StatePulseDeflSuccess(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	inline void Tick() {sm.Off();}
	
};

struct StatePulseDeflFail : BaseStatePulse
{
	StatePulseDeflFail(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	inline void Tick()
	{
		sm.stopDefl = true;
		sm.Off();
	}
	
};

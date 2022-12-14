// (определение некоторых классов и методов, объявленных в ControlPulseD.h)
// сделано на основании ControlPulse.cpp из firmware, но без обертки для железа

#include "ControlPulseD.h"

const int NO_BAD_TILL_PRESS        = 70;
const int NO_END_SEARCH_TILL_PRESS = 120;

int32_t med5(int32_t* buf) // Медиана из 5 элементов, буфер на 5 элементов
{
	int32_t res = 0;
	
	for( int32_t i = 0; i < 5; ++i)
	{
		int32_t thr = 0;
		int32_t s   = 0;
		for( int32_t j = 0; j < 5; ++j)
		{
			if( i == j) continue;
			
			if( buf[i] > buf[j] )      {s++};
			else if( buf[i] < buf[j] ) {s--};
			else                       {thr++};
			
			if( fnAbs(s) <= thr )
			{
				res = buf[i];
			}			
		}
	}
	
	return res;
};

struct StatePulseInfl0 : BaseStatePulse
{
	StatePulseInfl0(ControlPulse& sm) : BaseStatePulse(sm) {}
	
	virtual void NewPulse(PulseEvent& PulseEvent) 
	{
		sm.buf[sm.cursor] = PulseEvent;
		
		if( PulseEvent.press > sm.Pmin && !sm.buf[sm.cursor].bad)
		{			
			sm.Nb = 1;
			sm.ChangeState(STP::STATE_1);
			sm.i1 = sm.cursor;
		}
		
		++sm.cursor %= sm.rail;
		// ++sm.sz;
	}
	
};
	
struct StatePulseInfl1 : BaseStatePulse //начинаем копить хорошие пульсации
{
	StatePulseInfl1(ControlPulse& sm) : BaseStatePulse(sm) {}
								
	virtual void Enter()
	{
		sm.Nb = 1;
	}
		
	virtual void NewPulse(PulseEvent& PulseEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;			
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;
			// ++sm.sz;				
			
			return;
		}
			
		//Если хорошая
		sm.rangeBuf[sm.rangeCursor] = sm.buf[sm.cursor].range; //!!!sm.rangeBuf[sm.rangeCursor++] = sm.buf[sm.cursor].range;
		++sm.rangeCursor %= sm.rangeRail;
		sm.Nb++;

		if( sm.Nb > 1 &&                  
			sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos > sm.wait * sm.Fs) // 2 пульсации - 3 секунды
		{
			sm.Nb = 1;
		}			
		
		if ( sm.Nb == 3 ) // есть 3 пульсации - продолжаем
		{	
			//среднее по двум пульсациям
			sm.Lvl = sm.buf[sm.cursor].val + sm.buf[sm.i1].val;
			sm.Lvl /= 2;
			sm.maxLvl = sm.Lvl;
			
			if( sm.buf[sm.i2].range > sm.Lvl / 2) //*0.5 //первая значимая
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
		// ++sm.sz;			

	}
	
};	

struct StatePulseInfl2 : BaseStatePulse
{
	StatePulseInfl2(ControlPulse& sm) : BaseStatePulse(sm), k1000(0), maxBadTime(sm.Fs*5), badTime(0), imax(0), timer(0) {}
				
	virtual void Enter()
	{
		k1000 = 0;
		badTime = 0;
		imax = 0;	
		timer = 0;
		
		//МЕТКА
		// _MARK( createLowBoundary(ADType_Pulse, ADDir_INFL); )
		//МЕТКА
	}		
		
	virtual void NewPulse(PulseEvent& PulseEvent) 
	{
		// критерий браковки по времени и по количеству бракованных пульсаций 
    // будет плохо работать, если диастола на накачке захвачена рано!
		
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;
		
		//критерий браковки по времени
		//всю браковку начинаем после NO_BAD_TILL_PRESS мм
		
		if( sm.buf[prevCursor].press < PrsSet(NO_BAD_TILL_PRESS) ) 
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
				//sm.Off();	
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
					//sm.Off();						
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
	
	virtual void Tick()
	{
		timer++;
		if(sm.rangeBuf[sm.rangeCursor].press < PrsSet(NO_END_SEARCH_TILL_PRESS) ) return;
		
		if( (sm.Lvl < sm.maxLvl * 3 / 5 && timer > sm.wait * sm.Fs ) /*|| sm.Nlow > 3*/ )
		{
			sm.inflEnd = sm.buf[sm.ilast];
			
			//проверка, что (сист*0.15) > (сист - диаст)
			if( sm.inflEnd.press * 3 / 20 > ( sm.inflEnd.press - sm.inflBeg.press ) )  
			{
				sm.ChangeState(STP::STATE_1);
			}
			else
			{
				sm.ChangeState(STP::STATE_SUCCESS);
			}
		}
	}	
	
		
	int32_t k1000;
	const int maxBadTime;
	int32_t badTime;
	int32_t imax;
	int32_t timer;
};	

//struct StatePulseInfl3 : BaseStatePulse
//{
//	StatePulseInfl3(ControlPulse& sm) : BaseStatePulse(sm) {}
//		
//	virtual void Enter()
//	{	
//		//МЕТКА
//		_MARK( createHighBoundary(ADType_Pulse, ADDir_INFL); )
//		//МЕТКА
//		//sm.savedSzInfl = sm.sz;
//		sm.InflSuccess = true;
//	}			
//		
//	virtual void Tick()
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
//	virtual void NewPulse(PulseEvent& PulseEvent) 
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
		
	virtual void Enter()
	{	
		//МЕТКА
		// _MARK( createHighBoundary(ADType_Pulse, ADDir_INFL); )
		//МЕТКА
		//sm.savedSzInfl = sm.sz;
		sm.InflSuccess = true;
	}			
		
	virtual void Tick()
	{
		sm.Off();
	}
	
};

struct StatePulseInflFail : BaseStatePulse
{
	StatePulseInflFail(ControlPulse& sm) : BaseStatePulse(sm) {}
					
	virtual void Tick()
	{
		sm.Off();
	}
	
};

// СПУСК

struct StatePulseDefl0 : BaseStatePulse
{
	StatePulseDefl0(ControlPulse& sm) : BaseStatePulse(sm) {}
	
	virtual void Enter(int PressMax)
	{
		sm.Pmax = PressMax;
	}
		
	virtual void NewPulse(PulseEvent& PulseEvent) 
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
								
	virtual void Enter()
	{
		// МЕТКА
		// createHighBoundary(ADType_Pulse, ADDir_DEFL);
		// МЕТКА		
	}
		
	virtual void NewPulse(PulseEvent& PulseEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = PulseEvent;			
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;
			// ++sm.sz;				
			
			return;
		}
			
		//Если хорошая
		sm.rangeBuf[sm.rangeCursor] = sm.buf[sm.cursor].range; ///!!!sm.rangeBuf[sm.rangeCursor++] = sm.buf[sm.cursor].range;
		++sm.rangeCursor %= sm.rangeRail;
		sm.Nb++;

		if( sm.Nb > 1 &&                  
			sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos > sm.wait * sm.Fs) // 2 пульсации - 3 секунды
		{
			sm.Nb = 1;
		}			
		
		if ( sm.Nb == 3 ) //есть 3 пульсации - продолжаем
		{	
			//среднее по двум пульсациям
			sm.Lvl = sm.buf[sm.cursor].val + sm.buf[sm.i1].val;
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
		// ++sm.sz;			

	}
	
};	


struct StatePulseDefl2 : BaseStatePulse
{
	StatePulseDefl2(ControlPulse& sm) : BaseStatePulse(sm), k1000(0), maxBadTime(sm.Fs*5), badTime(0), imax(0), timer(0) {}
		
	virtual void Enter()
	{
		k1000 = 0;
		badTime = 0;
		imax = 0;	
		timer = 0;
		
//		//МЕТКА
//		createHighBoundary(ADType_Pulse, ADDir_DEFL);
//		//МЕТКА		
	}
				
	virtual void NewPulse(PulseEvent& PulseEvent) 
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
				//sm.Off();	
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
					//sm.Off();						
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
	
		
	virtual void Tick()
	{
		timer++;
		if( (sm.Lvl < sm.maxLvl * 3 / 5 && timer > sm.wait * sm.Fs ) /*|| sm.Nlow > 3*/ )
		{
			sm.deflEnd = sm.buf[sm.ilast];
			
			//проверка, что (сист*0.15) > (сист - диаст)
			if( sm.deflBeg.press * 3 / 20 > ( sm.deflBeg.press - sm.deflEnd.press ) )  
			{
				sm.ChangeState(STP::STATE_1);
			}
			else
			{
				sm.ChangeState(STP::STATE_3);
			}
		}
	}		
	
	int32_t k1000;
	const int maxBadTime;
	int32_t badTime;
	int32_t imax;
	int32_t timer;
};	


struct StatePulseDefl3 : BaseStatePulse
{
	StatePulseDefl3(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	virtual void Enter()
	{
		//МЕТКА
		// _MARK( createLowBoundary(ADType_Pulse, ADDir_DEFL); )
		//МЕТКА		
		//sm.savedSzDefl = sm.sz;
		sm.DeflSuccess = true;
	}		
		
	virtual void Tick()
	{
		if( sm.deflEnd.press - PrsSet(10) > GET_SERVICE(BpmMediator)->PrsMsr ||  PrsSet(30) > GET_SERVICE(BpmMediator)->PrsMsr ) //?
		{
			//sm.Pmax = GET_SERVICE(BpmMediator)->PrsMsr;
			sm.ChangeState(STP::STATE_SUCCESS);
			sm.stopDefl = true;
		}
	}
				
	virtual void NewPulse(PulseEvent& PulseEvent) 
	{ 
		//Добавляем пульсацию в буфер
		if( sm.cursor >= sm.rail ) return;
		sm.buf[sm.cursor] = PulseEvent;
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;		
	}
	
};

struct StatePulseDeflSuccess : BaseStatePulse
{
	StatePulseDeflSuccess(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	virtual void Tick()
	{
		sm.Off();
	}
	
};

struct StatePulseDeflFail : BaseStatePulse
{
	StatePulseDeflFail(ControlPulse& sm) : BaseStatePulse(sm) {}
		
	virtual void Tick()
	{
		sm.stopDefl = true;
		sm.Off();
	}
	
};

//-----------------------------------------------------------------------------
ControlPulse::ControlPulse(const int _fs)
: Fs(_fs)
{
	
	//State Infl
	stateArrayInfl[STP::STATE_0] =       new StatePulseInfl0(*this);
	stateArrayInfl[STP::STATE_1] =       new StatePulseInfl1(*this);
	stateArrayInfl[STP::STATE_2] =       new StatePulseInfl2(*this);	
//	stateArrayInfl[STP::STATE_3] =       new StatePulseInfl3(*this);
	stateArrayInfl[STP::STATE_SUCCESS] = new StatePulseInflSuccess(*this);
	stateArrayInfl[STP::STATE_FAIL] =    new StatePulseInflFail(*this);	
	
	//State Defl
	stateArrayDefl[STP::STATE_0] =       new StatePulseDefl0(*this);
	stateArrayDefl[STP::STATE_1] =       new StatePulseDefl1(*this);
	stateArrayDefl[STP::STATE_2] =       new StatePulseDefl2(*this);	
	stateArrayDefl[STP::STATE_3] =       new StatePulseDefl3(*this);
	stateArrayDefl[STP::STATE_SUCCESS] = new StatePulseDeflSuccess(*this);
	stateArrayDefl[STP::STATE_FAIL] =    new StatePulseDeflFail(*this);
	

	currentStateMachine = stateArrayInfl;

	Pmin    = PrsSet(40);
	Pmax    = 0;//PrsSet(240);
	//
	inflBeg.Reset();
	inflEnd.Reset();
	deflBeg.Reset();
	deflEnd.Reset();	
	//
	stopInfl = false;
	stopDefl = false;
	
	Reset();
}
//-----------------------------------------------------------------------------
void ControlPulse::Reset()
{
	currentState = STP::STATE_0;
  nextState    = STP::STATE_0;
  stateChanged = true;
	
	
	//Буфер событий тонов
	cursor = 0;
	first  = 0;
	sz     = 0;
	
//	//
//	inflBeg.Reset();
//	inflEnd.Reset();
//	deflBeg.Reset();
//	deflEnd.Reset();
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
	
//	Pmin    = PrsSet(40);
//	Pmax    = 0;
	//	
}
//-----------------------------------------------------------------------------
void ControlPulse::EventNewPulse(PulseEvent& PulseEvent)
{
	if(!ON) return;
	
	if( IsStateChanged() ) 
	{
		currentStateMachine[currentState]->Exit();
		currentState = nextState;
		currentStateMachine[currentState]->Enter();
	}
	
	currentStateMachine[currentState]->NewPulse(PulseEvent);
}
//-----------------------------------------------------------------------------
void ControlPulse::EventTick()
{
	if(!ON) return;
	
	if( IsStateChanged() ) 
	{
		currentStateMachine[currentState]->Exit();
		currentState = nextState;
		// createDebugMarkStepPulse(currentState);
		currentStateMachine[currentState]->Enter();
	}	
	
	currentStateMachine[currentState]->Tick();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------








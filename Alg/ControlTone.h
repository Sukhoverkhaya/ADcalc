// (определение некоторых классов и методов, объявленных в ControlTone.h)
// сделано на основании ControlTone.cpp из firmware, но без обертки для железа

#include "ControlToneD.h"
#include "arithm.h"

const int NO_BAD_TILL_PRESS        = 70;  // границы, в которых гарантированно
const int NO_END_SEARCH_TILL_PRESS = 120; // должно производиться измерение (??)

#define PrsSet(c)  (1000*c) 	// установить значение давления // во всем проекте регистры давления в	mmHg*1000 

// Накачка

// 0. Ждем, пока придет первая хороший тон при давлении выше 30, чтобы переключть в State1
struct StateToneInfl0 : BaseStateTone // наследуем от BaseStateTone
{
    // при передаче в конструктор ссылки на объект типа ControlTone
    // запускаем конструктор класса BaseStateTone c тем же элементом на входе
	StateToneInfl0(ControlTone& sm) : BaseStateTone(sm) {}
	
	void NewTone(ToneEvent& toneEvent) // пришло новое событие тон
	{
		sm.buf[sm.cursor] = toneEvent;
		
        // в оригинале берётся давление из GET_SERVICE(BpmMediator)->PrsMsr,
        // но тут берём поля press пришедшего на вход события тон
		if( toneEvent.press > PrsSet(30) && !sm.buf[sm.cursor].bad)
		{		
			sm.Nb = 1;
			sm.ChangeState(STT::STATE_1);
			sm.i1 = sm.cursor;
		}
		
		++sm.cursor %= sm.rail;
		// ++sm.sz;
		
	}
	
};

// 1. Зашли в рабочую зону и начали копить хорошие пульсации.
// когда находим первую значимую пульсацию, переключаем в состояние 2
// и записываем диастолическое в sm.inflBeg	
struct StateToneInfl1 : BaseStateTone // начинаем копить хорошие пульсации
{
	StateToneInfl1(ControlTone& sm) : BaseStateTone(sm) {}
								
	void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;		
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;
			// ++sm.sz;				
			
			return;
		}
			
		//Если хорошая
		sm.medBuf[sm.medCursor] = sm.buf[sm.cursor].val;
		++sm.medCursor %= sm.medRail;
		sm.Nb++;

		if( sm.Nb > 1 &&                  
			(sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos) > sm.wait * sm.Fs) // 2 пульсации - 3 секунды
		{
			sm.Nb = 1;
		}			
		
		if ( sm.Nb == 3 ) //есть 3 пульсации - продолжаем
		{	
			//среднее по двум пульсациям
			sm.maxLvl = sm.buf[sm.cursor].val + sm.buf[sm.i1].val;
			sm.maxLvl /= 2;
			
			if( sm.buf[sm.i2].val > sm.maxLvl / 5) //первая значимая
			{
				sm.inflBeg  = sm.buf[sm.i2]; //запоминаем пульсацию начала диастолич.
				sm.ChangeState(STT::STATE_2);
			}
			else
			{
				sm.Nb = 2;
			}
			
			sm.ilast = sm.cursor; 
			sm.i3s   = sm.cursor;
		}
		
		//итерация указателей на последние 3 хороших
		sm.i2 = sm.i1;
		sm.i1 = sm.cursor;
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;			
	}
	
};	

struct StateToneInfl2 : BaseStateTone
{
    int32_t badTime;
	int32_t timer;
	const int maxBadTime;

	StateToneInfl2(ControlTone& sm) 
	:  BaseStateTone(sm)
		,badTime(0)
		,timer(0)
		,maxBadTime(sm.Fs*5) 
		{}
				
	void Enter()
	{
		timer = 0;
		badTime = 0;
		//МЕТКА
		// _MARK( createLowBoundary(ADType_Tone, ADDir_INFL); )
        // return 1001;
		//МЕТКА
	}
		
	void NewTone(ToneEvent& toneEvent) 
	{
		// Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;
		
		if(sm.buf[sm.cursor].val > sm.maxLvl / 5) // *0.2
		{
			sm.i3s = sm.cursor; // индекс последнего значимого пика
			timer = 0; // Перезапускаем таймер
			if ( !sm.buf[sm.cursor].bad ) sm.ilast = sm.cursor;
		}				
		if (!sm.buf[sm.cursor].bad) //пересчитываем средний уровень только по хорошим
		{
			//пересчет-2, по медиане из 3-х
			sm.medBuf[sm.medCursor] = sm.buf[sm.cursor].val; //заполнение буфера
			++sm.medCursor %= sm.medRail;
			sm.maxLvl = fnMax(med3(sm.medBuf), sm.maxLvl); 
		}

		//критерий браковки по времени
		if(toneEvent.press < PrsSet(NO_BAD_TILL_PRESS)) //До NO_BAD_TILL_PRESS браковка не работает
		{
			//NOP
		}
		else if(sm.buf[sm.cursor].bad) // Если плохая
		{
			badTime += sm.buf[sm.cursor].pos - sm.buf[sm.cursor - 1].pos;
			if( badTime > maxBadTime ) //измерение забраковано по макс интервалу бракованных пиков
			{
				sm.ChangeState(STT::STATE_FAIL);
			}			
		}
		else if(badTime > 0) //Если хорошая
		{
			badTime += sm.buf[sm.cursor].pos - sm.buf[sm.cursor - 1].pos; //NOTE(romanm): Мы уже не считаем буферы круговыми, так что так можно
			if( badTime > maxBadTime ) //измерение забраковано по макс интервалу бракованных пиков
			{
				sm.ChangeState(STT::STATE_FAIL);
			}	
			badTime = 0;
		}
		
			
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;
	}
	
	void Tick() // пока подаем PrsMsr снаружи
	{
		timer++;
		// cerr << timer << endl;
		if( sm.buf[sm.cursor].press < PrsSet(NO_END_SEARCH_TILL_PRESS) ) return;

		if( timer > sm.wait * sm.Fs )
		// if((sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos) > (sm.wait * sm.Fs))
		{
			timer = 0;	
			sm.inflEnd = sm.buf[sm.ilast];
			
			//проверка, что (сист*0.15) > (сист - диаст) или систалическое меньше 60, тогда продолжаем поиск
			if( sm.inflEnd.press * 3 / 20 > (sm.inflEnd.press - sm.inflBeg.press) )  
			{
				sm.ChangeState(STT::STATE_1);
			}
			else
			{
				sm.ChangeState(STT::STATE_SUCCESS);
			}
		}
	}
	
};	

struct StateToneInflSuccess : BaseStateTone
{
	StateToneInflSuccess(ControlTone& sm) : BaseStateTone(sm) {}
		
	void Enter()
	{
		//МЕТКА
		// _MARK( createHighBoundary(ADType_Tone, ADDir_INFL); )
        // return 10002;
		//МЕТКА
		//sm.savedSzInfl = sm.sz;
		sm.InflSuccess = true;
	}				
		
	void Tick()
	{
		sm.Off();
	}
	
};

struct StateToneInflFail : BaseStateTone
{
	StateToneInflFail(ControlTone& sm) : BaseStateTone(sm) {}
		
	void Tick()
	{
		sm.Off();
	}
	
};


// СПУСК
struct StateToneDefl0 : BaseStateTone
{
	StateToneDefl0(ControlTone& sm) : BaseStateTone(sm) {}
				
	void Enter() 
	{
		// sm.Pmax = Prs;
	}
		
	void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;

		// cerr << toneEvent.press << " " << sm.Pmax - PrsSet(5) << endl;
		
		if( toneEvent.press < sm.Pmax - PrsSet(5) && !sm.buf[sm.cursor].bad)
		{		
			sm.Nb = 1;
			sm.i1 = sm.cursor;
			sm.i2 = sm.cursor; /// from skv
			sm.ChangeState(STT::STATE_1);
		}
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;		
	}
		
};	

struct StateToneDefl1 : BaseStateTone
{
	StateToneDefl1(ControlTone& sm) : BaseStateTone(sm) {}
		
	void Enter()
	{	
		//МЕТКА
		// _MARK( createHighBoundary(ADType_Tone, ADDir_DEFL); )
        // return 10003;
		//МЕТКА
	}
		
	void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;			
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;
			// ++sm.sz;				
			
			return;
		}
			
		// Если хорошая
		sm.medBuf[sm.medCursor] = sm.buf[sm.cursor].val;
		++sm.medCursor %= sm.medRail;
		sm.Nb++;

		if( sm.Nb > 1 &&                  
			sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos > sm.wait * sm.Fs) // 2 пульсации - 3 секунды
		{
			sm.Nb = 1;
		}			
		
		if ( sm.Nb == 3 ) // есть 3 пульсации - продолжаем
		{	
			//среднее по двум пульсациям
			sm.maxLvl = sm.buf[sm.cursor].val + sm.buf[sm.i1].val;
			sm.maxLvl /= 2;
			
			if( sm.buf[sm.i2].val > sm.maxLvl / 5) //первая значимая
			{
				sm.deflBeg  = sm.buf[sm.i2]; //запоминаем пульсацию начала диастолич.
				sm.ChangeState(STT::STATE_2);
			}
			else
			{
				sm.Nb = 2;
			}
			
			sm.ilast = sm.cursor; 
			sm.i3s   = sm.cursor;
			
		}
		
		//итерация указателей на последние 3 хороших
		sm.i2 = sm.i1;
		sm.i1 = sm.cursor;
		
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;			

	}
		
};	

struct StateToneDefl2 : BaseStateTone
{
    int32_t badTime;
	int32_t timer;
	const int maxBadTime;

	StateToneDefl2(ControlTone& sm)
	:  BaseStateTone(sm)
		,badTime(0)
		,timer(0)
		,maxBadTime(sm.Fs*5) 
		{}
		
	void Enter()
	{
		timer = 0;
		badTime = 0;		
//		//МЕТКА
//		_MARK( createHighBoundary(ADType_Tone, ADDir_DEFL); )
        // return 1004;
//		//МЕТКА
	}
				
	void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;
		
		if( sm.buf[sm.cursor].val > sm.maxLvl / 5 ) //*0.2
		{
			sm.i3s = sm.cursor; //индекс последнего значимого пика
			timer = 0; //Перезапускаем таймер
			if ( !sm.buf[sm.cursor].bad  ) sm.ilast = sm.cursor;
		}				
		if ( !sm.buf[sm.cursor].bad ) //пересчитываем средний уровень только по хорошим
		{
			//пересчет-2, по медиане из 3-х
			sm.medBuf[sm.medCursor] = sm.buf[sm.cursor].val; //заполнение буфера
			++sm.medCursor %= sm.medRail;
			sm.maxLvl = fnMax( med3(sm.medBuf), sm.maxLvl); 
		}

		//критерий браковки по времени
		if( sm.buf[sm.cursor].bad)
		{
			badTime += sm.buf[sm.cursor].pos - sm.buf[sm.cursor - 1].pos;
			if( badTime > maxBadTime ) //измерение забраковано по макс интервалу бракованных пиков
			{
				sm.ChangeState(STT::STATE_FAIL);
			}			
		}
		else //NOTE(romanm): Эта ветка бредовая
		{
			if(badTime > 0)
			{
				badTime += sm.buf[sm.cursor].pos - sm.buf[sm.cursor - 1].pos; //NOTE(romanm): Мы уже не считаем буферы круговыми, так что так можно
				if( badTime > maxBadTime ) //измерение забраковано по макс интервалу бракованных пиков
				{
					sm.ChangeState(STT::STATE_FAIL);
				}	
				badTime = 0;
			}
		}
			
		//Двигаем буфер
		++sm.cursor %= sm.rail;
		// ++sm.sz;
	}
	
	void Tick()
	{
		timer++;
		if( timer > sm.wait * sm.Fs )
		// if((sm.buf[sm.cursor].pos - sm.buf[sm.i1].pos) > (sm.wait * sm.Fs))
		{
			timer = 0;	
			sm.deflEnd = sm.buf[sm.ilast];
			
			//проверка, что (сист*0.15) > (сист - диаст)
			if( sm.deflBeg.press * 3 / 20 > ( sm.deflBeg.press - sm.deflEnd.press))  
			{
				sm.ChangeState(STT::STATE_1);
			}
			else
			{
				sm.ChangeState(STT::STATE_3);
			}
		}
	}
};	
	

struct StateToneDefl3 : BaseStateTone
{
	StateToneDefl3(ControlTone& sm) : BaseStateTone(sm) {}
		
	void Enter()
	{
		//МЕТКА
		// _MARK( createLowBoundary(ADType_Tone, ADDir_DEFL); )
        // return 1005;
		//МЕТКА
		//sm.savedSzDefl = sm.sz;
		sm.DeflSuccess = true;
	}		
		
	void Tick()
	{
		if( sm.deflEnd.press - PrsSet(10) > sm.buf[sm.cursor].press ||  PrsSet(30) > sm.buf[sm.cursor].press ) //?
		{
			//sm.Pmax = GET_SERVICE(BpmMediator)->PrsMsr;
			sm.ChangeState(STT::STATE_SUCCESS);
			sm.stopDefl = true;
		}
	}
				
	void NewTone(ToneEvent& ToneEvent) 
	{ 
		//Добавляем пульсацию в буфер
		if( sm.cursor >= sm.rail ) return;
		sm.buf[sm.cursor] = ToneEvent;
		//Двигаем буфер
		++sm.cursor %= sm.rail;
	}
	
};

struct StateToneDeflSuccess : BaseStateTone
{
	StateToneDeflSuccess(ControlTone& sm) : BaseStateTone(sm) {}
		
	void Tick()
	{
		sm.Off();
	}
	
};

struct StateToneDeflFail : BaseStateTone
{
	StateToneDeflFail(ControlTone& sm) : BaseStateTone(sm) {}
		
	void Tick()
	{
		sm.stopDefl = true;
		sm.Off();
	}
	
};


//-----------------------------------------------------------------------------
ControlTone::
ControlTone(const int _fs)
: Fs(_fs)
{
// 	//State Infl
// 	stateArrayInfl[STT::STATE_0] =       new StateToneInfl0(*this);
// 	stateArrayInfl[STT::STATE_1] =       new StateToneInfl1(*this);
// 	stateArrayInfl[STT::STATE_2] =       new StateToneInfl2(*this);	
// //	stateArrayInfl[STT::STATE_3] =       new StateToneInfl3(*this);
// 	stateArrayInfl[STT::STATE_SUCCESS] = new StateToneInflSuccess(*this);
// 	stateArrayInfl[STT::STATE_FAIL] =    new StateToneInflFail(*this);
	                                             
// 	//State Defl                                 
// 	stateArrayDefl[STT::STATE_0] =       new StateToneDefl0(*this);
// 	stateArrayDefl[STT::STATE_1] =       new StateToneDefl1(*this);
// 	stateArrayDefl[STT::STATE_2] =       new StateToneDefl2(*this);	
// 	stateArrayDefl[STT::STATE_3] =       new StateToneDefl3(*this);
// 	stateArrayDefl[STT::STATE_SUCCESS] = new StateToneDeflSuccess(*this);
// 	stateArrayDefl[STT::STATE_FAIL] =    new StateToneDeflFail(*this);
	

	// currentStateMachine = stateArrayInfl;
	buf = bufInfl;
	stopInfl = false;
	stopDefl = false;
	
	inflBeg.Reset();
	inflEnd.Reset();
	deflBeg.Reset();
	deflEnd.Reset();	
	
	Reset();
}
//-----------------------------------------------------------------------------
void ControlTone::Reset()
{
    // currentState = STT::STATE_0;
  nextState    = STT::STATE_0;
  stateChanged = true;
	
	//Буфер событий тонов
	cursor = 0;
	first  = 0;
	sz     = 0;
	
	//
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
	maxLvl = 0;
	Nb      = 0;
	//
	Pmax    = 0;

	medCursor = 0;
}
//-----------------------------------------------------------------------------
// void ControlTone::EventNewTone(ToneEvent& toneEvent)
// {
// 	if(!ON) return;
	
// 	if( IsStateChanged() ) 
// 	{
// 		currentStateMachine[currentState]->Exit();
// 		currentState = nextState;
// 		currentStateMachine[currentState]->Enter();
// 	}
	
// 	currentStateMachine[currentState]->NewTone(toneEvent);
// }
// //-----------------------------------------------------------------------------
// void ControlTone::EventTick()
// {
// 	if(!ON) return;
	
// 	if( IsStateChanged() ) 
// 	{
// 		currentStateMachine[currentState]->Exit();
// 		currentState = nextState;
// 		createDebugMarkStepTone(currentState);
// 		currentStateMachine[currentState]->Enter();
// 	}	
	
// 	currentStateMachine[currentState]->Tick();
// }
// //-----------------------------------------------------------------------------
// //-----------------------------------------------------------------------------






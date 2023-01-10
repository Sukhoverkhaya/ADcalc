// на основании ControlTone.h из firmware, но без обертки для железа
#pragma once

#include "StateMachine.h"
#include "ToneDetect.h"
#include "arithm.h"

#define PrsSet(c)  (1000*c) 	// установить значение давления // во всем проекте регистры давления в	mmHg*1000 

const int NO_BAD_TILL_PRESS        = 70;  // границы, в которых гарантированно
const int NO_END_SEARCH_TILL_PRESS = 120; // должно производиться измерение (??)

struct STT // Состояния коненчого автомата для тонов
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

class ControlTone : public ST  // Наследование от ST из StateMachine
{	
public:

	const int32_t Fs;               // частота дискретизации
	STT::States nextState;	        // состояние (одно из States)
	
	static const int32_t wait = 3;   // ожидание следующего тона
	static const int32_t rail = 100; // размер буфера
	
	ToneEvent* buf;                  // указатель на первый элемент буфера
	
	ToneEvent bufInfl[rail];
	ToneEvent bufDefl[rail];
	bool InflSuccess;
	bool DeflSuccess;
// _____________________________________
	int32_t sz;
	int32_t cursor;
	int32_t first;

	int32_t ilast;
	int32_t i1;
	int32_t i2;
	int32_t i3s;
	//
	int32_t maxLvl;
	int32_t Nb;
	//
	int32_t Pmax;

	// Буфер для вычисления медианы
	static const int medRail = 5;
	int32_t medBuf[medRail];
	int32_t medCursor;
	//
// _____________________________________
	bool stopInfl;
	bool stopDefl;

	//Results	        // если правильно понимаю, 
	ToneEvent inflBeg; 	// inflEnd.startMarkPos, inflBeg.startMarkPos
	ToneEvent inflEnd;	// - САД и ДАД на накачке соответственно (??? ПРЕДПОЛОЖИТЕЛЬНО)
	ToneEvent deflBeg;	// (аналогично на спуске)
	ToneEvent deflEnd;
// _____________________________________

	ControlTone(const int _fs)
	: Fs(_fs)
	{
		buf = bufInfl;
		stopInfl = false;
		stopDefl = false;
		
		inflBeg.Reset();
		inflEnd.Reset();
		deflBeg.Reset();
		deflEnd.Reset();	
		
		Reset();
	};

	inline void Reset()
	{
		nextState    = STT::STATE_0;
		stateChanged = true;
			
		cursor = 0;
		first  = 0;
		sz     = 0;

		ilast = 0;
		i1    = 0;
		i2    = 0;
		i3s   = 0;

		maxLvl = 0;
		Nb      = 0;

		Pmax    = 0;

		medCursor = 0;
	};

	inline void StartInflST() // начало оценки на накачке
	{
		Reset(); // ресет текущего класса
		On();    // унаследовано от ST (получаем ON = true)

		buf = bufInfl; // буфер на 100 значений ToneEvent
		stopInfl = false;
		stopDefl = false;

		InflSuccess = 0;
		DeflSuccess = 0;
		
		inflBeg.Reset(); // ресет класса ToneEvent
		inflEnd.Reset();
		deflBeg.Reset();
		deflEnd.Reset();	
	}

	inline void StartDeflST() // начало оценки на спуске
	{
		Reset(); // ресет текущего класса
		On();    // унаследовано от ST (получаем ON = true)
		buf = bufDefl;  // буфер на 100 значений ToneEvent
	}	
//______________________________________________
	
	inline void ChangeState(STT::States _state) // изменение состояния на одно из перечисления States
	{
		nextState   = _state;
		stateChanged = true;
	}
	
	bool IsStateChanged() // если состояние изменилось, возвращаем true и изменяем маркер переключения состояния на false 
                            // не вернет тру, пока не будет stateChanged, то есть пока не будет вызван метод ChangeState
	{
		if(stateChanged)
		{
			stateChanged = false;
			return true;
		}
		
		return false;
	}	
};

struct BaseStateTone
{
public:
    ControlTone& sm;

    BaseStateTone(ControlTone& _sm) : sm(_sm) { } 

    virtual inline void Tick()      { };
    virtual inline void NewTone(ToneEvent& toneEvent) { };
    virtual inline void Enter()     { };
};

//_________________________________________________________________
// Накачка

// 0. Ждем, пока придет первая хороший тон при давлении выше 30, чтобы переключть в State1
struct StateToneInfl0 : BaseStateTone // наследуем от BaseStateTone
{
    // при передаче в конструктор ссылки на объект типа ControlTone
    // запускаем конструктор класса BaseStateTone c тем же элементом на входе
	StateToneInfl0(ControlTone& sm) : BaseStateTone(sm) {}
	
	inline void NewTone(ToneEvent& toneEvent) // пришло новое событие тон
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
	}
	
};

// 1. Зашли в рабочую зону и начали копить хорошие пульсации.
// когда находим первую значимую пульсацию, переключаем в состояние 2
// и записываем диастолическое в sm.inflBeg	
struct StateToneInfl1 : BaseStateTone // начинаем копить хорошие пульсации
{
	StateToneInfl1(ControlTone& sm) : BaseStateTone(sm) {}
								
	inline void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;		
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;		
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
				
	inline void Enter()
	{
		timer = 0;
		badTime = 0;
	}
		
	inline void NewTone(ToneEvent& toneEvent) 
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
	}
	
	inline void Tick() // пока подаем PrsMsr снаружи
	{
		timer++;
		if( sm.buf[sm.cursor].press < PrsSet(NO_END_SEARCH_TILL_PRESS) ) return;

		if( timer > sm.wait * sm.Fs )
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
		
	inline void Enter() {sm.InflSuccess = true;}				
		
	inline void Tick() {sm.Off();}
	
};

struct StateToneInflFail : BaseStateTone
{
	StateToneInflFail(ControlTone& sm) : BaseStateTone(sm) {}
		
	inline void Tick() {sm.Off();}
	
};


// СПУСК
struct StateToneDefl0 : BaseStateTone
{
	StateToneDefl0(ControlTone& sm) : BaseStateTone(sm) {}
		
	inline void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;
		
		if( toneEvent.press < sm.Pmax - PrsSet(5) && !sm.buf[sm.cursor].bad)
		{		
			sm.Nb = 1;
			sm.i1 = sm.cursor;
			sm.i2 = sm.cursor; /// from skv
			sm.ChangeState(STT::STATE_1);
		}
		
		//Двигаем буфер
		++sm.cursor %= sm.rail;
	}
		
};	

struct StateToneDefl1 : BaseStateTone
{
	StateToneDefl1(ControlTone& sm) : BaseStateTone(sm) {}
		
	inline void NewTone(ToneEvent& toneEvent) 
	{
		//Добавляем пульсацию в буфер
		sm.buf[sm.cursor] = toneEvent;			
		
		//Если пришла плохая - ничего не делаем
		if( sm.buf[sm.cursor].bad )
		{
			//Двигаем буфер
			++sm.cursor %= sm.rail;			
			
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
		
	inline void Enter()
	{
		timer = 0;
		badTime = 0;		
	}
				
	inline void NewTone(ToneEvent& toneEvent) 
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
	
	inline void Tick()
	{
		timer++;
		if( timer > sm.wait * sm.Fs )
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
		
	inline void Enter() { sm.DeflSuccess = true;}		
		
	inline void Tick()
	{
		if( sm.deflEnd.press - PrsSet(10) > sm.buf[sm.cursor].press ||  PrsSet(30) > sm.buf[sm.cursor].press ) //?
		{
			sm.ChangeState(STT::STATE_SUCCESS);
			sm.stopDefl = true;
		}
	}
				
	inline void NewTone(ToneEvent& ToneEvent) 
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
		
	inline void Tick() {sm.Off();}
	
};

struct StateToneDeflFail : BaseStateTone
{
	StateToneDeflFail(ControlTone& sm) : BaseStateTone(sm) {}
		
	inline void Tick()
	{
		sm.stopDefl = true;
		sm.Off();
	}
	
};

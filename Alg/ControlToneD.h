// на основании ControlTone.h из firmware, но без обертки для железа
#pragma once

#include "StateMachine.h"
#include "ToneDetect.h"

// классы ниже прописаны в cpp
// накачка
class  StateToneInfl0; 
class  StateToneInfl1;  
class  StateToneInfl2;  
class  StateToneInfl3;	
class  StateToneInflSuccess;
class  StateToneInflFail;

// спуск
class  StateToneDefl0;
class  StateToneDefl1;
class  StateToneDefl2;		
class  StateToneDefl3;
class  StateToneDeflSuccess;
class  StateToneDeflFail;

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
// class ControlTone : ST  // Наследование от ST из StateMachine
{	
public:
    // friend class  StateToneInfl0;
	// friend class  StateToneInfl1;
	// friend class  StateToneInfl2;
	// friend class  StateToneInfl3;		
	// friend class  StateToneInflSuccess;
	// friend class  StateToneInflFail;

	// friend class  StateToneDefl0;
	// friend class  StateToneDefl1;
	// friend class  StateToneDefl2;		
	// friend class  StateToneDefl3;
	// friend class  StateToneDeflSuccess;
	// friend class  StateToneDeflFail;
// _____________________________________

	const int Fs;               // частота дискретизации
	STT::States nextState;	        // состояние (одно из States)

	// BaseStateTone*  stateArrayInfl[6]; // пока непонятно
	// BaseStateTone*  stateArrayDefl[6];
	// BaseStateTone** currentStateMachine;
	
	static const int wait = 3;   // ожидание следующего тона
	static const int rail = 100; // размер буфера
	
	ToneEvent* buf;                  // указатель на первый элемент буфера
	
	// friend class ControlAd;
	ToneEvent bufInfl[rail];
	ToneEvent bufDefl[rail];
	// int32_t savedSzInfl;
	// int32_t savedSzDefl;
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
	// STT::States currentState; // текущее состояние

	//Results	        // если правильно понимаю, 
	ToneEvent inflBeg; 	// inflEnd.startMarkPos, inflBeg.startMarkPos
	ToneEvent inflEnd;	// - САД и ДАД на накачке соответственно (??? ПРЕДПОЛОЖИТЕЛЬНО)
	ToneEvent deflBeg;	// (аналогично на спуске)
	ToneEvent deflEnd;
// _____________________________________

	ControlTone(const int _fs); // конструктор, на входе частота дискретизации
	void Reset(); // прописана в cpp
	void EventNewTone(ToneEvent& toneEvent); // прописана в cpp, на входе ссылка на объект типа ToneEvent (событие: тон)
	void EventTick(); // прописана в cpp

	void StartInflST() // начало оценки на накачке
	{
		Reset(); // ресет текущего класса
		On();    // унаследовано от ST (получаем ON = true)
		// currentStateMachine = stateArrayInfl; // пока непонятно
		buf = bufInfl; // буфер на 100 значений ToneEvent
		stopInfl = false;
		stopDefl = false;
		// savedSzInfl = 0;
		// savedSzDefl = 0;
		InflSuccess = 0;
		DeflSuccess = 0;
		
		inflBeg.Reset(); // ресет класса ToneEvent
		inflEnd.Reset();
		deflBeg.Reset();
		deflEnd.Reset();	
		
		// createDebugMarkStepTone(currentState);
	}

	void StartDeflST() // начало оценки на спуске
	{
		Reset(); // ресет текущего класса
		On();    // унаследовано от ST (получаем ON = true)
		// currentStateMachine = stateArrayDefl;
		buf = bufDefl;  // буфер на 100 значений ToneEvent
	}	
	
    // закомментив то, что снизу, оставляем метод Off в форме, унаследованной от ST

	// virtual void Off()
	// // {
	// // 	// if(currentStateMachine == stateArrayDefl)
	// // 	// {
	// // 	// 	savedSzDefl =  sz;
	// // 	// }
	// // 	// else
	// // 	// {
	// // 	// 	savedSzInfl = sz;
	// // 	// }
	// 	ST::Off();
	// }	

//______________________________________________
	
	void ChangeState(STT::States _state) // изменение состояния на одно из перечисления States
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
    ControlTone& sm; // ссылка на объект типа ControlTone

    BaseStateTone(ControlTone& _sm) : sm(_sm) { } // конструктор, вход - ссылка на объект типа ControlTone
    
    // методы ниже прописаны в cpp (!! проверить, нужны ли тут)
    virtual void Tick()      { }; // прописаны в cpp (!! проверить, нужен ли тут)
    virtual void NewTone(ToneEvent& toneEvent) { };
    virtual void Enter()     { };
    virtual void Exit()      { };	
};
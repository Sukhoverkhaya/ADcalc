// на основании ControlPulse.h из firmware, но без обертки для железа
#pragma once

#include "StateMachine.h"
#include "PulseDetect.h"

// struct PulseEvent
// {
// public:

// 	bool bad;
// 	int32_t  pos;
// 	int32_t  val;
// 	int32_t  range;
// 	int32_t  press;
// 	int32_t  imax;
// 	int32_t  startMarkPos;
	
// 	void Reset()
// 	{
// 		bad = false;
// 		pos = 0;
// 		val = 0;
// 		range = 0;
//         press = 0;
// 		imax  = 0;
// 		startMarkPos = 0;
// 	}
// };	

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

// class  StatePulseInfl0;
// class  StatePulseInfl1;
// class  StatePulseInfl2;
// class  StatePulseInfl3;		
// class  StatePulseInflSuccess;
// class  StatePulseInflFail;

// class  StatePulseDefl0;
// class  StatePulseDefl1;
// class  StatePulseDefl2;		
// class  StatePulseDefl3;
// class  StatePulseDeflSuccess;
// class  StatePulseDeflFail;

struct BaseStatePulse;
class ControlPulse : public ST
{	
	// friend class ControlAd;	

	// friend class  StatePulseInfl0;	
	// friend class  StatePulseInfl1;
	// friend class  StatePulseInfl2;
	// friend class  StatePulseInfl3;		
	// friend class  StatePulseInflSuccess;
	// friend class  StatePulseInflFail;

	// friend class  StatePulseDefl0;
	// friend class  StatePulseDefl1;
	// friend class  StatePulseDefl2;		
	// friend class  StatePulseDefl3;
	// friend class  StatePulseDeflSuccess;
	// friend class  StatePulseDeflFail;
	
public:

	bool stopInfl;
	bool stopDefl;
	STP::States currentState;

	// Results	
	PulseEvent inflBeg;
	PulseEvent inflEnd;
	PulseEvent deflBeg;
	PulseEvent deflEnd;

	const int Fs;

	STP::States nextState;

	BaseStatePulse*  stateArrayInfl[6];
	BaseStatePulse*  stateArrayDefl[6];
	BaseStatePulse** currentStateMachine;
	
	static const int wait = 3;
	static const int rail = 100;
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
	static const int rangeRail = 5;
	int32_t rangeBuf[rangeRail];
	int32_t rangeCursor;
	//

	ControlPulse(const int _fs);

	// void EventNewPulse(PulseEvent& pulseEvent);
	// void EventTick();
	void StartInflST()
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

		// createDebugMarkStepPulse(currentState);
	}

	void StartDeflST()
	{
		Reset();
		On();
		// currentStateMachine = stateArrayDefl;
		buf = bufDefl;
	}
	
	// void Off()
	// {
	// 	// if(currentStateMachine == stateArrayDefl)
	// 	// {
	// 	// 	savedSzDefl =  sz;
	// 	// }
	// 	// else
	// 	// {
	// 	// 	savedSzInfl = sz;
	// 	// }
	// 	ST::Off();
	// }

	void Reset();

	void ChangeState(STP::States _state)
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

struct BaseStatePulse
{
public:
    ControlPulse& sm;

    BaseStatePulse(ControlPulse& _sm) : sm(_sm) { }
    
    virtual void Tick()      { };
    virtual void NewPulse(PulseEvent& PulseEvent)  { };
    virtual void Enter()     { };
    virtual void Exit()      { };	
};
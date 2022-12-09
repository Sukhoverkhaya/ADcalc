//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// #ifndef _CONTROL_AD_H
// #define _CONTROL_AD_H

// #include "Stm32f10x_Ext.h"		// Библиотека расширенных типов данных

#include "StateMachine.h"
#include "ControlTone.h"     	    //// by skv: используем
// #include "ControlPulse.h"    	//// by skv: используем
// #include "GlobalServices.h"
// #include "PeakDetectorTone.h"
// //-----------------------------------------------------------------------------


struct STAD //Состояния для автоматов пульсаций
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


// class StateAdInfl0;
// class StateAdInfl1;
// class StateAdInflSuccess;
// class StateAdInflFail;
// class StateAdDefl0;
// class StateAdDefl1;
// class StateAdDeflSuccess;
// class StateAdDeflFail;

struct BaseStateAd;

class ControlAd : ST
{	

bool stopInfl;
bool stopDefl;

ControlTone&  controlTone;
// ControlPulse& controlPulse;
STAD::States  currentState;
STAD::States  nextState;

BaseStateAd*  stateArrayInfl[6];
BaseStateAd*  stateArrayDefl[6];
BaseStateAd** currentStateMachine;

// public:	

// 	friend class StateAdInfl0;
// 	friend class StateAdInfl1;
// 	friend class StateAdInflSuccess;
// 	friend class StateAdInflFail;
// 	friend class StateAdDefl0;
// 	friend class StateAdDefl1;
// 	friend class StateAdDeflSuccess;
// 	friend class StateAdDeflFail;

	
// public:
	// ControlAd(ControlTone& _controlTone, ControlPulse& _controlPulse );
	ControlAd(ControlTone& _controlTone);

	void EventNew();
	void EventTick();

	void StartInflST()      // by skv: Старт (инициализация) для режима накачки
	{
		Reset();
		On();
		currentStateMachine = stateArrayInfl;
		stopInfl = false;
		stopDefl = false;
		// controlPulse.StartInflST();  /// by skv: Из ControlPulse: заполнение полей и запуск ресетов
		controlTone. StartInflST();	 /// by skv: Из ControlTone: заполнение полей и запуск ресетов
		// GET_SERVICE(PeakDetectorTone)->Reset();
	}

	void StartDeflST()		 // by skv: Старт (инициализация) для режима спуска
	{
		Reset();
		On();
		currentStateMachine = stateArrayDefl;
		// controlPulse.StartDeflST();
		controlTone.StartDeflST();
		// GET_SERVICE(PeakDetectorTone)->Reset();
	}	
	
// 	void OffAll();

// protected:
	
// 	bool EnoughOffset();

	void Reset();

	virtual void ChangeState(STAD::States _state)    // by skv: Переключатель состояний (?)
	{
		nextState   = _state;
		stateChanged = true;
	}
	
	virtual bool IsStateChanged()                   // by skv: Идентификатор переключения состояний
	{
		if(stateChanged)
		{
			stateChanged = false;
			return true;
		}
		
// 		return false;
	}	

};	

struct BaseStateAd
{
	ControlAd& sm;

	BaseStateAd(ControlAd& _sm) : sm(_sm) { }
	
	virtual void Tick()      { };    // by skv: ! посмотреть, какие функции назначили
	virtual void NewEvent()  { };    // этим методам в исполняемом коде (!! особенно Tick)
	virtual void Enter()     { };
	virtual void Exit()      { };	
	
};





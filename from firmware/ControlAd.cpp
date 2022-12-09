//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#include "ControlAd.h"

// by skv: STT - из ControlTone.h - состояние конечного автомата для тонов

struct StateAdInfl0 : BaseStateAd  // by skv: наследование от класса BaseStateAd: накачка в состоянии 1
{
	StateAdInfl0(ControlAd& sm) : BaseStateAd(sm) {} // наследование конструктора BaseStateAd
	
	virtual void NewEvent() 
	{
		// if( GET_SERVICE(BpmMediator)->PrsMsr < PrsSet(minPressBound) ) return;
		
		const int minPressBound = GET_SERVICE(BpmMediator)->settings->MinPressBound;
		
		if( sm.controlTone.currentState == STT::STATE_SUCCESS && sm.controlPulse.currentState == STP::STATE_SUCCESS)
		{
			if( GET_SERVICE(BpmMediator)->PrsMsr < PrsSet(minPressBound) ) return;
			if( !sm.EnoughOffset()) return;
			
			sm.stopInfl = true;
			sm.ChangeState(STAD::STATE_SUCCESS);
			
			_MARK( createMarkResTone(0, ADDir_INFL, sm.controlTone.inflEnd.press, sm.controlTone.inflBeg.press, sm.controlTone.inflEnd.startMarkPos, sm.controlTone.inflBeg.startMarkPos); )
			_MARK( createMarkResPulse(0, ADDir_INFL, sm.controlPulse.inflEnd.press, sm.controlPulse.inflBeg.press, sm.controlPulse.inflEnd.startMarkPos, sm.controlPulse.inflBeg.startMarkPos);	)
		}
		else if( sm.controlTone.currentState == STT::STATE_SUCCESS )		
		{
			if( GET_SERVICE(BpmMediator)->PrsMsr < PrsSet(minPressBound) ) return;
			if( !sm.EnoughOffset()) return;
						
			sm.stopInfl = true;
			sm.ChangeState(STAD::STATE_SUCCESS);
			sm.controlPulse.Off();
			
			// Метка АД по тонам
			_MARK(  createMarkResTone(0, ADDir_INFL, sm.controlTone.inflEnd.press, sm.controlTone.inflBeg.press, sm.controlTone.inflEnd.startMarkPos, sm.controlTone.inflBeg.startMarkPos); )
			// И если успела метка по пульсациям
			_MARK( 
				u8 errCode = sm.controlPulse.currentState == STP::STATE_SUCCESS ? 0 : 1;
				createMarkResPulse(errCode, ADDir_INFL, sm.controlPulse.inflEnd.press, sm.controlPulse.inflBeg.press, sm.controlPulse.inflEnd.startMarkPos, sm.controlPulse.inflBeg.startMarkPos);			
			)
		}
		else if( sm.controlTone.currentState == STT::STATE_FAIL && sm.controlPulse.currentState == STP::STATE_FAIL )
		{
			if( GET_SERVICE(BpmMediator)->PrsMsr < PrsSet(minPressBound) ) return;
			
			sm.ChangeState(STAD::STATE_FAIL);
			sm.controlTone.Off();
			sm.controlPulse.Off();
			
			_MARK( createMarkResTone (1, ADDir_INFL,  sm.controlTone.inflEnd.press, sm.controlTone.inflBeg.press, sm.controlTone.inflEnd.startMarkPos, sm.controlTone.inflBeg.startMarkPos); )
			_MARK( createMarkResPulse(1, ADDir_INFL, sm.controlPulse.inflEnd.press, sm.controlPulse.inflBeg.press, sm.controlPulse.inflEnd.startMarkPos, sm.controlPulse.inflBeg.startMarkPos); )				
			
		}
		else if( sm.controlPulse.currentState == STP::STATE_SUCCESS )		
		{
			if( GET_SERVICE(BpmMediator)->PrsMsr < PrsSet(minPressBound) ) return;
			if( !sm.EnoughOffset()) return;
			
			sm.stopInfl = true;
			sm.ChangeState(STAD::STATE_SUCCESS);
			sm.controlTone.Off();
			
			//Метка АД по пульсациям
			_MARK(  createMarkResPulse(0, ADDir_INFL, sm.controlPulse.inflEnd.press, sm.controlPulse.inflBeg.press, sm.controlPulse.inflEnd.startMarkPos, sm.controlPulse.inflBeg.startMarkPos); )
			//Метка АД по тонам, провал
			_MARK(  
				u8 errCode = 1;
				createMarkResTone(errCode, ADDir_INFL, sm.controlTone.inflEnd.press, sm.controlTone.inflBeg.press, sm.controlTone.inflEnd.startMarkPos, sm.controlTone.inflBeg.startMarkPos);					
			)
			
		}
		
		
	}
	
};

struct StateAdInfl1 : BaseStateAd
{
	StateAdInfl1(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
	
	}
	
};

struct StateAdInflSuccess : BaseStateAd
{
	StateAdInflSuccess(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
		sm.Off();
	}
	
};

struct StateAdInflFail : BaseStateAd
{
	StateAdInflFail(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
		sm.Off();
	}
	
};


struct StateAdDefl0 : BaseStateAd
{
	StateAdDefl0(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
		
		if( sm.controlTone.currentState == STT::STATE_SUCCESS && sm.controlPulse.currentState == STP::STATE_SUCCESS)
		{
			sm.stopDefl = true;
			sm.ChangeState(STAD::STATE_SUCCESS);	
			
			_MARK( createMarkResPulse(0, ADDir_DEFL, sm.controlPulse.deflBeg.press, sm.controlPulse.deflEnd.press, sm.controlPulse.deflBeg.startMarkPos, sm.controlPulse.deflEnd.startMarkPos); )
			_MARK( createMarkResTone( 0, ADDir_DEFL, sm.controlTone.deflBeg.press, sm.controlTone.deflEnd.press, sm.controlTone.deflBeg.startMarkPos, sm.controlTone.deflEnd.startMarkPos); )
			
		}		
		else if( sm.controlTone.currentState == STT::STATE_SUCCESS && 
								( sm.controlPulse.currentState ==  STP::STATE_0 ||  sm.controlPulse.currentState ==  STP::STATE_1 || sm.controlPulse.currentState ==  STP::STATE_FAIL) )		
		{
			sm.stopDefl = true;
			sm.ChangeState(STAD::STATE_SUCCESS);
			sm.controlPulse.Off();
			
			//Метка АД по тонам
			_MARK(  createMarkResTone(0, ADDir_DEFL, sm.controlTone.deflBeg.press, sm.controlTone.deflEnd.press, sm.controlTone.deflBeg.startMarkPos, sm.controlTone.deflEnd.startMarkPos); )
			//И если успела метка по пульсациям
			_MARK(
				u8 errCode = sm.controlPulse.currentState == STP::STATE_SUCCESS ? 0 : 1;
				createMarkResPulse(errCode, ADDir_DEFL, sm.controlPulse.deflBeg.press, sm.controlPulse.deflEnd.press, sm.controlPulse.deflBeg.startMarkPos, sm.controlPulse.deflEnd.startMarkPos);
			)
			
			
		}
		else if( sm.controlTone.currentState == STT::STATE_FAIL && sm.controlPulse.currentState == STP::STATE_FAIL )
		{
			sm.ChangeState(STAD::STATE_FAIL);
			sm.controlTone.Off();
			sm.controlPulse.Off();
			
			//Метка АД по тонам и пульсациям
			_MARK(  createMarkResTone (1, ADDir_DEFL, sm.controlTone.deflBeg.press,  sm.controlTone.deflEnd.press, sm.controlTone.deflBeg.startMarkPos, sm.controlTone.deflEnd.startMarkPos); )
			_MARK(  createMarkResPulse(1, ADDir_DEFL, sm.controlPulse.deflBeg.press, sm.controlPulse.deflEnd.press, sm.controlPulse.deflBeg.startMarkPos, sm.controlPulse.deflEnd.startMarkPos); )			
		}
		
		else if( sm.controlPulse.currentState == STP::STATE_SUCCESS && 
						( sm.controlTone.currentState ==  STT::STATE_0 ||  sm.controlTone.currentState ==  STT::STATE_1 || sm.controlTone.currentState ==  STT::STATE_FAIL) )
		{
			sm.stopDefl = true;
			sm.ChangeState(STAD::STATE_SUCCESS);
			sm.controlTone.Off();
			
			//Метка АД по пульсациям
			_MARK( createMarkResPulse(0, ADDir_DEFL, sm.controlPulse.deflBeg.press, sm.controlPulse.deflEnd.press, sm.controlPulse.deflBeg.startMarkPos, sm.controlPulse.deflEnd.startMarkPos); )
			//Метка АД по тонам, провал
			_MARK( 
				u8 errCode = 1;
				createMarkResTone(errCode, ADDir_DEFL, sm.controlTone.deflBeg.press, sm.controlTone.deflEnd.press, sm.controlTone.deflBeg.startMarkPos, sm.controlTone.deflEnd.startMarkPos);		
			)
	
		}
	}
};

struct StateAdDefl1 : BaseStateAd
{
	StateAdDefl1(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
	
	}
	
};

struct StateAdDeflSuccess : BaseStateAd
{
	StateAdDeflSuccess(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
		sm.Off();
	}
	
};

struct StateAdDeflFail : BaseStateAd
{
	StateAdDeflFail(ControlAd& sm) : BaseStateAd(sm) {}
	
	virtual void NewEvent() 
	{
		sm.Off();
	}
	
};




//-----------------------------------------------------------------------------
ControlAd::ControlAd(ControlTone& _controlTone, ControlPulse& _controlPulse )
:  controlTone(_controlTone)
	,controlPulse(_controlPulse)
{
	
	//State Infl
	stateArrayInfl[STAD::STATE_0] =       new StateAdInfl0(*this);
	stateArrayInfl[STAD::STATE_1] =       new StateAdInfl1(*this);
	stateArrayInfl[STAD::STATE_SUCCESS] = new StateAdInflSuccess(*this);
	stateArrayInfl[STAD::STATE_FAIL] =    new StateAdInflFail(*this);	
	
	//State Defl
	stateArrayDefl[STAD::STATE_0] =       new StateAdDefl0(*this);
	stateArrayDefl[STAD::STATE_SUCCESS] = new StateAdDeflSuccess(*this);
	stateArrayDefl[STAD::STATE_FAIL] =    new StateAdDeflFail(*this);
	

	currentStateMachine = stateArrayInfl;
	stopInfl = false;
	stopDefl = false;
	
	//	
	Reset();
}
//-----------------------------------------------------------------------------
void ControlAd::Reset()
{
  currentState = STAD::STATE_0;
  nextState    = STAD::STATE_0;
  stateChanged = true;
	
}
//-----------------------------------------------------------------------------
void ControlAd::EventNew()
{
	if(!ON) return;
	
	if( IsStateChanged() ) 
	{
		currentStateMachine[currentState]->Exit();
		currentState = nextState;
		currentStateMachine[currentState]->Enter();
	}
	
	currentStateMachine[currentState]->NewEvent();
}
//-----------------------------------------------------------------------------
void ControlAd::EventTick()
{
	if(!ON) return;
	
	if( IsStateChanged() ) 
	{
		currentStateMachine[currentState]->Exit();
		currentState = nextState;
		currentStateMachine[currentState]->Enter();
	}	
	
	currentStateMachine[currentState]->Tick();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "PostAnalysisTone.h"
#include "PostAnalysisPulse.h"
#include "ControlTone.h"
#include "ControlPulse.h"
#include "Pause.h"
#include "SystemState.h"
void CommandStartPostAnalysis::operator()()
{
//	postAnalysisTone.Do(
//		GET_SERVICE(ControlTone)->bufInfl,
//		GET_SERVICE(ControlTone)->savedSzInfl,
//		GET_SERVICE(ControlTone)->bufDefl,
//		GET_SERVICE(ControlTone)->savedSzDefl	
//	);
//	
//	postAnalysisPulse.Do(
//		GET_SERVICE(ControlPulse)->bufInfl,
//		GET_SERVICE(ControlPulse)->savedSzInfl,
//		GET_SERVICE(ControlPulse)->bufDefl,
//		GET_SERVICE(ControlPulse)->savedSzDefl	
//	);
//	
//	Pause(1000);//Пауза чтобы успели записатся метки, главное чтобы не была вызвана из того потока в который должны метки записаться
//  GET_SERVICE(AdcTask)->EventFromTask( CommandTonePresOff() );
//++++	Pause(15000);//Блокируем задачу BpmTask на 15 сек
//++++	extern volatile bool GlobalBpmAlgBusy;
//++++	GlobalBpmAlgBusy = false;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "ControlTone.h" 
#include "ControlPulse.h" 
#include "WorkMode.h"

struct Event
{
    int pos;
    int amp;
};

struct AD
{
    Event SAD;
    Event DAD;
};

struct Res
{
    int32_t a1;
    int32_t a2;

    Res(int32_t a, int32_t b)
    {
        a1 = a;
        a2 = b;
    }
};

enum ADResult
{
    FAIL = -1,
    NOTHING,
    inflDAD,
    inflSAD,
    deflSAD,
    deflDAD,
};

class ControlAd
{
public:
    STP::States statePulse;
    STT::States stateTone;

    AD inflAD;
    AD deflAD;
    int Fs;

    ControlTone& controltone;
    ControlPulse& controlpulse;

    BaseStateTone* StateToneInfl[6];
    BaseStateTone* StateToneDefl[6];

    BaseStatePulse* StatePulseInfl[6];
    BaseStatePulse* StatePulseDefl[6];

    BaseStateTone** CurrentToneStateMachine;
    BaseStatePulse** CurrentPulseStateMachine;

    WorkMode* mode;
    int PressMax; // вершина треугольника давления

    ofstream& writead;

    ADResult res;

    ControlAd(int _fs, ControlTone& _controltone, ControlPulse& _controlpulse, ofstream& _writead) 
    : Fs(_fs), controltone(_controltone), controlpulse(_controlpulse), writead(_writead)
    {
        mode = new WorkMode(_fs);
        PressMax = 0;
        res = NOTHING;

        StateToneInfl[STT::STATE_0] = new StateToneInfl0(_controltone);
        StateToneInfl[STT::STATE_1] = new StateToneInfl1(_controltone);
        StateToneInfl[STT::STATE_2] = new StateToneInfl2(_controltone);
        // StateToneInfl[STT::STATE_3] = new StateToneInfl3(_controltone);
        StateToneInfl[STT::STATE_SUCCESS] = new StateToneInflSuccess(_controltone);
        StateToneInfl[STT::STATE_FAIL] = new StateToneInflFail(_controltone);

        StateToneDefl[STT::STATE_0] = new StateToneDefl0(_controltone);
        StateToneDefl[STT::STATE_1] = new StateToneDefl1(_controltone);
        StateToneDefl[STT::STATE_2] = new StateToneDefl2(_controltone);
        StateToneDefl[STT::STATE_3] = new StateToneDefl3(_controltone);
        StateToneDefl[STT::STATE_SUCCESS] = new StateToneDeflSuccess(_controltone);
        StateToneDefl[STT::STATE_FAIL] = new StateToneDeflFail(_controltone);

        StatePulseInfl[STT::STATE_0] = new StatePulseInfl0(_controlpulse);
        StatePulseInfl[STT::STATE_1] = new StatePulseInfl1(_controlpulse);
        StatePulseInfl[STT::STATE_2] = new StatePulseInfl2(_controlpulse);
        StatePulseInfl[STT::STATE_SUCCESS] = new StatePulseInflSuccess(_controlpulse);
        StatePulseInfl[STT::STATE_FAIL] = new StatePulseInflFail(_controlpulse);

        StatePulseDefl[STT::STATE_0] = new StatePulseDefl0(_controlpulse);
        StatePulseDefl[STT::STATE_1] = new StatePulseDefl1(_controlpulse);
        StatePulseDefl[STT::STATE_2] = new StatePulseDefl2(_controlpulse);
        StatePulseDefl[STT::STATE_3] = new StatePulseDefl3(_controlpulse);
        StatePulseDefl[STT::STATE_SUCCESS] = new StatePulseDeflSuccess(_controlpulse);
        StatePulseDefl[STT::STATE_FAIL] = new StatePulseDeflFail(_controlpulse);

        ResetTone();
        ResetPulse();
    };

    void ResetTone()
    {
        stateTone = STT::STATE_0;
        // notfoundbyTone = true; /// костыль
        // controltone.Reset();
    };

    void ResetPulse()
    {
        statePulse = STP::STATE_0;
        // notfoundbyPulse = true; //// костыль
        // controlpulse.Reset();
    };

    int Exe(ToneEvent& _toneEv) // вызывается только для событий тонов
    {  
        if (controltone.ON) { CurrentToneStateMachine[controltone.nextState] -> NewTone(_toneEv); }
    };

    int Exe(PulseEvent& _pulseEv)  // вызывается только для событий пульсаций
    {
        if (controlpulse.ON) { CurrentPulseStateMachine[controlpulse.nextState] -> NewPulse(_pulseEv);};

    };

    Res Mode(int32_t press, int32_t tone) // вызывается для каждой входящей точки сигнала, а не только для событий тонов или пульсаций
    {
        mode -> ModeControl(press, tone);

        if (mode->inflbeg)
        {
            ResetTone();
            ResetPulse();
            controltone.StartInflST();
            controlpulse.StartInflST();

            CurrentToneStateMachine = StateToneInfl;
            CurrentPulseStateMachine = StatePulseInfl;
        }
        else if (mode->deflbeg)
        {
            ResetTone();
            ResetPulse();
            controltone.StartDeflST();
            controlpulse.StartDeflST();

            CurrentToneStateMachine = StateToneDefl;
            CurrentPulseStateMachine = StatePulseDefl;

            CurrentToneStateMachine[stateTone]->sm.Pmax = mode->PressMax;
            CurrentPulseStateMachine[statePulse]->sm.Pmax = mode->PressMax;
            mode -> Reset();
        };

        if (controltone.ON) 
        {
            CurrentToneStateMachine[controltone.nextState] -> Tick();

            if (controltone.IsStateChanged()) 
            {
                CurrentToneStateMachine[controltone.nextState] -> Enter();
                if (controltone.InflSuccess)
                {
                    cerr << "INFL T " << controltone.inflBeg.pos << " " << controltone.inflBeg.press/1000 << " " << controltone.inflEnd.pos << " " << controltone.inflEnd.press/1000 << endl;
                    controltone.InflSuccess = false;
                    writead << controltone.inflBeg.pos << '	' << controltone.inflBeg.press << '	' << controltone.inflEnd.pos << '	' << controltone.inflEnd.press << endl;
                }
                if (controltone.DeflSuccess)
                {
                    cerr << "DEFL T " << controltone.deflBeg.pos << " " << controltone.deflBeg.press/1000 << " " << controltone.deflEnd.pos << " " << controltone.deflEnd.press/1000 << endl;
                    controltone.DeflSuccess = false;
                    writead << controltone.deflBeg.pos << '	' << controltone.deflBeg.press << '	' << controltone.deflEnd.pos << '	' << controltone.deflEnd.press << endl;
                }
                // CurrentToneStateMachine[controltone.nextState] -> Enter();
                // if (controltone.DeflSuccess)
                // {
                //     controltone.DeflSuccess = false;
                //     return Res(controltone.deflBeg.press/1000, controltone.deflEnd.press/1000);
                // }
                // else
                // {
                //     if (controltone.InflSuccess)
                //     {
                //         controltone.InflSuccess = false;
                //         return Res(controltone.inflBeg.press/1000, controltone.inflEnd.press/1000);
                //     }
                // }
            };
        };

        if (controlpulse.ON) 
        {
            CurrentPulseStateMachine[controlpulse.nextState] -> Tick();

            if (controlpulse.IsStateChanged()) 
            {
                CurrentPulseStateMachine[controlpulse.nextState] -> Enter();
                if (controlpulse.InflSuccess)
                {
                    cerr << "INFL P " << controlpulse.inflBeg.press/1000 << " " << controlpulse.inflEnd.press/1000 << endl;
                    controlpulse.InflSuccess = false;
                }
                if (controlpulse.DeflSuccess)
                {
                    cerr << "DEFL P " << controlpulse.deflBeg.press/1000 << " " << controlpulse.deflEnd.press/1000 << endl;
                    controlpulse.DeflSuccess = false;
                }
                // if (controlpulse.DeflSuccess)
                // {
                //     controlpulse.DeflSuccess = false;
                //     return Res(controlpulse.deflBeg.press/1000, controlpulse.deflEnd.press/1000);
                // }
                // else
                // {
                //     if (controlpulse.InflSuccess)
                //     {
                //         controlpulse.InflSuccess = false;
                //         return Res(controlpulse.inflBeg.press/1000, controlpulse.inflEnd.press/1000);
                //     }
                // }
            };
        };

        // return Res(0,0);
    }
};
#include "ControlTone.h" 
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

// enum Modes
// {
//     WAIT = 0,
//     INFL,
//     DEFL,
// };

enum ADResult
{
    FAIL = -1,
    NOTHING,
    inflDAD,
    inflSAD,
    deflSAD,
    deflDAD,
};

// #define ISVALID(press, tone) ((press > PrsSet(15) && tone > -1e7) ? true : false)

class ControlAd
{
public:
    STT::States state;
    AD inflAD;
    AD deflAD;
    int Fs;
    ControlTone& controltone;

    BaseStateTone* StateToneInfl[3];
    BaseStateTone* StateToneDefl[4];

    BaseStateTone** CurrentStateMachine;

    WorkMode* mode;
    int PressMax; // вершина треугольника давления
    int timer;

    bool notfound;

    ADResult res;

    ControlAd(int _fs, ControlTone& _controltone) 
    : Fs(_fs), controltone(_controltone)
    {
        mode = new WorkMode(_fs);
        PressMax = 0;
        res = NOTHING;

        StateToneInfl[STT::STATE_0] = new StateToneInfl0(_controltone);
        StateToneInfl[STT::STATE_1] = new StateToneInfl1(_controltone);
        StateToneInfl[STT::STATE_2] = new StateToneInfl2(_controltone);

        StateToneDefl[STT::STATE_0] = new StateToneDefl0(_controltone);
        StateToneDefl[STT::STATE_1] = new StateToneDefl1(_controltone);
        StateToneDefl[STT::STATE_2] = new StateToneDefl2(_controltone);
        StateToneDefl[STT::STATE_3] = new StateToneDefl3(_controltone);

        Reset();
    };

    void Reset()
    {
        state = STT::STATE_0;
        timer = 0;
        notfound = true;
        controltone.Reset();
    }

    int Exe(ToneEvent& _toneEv)  // вызывается только для событий тонов (или пульсаций)
    {

        if (mode->mode != WAIT && notfound)
        {
            state = controltone.nextState;
            CurrentStateMachine[state] -> NewTone(_toneEv);
            CurrentStateMachine[state] -> Tick();

            if (controltone.IsStateChanged()) 
            {
                if(!controltone.ON) {CurrentStateMachine[controltone.nextState] -> Enter();};
                if (controltone.nextState == 1) {res = mode->mode == INFL ? inflDAD : deflSAD;}
                else if (controltone.nextState == 4) {res = mode->mode == INFL ? inflSAD : deflDAD; notfound = false;}
            };
        }

        return res;
    };

    void Mode(int32_t press, int32_t tone) // вызывается для каждой входящей точки сигнала, а не только для событий тонов или пульсаций
    {
        mode -> ModeControl(press, tone);
        if (mode->inflbeg)
        {
            Reset();
            CurrentStateMachine = StateToneInfl;
            controltone.On();
        }
        else if (mode->deflbeg)
        {
            Reset();
            CurrentStateMachine = StateToneDefl;
            CurrentStateMachine[state]->sm.Pmax = mode->PressMax;
            mode -> Reset();
            controltone.On();
        }
    }
};
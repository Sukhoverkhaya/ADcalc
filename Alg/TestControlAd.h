#include "ControlTone.h" 

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

enum Modes
{
    WAIT = 0,
    INFL,
    DEFL,
};

enum ADResult
{
    NOTHING = 0,
    inflDAD,
    inflSAD,
    deflSAD,
    deflDAD,
};

#define ISVALID(press, tone) ((press > PrsSet(15) && tone > -1e7) ? true : false)

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

    Modes mode;
    int PressMax; // вершина треугольника давления
    int timer;

    bool notfound;

    ADResult res;

    ControlAd(int _fs, ControlTone& _controltone) 
    : Fs(_fs), controltone(_controltone)
    {
        mode = WAIT;
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

        if (mode == INFL && notfound)
        {
            state = controltone.nextState;
            if (state >= 0 && state <= 2){ // этот if - временный костыль
                CurrentStateMachine[state] -> NewTone(_toneEv);
                CurrentStateMachine[state] -> Tick();

                if (controltone.nextState != state) 
                {
                    if (state == 1) {res = inflDAD;}
                    else if (state == 2 && controltone.nextState == 4) {res = inflSAD; notfound = false;}
                };
            };
        }
        else if (mode == DEFL && notfound)
        {
            state = controltone.nextState;
            if (state >= 0 && state <= 3){ // этот if - временный костыль
                CurrentStateMachine[state] -> NewTone(_toneEv);
                CurrentStateMachine[state] -> Tick();

                if (controltone.nextState != state) 
                {
                    if (state == 1) {res = deflSAD;}
                    else if (state == 3 && controltone.nextState == 4) {res = deflDAD; notfound = false;}
                };
            };
        };

        return res;
    };

    void Mode(int32_t press, int32_t tone) // вызывается для каждой входящей точки сигнала, а не только для событий тонов или пульсаций
    {
        if (mode == INFL){
            if (press >= PressMax)
            {
                PressMax = press;
                timer = 0;
            }
            else 
            {
                timer++;
            }
        };

        bool isval = ISVALID(press, tone);

        if (isval && mode == WAIT) 
        {
            mode = INFL; 
            Reset();
            CurrentStateMachine  = StateToneInfl;
        }
        else if (!isval && mode == DEFL) 
        {
            mode = WAIT;
        }
        else if (isval && mode == INFL && timer > 2*Fs) 
        {
            mode = DEFL; 
            Reset();
            CurrentStateMachine = StateToneDefl;
            CurrentStateMachine[state]->Enter(PressMax);
            PressMax = 0;
        }; 
    };
};
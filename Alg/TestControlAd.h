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
    PUMP,
    DESC,
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
    STT::States nextstate;
    AD inflAD;
    AD deflAD;
    int Fs;
    ControlTone& controltone;
    // StateFunc localstatemachine;
    BaseStateTone* StateToneInfl[6];

    StateToneInfl0* st0;
    StateToneInfl1* st1;
    StateToneInfl2* st2;
    // StateToneInfl3* st3;
    // StateToneInflSuccess* st4;
    // StateToneInflFail* st5;

    StateToneDefl0* std0;
    StateToneDefl1* std1;
    StateToneDefl2* std2;
    StateToneDefl3* std3;
    // StateToneDeflSuccess* std4;
    // StateToneDeflFail std5;


    Modes mode;
    int PressMax; // вершина треугольника давления
    int timer;

    bool notfound;

    ADResult res;

    ControlAd(int _fs, ControlTone& _controltone) 
    : Fs(_fs), controltone(_controltone)
    {

        Reset();
        mode = WAIT;
        PressMax = 0;
        res = NOTHING;
        // StateToneInfl[STT::STATE_0] = new StateToneInfl0(controltone);
        // StateToneInfl[STT::STATE_1] = new StateToneInfl1(controltone);
        // StateToneInfl[STT::STATE_2] = new StateToneInfl2(controltone);
        // StateToneInfl[STT::STATE_SUCCESS] = new StateToneInflSuccess(controltone);

        // testfunc = new StateToneInfl0(controltone);
        st0 = new StateToneInfl0(controltone);
        st1 = new StateToneInfl1(controltone);
        st2 = new StateToneInfl2(controltone);
        // st4 = new StateToneInflSuccess(controltone);

        std0 = new StateToneDefl0(controltone);
        std1 = new StateToneDefl1(controltone);
        std2 = new StateToneDefl2(controltone);
        std3 = new StateToneDefl3(controltone);
        // std4 = new StateToneDeflSuccess(controltone);
    };

    void Reset()
    {
        state = STT::STATE_0;
        nextstate = STT::STATE_0;
        timer = 0;
        notfound = true;
        controltone.Reset();
    }

    int Exe(ToneEvent& _toneEv)  // вызывается только для событий тонов (или пульсаций)
    {

        if (mode == PUMP && notfound)
        {
            state = controltone.nextState;

            if (state == 0)
            {
                st0 -> NewTone(_toneEv);
            }
            else if (state == 1)
            {
                st1 -> NewTone(_toneEv);
                nextstate = controltone.nextState;
                if (nextstate != state) 
                {
                    // cerr << _toneEv.pos << ".1." << _toneEv.press/1000 << endl;
                    res = inflDAD;
                };
            }
            else if (state == 2)
            {
                st2 -> NewTone(_toneEv);
                st2 -> Tick(_toneEv.press);
                nextstate = controltone.nextState;

                if (nextstate != state && nextstate == 4) 
                {
                    // cerr << _toneEv.pos << ".2." << _toneEv.press/1000 << endl;
                    notfound = false;
                    res = inflSAD;
                };
            }
        }
        else if (mode == DESC && notfound)
        {
            state = controltone.nextState;

            if (state == 0)
            {
                std0 -> NewTone(_toneEv);
            }
            else if (state == 1)
            {
                std1 -> NewTone(_toneEv);
                nextstate = controltone.nextState;
                if (nextstate != state) 
                {
                    // cerr << _toneEv.pos << ".1d." << _toneEv.press/1000 << endl;
                    res = deflSAD;
                };
            }
            else if (state == 2)
            {
                std2 -> NewTone(_toneEv);
                std2 -> Tick();
            }
            else if (state == 3)
            {
                std3 -> NewTone(_toneEv);
                std3 -> Tick(_toneEv.press);

                nextstate = controltone.nextState;

                if (nextstate != state && nextstate == 4) 
                {
                    // cerr << _toneEv.pos << ".2d." << _toneEv.press/1000 << endl;
                    notfound = false;
                    res = deflDAD;
                };
            };
        };

        return res;
    };

    void Mode(int32_t press, int32_t tone) // вызывается для каждой входящей точки сигнала, а не только для событий тонов или пульсаций
    {
        if (mode == PUMP){
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

        if (isval && mode == WAIT) {mode = PUMP; Reset();}
        else if (!isval && mode == DESC) 
        {
            // фрагмент спуска закончился - если не получили нужные метки АД, задаем им значения по умолчанию
            // (если сад и дад найдены, в этом моменте значение res должно быть 2)

            // переключаемся на ожидание накачки 
            mode = WAIT;
        }
        else if (isval && mode == PUMP && timer > 2*Fs) 
        {
            // фрагмент накачки закончился - если не получили нужные метки АД, задаем им значения по умолчанию

            // переключаемся на спуск
            mode = DESC; 
            Reset();
            std0->Enter(PressMax);
            PressMax = 0;
        }; 

        return res;
    };
};
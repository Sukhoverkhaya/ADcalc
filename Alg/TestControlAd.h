#include "ControlTone.h" 
#include "ControlPulse.h" 
#include "WorkMode.h"
#include <fstream>

class ControlAd
{
public:

    int32_t Fs;
    int32_t div;

    ControlTone* controltone;
    ControlPulse* controlpulse;

    BaseStateTone* StateToneInfl[6];
    BaseStateTone* StateToneDefl[6];

    BaseStatePulse* StatePulseInfl[6];
    BaseStatePulse* StatePulseDefl[6];

    BaseStateTone** CurrentToneStateMachine;
    BaseStatePulse** CurrentPulseStateMachine;

    WorkMode* mode;

    ofstream& writead;

    ControlAd(int32_t _fs, int32_t _div, ofstream& _writead) 
    : Fs(_fs), div(_div), writead(_writead)
    {
        mode = new WorkMode(_fs);

        controltone = new ControlTone(Fs);
        controlpulse = new ControlPulse(Fs);

        StateToneInfl[STT::STATE_0] = new StateToneInfl0(*controltone);
        StateToneInfl[STT::STATE_1] = new StateToneInfl1(*controltone);
        StateToneInfl[STT::STATE_2] = new StateToneInfl2(*controltone);
        // StateToneInfl[STT::STATE_3] = new StateToneInfl3(controltone);
        StateToneInfl[STT::STATE_SUCCESS] = new StateToneInflSuccess(*controltone);
        StateToneInfl[STT::STATE_FAIL] = new StateToneInflFail(*controltone);

        StateToneDefl[STT::STATE_0] = new StateToneDefl0(*controltone);
        StateToneDefl[STT::STATE_1] = new StateToneDefl1(*controltone);
        StateToneDefl[STT::STATE_2] = new StateToneDefl2(*controltone);
        StateToneDefl[STT::STATE_3] = new StateToneDefl3(*controltone);
        StateToneDefl[STT::STATE_SUCCESS] = new StateToneDeflSuccess(*controltone);
        StateToneDefl[STT::STATE_FAIL] = new StateToneDeflFail(*controltone);

        StatePulseInfl[STP::STATE_0] = new StatePulseInfl0(*controlpulse);
        StatePulseInfl[STP::STATE_1] = new StatePulseInfl1(*controlpulse);
        StatePulseInfl[STP::STATE_2] = new StatePulseInfl2(*controlpulse);
        StatePulseInfl[STP::STATE_SUCCESS] = new StatePulseInflSuccess(*controlpulse);
        StatePulseInfl[STP::STATE_FAIL] = new StatePulseInflFail(*controlpulse);

        StatePulseDefl[STP::STATE_0] = new StatePulseDefl0(*controlpulse);
        StatePulseDefl[STP::STATE_1] = new StatePulseDefl1(*controlpulse);
        StatePulseDefl[STP::STATE_2] = new StatePulseDefl2(*controlpulse);
        StatePulseDefl[STP::STATE_3] = new StatePulseDefl3(*controlpulse);
        StatePulseDefl[STP::STATE_SUCCESS] = new StatePulseDeflSuccess(*controlpulse);
        StatePulseDefl[STP::STATE_FAIL] = new StatePulseDeflFail(*controlpulse);
    };

    inline void Exe(ToneEvent& _toneEv) // вызывается только для событий тонов
    {  
        if (controltone->ON) {CurrentToneStateMachine[controltone->nextState] -> NewTone(_toneEv); }
    };

    inline void Exe(PulseEvent& _pulseEv)  // вызывается только для событий пульсаций
    {
        if (controlpulse->ON) {CurrentPulseStateMachine[controlpulse->nextState] -> NewPulse(_pulseEv);};

    };

    inline void Reset()
    {
        if (mode->inflbeg)
        {
            controltone->StartInflST();
            controlpulse->StartInflST();

            CurrentToneStateMachine = StateToneInfl;
            CurrentPulseStateMachine = StatePulseInfl;
        }
        else if (mode->deflbeg)
        {
            controltone->StartDeflST();
            controlpulse->StartDeflST();

            CurrentToneStateMachine = StateToneDefl;
            CurrentPulseStateMachine = StatePulseDefl;

            CurrentToneStateMachine[STT::STATE_0]->sm.Pmax = mode->PressMax;
            CurrentPulseStateMachine[STP::STATE_0]->sm.Pmax = mode->PressMax;

            mode->Reset();
        };
    };

    inline void WriteAd()
    {
        // запись значений АД в файл (если успешна оценка по пульсациям, то пишем её значения, если нет - пишем значения оценки по тонам (если она успешна))
        // (выполняется при любом переходе между накачкой/спуском и cпуском/ожиданием) 
        if (mode->deflbeg)
        {
            // если успешна оценка по пульсациям на накачке, то пишем её результат
            if (controlpulse->InflSuccess)
            {
                writead << controlpulse->inflBeg.pos*div << '	' << controlpulse->inflBeg.press << '	' << controlpulse->inflEnd.pos*div << '	' << controlpulse->inflEnd.press << endl;
            }
            else // в противном случае, проверяем, успешна ли оценка по тонам на накачке
            {
                if (controltone->InflSuccess) // и если да, то пишем её результат
                {
                    writead << controltone->inflBeg.pos*div << '	' << controltone->inflBeg.press << '	' << controltone->inflEnd.pos*div << '	' << controltone->inflEnd.press << endl;
                }
            }
        }
        else if (mode->waitbeg) // !!!! ПРОБЛЕМА: если запись не заканчивается ожиданием, последнее измерение не будет записано
        {
            // с теми же приоритетами, что на накачке, проверяем спуск
            if (controlpulse->DeflSuccess)
            {
                writead << controlpulse->deflBeg.pos*div << '	' << controlpulse->deflBeg.press << '	' << controlpulse->deflEnd.pos*div << '	' << controlpulse->deflEnd.press << endl;
            }
            else 
            {
                if (controltone->DeflSuccess)
                {
                    writead << controltone->deflBeg.pos*div << '	' << controltone->deflBeg.press << '	' << controltone->deflEnd.pos*div << '	' << controltone->deflEnd.press << endl;
                }
            }
        }
    };

    inline void Mode(int32_t press, int32_t tone) // вызывается для каждой входящей точки сигнала, а не только для событий тонов или пульсаций
    {
        mode->ModeControl(press, tone);

        WriteAd();  // запись результов оценки АД (если есть, что писать)
        Reset();    // обновление состояний алгоритмов в зависимости от режима работы (режим = накачка/спуск/ожидание)

        // если активен алгоритм оценки АД по тонам
        if (controltone->ON) 
        {
            // вызываем метод, который должен выполняться для каждой входящей точки, а не только для событий тонов
            CurrentToneStateMachine[controltone->nextState] -> Tick();

            // если алгоритм перешёл в следующее состояние
            if (controltone->IsStateChanged()) 
            {
                // вызываем метод нового состояния, необходимый для начала работы в нём
                CurrentToneStateMachine[controltone->nextState] -> Enter();
            };
        };

        // если активен алгоритм оценки АД по пульсациям
        if (controlpulse->ON) 
        {
            // вызываем метод, который должен выполняться для каждой входящей точки, а не только для событий тонов
            CurrentPulseStateMachine[controlpulse->nextState] -> Tick();

            // если алгоритм перешёл в следующее состояние
            if (controlpulse->IsStateChanged()) 
            {
                // вызываем метод нового состояния, необходимый для начала работы в нём
                CurrentPulseStateMachine[controlpulse->nextState] -> Enter();
            };
        };


    //     if (controltone->InflSuccess)
    //     {
    //         cerr << "INFL T " << controltone->inflBeg.pos << " " << controltone->inflBeg.press/1000 << " " << controltone->inflEnd.pos << " " << controltone->inflEnd.press/1000 << endl;
    //         controltone->InflSuccess = false;
    //         writead << controltone->inflBeg.pos << '	' << controltone->inflBeg.press << '	' << controltone->inflEnd.pos << '	' << controltone->inflEnd.press << endl;
    //     }
    //     if (controltone->DeflSuccess)
    //     {
    //         cerr << "DEFL T " << controltone->deflBeg.pos << " " << controltone->deflBeg.press/1000 << " " << controltone->deflEnd.pos << " " << controltone->deflEnd.press/1000 << endl;
    //         controltone->DeflSuccess = false;
    //         writead << controltone->deflBeg.pos << '	' << controltone->deflBeg.press << '	' << controltone->deflEnd.pos << '	' << controltone->deflEnd.press << endl;
    //     }
    //     if (controlpulse->InflSuccess)
    //     {
    //         cerr << "INFL P " << controlpulse->inflBeg.press/1000 << " " << controlpulse->inflEnd.press/1000 << endl;
    //         controlpulse->InflSuccess = false;
    //     }
    //     if (controlpulse->DeflSuccess)
    //     {
    //         cerr << "DEFL P " << controlpulse->deflBeg.press/1000 << " " << controlpulse->deflEnd.press/1000 << endl;
    //         controlpulse->DeflSuccess = false;
    //     }
    };
};

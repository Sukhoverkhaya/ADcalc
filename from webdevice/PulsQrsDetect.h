#pragma once
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#include "xLog.h"
//#include <cstring>
#include "Events.h"
//#include "Soft/SignalProcessing/Alg/DiffFilter.h"
//#include "Soft/SignalProcessing/Alg/SumFilter.h"
//#include "Soft/SignalProcessing/Alg/DelayBuf.h"

namespace SignalsProcessing
{

    struct PulsQrsDetect
    {
        AlgorithmEvent&     mQrsEvent;

        inline PulsQrsDetect(AlgorithmEvent& qrs) // детектор пульсаций без привязки к QRS
        : mQrsEvent{qrs}
        {
            xLog() << MY_FUNC;
        }

        const int32_t cTick{1};

        int32_t rgDelay{0};
        //-----------------------------------------------------------------------------
        inline void Start() // Начало цикла детекции периода сигнала
        {
        }

        inline void Stop()
        {
        }
        //-----------------------------------------------------------------------------
        inline int32_t Exe(int32_t prs,int32_t tch) // Детектор c привязкой к детектору QRS
        {
            int32_t res{0};

            if (mQrsEvent.Fact)
            {
                // переделать с учетом mQrsEvent.Shift
                rgDelay = 200;
            }

            if( rgDelay > 0)
            {
                rgDelay--;

                if(rgDelay == 0)
                {
                    res = Pars();      // pacчет параметров пульсовой волны
                }
            }

            return res;
        }

        int32_t Pars(void)  // pacчет параметров
        {
            return 1000;
        }
    };
}
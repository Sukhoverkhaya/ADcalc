#pragma once

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//#include "xLog.h"
//#include "EcoBuf.h"
#include <stdint.h>
#include <type_traits>

namespace SignalsProcessing
{
    struct AlgorithmEvent //TODO возможно AlgorithmEvent нужно убрать!
    {
//        const int32_t longInterval; // максимальный интервал, превышение которого делает событие невалидным
        AlgorithmEvent(){}//(int32_t interval) : longInterval{interval} {}

//        bool    isValid{false}; // разрешение использования данного события
        bool    Fact{false};    // факт события, действует один отсчет АЦП
//        int32_t Interval{0};    // интервал между текущим и предыдущем сообытиями
//        int32_t Shift{0};       // запаздывание детекции по отношению к реальному событию
/*
        inline void Ok() // переход в работоспособное состояние
        {
            isValid = true;
        }
        inline void Fail() // переход в неработоспособное состояние
        {
            isValid = false;
            Fact = false;
        }
*/
        inline void Clear() // сброс предыдущего результата, вызывается в каждой точке сигнала
        {
            Fact = false;
        }

        inline void Set()   //установка факта события (int32_t interval,int32_t shift)
        {
//            if(isValid)
            Fact = true;
        }
    };
}

namespace Events
{
    enum class EventType : int8_t
    {
        AVGHR = 0,
        SEC,
        POINTS,
        SPO,
        ADPULS,
        ADTON,
        MOVE,
        VOL,
        XXX,

        _First_ = AVGHR,   // участник перечисления для первого элемента
        _Last_ = XXX,      // участник перечисления для последнего элемента
        _All_ = (int8_t)0xff       // Используется для фильтров, указывает на все типы меток
    };

    static EventType operator++(EventType& x)
    {
        // std::underlying_type преобразовывает тип ColorTypes в целочисленный тип, под которым данный enum был объявлен
        return x = static_cast<EventType>(std::underlying_type<EventType>::type(x) + 1);
    }

    static EventType operator*(EventType c)
    {
        return c;
    }

    static EventType begin(EventType)
    {
        return EventType::_First_;
    }

    static EventType end(EventType)
    {
        EventType l = EventType::_Last_;
        return ++l;
    }

    constexpr size_t sizeEventType()
    {
        return std::underlying_type<EventType>::type(EventType::_Last_) -  std::underlying_type<EventType>::type(EventType::_First_) + 1;
    }

#pragma pack(push,1)
    struct IEvent
    {
        EventType Type;
        int8_t Size;
        int32_t Point;
    };

    struct IEventInt32 : IEvent
    {
        int32_t Val;
    };

    struct IEventInt32Int32 : IEvent
    {
        int32_t Val1;
        int32_t Val2;
    };

    struct AvgHrEvent : IEventInt32
    {
        inline AvgHrEvent(const int32_t& pnt, const int32_t& val) : IEventInt32{ {EventType::AVGHR, sizeof(AvgHrEvent), pnt} , val}
        {
        }
    };

    struct SpoEvent : IEventInt32
    {
        inline SpoEvent(const int32_t& pnt, const int32_t& val) : IEventInt32{ {EventType::SPO, sizeof(SpoEvent), pnt}, val}
        {
        }

    };
    struct SecEvent : IEventInt32//секундные события
    {
        inline SecEvent(const int32_t & pnt, const int32_t& val) : IEventInt32{ {EventType::SEC, sizeof(SecEvent), pnt}, val}
        {
        }

    };

    //Метка привязки к счетчику точек. Используется для привязки в длках паковки при отсутствии ведущего сигнала.
    //Например в чистом АД, где события могут приходить вне интервалов измерения давления
    //Да, счетчик дублируется в переменных Point и в Val. Оставил это для удобства так как в паковщике Point выбрасывается
    //CHY
    struct PointsEvent : IEventInt32
    {
        inline PointsEvent(const int32_t & pnt) : IEventInt32{ {EventType::POINTS, sizeof(PointsEvent), pnt}, pnt}
        {
        }

    };

    struct VolumeEvent : IEventInt32Int32
    {
        inline VolumeEvent(const int32_t & pnt, const int32_t& val1, const int32_t& val2) : IEventInt32Int32{ {EventType::VOL, sizeof(VolumeEvent), pnt}, val1, val2}
        {
        }

    };


#pragma pack(pop)
}
#pragma once
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <iomanip>
#include <mutex>
#include <tuple>
#include <vector>
#include <map>

//-----------------------------------------------------------------------------
enum class xCOLOR : char { DEFAULT = 0,RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,WHITE };
//-----------------------------------------------------------------------------
struct xTab
{
    int ntab;
    int sz;
    int operator()() { return ntab * sz; }
};

struct xBool
{
    bool flg;
};
//-----------------------------------------------------------------------------
struct xLog
{
    static inline std::mutex Mutex{}; // закрываем все экземпляры
    xCOLOR rgClr{xCOLOR::DEFAULT};

    xLog() { Mutex.lock(); }
    xLog(xCOLOR c) { Mutex.lock(); std::cout << get_esc_color(c); }
    ~xLog()
    {
        std::cout << std::dec;
        std::cout << std::endl;
        std::cout << get_esc_color(xCOLOR::DEFAULT);
        Mutex.unlock();
    }

    xLog& operator<<(std::ios_base&(*z)(std::ios_base&)) { std::cout << z; return *this; }  // перехват, чтобы после спецификатора не вставлять пробел
    xLog& operator<<(xCOLOR c) { std::cout << get_esc_color(c); return *this; }             //
    xLog& operator<<(bool z) { if(z) std::cout << "true"; else std::cout << "false"; return *this; }
    xLog& operator<<(const char* z) { if(z) std::cout << z; else std::cout << "nullptr"; return *this; }
    xLog& operator<<(xTab z) { for(int i = 0; i < z(); i ++) std::cout << " "; return *this; }

    xLog& operator<<(int8_t z) { std::cout << int(z); return *this; }
    xLog& operator<<(uint8_t z) { std::cout << int(z); return *this; }
    xLog& operator<<(xBool z) { xCOLOR cl = rgClr; if(z.flg) { std::cout << get_esc_color(xCOLOR::GREEN) << "true"; } else { std::cout << get_esc_color(xCOLOR::RED) << "false"; } std::cout << get_esc_color(cl); return *this; }

    //----
    inline const char* get_esc_color(xCOLOR color)
    {
        static constexpr char esc[8][6] = { "\x1b[0m","\x1b[31m","\x1b[32m","\x1b[33m","\x1b[34m","\x1b[35m","\x1b[36m","\x1b[37m" };
        rgClr = color;
        return esc[static_cast<int>(color)];
    }
    //----
    xLog& operator<<(const std::string& z) { std::cout << z.data(); return *this; }
    xLog& operator<<(std::basic_string<char>& z) { std::cout << z.data(); return *this; }
    template<typename TF,typename TS> xLog& operator<<(std::tuple<TF,TS>& z) { *this << '{' << std::get<0>(z) << ',' << std::get<1>(z) << '}'; return *this; }
    template<typename TF,typename TS> xLog& operator<<(std::pair<TF,TS>& z) { *this << '{' << z.first << ',' << z.second << '}'; return *this; }

    template<class T> xLog& operator<<(const std::vector<T>& vec)
    {
        std::cout << '{';
        for(size_t i = 0; i < vec.size(); i++)
        {
            if(i>1) std::cout <<';';
            *this << vec[i];
        }
        std::cout << '}';
        return *this;
    }
    template<typename TF,typename TS> xLog& operator<<(const std::map<TF,TS>& map)
    {
        std::cout << '{';
        int i = 0;
        for(const auto& obj : map)
        {
            if(i>1) std::cout <<';';
            *this << '{' << obj.first << ':' << obj.second << '}';
            i++;
        }
        std::cout << '}';
        return *this;
    }
    //----
    template<class T> xLog& operator<<(const T& z) { std::cout << z; return *this; }
};
//-----------------------------------------------------------------------------
#ifdef  __FUNCSIG__
#define MY_FUNC __FUNCTION__
#else
#define MY_FUNC __PRETTY_FUNCTION__
#endif
//-----------------------------------------------------------------------------
template <typename... Args> void xIGNORE(Args&&...) { } // игнорирования списка параметров
//-----------------------------------------------------------------------------
inline void xAssert(bool flg, const char* txt)
{
#ifdef NDEBUG
    xIGNORE(flg,txt);
#else
    if(!flg)
    {
        xLog(xCOLOR::RED) << txt;
        std::exit(1);
    }
#endif
};

inline int xExit(int code)
{
#ifdef NDEBUG
    system("pause");
#endif
    return code;
};
//-----------------------------------------------------------------------------
#define binBuf(obj) {obj,sizeof(obj)}
//-----------------------------------------------------------------------------
inline int operator "" _kB(unsigned long long z)
{
    return static_cast<int>(z * 1024);
}
//-----------------------------------------------------------------------------

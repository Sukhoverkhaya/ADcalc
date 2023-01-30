#pragma once

// коэффициенты

struct FiltOptions
{
    int32_t b[3];
    int32_t a[3];
    int32_t kf;  //коэффициент компенсации амплитуды == a[0]/kf 
};

enum FsOpt // читобы к-ты фильтров с одинаковыми характеристиками выбирались в зависимости от частоты дискретизации
{
    Hz125 = 0,
    Hz250 = 1,
    Hz1000 = 2,
};

// // butter low 2 порядка до 60 Гц
// const FiltOptions decimlowpass_opt_1000 = 
//                         {3,   6,  3,
//                         108,    -159,   63,
//                         108}; // из проги для прибора

// const FiltOptions decimlowpass_opt_250 = 
//                         {11,   22,  11,
//                         40,    -3,   7,
//                         40}; // свой

// // butter high 2 порядка от 30 Гц
// const FiltOptions highpass_opt_1000 = 
//                         {122,  -224,   122,
//                         128,    -222,   98,
//                         128}; // из проги для прибора

// const FiltOptions highpass_opt_250 = 
//                         {32,  -64,   32,
//                         55,    -54,   19,
//                         55}; // свой

// // butter low 2 порядка до 10 Гц
// const FiltOptions lowpass_opt_250 = 
//                         {2, 4,  2,
//                         150,    -247,   105,
//                         150}; // из проги для прибора (сигнал 1000Гц после децимации в 4 раза)

// const FiltOptions lowpass_opt_1000 = 
//                         {1, 2,  1,
//                         1116,    -2133,   1021,
//                         1116}; // свой (если понадобится погонять 1000Гц без децимации)

// // для выделения пульсовой волны (из прибора) butter low 2 порядка до 10 Гц
// const FiltOptions presspls_lowpass_opt_125 = 
//                         {17, 34, 17,
//                         369, -482, 181,
//                         369};

// // butter high от 0.3 Гц
// const FiltOptions pls_highpass_opt_125 = 
//                         {42, -84, 42,
//                         43, -85, 42,
//                         43};

// // butter 2 low до 20 Гц
// const FiltOptions pls_dcmlowpass_opt_1000 = 
//                         {1, 2, 1,
//                         281, -512, 235,
//                         281}; // из проги (для сырого сигнала 1000 Гц)

// const FiltOptions pls_dcmlowpass_opt_250 = 
//                         {3, 6, 3,
//                         65, -85, 32,
//                         65}; // свой (для сырого сигнала 250 Гц)


struct FiltPack
{
    // note: название фильтра по принципу: архитектура_порядок_частота среза_тип
    // каждый фильр имеет набор к-тов для случаев 125 и/или 250 и/или 1000 Гц
    FiltOptions butter_2_60_low[3];
    FiltOptions butter_2_10_low[3];
    FiltOptions butter_2_30_high[3];
    FiltOptions butter_2_03_high[3];
    FiltOptions butter_2_20_low[3];

    FiltPack()
    {
        butter_2_60_low[Hz250] =    FiltOptions    {11,   22,  11,
                                                    40,    -3,   7,
                                                    40};

        butter_2_60_low[Hz1000] =   FiltOptions     {3,   6,  3,
                                                    108,    -159,   63,
                                                    108};

        butter_2_10_low[Hz125] =    FiltOptions     {17, 34, 17,
                                                    369, -482, 181,
                                                    369};

        butter_2_10_low[Hz250] =    FiltOptions     {2, 4,  2,
                                                    150,    -247,   105,
                                                    150};

        butter_2_10_low[Hz1000] =   FiltOptions     {1, 2,  1,
                                                    1116,    -2133,   1021,
                                                    1116};

        butter_2_30_high[Hz250] =   FiltOptions     {32,  -64,   32,
                                                    55,    -54,   19,
                                                    55};

        butter_2_30_high[Hz1000] =  FiltOptions     {122,  -224,   122,
                                                    128,    -222,   98,
                                                    128};

        butter_2_03_high[Hz125] =   FiltOptions     {42, -84, 42,
                                                    43, -85, 42,
                                                    43};

        butter_2_20_low[Hz1000] =   FiltOptions     {1, 2, 1,
                                                    281, -512, 235,
                                                    281};

        butter_2_20_low[Hz250] =    FiltOptions     {3, 6, 3,
                                                    65, -85, 32,
                                                    65};
        
    };
};


struct Filter
{
private:
    FiltOptions opt;
    int32_t StkX[2];
    int32_t StkY[2];
    int32_t pnStk; // указатель для кольцевых стеков StkX и StkY
    FsOpt fs;

public:  

    Filter(int32_t Fs)
    {
        if (Fs == 125) {fs = Hz125;}
        else if (Fs == 250) {fs = Hz250;}
        else if (Fs == 1000) {fs = Hz1000;};

        Reset();
    }

    inline SetOptions(FiltOptions* _opt) {opt = _opt[fs];}; // установка нужного набора коэффициентов 

    inline void Reset() // перезапуск перед началом любого цикла обработки
    {
        pnStk = 0;
        memset( StkX, 0, sizeof(StkX) );
 	    memset( StkY, 0, sizeof(StkX) );
    };

    inline int32_t Exe(int32_t x)
    {
        int32_t p = pnStk;
        int32_t y = 0;
        int32_t* b = opt.b;
        int32_t* a = opt.a;

        y -= StkY[p] * a[2];
        y += StkX[p] * b[2];
        p++; p%=2;

        y -= StkY[p] * a[1];
        y += StkX[p] * b[1];
        p++; p%=2;

        y += x*b[0];

        int32_t z = y/opt.kf; // компенсация амплитуды
        y /= a[0];

        StkY[pnStk] = y;
        StkX[pnStk] = x;
        pnStk++; pnStk %= 2;

        return z;
    }
};

struct Decimator  // Суммирующий дециматор
{
    const int32_t dcm;	// значение дециматора
    // const int32_t numOfChannel; // количество каналов для одновременной децимации	

    int32_t rgS;	// регистр накопления точек
    int32_t  cnt;	// счетчик точек дециматора

    bool rdyFlag;  // флаг готовности децимации
    int32_t out; // запрашивать только когда rdyFlag == true

    Decimator(int32_t _div)
    : dcm(_div) {Reset();};

    inline void Reset()
    {
        rgS = 0;
        cnt = 0;
        rdyFlag = 0;
        rdyFlag = false;
        out = 0;
    }

    inline void Exe(int32_t x)
    {
        rgS += x;
        cnt++;
        rdyFlag = false;

        if (cnt == dcm)
        {
            out = rgS/dcm;
            rgS = 0;
            cnt = 0;
            rdyFlag = true;
        }
    }; 
};

struct BaselineFilter
{
    int32_t k;
	int32_t lv;

	BaselineFilter() {Reset();};

	inline void Reset()
    {
        k = 200;
        lv = 0;
    };

	inline int32_t Exe(int32_t x)
    {
        int32_t err = 0;
        err = k*x - lv; if (err < 0) {err *= 10;}
        lv += (err/k);

        return (x - lv/k * 7/8);
    };
};

struct DiffFilter{ // Дифференциальный фильтр из вебдивайса, с изменениями по примеру Виктора из микробокса
    
    int32_t N;               // длина буфера (в элементах)
    int32_t* buf = nullptr;  // указатель на первый элемент буфера
    int32_t current;
    int32_t last;
    int32_t windowLen;

    DiffFilter(int32_t _bufsize, int32_t _windowlen)
    : N(_bufsize), windowLen(_windowlen)
    {
        buf = new int32_t[N];
        current = 0;

        Reset();
    };

    inline void Reset()
    {
        memset ( buf, 0, N*sizeof(int32_t) );
        last = (current + N - windowLen) % N;
    };

    //-----------------------------------------------------------------------------

    inline int32_t Exe(int32_t x)
    {
        int32_t res = x - buf[last];
        buf[current] = x;
        ++current %= N;
        ++last %= N;

        return res;
    };

};
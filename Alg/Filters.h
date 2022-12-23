#pragma once

// коэффициенты

struct FiltOptions
{
    int32_t b[3];
    int32_t a[3];
    int32_t kf;  //коэффициент компенсации амплитуды == a[0]/kf 
};


// если исходить из того, что к-ты взяты из кода для прибора,
// который, судя по всему, работает с частотой дискретизации 1000 ГЦ,
// то decimlowpass и highpass работают с исходным сигналом в 100 Гц,
// тогда как lowpass применяется уже после прореживания в 4 раза, т.е. на 250 Гц

// предположительно: decimlowpass - butter low 2 порядка до 60 Гц
//                   highpass - butter high 2 порядка от 30 Гц
//                   lowpass - butter low 2 порядка до 10 Гц

const FiltOptions decimlowpass_opt_1000 = {3,   6,  3,
                        108,    -159,   63,
                        108}; // из проги для прибора

const FiltOptions decimlowpass_opt_250 = {11,   22,  11,
                        40,    -3,   7,
                        40}; // свой

const FiltOptions highpass_opt_1000 = {122,  -224,   122,
                        128,    -222,   98,
                        128}; // из проги для прибора

const FiltOptions highpass_opt_250 = {32,  -64,   32,
                        55,    -54,   19,
                        55}; // свой

const FiltOptions lowpass_opt_250 = {2, 4,  2,
                    150,    -247,   105,
                    150}; // из проги для прибора

const FiltOptions lowpass_opt_1000 = {1, 2,  1,
                    1116,    -2133,   1021,
                    1116}; // свой (если понадобится погонять 1000Гц без децимации)


struct Filter
{
private:
    FiltOptions opt;
    int32_t StkX[2];
    int32_t StkY[2];
    int32_t pnStk; // указатель для кольцевых стеков StkX и StkY


    inline SetOptions(FiltOptions _opt) {opt = _opt;}; // установка нужного набора коэффициентов

public:

    // оформить лаконичнее, не через такой хардкод
    inline SetDecimLowpass(int32_t _fs) 
    {   if (_fs == 1000) SetOptions(decimlowpass_opt_1000); Reset();
        if (_fs == 250) SetOptions(decimlowpass_opt_250); Reset();}; 
    inline SetHighPass(int32_t _fs) 
    {   if (_fs == 1000) SetOptions(highpass_opt_1000); Reset();
        if (_fs == 250) SetOptions(highpass_opt_250); Reset();};   
    inline SetLowPass(int32_t _fs) 
    {   if (_fs == 1000) SetOptions(lowpass_opt_1000); Reset();
        if (_fs == 250) SetOptions(lowpass_opt_250); Reset();};     

    inline Reset() // перезапуск перед началом любого цикла обработки
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

    inline Reset()
    {
        rgS = 0;
        cnt = 0;
        rdyFlag = 0;
        rdyFlag = false;
        out = 0;
    }

    inline Exe(int32_t x)
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

	inline Reset()
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
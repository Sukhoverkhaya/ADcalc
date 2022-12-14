struct InputPack{   // Фиксированный набор точек на входе в момент времени
    float ecg;      // амплитуда сигнала ЭКГ
    float pres;     // амплитуда сигнала давления
    float tone;     // амплитуда тонов
    int ca;         // метки алгоритма управления (control algorhytm)
};

struct State{                    // состояние функции, чтобы не тащить буфер
    int prev_ecg_pos;            // предыдущая позиция qrs
    float prev_ecg_amp;          // 
    int prev_pres_min_pos;
    float prev_pres_min_amp;
    int prev_pres_max_pos;
    float prev_pres_max_amp;
    int prev_tone_pos;
    int prev_tone_amp;
};

// Набор точек попадает в детектор (должен быть какой-то элемент для хранения состояния, чтобы не тащить буфер), 
// на выходе - флаг(и) события:
//      - нет события
//      - qrs *
//      - начало пульсации
//      - конец пульсации *
//      - тон *
// При установке флагов, помеченных *, элементы события поступают в параметризатор
// Выход параметризатора - структура, соответствующая событию

struct ECGEv{      // Детектированное собитие: элемент QRS (зубец R?)
    float amp;     // Амплитуда
    float ewidth;  // Энерг ширина
    float prem;    // Преждевеременность
    int rejres;    // Код причины браковки
};

struct PresEv{     // Детектированное событие: пульсация (фронт) ?? Заполняется и возвращается, только когда найден максимум?
    float amin;    // Амплитуда минимума
    float amax;    // Амплитуда максимума
    float prem;    // Преждевеременность
    int rejres;    // Код причины браковки
};

struct ToneEv{       // Детектированное событие: тон
    float amp;       // Амплитуда
    float prenoise;  // Шум до
    float postnoise; // Шум после
    float ewidth;    // Энерг ширина
    float prem;      // Преждевеременность
    int rejres;      // Код причины браковки
};

int detector(InputPack points){
    // обращаясь к предыдущим элементам вектора структур InputPack (так как нельзя тащить буфер, скорее всего, на вход
    // подается ещё состояние с предыдущей итерации),
    // детектируем от 0 до 3-х событий

};
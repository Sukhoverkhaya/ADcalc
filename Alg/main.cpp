#include "header_rw.h"
#include "badsplit.h"
#include "Filters.h"

#include "TestControlAd.h"
#include "ToneDetect.h"
#include "PulseDetect.h"

int main(int argc, char* argv[]){

	string fname;     // имена читаемых бинаря и его хедера (совпадают)
	ifstream ih, ib;  // потоки на чтение бинаря и хедера
	ofstream filttone, filtpulse, mkptone, mkppulse, admkp; // потоки на запись

	//-----------------------------------------------------------------------------
	// читаем аргументы командной строки
	if (argc != 2) {
		return -1;
	} else {
		fname = argv[1];
	};

	//-----------------------------------------------------------------------------
	// Читаем хедер
	ih.open(fname + ".hdr");

	Header Head(ih);

	int32_t freq = Head.fs;            // частота дискретизации
	vector<double> lsbs = Head.lsbs;   // lsbs для всех каналов
	int nchan = Head.leadnum;          // число каналов
	int Len = Head.len;                // длина записи в отсчетах

	ih.close();

	//-----------------------------------------------------------------------------
	// Читаем бинарь и работаем по точке
	ib.open(fname + ".bin", ios::in | ios::binary);

	int SIZE = nchan * sizeof(int32_t);

	int32_t* readbuf = new int32_t[nchan];
	int32_t k = 1; // счетчик прочитанных точек

	int PRES = Head.getlead("Pres"); // номер канала
	int TONE = Head.getlead("Tone");

	// фильтры для тонов
	int32_t div = (freq==1000) ? 8 : 2; // !!!! к-т децимации (прореживания) нужно учитывать и в детекторах (при 1000 Гц равен 4)

	Decimator dcm(div);
	FiltPack opt; // набор опций (к-тов) разных фильтров

	Filter dcmlpass(freq); dcmlpass.SetOptions(opt.butter_2_60_low);
	Filter hpass(freq); hpass.SetOptions(opt.butter_2_30_high);
	Filter lpass(freq/div); lpass.SetOptions(opt.butter_2_10_low);

	BaselineFilter blfilt;

	// фильтры для пульсаций
	Filter dcmlow(freq); dcmlow.SetOptions(opt.butter_2_20_low);
	Filter pulseLow(freq/div); pulseLow.SetOptions(opt.butter_2_10_low);
	Filter pulseHigh(freq/div); pulseHigh.SetOptions(opt.butter_2_03_high);
	DiffFilter tacho(freq/div, div);

	// детекторы
	ToneDetect tonedet(freq/div);
	PulseDetect pulsedet(freq/div, 32);

	// алгоритм АД
	ControlTone ctrltone(freq/div); // вызывать эти два конструктора откуда-то из TestControlAD, 
	ControlPulse ctrlpulse(freq/div); // а не так вот внешне (ибо не исп.)
	ControlAd ctrlad(freq/div, ctrltone, ctrlpulse, admkp);

	// потоки на запись всех этапов фильтрации и разметки
	vector<string> splitpath = badsplit(fname,'/');
	string nameonly = splitpath[splitpath.size()-1]; // последний элемент (имя файла)
	string basename = splitpath[splitpath.size()-2]; // предпоследний элемент (имя базы)

	string fold = "D:/INCART/PulseDetectorVer1/data/" + basename + '/'; // путь на сохранение разметки

	filttone.open(fold + nameonly + "_filttone.txt");
	filtpulse.open(fold + nameonly + "_filtpulse.txt");
	mkptone.open(fold + nameonly + "_tone_mkp.txt");
	mkppulse.open(fold + nameonly + "_pulse_mkp.txt");
	admkp.open(fold + nameonly + "_ad_mkp.txt");

	filttone << "tone" << '	' << "rawTone" << '	' << "smoothTone" << '	' << "smth_dcm_abs" << '	' << "env" << '	' << "envfilt";
	filttone << '	' << "LvR" <<  '	' << "LvN" << '	' << "LvP" << endl;
	
	filtpulse << "pulse" << '	' << "pressDcm" << '	' << "smoothPulse" << '	' << "basePulse" << '	' << "tch";
	filtpulse << '	' << "LvR" << '	' << "LvZ" << '	' << "LvP" << endl;
	
	mkptone << "pos" << '	' << "bad" << endl;
	mkppulse << "pos" << '	' << "bad" << endl;
	admkp << "pos" << '	' << "amp" << '	' << "pos" << '	' << "amp" << endl;

	do {
		ib.read((char*)readbuf, SIZE);

		int32_t pulse = readbuf[PRES];
		int32_t tone = readbuf[TONE];

		ctrlad.Mode(pulse, tone);

		int32_t rawTone = dcmlpass.Exe(tone);
		int32_t smoothTone = hpass.Exe(rawTone);

		int32_t pressDcm = dcmlow.Exe(pulse);

		dcm.Exe(smoothTone); // уменьшение частоты
		if (dcm.rdyFlag)     // в div раз (получаем 125 Гц)
		{
			// фильтруем тоны
			int32_t smth_dcm_abs = fnAbs(dcm.out);
			int32_t env = lpass.Exe(smth_dcm_abs);
			int32_t envfilt = blfilt.Exe(env);	

			// фильтруем пульсации
			int32_t smoothPulse = pulseLow.Exe(pressDcm);
			int32_t basePulse = pulseHigh.Exe(smoothPulse);
			int32_t tch = tacho.Exe(basePulse);

			filttone << tone << '	' << rawTone << '	' << smoothTone << '	' << smth_dcm_abs << '	' << env << '	' << envfilt;
			filtpulse << pulse << '	' << pressDcm << '	' << smoothPulse << '	' << basePulse << '	' << tch;

			tonedet.Exe(envfilt, smoothTone*1000, pulse);
			pulsedet.Exe(tch*10, smoothPulse, basePulse);

			if (tonedet.peakflag)
			{
				mkptone << tonedet.toneEv.pos << '	' << tonedet.toneEv.bad << endl;
				ctrlad.Exe(tonedet.toneEv);
			}
			filttone << '	' << tonedet.LvR <<  '	' << tonedet.LvN << '	' << tonedet.LvP << endl;

			if (pulsedet.e.eventRdy)
			{
				mkppulse << pulsedet.pulse.pos << '	' << pulsedet.pulse.bad << endl;
				ctrlad.Exe(pulsedet.pulse);
			}
			filtpulse << '	' << pulsedet.LvR << '	' << pulsedet.LvZ << '	' << pulsedet.LvP << endl;
		};

		// note: если нужны конвертация в физ величину и калибровка, добавляем отдельно децимацию до 250 Гц (перед децимацией до 125),
		// в противном случае работаем там же, где и с тонами

		// int32_t pressDcm = dcmlow.Exe(pulse);

		// if (pulsedecimcnt250 == stoppulsedecim250) // (получаем 250 Гц)
		// {
		// 	// конвертация в физ. величину (??)

		// 	// калибровка (в соотв. с таблицей калибровки (надо ли делать??))

		// 	// выделение пульсаций
			
		// 	if (pulsedecimcnt125 == stoppulsedecim125) // если пункты выше не будут выполнятся, то это вложенное условие не нужно - сделать децимацию сразу до 125 Гц
		// 	                                                                                                                        // а лучше работать с пульсациями там же, где с тонами, т.к. та же частота
		// 	{

		// 	}

		// 	pulsedecimcnt250 = 0;
		// }
		// pulsedecimcnt250++;
		++k;
	} while (!(&ib)->eof());

	ib.close();
	filttone.close();
	filtpulse.close();
	mkptone.close();
	mkppulse.close();
	admkp.close();

	//-----------------------------------------------------------------------------
};

// int main(int argc, char* argv[]){

// 	string fname;     // имена читаемых бинаря и его хедера (совпадают)
// 	ifstream ih, ib;  // потоки на чтение бинаря и хедера

// 	ofstream odiff, opresmkp, otonemkp, oadmkp; // потоки на запись (здесь - файлы с дифференцированным сигналом и разметкой)

// 	//-----------------------------------------------------------------------------
// 	// читаем аргументы командной строки
// 	if (argc != 2) {
// 		return -1;
// 	} else {
// 		fname = argv[1];
// 	};

// 	//-----------------------------------------------------------------------------
// 	// Читаем хедер
// 	ih.open(fname + ".hdr");

// 	Header Head(ih);

// 	int32_t freq = Head.fs;            // частота дискретизации
// 	vector<double> lsbs = Head.lsbs;   // lsbs для всех каналов
// 	int nchan = Head.leadnum;          // число каналов
// 	int Len = Head.len;                // длина записи в отсчетах

// 	ih.close();

// 	//-----------------------------------------------------------------------------
// 	// Читаем бинарь и работаем по точке
// 	ib.open(fname + ".bin", ios::in | ios::binary);

// 	int SIZE = nchan * sizeof(int32_t);

// 	// для детекторов
// 	int32_t N = 0.1*freq;  		     // первый множитель - шаг фильтра в секундах (можно задавать извне!!)
// 	DiffFilter difffilt(N, false);   // Дифференциатор с шагом N
// 	int32_t diff;          		     // Выход дифференциатора (одна точка)

// 	SumFilter sumfilt(N);   		 // Интегратор с шагом N
// 	int32_t sum;         			 // Выход интегратора (одна точка)

// 	SignalsProcessing::PulsOnlyDetect pulsedet;
// 	pulsedet.Start(0,0);
// 	int32_t pulseres;      			// Выход первичного детектора пульсаций

// 	SignalsProcessing::ToneOnlyDetect tonedet;
// 	tonedet.Start();
// 	int32_t toneres;       			// Выход первичного детектора тонов

// 	// для алгоритма АД
// 	ControlTone cntrltone(freq);
// 	ControlPulse cntrlpulse(freq);
// 	ControlAd cntrlAD(freq, cntrltone, cntrlpulse);
// 	// ToneEvent tone;
// 	// a.Exe(tone);
// 	int ADres = 0;
// 	int prevADres = 0;
// 	int ADmark = 0;

// 	// открываем файлы на запись
// 	vector<string> splitpath = badsplit(fname,'/');
// 	string nameonly = splitpath[splitpath.size()-1]; // последний элемент (имя файла)
// 	string basename = splitpath[splitpath.size()-2]; // предпоследний элемент (имя базы)

// 	string fold = "D:/INCART/PulseDetectorVer1/data/" + basename + '/'; // путь на сохранение разметки

// 	// odiff.open(fold + nameonly + "_diff.txt");
// 	// opresmkp.open(fold + nameonly + "_pres_mkp.txt");
// 	// otonemkp.open(fold + nameonly + "_tone_mkp.txt");
// 	oadmkp.open(fold + nameonly + "_ad_mkp.txt");

// 	opresmkp << "pos" << endl;
// 	otonemkp << "pos" << endl;
// 	// oadmkp << "ADpos	ADval" << endl;

// 	int32_t* readbuf = new int32_t[nchan];
// 	int k = 0;                     // счетчик строк (также текущий номер отсчета)

// 	int32_t pulse;
// 	int32_t tone;

// 	int PRES = Head.getlead("Pres");
// 	int TONE = Head.getlead("Tone");

// 	ToneEvent toneEv;    // событие: тон
// 	PulseEvent pulseEv;  // событие: пульсация

// 	do {
// 		ib.read((char*)readbuf, SIZE);

// 		int pos;

// 		pulse = readbuf[PRES];
// 		tone = readbuf[TONE];

// 		// Res res = cntrlAD.Mode(pulse, tone);
// 		// if (res.a1 != 0)
// 		// {
// 		// 	oadmkp << res.a1 << "	" << res.a2 << endl;
// 		// }

// 		cntrlAD.Mode(pulse, tone);

// 		sum = sumfilt.Exe(pulse); // тахо (задержка 0.1*fs/2)
// 		diff = difffilt.Exe(sum); // фильтрованное тахо (задержка 0.1*fs/2)
// 		odiff << diff << endl;

// 		pulseres = pulsedet.Exe(sum, diff); // результат детектора пульсаций
// 		toneres = tonedet.Exe(tone, freq);  // результат детектора тонов

// 		// if (ISVALID(pulse, tone)){	

// 		if (pulseres != 0) {
// 			// пишутся позиции минимумов
// 			pos = k - (sumfilt.delay + difffilt.delay);  // ??делается поправка на задержку фильтров!!
// 			// opresmkp << pos << endl; // пишем в файл позицию минимума пульсации (начало фронта)
// 			pulseEv.Reset();
// 			pulseEv.pos 	= k;
// 			pulseEv.press 	= pulse;
// 			pulseEv.val 	= diff; // здесь должно быть значение после фильтра выделения пульсаций
// 			pulseEv.range 	= pulsedet.mPlsAmplitude;

// 			cntrlAD.Exe(pulseEv);
// 		};

// 		if (toneres != 0){
// 			// otonemkp << k << endl;   // пишем в файл позицию тона
// 			// формируем ToneEvent
// 			toneEv.Reset();
// 			toneEv.pos = k;
// 			toneEv.val = tone;
// 			toneEv.press = pulse;

// 			cntrlAD.Exe(toneEv);
// 		};

// 		// if (ADres != 0){
// 		// 	if (ADres != prevADres && (ADres - prevADres) == 1) // если знаечние на выходе изменилось (прошло новое событие) не больше, чем на +1 - ничего не пропустили
// 		// 	{

// 		// 	}
// 		// 	else (ADres != prevADres && (ADres - prevADres) != 1) // в противном случае пропущенные значения заполняем 0
// 		// 	{
// 		// 		int nskip = (ADres > prevADres) ? (ADres - prevADres - 1) : 

// 		// 	};
// 		// };
// 	// ////////////////////////////////////////////////
// 	// 	if (ADres != 0 && ADres != prevADres){

// 	// 		oadmkp << k << "	" << int(pulse*lsbs[PRES]) << "	" << endl;
// 	// 	};
// 	// 	prevADres = ADres;
// 	// ////////////////////////////////////////////////////
// 		// };
// 		// cerr << cntrlAD.mode << endl;

// 		// };

// 		k++;
// 	} while (!(&ib)->eof());
// 	// cerr << k << endl;          	// проверка количества прочитанных строк

// 	// odiff.close();
// 	// otonemkp.close();
// 	// opresmkp.close();
// 	oadmkp.close();
// 	ib.close();
// 	//-----------------------------------------------------------------------------
// };
// /////////////////////////////////////////////////////////////////////////////////////////////////////
// int main(int argc, char* argv[]){

// 	string fname;     // имена читаемых бинаря и его хедера (совпадают)
// 	ifstream ih, ib;  // потоки на чтение бинаря и хедера

// 	ofstream odiff, opresmkp, otonemkp, oadmkp; // потоки на запись (здесь - файлы с дифференцированным сигналом и разметкой)

// 	//-----------------------------------------------------------------------------
// 	// читаем аргументы командной строки
// 	if (argc != 2) {
// 		return -1;
// 	} else {
// 		fname = argv[1];
// 	};

// 	//-----------------------------------------------------------------------------
// 	// Читаем хедер
// 	ih.open(fname + ".hdr");

// 	Header Head(ih);

// 	int32_t freq = Head.fs;            // частота дискретизации
// 	vector<double> lsbs = Head.lsbs;   // lsbs для всех каналов
// 	int nchan = Head.leadnum;          // число каналов
// 	int Len = Head.len;                // длина записи в отсчетах

// 	ih.close();

// 	//-----------------------------------------------------------------------------
// 	// Читаем бинарь и работаем по точке
// 	ib.open(fname + ".bin", ios::in | ios::binary);

// 	int SIZE = nchan * sizeof(int32_t);

// 	// для детекторов
// 	int32_t N = 0.1*freq;  		     // первый множитель - шаг фильтра в секундах (можно задавать извне!!)
// 	DiffFilter difffilt(N, false);   // Дифференциатор с шагом N
// 	int32_t diff;          		     // Выход дифференциатора (одна точка)

// 	SumFilter sumfilt(N);   		 // Интегратор с шагом N
// 	int32_t sum;         			 // Выход интегратора (одна точка)

// 	ToneDetect tonedet(freq);
// 	int32_t toneres;       			// Выход первичного детектора тонов

// 	// открываем файлы на запись
// 	otonemkp.open("tonepeaks.txt");

// 	otonemkp << "pos" << endl;

// 	int32_t* readbuf = new int32_t[nchan];
// 	int k = 0;                     // счетчик строк (также текущий номер отсчета)

// 	int32_t pulse;
// 	int32_t tone;

// 	int PRES = Head.getlead("Pres");
// 	int TONE = Head.getlead("Tone");

// 	do {
// 		ib.read((char*)readbuf, SIZE);

// 		pulse = readbuf[PRES];
// 		tone = readbuf[TONE];

// 		sum = sumfilt.Exe(pulse); // тахо (задержка 0.1*fs/2)
// 		diff = difffilt.Exe(sum); // фильтрованное тахо (задержка 0.1*fs/2)

// 		tonedet.Exe(tone);  // результат детектора тонов
// 		if (tonedet.peakflag)
// 		{
// 			otonemkp << tonedet.pos << endl;
// 		}

// 	} while (!(&ib)->eof());

// 	otonemkp.close();
// };

// struct test
// {
// 	int a;

// 	test(){a = 0;};

// 	reset(int b){a = b;};
// };

// int main(int argc, char* argv[]){
// 	test* M[5];
// 	test** C;

// 	for(int i=0; i<5;i++){
// 		M[i] = new test;
// 	};

// 	C = M;

// 	C[4] -> reset(5);
// 	cerr << C[4] -> a << endl;
// 	// cerr << *C[4] << endl;
// };

// int main(int argc, char* argv[]) // тест estimate_AD_tone.h
// {
// 	SKV::ESTIMATE_AD_TONE test;
// 	test.Init(10);
// 	test.SetMode(true);

// 	cerr << test.kPres << endl;
// 	cerr << test.mode << endl;
	
// }

//____________________________________

// struct A
// {
// 	enum B
// 	{
// 		state1 = 0,
// 		state2,
// 		state3,
// 		state4,
// 	};
// };

// int main(int argc, char* argv[]) 
// {

// 	A::B state;
// 	string str = "hello, world!";

// 	state = A::B::state3;

// 	const int a = 1000;

// 	cerr << str[state] << endl;
// 	cerr << a << endl;
// };

// // ________________________________________
// int main(int argc, char* argv[]) 
// {
// 	int fs = 1000;
// 	ControlTone cntrltone(fs);
// 	ControlAd a(fs, cntrltone);
// 	ToneEvent tone;

// 	a.Exe(tone);

// 	cerr << a.state << endl;
// };

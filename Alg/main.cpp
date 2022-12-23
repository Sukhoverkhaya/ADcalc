#include "header_rw.h"
// #include "PulsOnlyDetect.h"
// #include "ToneOnlyDetect.h"
#include "DiffFilter.h"
#include "SumFilter.h"
#include "badsplit.h"
#include "Filters.h"

#include "TestControlAd.h"
#include "ToneDetect.h"
// #include "estimate_AD_tone.h"

int main(int argc, char* argv[]){

	string fname;     // имена читаемых бинаря и его хедера (совпадают)
	ifstream ih, ib;  // потоки на чтение бинаря и хедера
	ofstream filttone, mkptone; // потоки на запись

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
	// int32_t k = 1;

	int PRES = Head.getlead("Pres"); // номер канала
	int TONE = Head.getlead("Tone");

	// фильтры для тонов
	int32_t div = (freq==1000) ? 4 : 1; // !!!! к-т децимации (прореживания) нужно учитывать и в детекторах (при 1000 Гц равен 4)

	Decimator dcm(div);
	Filter dcmlpass; dcmlpass.SetDecimLowpass(freq);
	Filter hpass; hpass.SetHighPass(freq);
	Filter lpass; lpass.SetLowPass(freq/div);
	BaselineFilter blfilt;

	// детектор
	ToneDetect tonedet(freq/div);

	// алгоритм АД
	ControlTone ctrltone(freq/div); // вызывать эти два конструктора откуда-то из TestControlAD, 
	ControlPulse ctrlpulse(freq/div); // а не так вот внешне (ибо не исп.)
	ControlAd ctrlad(freq/div, ctrltone, ctrlpulse);

	// потоки на запись всех этапов филтрации тонов
	filttone.open("filttone.txt");
	mkptone.open("tonepeaks.txt");

	do {
		ib.read((char*)readbuf, SIZE);

		int32_t pulse = readbuf[PRES];
		int32_t tone = readbuf[TONE];

		int32_t rawTone = dcmlpass.Exe(tone);
		int32_t smoothTone = hpass.Exe(rawTone);
		dcm.Exe(smoothTone); // уменьшение частоты
		if (dcm.rdyFlag)     // в div раз
		{
			int32_t smth_dcm_abs = fnAbs(dcm.out);
			int32_t env = lpass.Exe(smth_dcm_abs);
			int32_t envfilt = blfilt.Exe(env);	

			filttone << tone << '	' << rawTone << '	' << smoothTone << '	' << smth_dcm_abs << '	' << env << '	' << envfilt;

			ctrlad.Mode(pulse, envfilt);
			tonedet.Exe(envfilt, smoothTone*1000, pulse);
			if (tonedet.peakflag)
			{
				mkptone << tonedet.toneEv.pos << '	' << tonedet.toneEv.bad << endl;
				ctrlad.Exe(tonedet.toneEv);
			}
			filttone << '	' << tonedet.LvR <<  '	' << tonedet.LvN << '	' << tonedet.LvP << endl;
		}
		
		// tonedet.Exe(tone, pulse);  // результат детектора тонов
		// if (tonedet.peakflag)
		// {
		// 	// cerr << tonedet.toneEv.bad << endl;
		// 	// if (!tonedet.toneEv.bad)
		// 	// {
		// 	// 	++k;
		// 	// }
		// }
		
	} while (!(&ib)->eof());
	ib.close();
	filttone.close();
	mkptone.close();

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

#include "header_rw.h"
#include "PulsOnlyDetect.h"
#include "ToneOnlyDetect.h"
#include "DiffFilter.h"
#include "SumFilter.h"
#include "badsplit.h"

int main(int argc, char* argv[]){
	string fname;     // имена читаемых бинаря и его хедера (совпадают)
	ifstream ih, ib;  // потоки на чтение бинаря и хедера

	ofstream odiff, opresmkp, otonemkp; // потоки на запись (здесь - файлы с дифференцированным сигналом и разметкой)

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

	int32_t N = 0.1*freq;  		     // первый множитель - шаг фильтра в секундах (можно задавать извне!!)
	DiffFilter difffilt(N, false);   // Дифференциатор с шагом N
	int32_t diff;          		     // Выход дифференциатора (одна точка)

	SumFilter sumfilt(N);   		 // Интегратор с шагом N
	int32_t sum;         			 // Выход интегратора (одна точка)

	SignalsProcessing::PulsOnlyDetect pulsedet;
	pulsedet.Start(0,0);
	int32_t pulseres;      			// Выход первичного детектора пульсаций

	SignalsProcessing::ToneOnlyDetect tonedet;
	tonedet.Start();
	int32_t toneres;       			// Выход первичного детектора тонов

	// открываем файлы на запись
	vector<string> splitpath = badsplit(fname,'/');
	string nameonly = splitpath[splitpath.size()-1]; // последний элемент (имя файла)
	string basename = splitpath[splitpath.size()-2]; // предпоследний элемент (имя базы)

	string fold = "D:/INCART/PulseDetectorVer1/data/"+basename+'/'; // путь на сохранение разметки

	odiff.open(fold + nameonly + "_diff.txt");
	opresmkp.open(fold + nameonly + "_pres_mkp.txt");
	otonemkp.open(fold + nameonly + "_tone_mkp.txt");

	opresmkp << "pos" << endl;
	otonemkp << "pos" << endl;

	int32_t* readbuf = new int32_t[nchan];
	int k = 0;                     // счетчик строк (также текущий номер отсчета)

	int32_t pulse;
	int32_t tone;

	int PRES = Head.getlead("Pres");
	int TONE = Head.getlead("Tone");

	do {
		ib.read((char*)readbuf, SIZE);

		int pos;

		pulse = readbuf[PRES];
		tone = readbuf[TONE];

		sum = sumfilt.Exe(pulse); // тахо (задержка 0.1*fs/2)
		diff = difffilt.Exe(sum); // фильтрованное тахо (задержка 0.1*fs/2)
		odiff << diff << endl;

		pulseres = pulsedet.Exe(sum, diff);
		toneres = tonedet.Exe(tone, freq);

								    
		if (pulseres != 0) {
			// пишутся позиции минимумов
			pos = k - (sumfilt.delay + difffilt.delay);  // ??делается поправка на задержку фильтров!!
			opresmkp << pos << endl;
		};

		if (toneres != 0){
			otonemkp << k << endl;
		};

		k++;
	} while (!(&ib)->eof());
	// cerr << k << endl;          // проверка количества прочитанных строк

	odiff.close();
	otonemkp.close();
	opresmkp.close();
	ib.close();
	//-----------------------------------------------------------------------------
};


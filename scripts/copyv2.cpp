#include "header_rw.h"
#include "PulsOnlyDetect.h"
#include "ToneOnlyDetect.h"
#include "DiffFilter.h"
#include "SumFilter.h"


struct Bounds{
	int ibeg;
	int iend;
};

int main(int argc, char* argv[]){

	std::string fname; // имена читаемых бинаря и его хедера (совпадают)
	std::ifstream ih, ib; // потоки на чтение бинаря и хедера

	std::ofstream odiff, opresmkp, otonemkp; // потоки на запись (здесь - файлы с дифференцированным сигналом и разметкой)

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
	vector<double> lsbs = Head.lsbs;  // lsbs для всех каналов
	int nchan = Head.leadnum;         // число каналов
	int Len = Head.len;               // длина записи в отсчетах

	ih.close();
	//-----------------------------------------------------------------------------
	//Читаем бинарь и работаем по точке
	ib.open(fname + ".bin", ios::in | ios::binary);

	int SIZE = nchan * sizeof(int32_t);

	int32_t N = 0.5*freq;  // первый множитель - шаг фильтра в секундах (можно задавать извне!!)

	DiffFilter difffilt(N, false);    // Дифференциатор с шагом N
	int32_t diff;          // Выход дифференциатора (одна точка)

	SumFilter sumfilt(N);    // Интегратор с шагом N
	int32_t sum;          // Выход интегратора (одна точка)

	SignalsProcessing::PulsOnlyDetect pulsedet;
	pulsedet.Start(0,0);
	int32_t pulseres;      // Выход первичного детектора пульсаций

	SignalsProcessing::ToneOnlyDetect tonedet;
	tonedet.Start();
	int32_t toneres;       // Выход первичного детектора тонов

	// открываем файлы на запись
	odiff.open("diff.txt");
	opresmkp.open("pres_mkp.txt");
	otonemkp.open("tone_mkp.txt");

	int32_t* readbuf = new int32_t[nchan];
	int k = 0; // счетчик строк (также текущий номер отсчета)

	int32_t pulse;
	int32_t tone;

	do {
		ib.read((char*)readbuf, SIZE);

		pulse = readbuf[1];
		tone = readbuf[2];

		sum = sumfilt.Exe(pulse); // тахо
		diff = difffilt.Exe(sum); // фильтрованное тахо
		odiff << diff << endl;

		pulseres = pulsedet.Exe(sum, diff);
		toneres = tonedet.Exe(tone, freq);

								// не делается поправка на задержку фильтров!!
		if (pulseres != 0){
			opresmkp << pulsedet.rgFrontTime << endl;
		};

		if (toneres != 0){
			otonemkp << k << endl;
		};

		k++;

	} while (k < Len);
	// cerr << k << endl; // проверка количества прочитанных строк

	// odiff.close();
	otonemkp.close();
	opresmkp.close();
	ib.close();
	//-----------------------------------------------------------------------------
};
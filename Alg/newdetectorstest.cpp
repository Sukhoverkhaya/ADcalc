#include "header_rw.h"
#include "DiffFilter.h"
#include "SumFilter.h"
#include "badsplit.h"

#include "ToneDetect.h"

int main(int argc, char* argv[]){

	cerr << "I'm here" << endl;

	string fname;     // имена читаемых бинаря и его хедера (совпадают)
	ifstream ih, ib;  // потоки на чтение бинаря и хедера

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

	ToneDetect tonedet(freq);

	int32_t* readbuf = new int32_t[nchan];
	int k = 0;                     // счетчик строк (также текущий номер отсчета)

	int32_t pulse;
	int32_t tone;

	int PRES = Head.getlead("Pres");
	int TONE = Head.getlead("Tone");

	do {
		ib.read((char*)readbuf, SIZE);

		pulse = readbuf[PRES];
		tone = readbuf[TONE];

		tonedet.Exe(tone, pulse);  // результат детектора тонов

		
	} while (!(&ib)->eof());
	ib.close();
	//-----------------------------------------------------------------------------
};
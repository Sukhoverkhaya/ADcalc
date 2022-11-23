#include "header_rw.h"
#include "PulsOnlyDetect.h"
#include "ToneOnlyDetect.h"
#include "diff_filter.h"

struct Bounds{
	int ibeg;
	int iend;
};

int main(int argc, char* argv[]){

	std::string hdr_name, bin_name; 
	std::ifstream ih, ib;

	//-----------------------------------------------------------------------------
	// читаем аргументы командной строки
	if (argc != 2) {
		return -1;
	} else {
		hdr_name = argv[1];
		bin_name = argv[1];
	};
	//-----------------------------------------------------------------------------
	// Читаем хедер
	ih.open(hdr_name + ".hdr");

	Header Head(*(&ih));

	double freq = Head.fs;            // частота дискретизации
	vector<double> lsbs = Head.lsbs;  // веса младших разрядов для всех каналов
	int nchan = Head.leadnum;         // число каналов
	int Len = Head.len;               // длина записи в отсчетах

	ih.close();
	//-----------------------------------------------------------------------------
	//Читаем бинарь
	ib.open(bin_name + ".bin", ios::in | ios::binary);

	int SIZE = nchan * sizeof(int32_t);

	DiffFilter filt(5);
	int32_t diff;

	int32_t* readbuf = new int32_t[nchan];
	int k = 0; // счетчик строк
	do {
		ib.read((char*)readbuf, SIZE);
		diff = filt.Exe<true>(readbuf[1]);
		cerr << diff << endl;
		k++;
	} while (k < Len);
	// cerr << k << endl; // проверка количества прочитанных строк

	ib.close();
	//-----------------------------------------------------------------------------
};
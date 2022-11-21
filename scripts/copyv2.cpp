// #include "header.h"
// #include "PulsOnlyDetect.h"
// #include "DiffFilter.h"

// using namespace std;

// size_t atoi(string a) {
// 	size_t res = 0;

// 	for (int i = 0; i < a.size(); i++) {
// 		res = res * 10 + (a[i] - '0');
// 	}

// 	return res;
// };

// struct Bounds{
// 	int ibeg;
// 	int iend;
// };

// int main(int argc, char* argv[]) {
// 	string hdr_name, bin_name;

// 	size_t start_point = 0;		//включительно
// 	size_t end_point = 0;		//не включительно

// 	int SIZE;
// 	bool mode; // 1 - пайп -> файл | 0 - файл -> пайп

// 	ofstream oh, ob;
// 	ifstream ib, ih;

// 	istream* in_hdr;
// 	ostream* out_hdr;
// 	istream* in_bin;
// 	ostream* out_bin;

// 	if (argc == 1 || argc == 2) {
// 		cerr << "No arguments in console.\nUse -r to read, -w to write\n";
// 		return -1;
// 	} else if (argc == 3 || argc == 5) {
// 		hdr_name = argv[2];
// 		bin_name = argv[2];
// 		string console_flag = argv[1];

// 		if (console_flag == "-r") {
// 			ih.open(hdr_name + ".hdr");
// 			ib.open(bin_name + ".bin", ios::in | ios::binary);

// 			if (!(ih.is_open() && ib.is_open())) {
// 				cerr << "This file or files doesn't exist.\n"
// 					 << "Check your console args.\n";
// 			}

// 			in_hdr = &ih;
// 			in_bin = &ib;
// 			out_hdr = &cout;
// 			out_bin = &cout;
// 			mode = false;
// 		} else if (console_flag == "-w") {
// 			ob.open(bin_name + ".bin", ios::app | ios::binary);
// 			oh.open(hdr_name + ".hdr");

// 			in_hdr = &cin;
// 			in_bin = &cin;
// 			out_hdr = &oh;
// 			out_bin = &ob;
// 			mode = true;
// 		} else {
// 			cerr << "Wrong flags\n";
// 			return -1;
// 		}

// 		if (argc == 5) {
// 			start_point = atoi(argv[3]);
// 			end_point = atoi(argv[4]);

// 			if (start_point > end_point || start_point < 0 || end_point < 0) {
// 				cerr << "Incorrect points!\n";
// 				return -1;
// 			}
// 		}
// 	}
	
// 	Header Head(*in_hdr);
// 	Head.start_end[0] = end_point != 0 ? start_point : Head.start_end[0];
// 	Head.start_end[1] = end_point != 0 ? end_point   : Head.start_end[1];
// 	Head.Write(*out_hdr); 

// 	SIZE = Head.info[0] * sizeof(int32_t);

// 	if (mode) {
// 		char useless[2];
// 		cin.read((char *)useless, 1);
// 	} else {
// 		cout << "\n";
// 	}

// 	int32_t* buf = new int32_t[SIZE / sizeof(int32_t)];
// 	int32_t* speed = new int32_t[SIZE / sizeof(int32_t)];
// 	// size_t rw_counter = 0;

// 	int chanel = 2;
// 	struct Bounds validseg = {29100,42600};
// 	int k = 0;
// 	SignalsProcessing::DiffFilter <5>filt;
// 	do {
// 		// cerr << "hahaha" << endl;
// 		(in_bin)->read((char *)buf, SIZE);
// 		k++;
// 		// if (end_point == 0 || (rw_counter >= start_point && rw_counter < end_point)) {
// 		//*
// 			// for (int i = 0; i < SIZE / sizeof(int32_t); i++) {
// 			// 	cerr << buf[i] << ' ';
// 			// }
// 			// cerr << endl;
// 		//*
// 		// ПО ОДНОЙ ТОЧКЕ
// 		// if (k >= validseg.ibeg && k <= validseg.iend){
// 			// Дифференцирующий фильтр
// 			// for (int i = 0; i < SIZE / sizeof(int32_t); i++) {
// 			// 	speed[i] = filt.Exe<true>(buf[i]);
// 			// // cerr << speed << endl;
// 			// };
// 		// };
// 		// cerr << buf[chanel] << endl;
// 		(out_bin)->write((char*)buf, in_bin->gcount());
// 		// }
// 		// ++rw_counter;
// 	} while (!(in_bin)->eof());
// 	delete [] buf;
// }
//////////////////////////////////////////////////////////////////////////////////
#include "header.h"

using namespace std;

size_t atoi(string a) {
	size_t res = 0;

	for (int i = 0; i < a.size(); i++) {
		res = res * 10 + (a[i] - '0');
	}

	return res;
}

int main(int argc, char* argv[]) {
	string hdr_name, bin_name;

	size_t start_point = 0;		//включительно
	size_t end_point = 0;		//не включительно

	int SIZE;
	bool mode; //1 - пайп->файл | 0 - файл->пайп

	ofstream oh, ob;
	ifstream ib, ih;

	istream* in_hdr;
	ostream* out_hdr;
	istream* in_bin;
	ostream* out_bin;

	if (argc == 1 || argc == 2) {
		cerr << "No arguments in console.\nUse -r to read, -w to write\n";
		return -1;
	} else if (argc == 3 || argc == 5) {
		hdr_name = argv[2];
		bin_name = argv[2];
		string console_flag = argv[1];

		if (console_flag == "-r") {
			ih.open(hdr_name + ".hdr");
			ib.open(bin_name + ".bin", ios::in | ios::binary);

			if (!(ih.is_open() && ib.is_open())) {
				cerr << "This file or files doesn't exist.\n"
					 << "Check your console args.\n";
			}

			in_hdr = &ih;
			in_bin = &ib;
			out_hdr = &cout;
			out_bin = &cout;
			mode = false;
		} else if (console_flag == "-w") {
			ob.open(bin_name + ".bin", ios::app | ios::binary);
			oh.open(hdr_name + ".hdr");

			in_hdr = &cin;
			in_bin = &cin;
			out_hdr = &oh;
			out_bin = &ob;
			mode = true;
		} else {
			cerr << "Wrong flags\n";
			return -1;
		}

		if (argc == 5) {
			start_point = atoi(argv[3]);
			end_point = atoi(argv[4]);

			if (start_point > end_point || start_point < 0 || end_point < 0) {
				cerr << "Incorrect points!\n";
				return -1;
			}
		}
	}
	
	Header Head(*in_hdr);
	Head.start_end[0] = end_point != 0 ? start_point : Head.start_end[0];
	Head.start_end[1] = end_point != 0 ? end_point   : Head.start_end[1];
	Head.Write(*out_hdr); 

	SIZE = Head.info[0] * sizeof(int32_t);

	if (mode) {
		char useless[2];
		cin.read((char *)useless, 1);
	} else {
		cout << "\n";
	}

	int32_t* buf = new int32_t[SIZE / sizeof(int32_t)];
	size_t rw_counter = 0;

	do {
		(in_bin)->read((char *)buf, SIZE);
		if (end_point == 0 || (rw_counter >= start_point && rw_counter < end_point)) {
			(out_bin)->write((char*)buf, in_bin->gcount());
		};
		++rw_counter;

	} while (!(in_bin)->eof());
	cerr << rw_counter << endl;
	delete [] buf;
}

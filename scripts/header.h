#include <iostream>
#include <math.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <stddef.h>
#include <stdint.h>
#include <cstring>

#define LNG 40
#define MAXCH 20 // максимально возможное количество каналов в файле

using namespace std;

int mean_SUM[MAXCH] = {0};

struct Header {
	vector<float> info;
	vector<long long> start_end;
	vector<string> featurenames;
	vector<double> lsbs;
	vector<string> units;

	int  readed_from = -1;
	int  readed_to	= -1;
	// bool need_write_ft = false; 


	Header(istream &stream) {
		string type = "int32";    // тип данных
		double freq = 0;            // частота
		double lsb = 0;             // вес младшего разряда
		long long startPoint = 0;      // первая точка
		long long endPoint = 0;        // ? последняя точка сигнала
		string realTime;            // реальное время

		int fieldnum;
		string line;
		getline(stream, line);
		auto tmpstream = istringstream(line);
		tmpstream >> fieldnum >> freq >> lsb; // >> type;

		info.push_back(fieldnum);
		info.push_back(freq);
		info.push_back(lsb);

		featurenames.resize(fieldnum + 1);
		lsbs.resize(fieldnum);
		units.resize(fieldnum);

		stream >> startPoint >> endPoint >> featurenames[0];
		start_end.push_back(startPoint);
		start_end.push_back(endPoint);

		for (int i = 0; i < fieldnum; i++) stream >> featurenames[i + 1];
		for (int i = 0; i < fieldnum; i++) stream >> lsbs[i];
		for (int i = 0; i < fieldnum; i++) stream >> units[i];
	}

	Header(const Header &H) {
		this->info = H.info;
		this->start_end = H.start_end;
		this->featurenames = H.featurenames;
		this->lsbs = H.lsbs;
		this->units = H.units;
	}

	Header(const Header &H, int num_channels) {
		this->info = H.info;
		this->start_end = H.start_end;
		this->featurenames = H.featurenames;
		this->lsbs = H.lsbs;
		this->units = H.units;
		vector<string> new_names;
		if (num_channels == 12) {
			new_names = {"I", "II", "III", "AVR", "AVL", "AVF", "V1", "V2", "V3", "V4", "V5", "V6"};
		}

		this->info[0] = num_channels;

		for (int i = 0; i < num_channels; i++) {
        	if (i + 1 < H.featurenames.size()) {
        	    this->featurenames[i + 1] = new_names[i];
        	} else {
        	    this->featurenames.push_back(new_names[i]);
        	    this->lsbs.push_back(this->lsbs[0]);
        	    this->units.push_back(this->units[0]);
        	}
    	}		
	}

	void Write(ostream &output) {
		int CNT = featurenames.size();

		for (int i = 0; i < 3; i++) {
			output << info[i];
			if (i < 2) {
				output << " ";
			}
		}
		output << endl;
		output << start_end[0] << "\t" << start_end[1] << " " << featurenames[0] << endl;
		for (int i = 1; i < CNT; i++) {
			output << featurenames[i];
			if (i < CNT - 1) {
				output << "\t";
			}
		}
		output << endl;
		for (int i = 0; i < CNT - 1; i++) {
			output << lsbs[i];
			if (i < CNT - 2) {
				output << "\t";
			}
		}
		output << endl;
		for (int i = 0; i < CNT - 1; i++) {
			output << units[i];
			if (i < CNT - 2) {
				output << "\t";
			}
		}
	}
};

struct Evt {
	vector<int> 	info;
	string 			type;
	string 			filename;
	string			realTime;
	vector<string>	smth;
	vector<int>		start_end;
	vector<string>	se_names;

	Evt(istream &stream) {
		int fieldnum;
		stream >> fieldnum;
		info.resize(fieldnum + 2);
		smth.resize(4 * fieldnum);
		start_end.resize(2);
		se_names.resize(4);
		
		info[0]	= fieldnum;
		stream >> info[1] >> info[2] >> type >> filename;
		stream >> info[3] >> info[4] >> realTime;
		for (int i = 0; i < 4 * fieldnum; i++) { stream >> smth[i]; }
		char nothing;
		stream >> nothing;
		stream >> start_end[0] >> se_names[0] >> se_names[1];
		stream >> start_end[1] >> se_names[2] >> se_names[3];
	}

	void Write(ostream &output) {
		for (int i = 0; i < 3; i++) {output << info[i] << "\t";} 
		output << type << "\t" << filename << endl;
		output << info[3] << "\t" << info[4] << "\t" << realTime << endl;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < info[0]; j++) {
				output << smth[i * info[0] + j] << "\t";
			}
			output << endl;
		}
		output << "#\n";
		output << start_end[0] << "\t" << se_names[0] << "\t" << se_names[1] << "\n";
		output << start_end[1] << "\t" << se_names[2] << "\t" << se_names[3] << "\n";
	}
};

struct DifFilter {
    vector<int32_t> buf;
    int32_t k;
    bool need_restart;
	int d;

    DifFilter(int32_t delta) {
        for (int i = 0; i < delta; i++) {
            buf.push_back(0);
        }
        k = 0;
        need_restart = true;
    }

	DifFilter(const DifFilter &D) {
		this->buf = D.buf;
		this->k = D.k;
		this->need_restart = D.need_restart;
	}
};

struct SlideMeanFilter {
	vector<int32_t> buf;
	int32_t k;
	bool need_restart;
	SlideMeanFilter(int window) {
		this->buf.resize(window, 0);
		this->k = 0;
		this->need_restart = true;
	}
};

struct ThrDetector {
	int 		thr_on;
	int 		thr_off;
	int 		counter;
	bool 		detect_prev;
	vector<int>	out;

	ThrDetector(int thron, int throff) {
		this->thr_on  		= thron;
		this->thr_off 		= throff;
		this->counter		= 0;
		this->detect_prev	= false;
		this->out			= {};
	}
};

void exe_detector(ThrDetector* obj, int x) {
	bool 		detect_pr 		= obj->detect_prev;
	bool 		det;

	if (!detect_pr) {
		det = x > obj->thr_on;
	} else {
		det = x > obj->thr_off;
	}

	if (det != detect_pr) {
		obj->out.push_back(obj->counter);
	}
	obj->counter++;
	obj->detect_prev = det;
}

void Recalculation(int32_t* buf_r, int32_t* buf_w) {
	int32_t LR = buf_r[0];
    int32_t FR = buf_r[1];

    buf_w[0] = LR;
    buf_w[1] = FR;
    buf_w[2] = FR - LR;
    buf_w[3] = -(LR + FR) / 2;
    buf_w[4] = LR - FR / 2;
    buf_w[5] = FR - LR / 2;
    buf_w[6] = buf_r[2] - (LR + FR) / 3;
    buf_w[7] = buf_r[3] + LR - (LR + FR) / 3;
    buf_w[8] = buf_r[4] + FR - (LR + FR) / 3;
    buf_w[9] = buf_r[5] - (LR + FR) / 3;
    buf_w[10] = buf_r[6] + LR - (LR + FR) / 3;
    buf_w[11] = buf_r[7] + FR - (LR + FR) / 3;
}

int32_t exe(DifFilter* obj, int32_t* x) {
    int32_t k = obj->k;

    if (obj->need_restart) {
        for (int i = 0; i < obj->buf.size(); i++) {
            obj->buf[i] = *x;
        }
        obj->need_restart = false;
    }

    int32_t y = *x - obj->buf[k];
    obj->buf[k++] = *x;
    
    if (k > obj->buf.size() - 1) {
        k = 0;
    }
    obj->k = k;
    return y;
}

int32_t exeMean(SlideMeanFilter* obj, int32_t* x, int idx, int window, int channels) {
	static SlideMeanFilter         example(window);
    static vector<SlideMeanFilter> filters(channels, example);

	if (obj->need_restart) {
		for (int i = 0; i < window; i++) {
			obj->buf[i] = *x;
		}
		obj->need_restart = false;

		mean_SUM[idx] = *x * window;
	}

	int y = mean_SUM[idx] / window;
	mean_SUM[idx] += *x;
	mean_SUM[idx] -= obj->buf[(obj->k + 1) % window];
	obj->buf[obj->k++] = *x;

	if (obj->k > window - 1) {
        obj->k = 0;
    }
    return y;
}

int32_t exeMean2(int32_t* x, int idx, int window, int channels) {
	SlideMeanFilter         example(window);
    static vector<SlideMeanFilter> filters(channels, example);

	if (filters[idx].need_restart) {
		for (int i = 0; i < window; i++) {
			filters[idx].buf[i] = *x;
		}
		filters[idx].need_restart = false;

		mean_SUM[idx] = *x * window;
	}

	int y = mean_SUM[idx] / window;
	mean_SUM[idx] += *x;
	mean_SUM[idx] -= filters[idx].buf[(filters[idx].k + 1) % window];
	filters[idx].buf[filters[idx].k++] = *x;

	if (filters[idx].k > window - 1) {
        filters[idx].k = 0;
    }
    return y;
}

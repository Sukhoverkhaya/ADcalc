#include "header_rw.h"
#include "badsplit.h"
#include "Filters.h"

#include "TestControlAd.h"
#include "ToneDetect.h"
#include "PulseDetect.h"

int main(int argc, char* argv[])
{

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

	int PRES = Head.getlead("Pres"); // номер канала
	int TONE = Head.getlead("Tone");

	// фильтры для тонов
	int32_t div = freq/125; // !!!! к-т децимации (прореживания) до 125 Гц

	Decimator dcm(div);
	FiltPack opt; // набор опций (к-тов) разных фильтров

	Filter dcmlpass(freq); dcmlpass.SetOptions(opt.butter_2_60_low);
	Filter hpass(freq); hpass.SetOptions(opt.butter_2_30_high);
	Filter lpass(125); lpass.SetOptions(opt.butter_2_10_low);

	BaselineFilter blfilt;

	// фильтры для пульсаций
	Filter dcmlow(freq); dcmlow.SetOptions(opt.butter_2_20_low);
	Filter pulseLow(125); pulseLow.SetOptions(opt.butter_2_10_low);
	Filter pulseHigh(125); pulseHigh.SetOptions(opt.butter_2_03_high);
	DiffFilter tacho(125, div);

	// детекторы
	ToneDetect tonedet(125);
	PulseDetect pulsedet(125, 32);

	// алгоритм АД
	ControlAd ctrlad(125, div, admkp);

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
	mkppulse << "imax" << '	' << "imin" << '	' << "bad" << endl;
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

			// детектируем события 
			tonedet.Exe(envfilt, smoothTone*1000, pulse);
			pulsedet.Exe(tch*10, smoothPulse, basePulse);

			// если обнаружен тон
			if (tonedet.peakflag)
			{
				mkptone << tonedet.toneEv.pos*div << '	' << tonedet.toneEv.bad << endl;
				ctrlad.Exe(tonedet.toneEv);
			}
			filttone << '	' << tonedet.LvR <<  '	' << tonedet.LvN << '	' << tonedet.LvP << endl;

			// если обнаружена пульсация
			if (pulsedet.e.eventRdy)
			{
				mkppulse << pulsedet.pulse.pos*div << '	' << pulsedet.pulse.imin*div << '	' << pulsedet.pulse.bad << endl;
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

	} while (!(&ib)->eof());

	ib.close();
	filttone.close();
	filtpulse.close();
	mkptone.close();
	mkppulse.close();
	admkp.close();

	//-----------------------------------------------------------------------------
};
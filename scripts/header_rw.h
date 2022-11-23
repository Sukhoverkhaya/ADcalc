#pragma once

#include <iostream>
#include <math.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <stddef.h>
#include <stdint.h>
#include <cstring>

using namespace std;

struct Header { // Лёшина читалка хедеров с сокращениями и изменениями
	int leadnum; // число каналов
	int len;      // длина записи в отсчетах
	float fs;     // частота дискретизации
	vector<string> leadnames; // имена каналов (отведений)
	vector<double> lsbs;  // веса младших разрядов для всех каналов
	string type = "int32";         // тип данных 

	Header(istream &stream) {
		double lsb = 0;                // вес младшего разряда
		long long startPoint = 0;      // первая точка
		long long endPoint = 0;        // последняя точка сигнала
		string realTime;              // реальное время

		string line;
		getline(stream, line);
		auto tmpstream = istringstream(line);
		tmpstream >> leadnum >> fs >> lsb; // >> type;

		lsbs.push_back(lsb);

		leadnames.resize(leadnum);
		lsbs.resize(leadnum);

		stream >> startPoint >> endPoint >> realTime;
		len = endPoint;

		for (int i = 0; i < leadnum; i++) stream >> leadnames[i];
		for (int i = 0; i < leadnum; i++) stream >> lsbs[i];
	};
};

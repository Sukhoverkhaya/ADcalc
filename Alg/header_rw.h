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

// using namespace std;

struct Header {               // Лёшина читалка хедеров с сокращениями и изменениями
	int leadnum;              // число каналов
	int len;                  // длина записи в отсчетах
	float fs;                 // частота дискретизации
	std::vector<std::string> leadnames; // имена каналов (отведений)
	std::vector<double> lsbs;      // веса младших разрядов для всех каналов
	std::string type = "int32";    // тип данных 

	Header(std::istream &stream) {
		double lsb = 0;                // вес младшего разряда
		long long startPoint = 0;      // первая точка
		long long endPoint = 0;        // последняя точка сигнала
		std::string realTime;               // реальное время

		std::string line;
		getline(stream, line);
		auto tmpstream = std::stringstream(line);
		tmpstream >> leadnum >> fs >> lsb;     // >> type;

		lsbs.push_back(lsb);

		leadnames.resize(leadnum);
		lsbs.resize(leadnum);

		stream >> startPoint >> endPoint >> realTime;
		len = endPoint;

		for (int i = 0; i < leadnum; i++) stream >> leadnames[i];
		for (int i = 0; i < leadnum; i++) stream >> lsbs[i];
	};

	int getlead(std::string leadname){
		
		int leadnumber = -1;

		for (int i = 0; i < leadnames.size(); i++){
			if (leadnames[i] == leadname){
				leadnumber = i;
				break;
			}
		}
		
		return leadnumber;
	}
};

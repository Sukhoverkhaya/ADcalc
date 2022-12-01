#pragma once 

#include <iostream>
#include <vector>

using namespace std;

vector<string> badsplit(string str, char delimiter){ // кривой самопальный сплит

    int size = str.size(); // длина входной строки

    vector<string> splitted;             // выход - вектор строк
    string buf;                          // часть строки до искомого разделителя

    int k = 0;
    bool needwrite = true;

    for(int i = 0; i<size; i++){

        if (str[i] != delimiter){   // до встречи разделителя
            buf.push_back(str[i]);  // пушим в буфер
        } 
        else{                        // встретили разделитель -
            splitted.push_back(buf); // пушим содержимое в вектор
            buf = "";                // очищаем буфер
        }
    };

    // пушим буфер, который скопился последним (если скопился)
    if (buf != ""){
        splitted.push_back(buf);
    }

    return splitted;
}
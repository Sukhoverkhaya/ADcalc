#pragma once 

#include <stdio.h>

std::string badsplit(std::string name){ // очень захардкоженый сплит (просто чтобы достать имя файла из пути)

    int Size = 0;

    while (name[Size] != '\0') Size++;

    std::string newname;
    std::string buf;
    int k = 0;
    bool needwrite = true;

    for(int i = Size-1; i>0; i--){
        // if (name[i+1] == '.'){ // для случая имени файла с расширением
        //     needwrite = true;
        // } 
        // else if (name[i] == '/'){
        //     needwrite = false;
        // }
        if (name[i] == '/' || name[i] == '\\') needwrite = false; // для случая имени файла без расширения

        if (needwrite){
            buf.push_back(name[i]);
            k++;
        }
    };

    for(int i = k-1; i>=0; i--){
        newname.push_back(buf[i]);
    };

    return newname;
}
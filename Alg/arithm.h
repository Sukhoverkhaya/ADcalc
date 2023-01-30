// арифметические операции, которые задействуются в разых частях комплекса алгоритмов
#pragma once

#define fnMax(a, b)		( ((a) > (b)) ?  (a) : (b) ) // максимальное из двух значений
#define fnAbs(a)		( ((a) < 0) ?  -(a) : (a) )  // модуль

#define INT32_MAX 2147483647 // максимальное значение, доступное для записи с использованием Int32

int pow2(int a) // возведение двойки в целую степень
{
    int res = 1;
    if (a!=0)
    {
        for (int i=0; i<a; i++)
        {
            res*=2;
        };
    };

    return res;
};

int32_t med5(int32_t* buf) // Медиана из 5 элементов, буфер на 5 элементов
{
	int32_t res = 0;
	
	for( int32_t i = 0; i < 5; ++i)
	{
		int32_t thr = 0;
		int32_t s   = 0;
		for( int32_t j = 0; j < 5; ++j)
		{
			if( i == j) continue;
			
			if( buf[i] > buf[j] )      {s++;}
			else if( buf[i] < buf[j] ) {s--;}
			else                       {thr++;};
			
			if( fnAbs(s) <= thr )
			{
				res = buf[i];
			}			
		}
	}
	
	return res;
};

int32_t med3(int32_t* buf)       // поиск медианого значения в буфере из 3-х элементов
{
	int32_t& a = buf[0];
	int32_t& b = buf[1];
	int32_t& c = buf[2];
	
	if(a > b && a < c) return a;
	if(a < b && a > c) return a;
	
	if(b > a && b < c) return b;
	if(b < a && b > c) return b;
	
	return c;
};
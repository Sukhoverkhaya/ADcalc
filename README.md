# Разметка файлов базы и сравнение с референтной с расчетом статистик

## Структура проекта 

### C++

***Алгоритм разметки (папка Alg)***

[`ToneOnlyDetect.h`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/ToneOnlyDetect.h) - Детектор тонов. Взят из WebDevice, из изменений только привязка некоторых пороговых значений к подаваемой на вход частоте дискретизации (было захардкожено на 1000 Гц по понятным причинам, но минимум одна из используемых баз снята на 250 Гц).

[`PulsOnlyDetect.h`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/PulsOnlyDetect.h) - Детектор пульсаций (с параметризацией). Взят из WebDevice, изменений не вносилось.

[`DiffFilter.h`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/DiffFilter.h) - Дифференциатор, основа взята из WebDevice и частично переделана.

[`Sumfilter.h`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/SumFilter.h) - Интегратор, основа взята из WebDevice и частично переделана.

[`header_rw.h`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/header_rw.h) - Чтение хедеров, написано с использованием кода Алексея с изменениями.

[`badsplit.h`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/badsplit.h) - самодельный сплит (используется для целей формирования путей к сохраняемым файлам на основе путей обрабатываемых файлов).

[`main.cpp`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/Alg/main.cpp) - Сводит вышеперечисленное в алгоритм разметки одного файла. 
exe запускается по файлам базы из scripts/run.jl. 
Сохраняет результат в data/*Имя базы*/*Имя файла*_...

### Julia
***Папка scripts:***

[`run.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/run.jl) - запуск exe с алгоримом разметки (предварительно сбилдить Alg/main.cpp) по выбранной базе (см. комментарии в самом файле).

[`markup_reduction.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/markup_reduction.jl) - переведение разметки из формата на выходе алгоритма разметки в "шуишный" формат (в котором хранится референтная разметка).

[`markup_compare.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/markup_compare.jl) - сравнение референтной и тестовой разметок по выбранной базе с расчетом статистик.

[`visualisation.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/visualisation.jl) - используется для отладки.

***Папка src:***

[`readfiles.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/src/readfiles.jl) - взятый ранее скрипт для чтения бинарей с хедерами.

## Сценарий использования:

1. Натравливание алгоритма на базу > [`scripts/run.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/run.jl)

    * Указать имя базы и скорректирвать путь к ней. Имя базы должно соответствовать названию всех относящихся к ней папок (c бинарями, с реф. разметкой, с таблицами АД из КТ3 и т.д., где бы они ни лежали).

    > ВАЖНО, чтобы путь в качестве разделителей содержал только '/'. Подразумевается, что  последний элемент пути - имя бинаря и хедера без расширения, предпоследний - имя базы.

    * Предварительно создать папку data/*имя базы* в текущем проекте.

2. Переведение полученной разметки в "стандартный" формат > [`scripts/markup_reduction.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/markup_reduction.jl)

    * Указать все необходимые пути (см. комментарии в самом файле; отсутствующие папки для сохранения разметки будут созданы автоматически).

3. Сравнение тестовой разметки с реф (с расчетом статистик) > [`scripts/markup_compare.jl`](https://github.com/Sukhoverkhaya/ADcalc/blob/main/scripts/markup_compare.jl)

    * ! Добавить в папку ref from gui/*название базы* референтрую разметку для необходимых баз (не добавляется на прямую из проекта гуи, потому что там у папок с реф и тест разметкой для этого алгоритма разные названия, чтобы можно было просмотреть тестовую).

    * Указать все необходимые пути (см. комментарии в самом файле; отсутствующие папки для сохранения разметки будут созданы автоматически).


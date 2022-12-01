# запуск используемого алгоритма на плюсах по базе

basename = "Noise Base" # имя базы = имя папки с ней (если базы лежат в одном месте, для прогона по всем менять только эту строку)
dir = "D:/INCART/Pulse_Data/all bases/$basename" # полный путь к базе

allfiles = readdir(dir)
bins = filter(x -> split(x, ".")[end] == "bin", allfiles)
fnames = map(x -> split(x, ".")[end-1], bins)

# запуск алгоритма по всем файлам
for i in fnames run(`D:/INCART/PulseDetectorVer1/build/pulsedetcpp.exe "$dir/$i"`) end

# результат сохраняется в data/*имя базы, как в пути к файлу*/*имя файла, как в пути*_...


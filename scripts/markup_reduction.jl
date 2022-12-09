# Переведение разметки из формата, который (ПОКА) выдаёт плюсовый алгоритм, в "гуишный" формат
# (можно будет отсматривать в гуи + сравнивать с референтной, которая формируется в гуи)
using DataFrames

include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/help_func.jl")
include("../src/readfiles.jl") 
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/refmkpguifunctions.jl") 

function readcppmkp(fname, vseg)

    buf = Int32[]
    open(fname) do file
        line = readline(file) # пропускаем заголовок
        while !eof(file)
            line = readline(file)
            vls = split(line, "   ")
            val = parse(Int32, vls[1])
            push!(buf, val)
        end
    end

    mkp = map(i -> filter(x -> x >= i.ibeg && x <= i.iend, buf), vseg)

    return mkp
end

# пути
rawpath = "D:/INCART/Pulse_Data/all bases" # путь к бинарям
mkppath = "data" # локальный путь с папкой, где лежать результаты разметки после run.jl
adpath = "D:/INCART/Pulse_Data/ref AD" # путь к папке с таблицами АД из КТ3
basenm = "Noise Base" # имя базы (соответсвует имени всех относящихся к ней папок)
# filename = "PX11321102817293" # отладка

redalgmarkuppath = "formatted alg markup" # куда сохраняется итоговый формат разметки
#____________________________________________________________________________________________

allbasefiles = readdir("$rawpath/$basenm")
allbins = filter(x -> split(x, ".")[end]=="bin", allbasefiles)
allfnames = map(x -> split(x, ".")[1], allbins)

for filename in allfnames[1:end]
# filename = allfnames[1]
    # чтение исходного сигнала
    signals, fs, _, _ = readbin("$rawpath/$basenm/$filename")
    Tone = signals.Tone  # пульсации
    Pres = signals.Pres  # давление

    # получение валидных сегментов (тем же способом, что и в алгоритме - в противном случае вытянуть из самой разметки)
    vseg = get_valid_segments(Pres, Tone, 15, -1e7, 30*fs)

    Pres_mkp = readcppmkp("$mkppath/$basenm/$(filename)_pres_mkp.txt", vseg)
    Tone_mkp = readcppmkp("$mkppath/$basenm/$(filename)_tone_mkp.txt", vseg)

    # перетягиваем разметку из алгоритма в гуишную с поправкой на позицию начала сегмента, так как в гуишной
    # начало каждого сегмента принимается за ноль (но хранится информация о положении начала сегмента во всем сигнале)
    Pres_guimkp = map((x,d) -> map(y -> PresGuiMkp(y-d.ibeg, y-d.ibeg, 1), x), Pres_mkp, vseg)
    Tone_guimkp = map((x,d) -> map(y -> ToneGuiMkp(y-d.ibeg, 1), x), Tone_mkp, vseg)

    # получение референтных границ АД и рабочей зоны на накачке и на спуске (ПО АМПЛИТУДЕ)
    # ad_alg0 = read_alg_ad("$adpath/$basenm/$filename.ad")

    admkpfile = "$mkppath/$basenm/$(filename)_ad_mkp.txt"
    adi = NamedTuple{(:pos, :val), NTuple{2, Int}}[]
    open(admkpfile) do file # Открываем файл
        needwrite = false
        while !eof(file)
            line = readline(file)
            if needwrite
                values = split(line, '	')
                push!(adi, (pos = parse(Int, values[1]), val = parse(Int, values[2])))
            end
            needwrite = true
        end
    end

    ad_alg0 = NamedTuple{(:pump, :desc), NTuple{2, AD}}[]
    for i in 1:lastindex(vseg)
        localad = filter(x -> x.pos >= vseg[i].ibeg && x.pos <= vseg[i].iend, adi)
        adloc = AD(0,0)
        if isempty(localad)
            push!(ad_alg0, (pump = adloc, desc = adloc))
        elseif length(localad) == 1 # если нашло только одно, то это только дад на накачке
            push!(ad_alg0, (pump = AD(0, localad[1].val), desc = adloc))
        elseif length(localad) == 2 
            if localad[1].pos < localad[2].pos 
                push!(ad_alg0, (pump = AD(localad[2].val, localad[1].val), desc = adloc))
            else
                push!(ad_alg0, (pump = adloc, desc = AD(localad[1].val, localad[2].val)))
            end
        elseif length(localad) == 3
            push!(ad_alg0, (pump = AD(localad[2].val, localad[1].val), desc = AD(localad[3].val, 0)))
        elseif length(localad) == 4
            push!(ad_alg0, (pump = AD(localad[2].val, localad[1].val), desc = AD(localad[3].val, localad[4].val)))
        end
    end

    # ad_alg0

    # границы рабочей зоны (пик треугольника давления - 10 мм, минимум спуска + 10 мм)
    # на спуске (ПО АМПЛИТУДЕ)
    wzbounds = map(x -> get_workzone_bounds(Pres[x.ibeg:x.iend]), vseg)

    # запись разметки в структуру файлов
    try readdir(redalgmarkuppath)
    catch e mkdir(redalgmarkuppath) end
    try readdir("$redalgmarkuppath/$basenm")
    catch e mkdir("$redalgmarkuppath/$basenm") end
    try readdir("$redalgmarkuppath/$basenm/$filename")
    catch e mkdir("$redalgmarkuppath/$basenm/$filename") end

    for i in 1:lastindex(vseg)

        ad_alg = (pump = Bounds(ad_alg0[i].pump.SAD, ad_alg0[i].pump.DAD), desc = Bounds(ad_alg0[i].desc.SAD, ad_alg0[i].desc.DAD))
        x = vseg[i]
        # получение позиций границ рабочей зоны и АД
        pump_adbounds = get_bounds(Pres[x.ibeg:x.iend], ad_alg.pump, true)
        desc_adbounds = get_bounds(Pres[x.ibeg:x.iend], ad_alg.desc, false)
        iadbounds = (pump = pump_adbounds, desc = desc_adbounds)

        pump_wzbounds = get_bounds(Pres[x.ibeg:x.iend], wzbounds[i].pump, true)
        desc_wzbounds = get_bounds(Pres[x.ibeg:x.iend], wzbounds[i].desc, false)
        iwzbounds = (pump = pump_wzbounds, desc = desc_wzbounds)

        fold = "$redalgmarkuppath/$basenm/$filename/$i"
        try readdir(fold)
        catch e mkdir(fold) end    
        SaveRefMarkup("$fold/pres.csv", Pres_guimkp[i])
        SaveRefMarkup("$fold/tone.csv", Tone_guimkp[i])
        SaveRefMarkup("$fold/bounds.csv", Pres[x.ibeg:x.iend], x, iadbounds, iwzbounds)
    end
end
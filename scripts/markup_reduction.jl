# Переведение разметки из формата, который (ПОКА) выдаёт плюсовый алгоритм, в "гуишный" формат
# (можно будет отсматривать в гуи + сравнивать с референтной, которая формируется в гуи)
using DataFrames

include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/help_func.jl")
include("../src/readfiles.jl") 
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/refmkpguifunctions.jl") 

function readcppmkp(fname, vseg)

    buf = NamedTuple{(:pos, :bad), NTuple{2, Int64}}[]

    open(fname) do file
        line = readline(file) # пропускаем заголовок
        while !eof(file)
            line = readline(file)
            vls = split(line, '	')
            val = map(x -> parse(Int64, x), vls)
            push!(buf, (pos = val[1], bad = val[end]))
        end
    end

    mkp = map(i -> filter(x -> x.pos >= i.ibeg && x.pos <= i.iend, buf), vseg)

    return mkp
end

# пути
rawpath = "D:/INCART/Pulse_Data/all bases" # путь к бинарям
mkppath = "data" # локальный путь с папкой, где лежать результаты разметки после run.jl
adpath = "D:/INCART/Pulse_Data/ref AD" # путь к папке с таблицами АД из КТ3
basenm = "polifunctional" # имя базы (соответсвует имени всех относящихся к ней папок)
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

    Pres_mkp = readcppmkp("$mkppath/$basenm/$(filename)_pulse_mkp.txt", vseg)
    Tone_mkp = readcppmkp("$mkppath/$basenm/$(filename)_tone_mkp.txt", vseg)

    # перетягиваем разметку из алгоритма в гуишную с поправкой на позицию начала сегмента, так как в гуишной
    # начало каждого сегмента принимается за ноль (но хранится информация о положении начала сегмента во всем сигнале)
    Pres_guimkp = map((x,d) -> map(y -> PresGuiMkp(y.pos-d.ibeg, y.pos-d.ibeg, y.bad), x), Pres_mkp, vseg)
    Tone_guimkp = map((x,d) -> map(y -> ToneGuiMkp(y.pos-d.ibeg, y.bad), x), Tone_mkp, vseg)

    # получение референтных границ АД и рабочей зоны на накачке и на спуске (ПО АМПЛИТУДЕ)
    # ad_alg0 = read_alg_ad("$adpath/$basenm/$filename.ad")

    admkpfile = "$mkppath/$basenm/$(filename)_ad_mkp.txt"
    adi = NamedTuple{(:ibeg, :iend), NTuple{2, Int}}[]

    open(admkpfile) do file # Открываем файл
        needwrite = false
        while !eof(file)
            line = readline(file)
            if needwrite
                values = split(line, '	')
                push!(adi, (ibeg = parse(Int, values[1]), iend = parse(Int, values[3])))
            end
            needwrite = true
        end
    end

    # adi

    ad_alg0 = NamedTuple{(:pump, :desc), NTuple{2, AD}}[]
    for i in 1:lastindex(vseg)
        prseg = Pres[vseg[i].ibeg:vseg[i].iend]
        localad = filter(x -> x.ibeg >= vseg[i].ibeg && x.iend <= vseg[i].iend, adi)
        localad = map(x -> (ibeg = x.ibeg - vseg[i].ibeg, iend = x.iend - vseg[i].ibeg), localad)
        randpos = floor(length(prseg)/4)
        pumpadloc = AD(randpos-20, randpos-60) # подставляем, где не найдены границы на каком-то участке
        descadloc = AD(randpos-20, randpos-60) # чтобы гуи в любом случае было, что читать
        if isempty(localad)
            push!(ad_alg0, (pump = pumpadloc, desc = descadloc))
        elseif length(localad) == 1 # найдено только на накачке или на спуске
            if prseg[localad[1].ibeg] > prseg[localad[1].iend]      # значит на спуске
                push!(ad_alg0, (pump = pumpadloc, desc = AD(localad[1].ibeg, localad[1].iend)))
            else                                                    # значит на накачке
                push!(ad_alg0, (pump = AD(localad[1].iend, localad[1].ibeg), desc = descadloc))
            end
        elseif length(localad) == 2 # найдено на накачке и на спуске
            if localad[1].ibeg < localad[2].ibeg # скорее всего, всегда попадаем только в это условие, потому что всегда пишем в порядке накачка-спуск
                push!(ad_alg0, (pump = AD(localad[1].iend, localad[1].ibeg), desc = AD(localad[2].ibeg, localad[2].iend)))
            else
                push!(ad_alg0, (pump = AD(localad[2].iend, localad[2].ibeg), desc = AD(localad[1].ibeg, localad[1].iend)))
            end
        end
    end

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

        ad_alg = (  pump = Bounds(ad_alg0[i].pump.SAD, ad_alg0[i].pump.DAD), 
                    desc = Bounds(ad_alg0[i].desc.SAD, ad_alg0[i].desc.DAD))
        x = vseg[i]

        pump_wzbounds = get_bounds(Pres[x.ibeg:x.iend], wzbounds[i].pump, true)
        desc_wzbounds = get_bounds(Pres[x.ibeg:x.iend], wzbounds[i].desc, false)
        iwzbounds = (pump = pump_wzbounds, desc = desc_wzbounds)

        fold = "$redalgmarkuppath/$basenm/$filename/$i"
        try readdir(fold)
        catch e mkdir(fold) end    
        SaveRefMarkup("$fold/pres.csv", Pres_guimkp[i])
        SaveRefMarkup("$fold/tone.csv", Tone_guimkp[i])
        SaveRefMarkup("$fold/bounds.csv", Pres[x.ibeg:x.iend], x, ad_alg, iwzbounds)
    end
end
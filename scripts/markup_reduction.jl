using DataFrames

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

# readcppmkp("data/KT 07 AD ECG/PX11321102817293_pres_mkp.txt")


include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/help_func.jl")
include("../src/readfiles.jl") 
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/refmkpguifunctions.jl") 

# пути
rawpath = "D:/INCART/Pulse_Data/all bases"
mkppath = "data"
adpath = "D:/INCART/Pulse_Data/ref AD"
basenm = "Noise Base"
# filename = "PX11321102817293"

redalgmarkuppath = "formatted alg markup"

allbasefiles = readdir("$rawpath/$basenm")
allbins = filter(x -> split(x, ".")[end]=="bin", allbasefiles)
allfnames = map(x -> split(x, ".")[1], allbins)

for filename in allfnames[1:end]
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
    ad_alg0 = read_alg_ad("$adpath/$basenm/$filename.ad")

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
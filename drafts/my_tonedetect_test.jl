using Plots

include("../src/readfiles.jl")
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/help_func.jl")

binfile = "D:/INCART/Pulse_Data/all bases/KT 07 AD ECG/PX11321102817293"
# binfile = "D:/INCART/Pulse_Data/all bases/Noise Base/Zaytsev_29-01-20_18-19-31_"
signals, fs, _, _, lsbs = readbin(binfile)

Pres = signals.Pres
Tone = signals.Tone

vseg = get_valid_segments(Pres, Tone, 15, -1e7, 30*fs)

b = 3; ## номер валидного участка (одного измерения)

# seg = Tone[vseg[b].ibeg:vseg[b].iend]
# plot(seg)

# ПРОВЕРКА ДЛЯ ТОНОВ

## ФИЛЬТРЫ ##
fname = "data/KT 07 AD ECG/PX11321102817293_filttone.txt"

function readfilt(fname)
    colN = 0
    rowN = 0
    names = String[]
    open(fname) do file
        # зачитываем  имена столбцов
        line = readline(file)
        names = split(line, '	')
        colN = length(names) # число столбцов в файле
        
        # считаем кол-во строк в файле
        while !eof(file) 
            readline(file)
            rowN += 1 
        end
    end

    # читаем файл в матрицу
    data = Matrix{Int32}(undef, rowN, colN)
    open(fname) do file
        line = readline(file) # пропускаем заголовочную строку
        i = 1;
        while !eof(file) 
            line = readline(file)
            subs = split(line, '	')
            vals = map(x -> parse(Int32, x), subs)
            data[i,:] = vals
            i += 1
        end
    end

    signals = [data[:, col] for col in 1:colN] |> Tuple
    symb_names = Symbol.(names) |> Tuple

    res = NamedTuple{symb_names}(signals)

    return res
end

filtered = readfilt(fname)

k = (fs == 1000) ? 8 : 2 # к-т децимации

# plot(filtered.tone[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
# plot!(filtered.rawTone[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
# plot(filtered.smoothTone[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
# plot!(filtered.smth_dcm_abs[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
# plot(filtered.env[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
# plot!(filtered.envfilt[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])

#### РАЗМЕТКА ###
fname = "data/KT 07 AD ECG/PX11321102817293_tone_mkp.txt"

function readmkp(fname)
    mkp = []
    open(fname) do file
        line = readline(file)
        while !eof(file)
            line = readline(file)
            val = map(x -> parse(Int32, x), split(line, '	'))
            push!(mkp, (pos = val[1], bad = val[2]));
        end
    end

    return mkp
end

mkp = readmkp(fname)

vmkp = filter(x -> x.pos > floor(Int, vseg[b].ibeg/k) && x.pos < floor(Int, vseg[b].iend/k), mkp)
vpeaks = map(x -> x.pos - floor(Int, vseg[b].ibeg/k), vmkp)
vbad = map(x -> x.bad, vmkp)
badpeaks = map(x -> x.pos-floor(Int, vseg[b].ibeg/k), filter(x -> x.bad!=0, vmkp))
zerobadpeaks = map(x -> x.pos-floor(Int, vseg[b].ibeg/k), filter(x -> x.bad==0, vmkp))

segenv = filtered.envfilt[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvR = filtered.LvR[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvN = filtered.LvN[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvP = filtered.LvP[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]

plot(segenv, label = "sig")
plot!(segLvR, label = "LvR")
plot!(segLvN, label = "LvN")
plot!(segLvP, label = "LvP")
xlims!(6000, 12000)
ylims!(-1000, 10000)

scatter!(zerobadpeaks, segenv[zerobadpeaks])
scatter!(badpeaks, segenv[badpeaks])

# ПРОВЕРКА ДЛЯ ПУЛЬСАЦИЙ

## ФИЛЬТРЫ ##
fname = "data/KT 07 AD ECG/PX11321102817293_filtpulse.txt"

filtered = readfilt(fname)

plot(filtered.pulse[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot!(filtered.pressDcm[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot!(filtered.smoothPulse[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot(filtered.basePulse[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot!(filtered.tch[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])

#### РАЗМЕТКА ###
fname = "data/KT 07 AD ECG/PX11321102817293_pulse_mkp.txt"

mkp = readmkp(fname)

vmkp = filter(x -> x.pos > floor(Int, vseg[b].ibeg/k) && x.pos < floor(Int, vseg[b].iend/k), mkp)
vpeaks = map(x -> x.pos - floor(Int, vseg[b].ibeg/k), vmkp)
vbad = map(x -> x.bad, vmkp)
badpeaks = map(x -> x.pos-floor(Int, vseg[b].ibeg/k), filter(x -> x.bad!=0, vmkp))
zerobadpeaks = map(x -> x.pos-floor(Int, vseg[b].ibeg/k), filter(x -> x.bad==0, vmkp))

segenv = filtered.basePulse[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvR = filtered.LvR[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvZ = filtered.LvZ[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvP = filtered.LvP[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]

plot(segenv, label = "sig")
plot!(segLvR, label = "LvR")
plot!(segLvZ, label = "LvZ")
plot!(segLvP, label = "LvP")
# xlims!(2000, 6000)
ylims!(-2000, 5000)

scatter!(zerobadpeaks, segenv[zerobadpeaks])
scatter!(badpeaks, segenv[badpeaks])

filter(x -> x!=0, vbad)
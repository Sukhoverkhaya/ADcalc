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
k = (fs == 1000) ? 8 : 2 # к-т децимации

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
keys(filtered)

tone = resample(filtered.tone, k)
rawTone = resample(filtered.rawTone, k)
smooth = resample(filtered.smoothTone, k)
smth_dcm_abs = resample(filtered.smth_dcm_abs, k)
env = resample(filtered.env, k)
envfilt = resample(filtered.envfilt, k)
LvR = resample(filtered.LvR, k)
LvN = resample(filtered.LvN, k)
LvP = resample(filtered.LvP, k)

plot(tone[vseg[b].ibeg:vseg[b].iend-100])
plot!(rawTone[vseg[b].ibeg:vseg[b].iend-100])
plot!(smooth[vseg[b].ibeg:vseg[b].iend-100])
plot!(smth_dcm_abs[vseg[b].ibeg:vseg[b].iend-100])

plot(smth_dcm_abs[vseg[b].ibeg:vseg[b].iend-100])
plot!(env[vseg[b].ibeg:vseg[b].iend-100])
plot!(envfilt[vseg[b].ibeg:vseg[b].iend-100])

plot(envfilt[vseg[b].ibeg:vseg[b].iend-100], label = "envelope")
xlims!(10000, 11000)
ylims!(-1000, 4000)

plot!(LvR[vseg[b].ibeg:vseg[b].iend-100], label = "LvR")
plot!(LvN[vseg[b].ibeg:vseg[b].iend-100], label = "LvN")
plot!(LvP[vseg[b].ibeg:vseg[b].iend-100], label = "LvP")

#### РАЗМЕТКА ###
fname = "data/KT 07 AD ECG/PX11321102817293_tone_mkp.txt"

function readmkp(fname)
    mkp = []
    open(fname) do file
        line = readline(file)
        line = readline(file)
        line = readline(file)

        while !eof(file)
            line = readline(file)
            val = map(x -> parse(Int32, x), split(line, '	'))
            if length(val) == 2
                push!(mkp, (pos = val[1], bad = val[2]));
            elseif length(val) == 3
                push!(mkp, (ibeg = val[1], iend = val[2], bad = val[3]));
            end
        end
    end

    return mkp
end

mkp = readmkp(fname)

vmkp = filter(x -> x.pos > vseg[b].ibeg && x.pos < vseg[b].iend, mkp)
vpeaks = map(x -> x.pos - vseg[b].ibeg, vmkp)
vbad = map(x -> x.bad, vmkp)
badpeaks = map(x -> x.pos-vseg[b].ibeg, filter(x -> x.bad!=0, vmkp))
zerobadpeaks = map(x -> x.pos-vseg[b].ibeg, filter(x -> x.bad==0, vmkp))

segenv = envfilt[vseg[b].ibeg:vseg[b].iend]
segLvR = LvR[vseg[b].ibeg:vseg[b].iend]
segLvN = LvN[vseg[b].ibeg:vseg[b].iend]
segLvP = LvP[vseg[b].ibeg:vseg[b].iend]

delay = 100

plot(segenv[1:end-delay], label = "sig")
plot!(segLvR[1:end-delay], label = "LvR")
plot!(segLvN[1:end-delay], label = "LvN")
plot!(segLvP[1:end-delay], label = "LvP")
xlims!(8000, 12000)
ylims!(-1000, 7500)

scatter!(zerobadpeaks.+k, segenv[zerobadpeaks.+k])
scatter!(badpeaks.+k, segenv[badpeaks.+k])

# ПРОВЕРКА ДЛЯ ПУЛЬСАЦИЙ

## ФИЛЬТРЫ ##
fname = "data/KT 07 AD ECG/PX11321102817293_filtpulse.txt"

filtered = readfilt(fname)
pulse = resample(filtered.pulse, k)
pressDcm = resample(filtered.pressDcm, k)
smoothPulse = resample(filtered.smoothPulse, k)
basePulse = resample(filtered.basePulse, k)
tch = resample(filtered.tch, k)
LvR = resample(filtered.LvR, k)
LvZ = resample(filtered.LvZ, k)
LvP = resample(filtered.LvP, k)

plot(pulse[vseg[b].ibeg:vseg[b].iend-delay])
plot!(pressDcm[vseg[b].ibeg:vseg[b].iend-delay])
plot!(smoothPulse[vseg[b].ibeg:vseg[b].iend-delay])
plot(basePulse[vseg[b].ibeg:vseg[b].iend-delay])
plot!(tch[vseg[b].ibeg:vseg[b].iend-delay])

#### РАЗМЕТКА ###
fname = "data/KT 07 AD ECG/PX11321102817293_pulse_mkp.txt"

mkp = readmkp(fname)

vmkp = filter(x -> x.ibeg > vseg[b].ibeg && x.iend < vseg[b].iend, mkp)
vpeaks = map(x -> (ibeg = x.ibeg - vseg[b].ibeg, iend = x.iend - vseg[b].ibeg), vmkp)
vbad = map(x -> x.bad, vmkp)
badpeaks = map(x -> (ibeg = x.ibeg - vseg[b].ibeg, iend = x.iend - vseg[b].ibeg), filter(x -> x.bad!=0, vmkp))
zerobadpeaks = map(x -> (ibeg = x.ibeg - vseg[b].ibeg, iend = x.iend - vseg[b].ibeg), filter(x -> x.bad==0, vmkp))

segenv = tch[vseg[b].ibeg:vseg[b].iend-delay]
segLvR = LvR[vseg[b].ibeg:vseg[b].iend-delay]
segLvZ = LvZ[vseg[b].ibeg:vseg[b].iend-delay]
segLvP = LvP[vseg[b].ibeg:vseg[b].iend-delay]

# plot(basePulse[vseg[b].ibeg:vseg[b].iend-delay], label = "pulse")
plot(segenv.*10, label = "tacho")
plot!(segLvR, label = "LvR")
plot!(segLvZ, label = "LvZ")
plot!(segLvP, label = "LvP")
xlims!(5000, 8000)
ylims!(-2000, 5000)

begs = map(x -> x.ibeg, zerobadpeaks)
ends = map(x -> x.iend, zerobadpeaks)
scatter!(begs, segenv[begs])
scatter!(ends, segenv[ends])

begs = map(x -> x.ibeg, badpeaks)
ends = map(x -> x.iend, badpeaks)
scatter!(begs, segenv[begs])
scatter!(ends, segenv[ends])

filter(x -> x!=0, vbad)
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

seg = Tone[vseg[b].ibeg:vseg[b].iend]
plot(seg)

## ФИЛЬТРЫ ##
fname = "build/filttone.txt"
tone = Int32[]; rawtone = Int32[];
smoothTone = Int32[];   smth_dcm_abs = Int32[]; 
env = Int32[];  envfilt = Int32[]; 
LvR = Int32[]; LvN = Int32[]; LvP = Int32[];
open(fname) do file
    while !eof(file)
        line = readline(file)
        vals = map(x -> parse(Int32, x), split(line, '	'))
        push!(tone, vals[1]); push!(rawtone, vals[2]);
        push!(smoothTone, vals[3]); push!(smth_dcm_abs, vals[4]); 
        push!(env, vals[5]); push!(envfilt, vals[6]); 
        push!(LvR, vals[7]); push!(LvN, vals[8]); push!(LvP, vals[9]);
    end
end

k = (fs == 1000) ? 4 : 1 # к-т децимации

plot(tone[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot!(rawtone[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot(smoothTone[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot!(smth_dcm_abs[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot(env[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])
plot!(envfilt[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)])

#### РАЗМЕТКА ###
fname = "build/tonepeaks.txt"

mkp = []
open(fname) do file
    while !eof(file)
        line = readline(file)
        val = map(x -> parse(Int32, x), split(line, '	'))
        push!(mkp, (pos = val[1], bad = val[2]));
    end
end

vmkp = filter(x -> x.pos > floor(Int, vseg[b].ibeg/k) && x.pos < floor(Int, vseg[b].iend/k), mkp)
vpeaks = map(x -> x.pos - floor(Int, vseg[b].ibeg/k), vmkp)
vbad = map(x -> x.bad, vmkp)
badpeaks = map(x -> x.pos-floor(Int, vseg[b].ibeg/k), filter(x -> x.bad!=0, vmkp))
zerobadpeaks = map(x -> x.pos-floor(Int, vseg[b].ibeg/k), filter(x -> x.bad==0, vmkp))

segenv = envfilt[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvR = LvR[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvN = LvN[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]
segLvP = LvP[floor(Int, vseg[b].ibeg/k):floor(Int, vseg[b].iend/k)]

plot(segenv, label = "sig")
plot!(segLvR, label = "LvR")
plot!(segLvN, label = "LvN")
plot!(segLvP, label = "LvP")
xlims!(6000, 12000)
ylims!(-1000, 10000)

# segenv[badpeaks[18]]
# plot!([0, 16000], [segenv[badpeaks[18]], segenv[badpeaks[18]]]./3)
# # i1 = vpeaks[39] - 26
# i2 = i1 - 20
# maximum(segenv[i2:i1])

# plot!([i1,i1],[-1000, 5000])
# plot!([i2,i2],[-1000, 5000])
# plot!([7500, 10000],[segenv[vpeaks[39]]/3, segenv[vpeaks[39]]/3])

scatter!(zerobadpeaks, segenv[zerobadpeaks])
scatter!(badpeaks, segenv[badpeaks])

ad1 = 118669 - floor(Int, vseg[b].ibeg/k)
ad2 = 126095 - floor(Int, vseg[b].ibeg/k)

plot!([ad1, ad1], [-1000, 7500])
plot!([ad2, ad2], [-1000, 7500])
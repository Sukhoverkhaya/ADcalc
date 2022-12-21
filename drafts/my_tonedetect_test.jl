using Plots

include("../src/readfiles.jl")
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/help_func.jl")

binfile = "D:/INCART/Pulse_Data/all bases/KT 07 AD ECG/PX11321102817293"
signals, fs, _, _, lsbs = readbin(binfile)

Pres = signals.Pres
Tone = signals.Tone

vseg = get_valid_segments(Pres, Tone, 15, -1e7, 30*fs)

b = 4; ## номер валидного участка (одного измерения)

seg = Tone[vseg[b].ibeg:vseg[b].iend]

fname = "build/tonepeaks.txt"

peaks = Int32[]
open(fname) do file
    line = readline(file)
    while !eof(file)
        line = readline(file)
        val = parse(Int32, line)
        push!(peaks, val)
    end
end

vpeaks = filter(x -> x > vseg[b].ibeg && x < vseg[b].iend, peaks)
vpeaks = vpeaks.-vseg[b].ibeg

plot(seg)
scatter!(vpeaks, seg[vpeaks])
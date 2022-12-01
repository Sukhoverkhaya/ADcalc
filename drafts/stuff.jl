using Plots
include("../src/readfiles.jl")
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/help_func.jl")

# fname = "PX113211018182653"
# fdir = "D:/INCART/Pulse_Data/all bases/KT 07 AD ECG/"
# fname = "Abasova_16-01-21_13-36-57_"
basename = "KT 07 AD ECG"
fdir = "D:/INCART/Pulse_Data/all bases/$basename"
resdir = "D:/INCART/PulseDetectorVer1/data/$basename"

allfiles = readdir(fdir)
bins = filter(x -> split(x, ".")[end] == "bin", allfiles)
fnames = map(x -> split(x, ".")[end-1], bins)

fname = fnames[7]

# run(`D:/INCART/PulseDetectorVer1/build/pulsedetcpp.exe $fdir/$fname`)

binfile = "$fdir/$fname.bin"
signals, fs, _, _, lsbs = readbin(binfile)
keys(signals)

Pres = signals.Pres
Tone = signals.Tone

vseg = get_valid_segments(Pres, Tone, 15, -1e7, 30*fs)

b = 4; ## номер валидного участка (одного измерения)

seg = Pres[vseg[b].ibeg:vseg[b].iend]

diff = "$resdir/$(fname)_diff.txt"

dsig = Int32[]
open(diff) do file
    while !eof(file)
        line = readline(file)
        val = parse(Int32, line)
        push!(dsig, val)
    end
end
dsig = dsig.*lsbs[2]
# unique(dsig)
# findall(x -> x!=0.0, dsig)

diffseg = dsig[vseg[b].ibeg:vseg[b].iend]
plot(seg)
plot!(diffseg)

argmin(diffseg) - argmax(seg)

#######
fmkp = "$resdir/$(fname)_pres_mkp.txt"

mkp = Int32[]
open(fmkp) do file
    line = readline(file) # пропускаем заголовок
    while !eof(file)
        line = readline(file)
        vls = split(line, "   ")
        val = parse(Int32, vls[1])
        push!(mkp, val)
    end
end
mkp
segmkp = filter(x -> x>vseg[b].ibeg && x<vseg[b].iend, mkp)
segmkp = segmkp .- vseg[b].ibeg .- 61
scatter!(segmkp, seg[segmkp], markersize = 1)
scatter!(segmkp, diffseg[segmkp], markersize = 1)

#####
Tone = signals.Tone
segtone = Tone[vseg[b].ibeg:vseg[b].iend]

plot(segtone)

ftonemkp = "$resdir/$(fname)_tone_mkp.txt"
tonemkp = Int32[]
open(ftonemkp) do file
    line = readline(file) # пропускаем заголовок
    while !eof(file)
        line = readline(file)
        val = parse(Int32, line)
        push!(tonemkp, val)
    end
end
tonemkp

segtonemkp = filter(x -> x>vseg[b].ibeg+248 && x<vseg[b].iend, tonemkp);
segtonemkp = segtonemkp .- vseg[b].ibeg .- 248
scatter!(segtonemkp, segtone[segtonemkp], markersize=1)
# xlims!(7000, 9000)

using Plots
include("../src/readfiles.jl")

binfile = "D:/INCART/PulseDetectorVer1/data/PX113211018182653"
signals, fs, _, _, lsbs = readbin(binfile)
lsbs
fs
length(keys(signals))
length(signals[1])

Pres = signals.Pres
Pres[findfirst(x -> x>0, Pres)]
seg = Pres[30000:42600]

diff = "D:/INCART/PulseDetectorVer1/build/diff.txt"

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

diffseg = dsig[30000:42600]
plot(seg)
plot!(diffseg)

#######
fmkp = "D:/INCART/PulseDetectorVer1/build/pulse_mkp.txt"

mkp = Int32[]
open(fmkp) do file
    while !eof(file)
        line = readline(file)
        val = parse(Int32, line)
        push!(mkp, val)
    end
end
mkp
segmkp = filter(x -> x>30000 && x<42600, mkp)
segmkp = segmkp .- 30000 .+ 126
scatter!(segmkp, seg[segmkp], markersize = 1)
scatter!(segmkp, diffseg[segmkp], markersize = 1)

#####
Tone = signals.Tone
segtone = Tone[30000:42600]

plot(segtone)

ftonemkp = "D:/INCART/PulseDetectorVer1/build/tone_mkp.txt"
tonemkp = Int32[]
open(ftonemkp) do file
    while !eof(file)
        line = readline(file)
        val = parse(Int32, line)
        push!(tonemkp, val)
    end
end
tonemkp

segtonemkp = filter(x -> x>30000+61 && x<42600, tonemkp);
segtonemkp = segtonemkp .- 30000 .- 61
scatter!(segtonemkp, segtone[segtonemkp])
xlims!(7000, 9000)

using Plots

include("../src/readfiles.jl") 
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/refmkpguifunctions.jl")

function mkpread(filename::String, skipfirst::Bool)
    vecmkp = Int32[]
    open(filename) do file
        if skipfirst line = readline(file) end# пропускаем заголовок
        while !eof(file)
            line = readline(file)
            vls = split(line, "   ")
            pos = parse(Int32, vls[1])
            push!(vecmkp, pos)
        end
    end
    return vecmkp
end

base = "Noise Base"
fname = "Zaytsev_29-01-20_18-19-31_"
mes = 1

bin = "D:/INCART/Pulse_Data/all bases/$base/$fname"
bnd = "D:/INCART/PulseDetectorVer1/formatted alg markup/$base/$fname/$mes/bounds.csv"

signals, fs, timestart, units = readbin(binfile)

Pres = signals.Pres
Tone = signals.Tone
bounds = ReadRefMkp(bnd)

segPres = Pres[bounds.segm.ibeg:bounds.segm.iend]
segTone = Tone[bounds.segm.ibeg:bounds.segm.iend]

mkp = "D:/INCART/PulseDetectorVer1/data/$base/$fname"

## отладка для пульсаций
pres_mkp = mkpread("$(mkp)_pres_mkp.txt", true)

segpresmkp = filter(x -> x >= bounds.segm.ibeg && x <= bounds.segm.iend, pres_mkp)
segpresmkp = segpresmkp .- bounds.segm.ibeg

diff = mkpread("$(mkp)_diff.txt", false)

segdiff = diff[bounds.segm.ibeg:bounds.segm.iend]/100
# del = round(Int, (0.1*fs/2)*2)
# segdiff = segdiff[del:end]
# segPres = segPres.-mean(segPres)

plot(segPres)
plot!(segdiff)
# xlims!(10000,25000)

scatter!(segpresmkp, segPres[segpresmkp], markersize=1)
scatter!(segpresmkp, segdiff[segpresmkp], markersize=1)

############### отладка для тонов
tone_mkp = mkpread("$(mkp)_tone_mkp.txt", true)

segtonemkp = filter(x -> x >= bounds.segm.ibeg && x <= bounds.segm.iend, pres_mkp)
segtonemkp = segtonemkp .- bounds.segm.ibeg

plot(segTone)
scatter!(segtonemkp, segTone[segtonemkp], markersize=1)
xlims!(57500,65000)
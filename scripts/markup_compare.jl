using Plots
using DataFrames

include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/beat2beat.jl") # скрипт для сравнения по точкам от Юлии Александровны
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/refmkpguifunctions.jl")
include("../src/readfiles.jl") 
include("D:/ИНКАРТ/Pulse_Sukhoverkhaya/src/my_filt.jl")

# из разметки тонов читаем позиции, из пульсаций - минимумы или макисмумы
struct MKP
    pos::Int32
    type::Int64
end

function readmkp(fname)

    mkp = MKP[]

    open(fname) do file
        line = readline(file) # пропускаем заголовок
        while !eof(file)
            line = readline(file)
            vls = split(line, "   ")
            if length(vls) == 2
                pos = parse(Int32, vls[1])
            else
                pos = parse(Int32, vls[2])
            end
            type = parse(Int32, vls[end])
            push!(mkp, MKP(pos, type))
        end
    end

    return mkp
end

# находится ли точка внутри рабочей зоны
function isinworkzone(wzbounds::Bounds, point::Union{Int64, Int32})
    if point >= minimum([wzbounds.ibeg, wzbounds.iend]) && point <= maximum([wzbounds.ibeg, wzbounds.iend])
        return true
    else
        return false
    end
end

function bothwz(wzpump::Bounds, wzdesc::Bounds, point::Union{Int64, Int32})
    isinpump = isinworkzone(wzpump, point)
    isindesc = isinworkzone(wzdesc, point)

    return isinpump || isindesc
end

struct Metrics
    TP::Int64
    FP::Int64
    FN::Int64
    Se::Float64
    PPV::Float64
end

function metricscalc(comp_res::Vector{Tuple{Int64, Int64}}, ref_pres::Vector{MKP}, alg_pres::Vector{MKP}, wzpump::Bounds, wzdesc::Bounds)
    
    TP = 0; FP = 0; FN = 0
    for (r, a) in comp_res
        if r != -1 && a != -1 # есть и в реф, и в тест
            if bothwz(wzpump, wzdesc, ref_pres[r].pos) 
                # if ref_pres[r].type != 2 # если не отмечена в реф, как шум
                    TP += 1 
                # else # если отмечена в реф, как шум
                #     FP += 1
                # end
            end
        elseif r != -1 && a == -1 # есть в реф, нету в тест
            if bothwz(wzpump, wzdesc, ref_pres[r].pos)
                if ref_pres[r].type != 0 # если не отмечена в реф, как незначимая
                    FN += 1
                end
            end
        elseif r == -1 && a != 1 # нету в реф, есть в тест
            if bothwz(wzpump, wzdesc, alg_pres[a].pos)
                FP += 1
            end
        end
    end
    Se = round(TP/(TP+FN)*100, digits = 2)
    PPV = round(TP/(TP+FP)*100, digits = 2)

    return Metrics(TP, FP, FN, Se, PPV)
end

function metricscalc(mt::Vector{Metrics})
    tTP = sum(map(x -> x.TP, mt))
    tFP = sum(map(x -> x.FP, mt))
    tFN = sum(map(x -> x.FN, mt))
    tSe = round(tTP/(tTP+tFN)*100, digits = 2)
    tPPV = round(tTP/(tTP+tFP)*100, digits = 2)

    return Metrics(tTP, tFP, tFN, tSe, tPPV)
end

ref_fold = "ref from gui"
mkp_fold = "formatted alg markup"
basename = "Noise Base"

# reffiles = readdir("$ref_fold/$basename")
mkpfiles = readdir("$mkp_fold/$basename")

ftmm0 = fill(Metrics(0,0,0,0.0,0.0), length(mkpfiles)) # вектор метрик по измерениям по всей базе
fpmm0 = fill(Metrics(0,0,0,0.0,0.0), length(mkpfiles)) # для подсчета среднего по всем файлам

afmetrics_pres = NamedTuple[]
afmetrics_tone = NamedTuple[]

for i in 1:lastindex(mkpfiles) # по файлам
    # i = 11
    reffile = "$ref_fold/$basename/$(mkpfiles[i])"
    mkpfile = "$mkp_fold/$basename/$(mkpfiles[i])"

    binfile = "D:/INCART/Pulse_Data/all bases/$basename/$(mkpfiles[i]).bin"
    signals, fs, timestart, units = readbin(binfile);

    mkpmeasures = readdir(mkpfile)
    refmeasures = readdir(reffile)
    measures = length(mkpmeasures) <= length(refmeasures) ? mkpmeasures : refmeasures

    tmm0 = fill(Metrics(0,0,0,0.0,0.0), length(mkpmeasures)) # вектор метрик по измерениям внутри файла
    pmm0 = fill(Metrics(0,0,0,0.0,0.0), length(mkpmeasures)) # для подсчетасреднего по всем измерениям

    for j in 1:lastindex(measures) # по измерениям
        # j = 2
        rf = "$reffile/$(measures[j])"
        mkpf = "$mkpfile/$(measures[j])"

        bounds = ReadRefMkp("$rf/bounds.csv")

        ref_pres = readmkp("$rf/pres.csv")
        alg_pres = readmkp("$mkpf/pres.csv")

        ref_tone = readmkp("$rf/tone.csv")
        alg_tone = readmkp("$mkpf/tone.csv")

        # сравнение по пульсациям
        ref_pres_pos = map(x -> x.pos, ref_pres)
        alg_pres_pos = map(x -> x.pos, alg_pres) 
        pres_comp = calc_indexpairs(ref_pres_pos, alg_pres_pos, 0.5*fs)

        # сравнение по тонам
        ref_tone_pos = map(x -> x.pos, ref_tone)
        alg_tone_pos = map(x -> x.pos, alg_tone) 
        tone_comp = calc_indexpairs(ref_tone_pos, alg_tone_pos, 0.5*fs)

        ################ визуализация ##########
        binfile = "D:/INCART/Pulse_Data/all bases/$basename/$(mkpfiles[i]).bin"
        signals, fs, timestart, units = readbin(binfile);

        # графики по пульсациям
        # Pres = signals.Pres
        # seg = Pres[bounds.segm.ibeg:bounds.segm.iend]
        # # fsig_smooth = my_butter(seg, 2, 10, fs, "low")         # сглаживание
        # # pres_sig = my_butter(fsig_smooth, 2, 0.3, fs, "high")  # устранение постоянной составляющей

        # ref_pres_pos_1 = map(x -> x.pos, ref_pres[findall(x -> x.type == 1, ref_pres)]) # значимые 
        # ref_pres_pos_0 = map(x -> x.pos, ref_pres[findall(x -> x.type == 0, ref_pres)]) # незначимые 
        # ref_pres_pos_2 = map(x -> x.pos, ref_pres[findall(x -> x.type == 2, ref_pres)]) # шум 
        
        # plot(seg)
        # scatter!(ref_pres_pos_0, seg[ref_pres_pos_0], markersize = 2, label="незначимые")
        # scatter!(ref_pres_pos_1, seg[ref_pres_pos_1], markersize = 2, label="значимые")
        # scatter!(ref_pres_pos_2, seg[ref_pres_pos_2], markersize = 2, label="шум")
        # scatter!(alg_pres_pos, seg[alg_pres_pos], markersize = 1)
        # xlims!(20000, 30000)
        # xlims!(40000, 60000)
        
        # tpii = findall(x -> x[1] != -1 && x[2] != -1, pres_comp)
        # tpi = map(x -> pres_comp[x][2], tpii)
        # tppos = map( x -> alg_pres_pos[x], tpi)

        # scatter!(tppos, seg[tppos], markersize = 2, markershape = :star)

        ## графики по тонам
        # Tone = signals.Tone
        # seg = Tone[bounds.segm.ibeg:bounds.segm.iend]
        # # fsig_smooth = my_butter(seg, 2, 10, fs, "low")         # сглаживание
        # # pres_sig = my_butter(fsig_smooth, 2, 0.3, fs, "high")  # устранение постоянной составляющей

        # ref_tone_pos_1 = map(x -> x.pos, ref_tone[findall(x -> x.type == 1, ref_tone)]) # значимые 
        # ref_tone_pos_0 = map(x -> x.pos, ref_tone[findall(x -> x.type == 0, ref_tone)]) # незначимые 
        # ref_tone_pos_2 = map(x -> x.pos, ref_tone[findall(x -> x.type == 2, ref_tone)]) # шум 
        
        # plot(seg)
        # scatter!(ref_tone_pos_0, seg[ref_tone_pos_0], markersize = 2, label="незначимые")
        # scatter!(ref_tone_pos_1, seg[ref_tone_pos_1], markersize = 2, label="значимые")
        # scatter!(ref_tone_pos_2, seg[ref_tone_pos_2], markersize = 2, label="шум")
        # scatter!(alg_tone_pos, seg[alg_tone_pos], markersize = 1)
        # xlims!(20000, 30000)
        # xlims!(40000, 60000)
        
        # tpii = findall(x -> x[1] != -1 && x[2] != -1, tone_comp)
        # tpi = map(x -> tone_comp[x][2], tpii)
        # tppos = map( x -> alg_tone_pos[x], tpi)

        # scatter!(tppos, seg[tppos], markersize = 1)
        #########################################

        pmm0[j] = metricscalc(pres_comp, ref_pres, alg_pres, bounds.iwz.pump, bounds.iwz.desc)
        tmm0[j] = metricscalc(tone_comp, ref_tone, alg_tone, bounds.iwz.pump, bounds.iwz.desc)
    
        pmm = (filename = mkpfiles[i], measure = measures[j], TP = pmm0[j].TP, FP = pmm0[j].FP, FN = pmm0[j].FN, Se = pmm0[j].Se, PPV = pmm0[j].PPV)
        tmm = (filename = mkpfiles[i], measure = measures[j], TP = tmm0[j].TP, FP = tmm0[j].FP, FN = tmm0[j].FN, Se = tmm0[j].Se, PPV = tmm0[j].PPV)

        push!(afmetrics_pres, pmm)
        push!(afmetrics_tone, tmm)
    end

    sp = metricscalc(pmm0)
    tp = metricscalc(tmm0)

    fppm = (filename = "Result", measure = 0, TP = sp.TP, FP = sp.FP, FN = sp.FN, Se = sp.Se, PPV = sp.PPV)
    tppm = (filename = "Result", measure = 0, TP = tp.TP, FP = tp.FP, FN = tp.FN, Se = tp.Se, PPV = tp.PPV)
    push!(afmetrics_pres, fppm)
    push!(afmetrics_tone, tppm)

    fpmm0[i] = sp
    ftmm0[i] = tp
end

pt = metricscalc(fpmm0)
tt = metricscalc(ftmm0)

tpm = (filename = "Total", measure = 0, TP = pt.TP, FP = pt.FP, FN = pt.FN, Se = pt.Se, PPV = pt.PPV)
ttm = (filename = "Total", measure = 0, TP = tt.TP, FP = tt.FP, FN = tt.FN, Se = tt.Se, PPV = tt.PPV)

push!(afmetrics_pres, tpm)
push!(afmetrics_tone, ttm)

df_pres = DataFrame(afmetrics_pres)
df_tone = DataFrame(afmetrics_tone)

CSV.write("pres_stat.csv", df_pres, delim = ";")
CSV.write("tone_stat.csv", df_tone, delim = ";")
basename = "Noise Base"
dir = "D:/INCART/Pulse_Data/all bases/$basename"
allfiles = readdir(dir)
bins = filter(x -> split(x, ".")[end] == "bin", allfiles)
fnames = map(x -> split(x, ".")[end-1], bins)

for i in fnames
    # i = fnames[11]
    run(`D:/INCART/PulseDetectorVer1/build/pulsedetcpp.exe "$dir/$i"`)
end


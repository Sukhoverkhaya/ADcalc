function [evt, infl, defl, smooth_pres, sig] = test_calc_AD_v3( raw, Fs, plots )
% v3 - чистим лишнее + сигнал из аналайзе а (без дебажных каналов)

N = length(raw.tone);
kDez = 8;
t = kDez:kDez:N;

% сглаживающий фильтр
[b,a] = butter(2, 10*2/Fs, 'low');
% [b,a] = butter(2, 32*2/Fs, 'low');
% b = [   91   182    91 ];
% a = [   300    13    51];
z0 = filterInitState(b,a);
smooth.pres = fix(filter(b,a,raw.pres, z0*raw.pres(1)));

dev_raw = dcm(raw.pres, kDez);
% DEVICE, Fs = 125
flt.b = [17     34     17];
flt.a = [369    -482   181];
flt.kf = 369;
[ dev_smooth, ~ ] = device_filter_cmp(dev_raw, flt);

% ôèëüòð ïîñòîÿííîé ñîñò
[b,a] = butter(2, 0.3*2/Fs, 'high');
z0 = filterInitState(b,a);
base.pres = fix(filter(b,a,smooth.pres, z0*smooth.pres(1))); % ñâîé
% b =   [ 4334       -8668        4334 ];
% a =   [ 4381       -8668        4288 ];
% pulse = fix(filter(b,a,raw.smooth, z0*raw.pres(1))); % áåç ïðîðåæèâàíèÿ
% pulse = fix(filter(b,a,raw.smooth(1:2:end), z0*raw.pres(1)));
% pulse = pulse(fix((2:2*length(pulse)+1)/2));
% DEVICE, Fs = 125
flt.b = [4334       -8668        4334];
flt.a = [4381       -8668        4288];
flt.kf = 4381;
[ dev_base, ~ ] = device_filter_cmp(dev_smooth, flt);

sig.base = base.pres;
% тахо
b = zeros(fix(0.040*Fs),1);
b(1) = 1; b(end)=-1;
tacho.pres = filter(b,1,base.pres);
sig.tacho = tacho.pres;
%DEVICE
b = zeros(fix(0.040*125),1); b(1) = 1; b(end)= -1;
dev_tacho = zeros(size(dev_base));
dev_tacho(1+4:end) = - dev_base(1:end-4) + dev_base(1+4:end);

%% меняем данные матлаба на данные прибора

raw.pres = undcm(dev_raw, kDez, N);
tacho.pres = undcm(dev_tacho, kDez, N);
base.pres = undcm(dev_base, kDez, N);
smooth.pres = undcm(dev_smooth, kDez, N);

%% меняем данные матлаба на данные прибора
minDist = 0.250*Fs;
[ pk, trend_mx, labels ] = pkdet_AD(tacho.pres, base.pres, minDist, Fs);

[ all_evt ] = param1_AD(pk, tacho, smooth, base, Fs);

[ seg, all_evt, ibad ] = discard_AD( all_evt, Fs );

all_evt.pos = all_evt.imx;
all_evt.val = all_evt.Range;
evt = structOfSubVec(all_evt, ~ibad);

% [ infl, defl] = estimate_AD( all_evt, smooth.pres, Fs);
[ infl, defl] = estimate_AD_pulse_v2(all_evt, smooth.pres, Fs);

[ infl, defl, evt] = get_AD_from_pulse( evt, infl, defl );


%% конец обработки
smooth_pres = smooth.pres;
if ~plots
    return
end

% ãðàôèêè
figure(7); clf;
p = syncplot(3);

axes(p(1)); cla; 
plot(raw.pres./1e4); hold on; grid on;
% plot(smooth.pres./1e4);
% plot(raw.smooth./1e4);

if ~infl.fail && ~defl.fail
    x = [infl.p1, infl.p2]; y = raw.pres(x)/1e4;
    stem(x, y, 'r');
    x = [defl.p1, defl.p2]; y = raw.pres(x)/1e4;
    stem(x, y, 'r');
end

axes(p(2)); cla; 
plot(base.pres); hold on; grid on; % xlim auto;
% plot(pulse);
plot(raw.pres/100);
plot(all_evt.imn, all_evt.Amn, 'ob');
plot(all_evt.imx, all_evt.Amx, '*r');
plot(all_evt.ipres, all_evt.Range, '.-k');
plotSegToSignal(base.pres, seg, 'k', abs(diff(ylim)/4));

% stem(all_evt.imx([infl.ibeg, infl.ibeg]), [infl.maxLvl, infl.maxLvl], 'k*');
% stem(all_evt.imx([infl.imax, infl.imax]), [infl.maxLvl, infl.maxLvl], 'k*');
% stem(all_evt.imx([infl.iend, infl.iend]), [infl.maxLvl, infl.maxLvl], 'k*');
% 
% stem(all_evt.imx([defl.ibeg, defl.ibeg]), [defl.maxLvl, defl.maxLvl], 'k*');
% stem(all_evt.imx([defl.imax, defl.imax]), [defl.maxLvl, defl.maxLvl], 'k*');
% stem(all_evt.imx([defl.iend, defl.iend]), [defl.maxLvl, defl.maxLvl], 'k*');
% 
% stem([infl.p1, infl.p2], [infl.maxLvl, infl.maxLvl], 'r');
% stem([defl.p1, defl.p2], [defl.maxLvl, defl.maxLvl], 'r');


axes(p(3)); cla; 
% stem(all_evt.pos, -all_evt.bad, 'b'); hold on;
for i=1:8
    k = 2^(i-1);
    y = (bitand(all_evt.bad, k) > 0)*i;
    stem(all_evt.pos, -y, '*', 'b'); hold on;
end
stem(infl.s1, -1, 'r'); stem(infl.s2, -2, 'r'); stem(infl.s3, -3, 'r');
stem(defl.s1, -1, 'r'); stem(defl.s2, -2, 'r'); stem(defl.s3, -3, 'r');

% ÏÅÐÂÈ×ÍÛÉ ÄÅÒÅÊÒÎÐ:
% plot(tacho.pres); hold on; grid on;
% plot(trend_mx{1}, 'k');
% plot(trend_mx{2}*5e3, 'r');
% plot(trend_mx{3}, 'b');
% plot(labels,'k');

% ðåçóëüòàòû áðàêîâêè:
% stairs(all_evt.ipres,R2spo2(all_evt.R), 'r.-'); hold on; grid on;
% stairs(all_evt.ipres(~ibad),R2spo2(all_evt.R(~ibad)), '.-');
% stairs(all_evt.ipres,all_evt.R), '.-'); hold on; grid on;
% plot(all_evt.ipres(~ibad),all_evt.R(~ibad));

% ÏÀÐÀÌÅÒÐÛ ÁÐÀÊÎÂÊÈ:
% plot(all_evt.ipres, all_evt.Wprev/Fs); % øèðèíà ÄÎ
% plot(all_evt.ipres, all_evt.n1/Fs); % % âðåìÿ ôðîíòà
% plot(all_evt.ipres, all_evt.Range); hold on; % àáñîëþòíàÿ àìïëèòóäà
% plot(all_evt.ipres, all_evt.duty); % êîýô. ñêâàæíîñòè
% plot(all_evt.ipres, all_evt.kW); % îòíîøåíèå èíòåðâàëîâ äî/ïîñëå:
grid on;
end
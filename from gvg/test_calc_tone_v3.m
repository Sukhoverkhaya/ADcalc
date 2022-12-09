function [pk, infl, defl] = test_calc_tone_v3( sig, Fs, plots )
% N = length(sig.tone);
% % сглаживание давления
% n = Fs*2;
% sig.pres = smooth(sig.pres, n)';

N = length(sig.tone);
t = 4:4:N;

% ñãëàæèâàþùèé ôèëüòð
[b,a] = butter(2, 10*2/Fs, 'low');
% [b,a] = butter(2, 32*2/Fs, 'low');
% b = [   91   182    91 ];
% a = [   300    13    51];
z0 = filterInitState(b,a);
sig.pres = fix(filter(b,a,sig.pres, z0*sig.pres(1)));

%% ÏÐÅÄÎÁÐÀÁÎÒÊÀ
% anti-aliasing
[b,a] = butter(2, 60*2/Fs, 'low'); % выключать, если уже есть в приборе
z0 = filterInitState(b,a);
sig.smooth = fix(filter(b,a,sig.tone, z0*sig.tone(1)));
% DEVICE
flt.b = [3,      6,      3];
flt.a = [108,   -159,   63];
flt.kf = 108;
[ dev_smooth, ~ ] = device_filter_cmp(sig.tone, flt);

% верхние частоты
[b,a] = butter(2, 30*2/Fs, 'high');
% [b,a] = butter(2, 5*2/Fs, 'low');
z0 = filterInitState(b,a);
sig.smooth = fix(filter(b,a, sig.smooth, z0*sig.smooth(1)));
% DEVICE
flt.b = [112  -224   112]; %[99     -198     99]; %
flt.a = [128  -222    98]; %[170    -167     59]; %
flt.kf =  128; %170; %
[ dev_smooth, ~ ] = device_filter_cmp(dev_smooth, flt);

% прореживание
dev_smooth = dcmmean(abs(dev_smooth), 4); % 250 Ãö

% огибающая по модулю
[b,a] = butter(2, 10*2/Fs, 'low');
z0 = filterInitState(b,a);
sig.env = fix(filter(b,a, abs(sig.smooth), z0*abs(sig.smooth(1))));
% sig.env = abs(sig.smooth);
% DEVICE
flt.b = [2,      4,      2]; % [1 2 1]; %
flt.a = [150,   -247,   105]; % [1059  -2024 969]; %
flt.kf = 150; % 1059; %
[ dev_env, ~ ] = device_filter_cmp(dev_smooth, flt);

[ sig.env ] = tone_envelope_filt_baseline( dev_env );

% !! меняем на приборную огибающую
ind = fix(((1:N)-1)/4) + 1;
Ndcm = fix(N/4);
ind(ind > Ndcm) = Ndcm;
sig.env = double(sig.env(ind));

% ÏÐÈÁÎÐ ÏÈØÅÒ ÎÃÈÁÀÞÙÓÞ
% sig.env = sig.tone;
% sig.smooth = sig.tone;

%%
% пиковый детекто  c  разрядом
minDist = 0.200*Fs;

[ pk, trend_mx, labels ] = pkdet_tone(sig.env, minDist, Fs);

[ pk ] = param1_tone(pk, sig.env, sig.smooth, Fs);

[ seg, all_pk, ibad ] = discard_tone( sig.pres, pk, Fs );

pk = structOfSubVec(all_pk, ~ibad);

% [ state, infl, defl] = estimate_AD_tone( pk, sig.pres, Fs); % тест!
[infl, defl] = estimate_AD_tone_v2( pk, sig.pres, Fs);

[ infl, defl ] = get_AD_from_tones( pk, infl, defl, Fs );

% pk = structOfSubVec(pk, ~pk.bad);

if ~plots
    return
end

% графики
figure(6); clf;
p = syncplot(3);

axes(p(1)); cla; 
plot(sig.tone); hold on; grid on;
plot(sig.smooth);

if ~infl.fail && ~defl.fail
%     x = [infl.p1, infl.p2]; y = ylim;
%     stem(x, y, 'r');
%     x = [defl.p1, defl.p2]; y = ylim;
%     stem(x, y, 'r');
    
    x = [infl.sysPos, infl.diaPos]; y = ylim;
    stem(x, y, 'r');
    x = [defl.sysPos, defl.diaPos]; y = ylim;
    stem(x, y, 'r');
end

axes(p(2)); cla; 
% plot(abs(sig.smooth)); hold on; grid on;
plot(sig.env); hold on; grid on;
plot(all_pk.pos, all_pk.val, 'r.');

% plotSeg(seg, 'k', [0 5e5]); % ðåçóëüòàò áðàêîâêè

plot(all_pk.ibeg, all_pk.val*0.3, 'k*'); % ãðàíèöû
plot(all_pk.iend, all_pk.val*0.3, 'b*');

stem(all_pk.ins1, all_pk.val*0.3, 'k'); % ó÷àñòêè øóìà
stem(all_pk.ins2, all_pk.val*0.3, 'k');
stem(all_pk.ins3, all_pk.val*0.3, 'k');
stem(all_pk.ins4, all_pk.val*0.3, 'k');

plot(trend_mx{1}, 'k');
% plot(trend_mx{2}*1e5, 'r');
plot(trend_mx{3}/2.5, 'r');
plot(labels,'k');

axes(p(3)); cla;
stem(all_pk.pos, -all_pk.bad, 'b'); hold on;
stem(infl.s1, -1, 'r'); stem(infl.s2, -2, 'r'); stem(infl.s3, -3, 'r');
stem(defl.s1, -1, 'r'); stem(defl.s2, -2, 'r'); stem(defl.s3, -3, 'r');

% stem(pk.pos, pk.noise, 'k'); hold on;
% stem(all_pk.pos, all_pk.snr, 'k'); hold on;
% stem(pk.pos, pk.rawSnr, 'r');
% stem(pk.pos, pk.W/Fs);

% ÏÀÐÀÌÅÒÐÛ ÁÐÀÊÎÂÊÈ:
% plot(E.ipres, E.Wprev/Fs); % øèðèíà ÄÎ
% plot(E.ipres, E.n1/Fs); % % âðåìÿ ôðîíòà
% plot(E.ipres, E.A); hold on; % àáñîëþòíàÿ àìïëèòóäà
% plot(E.ipres, E.duty); % êîýô. ñêâàæíîñòè
% plot(E.ipres, E.kW); % îòíîøåíèå èíòåðâàëîâ äî/ïîñëå:
grid on;
end

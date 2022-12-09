fileBin = 'E:\AD запись тест\MB0180216120930.dat'; % манжета!

%% загружаем данные
[data, info] = readFromBin_v3(fileBin);
Fs = info.Fs;

ivalid = data(:,1) > 15 & data(:,2) > -1e7;
data(~ivalid,:) = nan;
seg = IDtoSeg(ivalid);
minLen = (30*Fs);
len = seg(:,2) - seg(:,1);
seg = seg(len > minLen, :);

%% загружаем метки
markFile = fullfile(extChange(fileBin, 'dat'), 'ExportMarks.bin');

%% вырезаем накачку
examNr = 2;
ex = seg(examNr,:);

% has_mark = false;
has_mark = exist(markFile);
if has_mark
    [marks, marksRes] = readMark(markFile);
    iexam_select = marks.pos >= ex(1) & marks.pos <= ex(2);
    iselect = marks.pos >= ex(1) & marks.pos <= ex(2);
    
    mrk = marks(iexam_select, :);
%     mrk = marks;
    mrk.pos = mrk.pos - ex(1) + 1;
    
    mrk_puls = mrk(mrk.event == 16, :);
    mrk_tone = mrk(mrk.event == 17, :);
    SM_puls = mrk(mrk.event == 18, :);
    SM_tone = mrk(mrk.event == 19, :);
    mrk_start = mrk(mrk.event == 15 | mrk.event == 10, :);
    
	mrk_puls.pos(2:end) = mrk_puls.pos(1:end-1); %%% !!! фикс смещение  параметров у пульсаций

% фикс
    iexam_select = marksRes.pos >= ex(1) & marksRes.pos <= ex(2);
    mrkRes = marksRes(iexam_select, :);
end

kPres = 1e4;
kTone = 1e3 * 1e3;

pres = data(ex(1):ex(2), 1);
tone = data(ex(1):ex(2), 2);

if max(pres) > 1000 % фиксим калибровку новых файлов
    pres = pres/1000;
end

raw.pres = kPres * pres;
raw.tone = kTone * tone;

plots = true;

%% получаем границы измрений через окстыли (т.к. позиция не пишется)
if has_mark 
    
[b,a] = butter(2, 10*2/Fs, 'low');
z0 = filterInitState(b,a);
smooth_pres = fix(filter(b,a,raw.pres, z0*raw.pres(1)));
    
    % приборные метки давления
    findUp = @(x) find(smooth_pres/kPres > x, 1);
    findDn = @(x) length(smooth_pres) - find(smooth_pres(end:-1:1)/kPres > x, 1) + 1;
    
    sysdia_tone = []; pos_tone = [];
    sysdia_puls = []; pos_puls = [];
    for k = 1:size(mrkRes,1)
        if mrkRes.Direction(k) == 0
            if mrkRes.TonPul(k) == 0
                pos_tone = [pos_tone, findUp(mrkRes.Sys(k)), findUp(mrkRes.Diast(k))]; 
                sysdia_tone = [sysdia_tone, mrkRes.Sys(k), mrkRes.Diast(k)];
            else
                pos_puls = [pos_puls, findUp(mrkRes.Sys(k)), findUp(mrkRes.Diast(k))]; 
                sysdia_puls = [sysdia_puls, mrkRes.Sys(k), mrkRes.Diast(k)];
            end
        else
            if mrkRes.TonPul(k) == 0
                pos_tone = [pos_tone, findDn(mrkRes.Sys(k)), findDn(mrkRes.Diast(k))]; 
                sysdia_tone = [sysdia_tone, mrkRes.Sys(k), mrkRes.Diast(k)];
            else
                pos_puls = [pos_puls, findDn(mrkRes.Sys(k)), findDn(mrkRes.Diast(k))]; 
                sysdia_puls = [sysdia_puls, mrkRes.Sys(k), mrkRes.Diast(k)];
            end
        end
    end
end
%%
[pk_pres, infl_puls, defl_puls, smooth_pres] = test_calc_AD_v3( raw, Fs, plots);
if has_mark 
    stem(mrk_puls.pos, mrk_puls.info);
    stem(SM_puls.pos, SM_puls.info);   %*150000
%     stem(mrk_start.pos, mrk_start.info, 'k*');
    subplot(3,1,1);
    stem(pos_tone, sysdia_tone, 'k*'); % òîíû
    stem(pos_puls, sysdia_puls, 'b*'); % ïóëüñàöèè
end
ax1 = gca;

[pk_tone, infl_tone, defl_tone] = test_calc_tone_v3( raw, Fs, plots);
if has_mark 
%     cla; 
    stem(mrk_tone.pos, mrk_tone.info);
    stem(SM_tone.pos, SM_tone.info); % *150000
    
%% ïðèáîðíûå ìåòêè äàâëåíèÿ
    stem(pos_tone, ones(size(pos_tone))*3, 'k*');    %*400000 
end
ax2 = gca;


linkaxes([ax1, ax2], 'x')

%%
figure(11); clf;
p = syncplot(2);

axes(p(1));
stem(pk_tone.pos, pk_tone.val); hold on; 
h = diff(ylim);
txt = {'tones'};
if isfield(infl_tone, 'diaPos') && isfield(infl_tone, 'sysPos')
    clr = 'r'; if (infl_tone.fail), clr = 'k'; end
    stem([infl_tone.diaPos, infl_tone.sysPos], [h, h], clr);
    txt{end+1} = ['infl: ' num2str(fix(infl_tone.Psys/kPres)), ' / ' num2str(fix(infl_tone.Pdia/kPres))];
end

if isfield(defl_tone, 'diaPos') && isfield(defl_tone, 'sysPos')
    clr = 'r'; if (defl_tone.fail), clr = 'k'; end
    stem([defl_tone.diaPos, defl_tone.sysPos], [h, h], clr);
    txt{end+1} = ['defl: ' num2str(fix(defl_tone.Psys/kPres)), ' / ' num2str(fix(defl_tone.Pdia/kPres))];
    if has_mark
        stem(pos_tone, ones(size(pos_tone))*h, 'k*'); 
        txt{end+1} = [num2str(sysdia_tone')];
        stem(mrk_tone.pos(mrk_tone.info==0), 10*mrk_tone.pres(mrk_tone.info==0));
    end
end
if ~isempty(txt), legend(txt); end

axes(p(2));
stem(pk_pres.pos, pk_pres.val); hold on; 
h = diff(ylim);
txt = {'pulse'};
if isfield(infl_puls, 'diaPos') && isfield(infl_puls, 'sysPos')
    clr = 'r'; if (infl_puls.fail), clr = 'm'; end
    stem([infl_puls.diaPos, infl_puls.sysPos], [h, h], clr);
    txt{end+1} = ['infl: ' num2str(fix(infl_puls.Psys/kPres)), ' / ' num2str(fix(infl_puls.Pdia/kPres))];
end
if isfield(defl_puls, 'diaPos') && isfield(defl_puls, 'sysPos')
    clr = 'r'; if (defl_puls.fail), clr = 'm'; end
    stem([defl_puls.diaPos, defl_puls.sysPos], [h, h], clr);
    txt{end+1} = ['defl: ' num2str(fix(defl_puls.Psys/kPres)), ' / ' num2str(fix(defl_puls.Pdia/kPres))];
    if has_mark
        stem(pos_puls, ones(size(pos_puls))*h, 'k*'); 
        txt{end+1} = [num2str(sysdia_puls')];
        stem(mrk_puls.pos(mrk_puls.info==0), 10*mrk_puls.pres(mrk_puls.info==0));
    end
end
if ~isempty(txt), legend(txt); end

ax3 = gca;

linkaxes([ax1, ax3], 'x')
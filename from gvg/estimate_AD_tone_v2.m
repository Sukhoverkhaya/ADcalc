function [infl, defl] = estimate_AD_tone_v2(evt, pres, Fs, kPres)
% v2 - один код для накачки и спуска + определение границ по пикам

if nargin < 4
    kPres = 1e4;
end
N = length(pres);
% !!! костыль - добавим в конец ложную пульсацию, чтобы срабатывало по превышению времени
evt.pos(end+1) = Inf; % N ?
evt.bad(end+1) = true;
evt.val(end+1) = -1;
evt.pres(end+1) = -1;

Np = numel(evt.val);
[Pmax, iPmax] = max(pres);

infl.s1 = 0; infl.s2 = 0; infl.s3 = 0;
defl.s1 = 0; defl.s2 = 0; defl.s3 = 0;

infl.upmode = true; % true - накачка, false - спуск
defl.upmode = false;  % true - накачка, false - спуск

infl.offpos = iPmax; % безусловный конец
defl.offpos = evt.pos(end);  % безусловный конец

infl.Pmax = Pmax;
defl.Pmax = Pmax;

infl.posPmax = iPmax;
defl.posPmax = iPmax;

P_low = 30*kPres;
P_high = infl.Pmax - 10*kPres; % ??? всегда позже, чем сработавший критерий 

%% определяем рабочие интервалы (НО: конец = offpos)
% НАКАЧКА
iend = 1;
while iend < Np && evt.pres(iend+1) < P_high && evt.pos(iend+1) < iPmax
	iend = iend+1;
end
ibeg = iend;
while ibeg > 1 && evt.pres(ibeg-1) > P_low
    ibeg = ibeg-1;  
end % ! пропустит, если первое событие - хорошее

infl.ibeg = ibeg;
infl.iend = iend;

% СПУСК
ibeg = Np;
while ibeg > 1 && evt.pres(ibeg-1) < P_high && evt.pos(ibeg-1) > iPmax
	ibeg = ibeg-1;
end
iend = ibeg;
while iend < Np && evt.pres(iend+1) > P_low
    iend = iend+1;
end % ! пропустит, если последнее событие - хорошее

defl.ibeg = ibeg;
defl.iend = iend;

%% ! пропустит, если последнее событие - хорошее

infl = ADstateMachine(evt, infl, Fs, kPres);
defl = ADstateMachine(evt, defl, Fs, kPres);

%% берем позиции на исходном сигнале - ЕСЛИ ДОШЛИ ДО ЭТОГО ЭТАПА
if ~infl.fail
    infl.p1 = evt.pos(infl.ifirst);
    infl.p2 = evt.pos(infl.ilast);
else
    disp('ТОНЫ: ОЦЕНКА НАКАЧКИ НЕ УДАЛАСЬ');
end

if ~defl.fail
    defl.p1 = evt.pos(defl.ifirst);
    defl.p2 = evt.pos(defl.ilast);
else
    disp('ТОНЫ: ОЦЕНКА СПУСКА НЕ УДАЛАСЬ');
end

function [state] = ADstateMachine(evt, state, Fs, kPres)

Np = length(evt.pos);
if nargin < 4
    kPres = 1e4;
end

state.maxLvl = 0;
state.fail = true;

% !!!
minPresBad = 70*kPres;
minPsys = 120*kPres;

Pmax = state.Pmax;
Pshift = 10*kPres; % отступ от максимального давления
Pmin = 40*kPres; % миниальное давление начала анализа
% minLvl = 0.5*kPres; % 0.5 mm
wait = 3; % 3 сек отсутствия пульсаций

% критерии браковки измерения:
BadTime = 0; %
maxBadTime = 5*Fs;

at = @(i) mod(i-1, 3) + 1;
buf = zeros(3,1);

Nb = 0;
maxLvl = 0; 
imax = 1;
Nlow = 0;

ilast = 0; % индекс последнего значимого и НЕ бракованного пика
i3s = 0; % индекс последнего значимого пика

% два предыдущих хороших пика:
i1 = -1; % i-1
i2 = -1; % i-2

step = 0;

if state.upmode % накачка 
    startCondition = @(pres) pres > Pmin;
else % спуск
    startCondition = @(pres) pres < Pmax - Pshift;
end

for i = state.ibeg : Np % state.iend - чтобы переполнялся
%     pos = evt.pos(i);
    if step == 0 
        if startCondition(evt.pres(i)) && ~evt.bad(i) % 30 mm
            Nb = 1; % первая хорошая пульсация
            step = step+1; state.s1 = evt.pos(i);
            i1=i; % итерация указателя
        end
        
    elseif step == 1 % начинаем копить хорошие пульсации
        if evt.bad(i), continue; end % если пришла плохая - ничего не делаем
        buf(at(i)) = evt.val(i); % добавил 15.12.2017
        Nb = Nb+1;
        if Nb > 1 && evt.pos(i) - evt.pos(i1) > wait*Fs % 2 пульсации - 3 секунды
            Nb = 1;
        end
        if Nb == 3 % есть 3 пульсации - продолжаем
            maxLvl = mean([evt.val(i), evt.val(i1)]);
            if evt.val(i2) > maxLvl*0.2 % первая значимая
                state.ifirst = i2;
                state.Pfirst = evt.pres(state.ifirst); % pres(evt.pos(state.ifirst));
                step = step+1; state.s2 = evt.pos(i);
            else
                Nb = 2; % ищем следующую первую пульсацию
            end
            ilast = i; 
            i3s = i; %
        end
        i2=i1; i1=i; % итерация указателей на последние 3 хороших

    elseif step == 2
        
        if evt.pos(i) - evt.pos(i3s) < wait*Fs % && отступаем от значимого 3 секунды
            if evt.val(i) > maxLvl*0.2
                i3s = i; % индекс последнего значимого пика
                if ~evt.bad(i), ilast = i; end % индекс последнего хорошего пика
            end
            if ~evt.bad(i) % пересчитываем средний уровень только по хорошим
%                 % пересчет-1, плавный
%                 if maxLvl > evt.val(i), k = 1/6; % вниз медленнее, чем вверх
%                 else, k = 1/3; end
%                 maxLvl = maxLvl*(1-k) + evt.val(i)*k; % плавно пересчитываем уровень
                % пересчет-2, по медиане из 3-х
                buf(at(i)) = evt.val(i);
                maxLvl = max(median(buf), maxLvl);
            end
        else % прошло более 3 сек с последнего хорошего пика выше 0.2
            state.ilast = ilast;
            state.Plast = evt.pres(state.ilast); % pres(evt.pos(state.ilast));
            state.fail = false;
            % проверка, что (сист*0.15) > (сист - диаст)
            [state.Pdia, state.Psys] = pres2diasyst(state);
            if state.Psys < minPsys, continue; end
            if state.Psys*3/20 > (state.Psys - state.Pdia) % || state.Psys < minPsys
                step = step-1; state.s2 = evt.pos(i);
            else
            	step = step+1; state.s3 = evt.pos(i);
            end
        end

%%%%%%%%%%%%%%%%
        % disp([evt.pos(i), evt.pos(i) - evt.pos(i3s)]);
        
%         if evt.pos(i) - evt.pos(i3s) >= wait*Fs % && îòñòóïàåì îò çíà÷èìîãî 3 ñåêóíäû
%             state.ilast = ilast;
%             state.Plast = evt.pres(state.ilast); % pres(evt.pos(state.ilast));
%             state.fail = false;
%             % ïðîâåðêà, ÷òî (ñèñò*0.15) > (ñèñò - äèàñò)
%             [state.Pdia, state.Psys] = pres2diasyst(state);
%             if state.Psys >= minPsys
%                 if state.Psys*3/20 > (state.Psys - state.Pdia) % || state.Psys < minPsys
%                     step = step-1; state.s2 = evt.pos(i+1);
%                 else
%                     step = step+1; state.s3 = evt.pos(i+1);
%                 end
%             end
%         end
%         if evt.val(i) > maxLvl*0.2
%             i3s = i; % èíäåêñ ïîñëåäíåãî çíà÷èìîãî ïèêà
%             if ~evt.bad(i), ilast = i; end % èíäåêñ ïîñëåäíåãî õîðîøåãî ïèêà
%         end
%         if ~evt.bad(i) % ïåðåñ÷èòûâàåì ñðåäíèé óðîâåíü òîëüêî ïî õîðîøèì
%             buf(at(i)) = evt.val(i);
%             maxLvl = max(median(buf), maxLvl);
%         end
%%%%%%%%%%%%%%

        % критерий браковки по времени
        if evt.bad(i) && (evt.pres(i) > minPresBad)
            BadTime = BadTime + evt.pos(i) - evt.pos(i-1);
            if BadTime > maxBadTime % измерение забраковано по макс интервалу бракованных пиков
                break;
            end
        else
            if BadTime > 0 % что эквивалентно: evt.bad(i-1) == false
                BadTime = BadTime + evt.pos(i) - evt.pos(i-1);
                if BadTime > maxBadTime % измерение забраковано по макс интервалу бракованных пиков
                    break;
                end
                BadTime = 0;
            end
        end
    end
    
    if evt.pos(i) >= state.offpos % с последним приоритетом - чтобы прогнало последний раз
        break; 
    end % безусловный выход
end
state.step = step;

% пересчет границ в сист/диаст давление в зависимости от накачки/спуска
function [Pdia, Psys] = pres2diasyst(state)
if state.upmode
    Pdia = state.Pfirst;
    Psys = state.Plast;
else
    Psys = state.Pfirst;
    Pdia = state.Plast;
end
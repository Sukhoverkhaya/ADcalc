# сравнение разметок по точкам с заданным радиусом поиска
# вход - позиции событий в референтной и тестовой разметках, радиус поиска (в отсчетах)
function bypointcompare(test::Vector, ref::Vector, radius::Int64)
    testcompres = fill(-1, (length(test), 1)) # индекс - индекс точки в тест, значение - индекс соответсвующей точки в реф
    refcompres = fill(-1, (length(ref), 1))   

    for i in 1:lastindex(test)
        # поиск индекса элемента в реф. разметке, соответсвующего элементу в тест. с учетом радиуса
        # i - индекс элемента в тест, pare - индекс элемента в реф (если есть)
        pair = findall(x -> x >= test[i]-radius && x <= test[i]+radius, ref) 
        if !isnothing(pair)             # если индекс парного элемента найден
            for j in pair
                if refcompres[j] == -1  # и ещё не был использован
                    refcompres[j] = i
                    testcompres[i] = j
                    break
                end
            end
        end
    end

    # там, где пары найдены, нет смысла их дублировать,
    # поэтому один массив с результатами сравнения берём целиком, а другой - только 
    # индексы без пары
    nopairref = findall(x -> x==-1, refcompres)
    reslen = length(testcompres)+length(nopairref)
    res = fill((-1,-1), reslen)

    testind = range(1, length(testcompres))
    res[1:length(testcompres)] = map((x,y) -> (x,y), testind, testcompres)
    
    if !isempty(nopairref)
        res[length(testcompres)+1:end] = map(x -> (-1,x[1]), nopairref)
    end

    return res
end

a = [1,2,3,4,5,6,7,8,9,10]
b = [1,2,3,4,5,16,17,18,19,100]

c,d = bypointcompare(a, b, 5)
c
d

for i in 1:10
    print
end
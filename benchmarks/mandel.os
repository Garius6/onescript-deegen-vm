// Mandelbrot: счётчик точек внутри множества на сетке width × height.
// Без массивов и stdlib — чистая арифметика и циклы.

Функция MandelIter(cx, cy, maxIter)
    zx = 0;
    zy = 0;
    i = 0;
    Пока i < maxIter Цикл
        zx2 = zx * zx;
        zy2 = zy * zy;
        Если zx2 + zy2 > 4 Тогда
            Возврат i;
        КонецЕсли;
        новый_zx = zx2 - zy2 + cx;
        zy = 2 * zx * zy + cy;
        zx = новый_zx;
        i = i + 1;
    КонецЦикла;
    Возврат maxIter;
КонецФункции

Функция CountSet(width, height, maxIter)
    inside = 0;
    py = 0;
    Пока py < height Цикл
        px = 0;
        Пока px < width Цикл
            cx = -2.0 + 3.0 * px / width;
            cy = -1.5 + 3.0 * py / height;
            n = MandelIter(cx, cy, maxIter);
            Если n = maxIter Тогда
                inside = inside + 1;
            КонецЕсли;
            px = px + 1;
        КонецЦикла;
        py = py + 1;
    КонецЦикла;
    Возврат inside;
КонецФункции

Сообщить(CountSet(120, 80, 200));

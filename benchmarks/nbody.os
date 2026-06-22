// N-Body симуляция: 3 тела, leapfrog integration.
// Адаптация из Computer Language Benchmarks Game (упрощённая, без stdlib/массивов).
// Считается энергия системы до и после N шагов симуляции.

Перем PI;
Перем DT;
Перем SOLAR_MASS;
Перем DAYS_PER_YEAR;

Перем x1, y1, z1, vx1, vy1, vz1, m1;
Перем x2, y2, z2, vx2, vy2, vz2, m2;
Перем x3, y3, z3, vx3, vy3, vz3, m3;

Процедура InitConstants()
    PI = 3.141592653589793;
    SOLAR_MASS = 4 * PI * PI;
    DAYS_PER_YEAR = 365.24;
    DT = 0.01;
КонецПроцедуры

Процедура InitBodies()
    // Sun
    x1 = 0; y1 = 0; z1 = 0;
    vx1 = 0; vy1 = 0; vz1 = 0;
    m1 = SOLAR_MASS;

    // Jupiter
    x2 = 4.84143144246472090;
    y2 = -1.16032004402742839;
    z2 = -0.103622044471123109;
    vx2 = 0.00166007664274403694 * DAYS_PER_YEAR;
    vy2 = 0.00769901118419740425 * DAYS_PER_YEAR;
    vz2 = -0.0000690460016972063023 * DAYS_PER_YEAR;
    m2 = 0.000954791938424326609 * SOLAR_MASS;

    // Saturn
    x3 = 8.34336671824457987;
    y3 = 4.12479856412430479;
    z3 = -0.403523417114321381;
    vx3 = -0.00276742510726862411 * DAYS_PER_YEAR;
    vy3 = 0.00499852801234917238 * DAYS_PER_YEAR;
    vz3 = 0.0000230417297573763929 * DAYS_PER_YEAR;
    m3 = 0.000285885980666130812 * SOLAR_MASS;

    // Offset sun's momentum so total momentum = 0
    px = vx2 * m2 + vx3 * m3;
    py = vy2 * m2 + vy3 * m3;
    pz = vz2 * m2 + vz3 * m3;
    vx1 = -px / SOLAR_MASS;
    vy1 = -py / SOLAR_MASS;
    vz1 = -pz / SOLAR_MASS;
КонецПроцедуры

// Approximate sqrt via Newton's method — нет stdlib.
Функция Корень(value)
    Если value <= 0 Тогда
        Возврат 0;
    КонецЕсли;
    guess = value;
    i = 0;
    Пока i < 20 Цикл
        guess = (guess + value / guess) / 2;
        i = i + 1;
    КонецЦикла;
    Возврат guess;
КонецФункции

Функция Energy()
    e = 0.5 * m1 * (vx1*vx1 + vy1*vy1 + vz1*vz1);
    e = e + 0.5 * m2 * (vx2*vx2 + vy2*vy2 + vz2*vz2);
    e = e + 0.5 * m3 * (vx3*vx3 + vy3*vy3 + vz3*vz3);

    dx = x1 - x2; dy = y1 - y2; dz = z1 - z2;
    d = Корень(dx*dx + dy*dy + dz*dz);
    e = e - m1 * m2 / d;

    dx = x1 - x3; dy = y1 - y3; dz = z1 - z3;
    d = Корень(dx*dx + dy*dy + dz*dz);
    e = e - m1 * m3 / d;

    dx = x2 - x3; dy = y2 - y3; dz = z2 - z3;
    d = Корень(dx*dx + dy*dy + dz*dz);
    e = e - m2 * m3 / d;

    Возврат e;
КонецФункции

Процедура Advance()
    // Velocity updates from pairwise gravitational interactions.
    dx = x1 - x2; dy = y1 - y2; dz = z1 - z2;
    d2 = dx*dx + dy*dy + dz*dz;
    d = Корень(d2);
    mag = DT / (d2 * d);
    vx1 = vx1 - dx * m2 * mag; vy1 = vy1 - dy * m2 * mag; vz1 = vz1 - dz * m2 * mag;
    vx2 = vx2 + dx * m1 * mag; vy2 = vy2 + dy * m1 * mag; vz2 = vz2 + dz * m1 * mag;

    dx = x1 - x3; dy = y1 - y3; dz = z1 - z3;
    d2 = dx*dx + dy*dy + dz*dz;
    d = Корень(d2);
    mag = DT / (d2 * d);
    vx1 = vx1 - dx * m3 * mag; vy1 = vy1 - dy * m3 * mag; vz1 = vz1 - dz * m3 * mag;
    vx3 = vx3 + dx * m1 * mag; vy3 = vy3 + dy * m1 * mag; vz3 = vz3 + dz * m1 * mag;

    dx = x2 - x3; dy = y2 - y3; dz = z2 - z3;
    d2 = dx*dx + dy*dy + dz*dz;
    d = Корень(d2);
    mag = DT / (d2 * d);
    vx2 = vx2 - dx * m3 * mag; vy2 = vy2 - dy * m3 * mag; vz2 = vz2 - dz * m3 * mag;
    vx3 = vx3 + dx * m2 * mag; vy3 = vy3 + dy * m2 * mag; vz3 = vz3 + dz * m2 * mag;

    // Position updates.
    x1 = x1 + DT * vx1; y1 = y1 + DT * vy1; z1 = z1 + DT * vz1;
    x2 = x2 + DT * vx2; y2 = y2 + DT * vy2; z2 = z2 + DT * vz2;
    x3 = x3 + DT * vx3; y3 = y3 + DT * vy3; z3 = z3 + DT * vz3;
КонецПроцедуры

Процедура Run(N)
    InitConstants();
    InitBodies();
    Сообщить(Energy());
    i = 0;
    Пока i < N Цикл
        Advance();
        i = i + 1;
    КонецЦикла;
    Сообщить(Energy());
КонецПроцедуры

Run(50000);

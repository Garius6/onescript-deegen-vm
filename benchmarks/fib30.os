// Compute-heavy classical benchmark — exponential recursion exercises call dispatch.

Функция Фиб(н)
    Если н < 2 Тогда
        Возврат н;
    КонецЕсли;
    Возврат Фиб(н - 1) + Фиб(н - 2);
КонецФункции

Сообщить(Фиб(30));

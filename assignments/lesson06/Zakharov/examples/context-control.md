# Фактическая проверка контекста

## Удаление старой пары

Трижды передана строка из 300 слов `word`. Перед третьим ответом клиент удалил самую
старую завершённую пару `user`–`assistant` и повторно посчитал вход:

```text
Tokens: input 329, answer 26, total 355 / 1024
Tokens: input 665, answer 26, total 691 / 1024
History shortened: removed 1 oldest pair(s).
Tokens: input 665, answer 26, total 691 / 1024
```

## Отказ для одной слишком длинной реплики

Ввод: 900 повторов слова `слово`. Генерация не запущена; клиент завершился с кодом 1:

```text
Error: the current message does not fit the 1024-token context; shorten it and try again
```

Оценка: оба защитных сценария сработали. Вход считал сам `llama-server`, а не длина строки.

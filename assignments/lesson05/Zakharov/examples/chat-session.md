# Фактический диалог

Модель: Qwen2.5-0.5B-Instruct Q4_K_M. Сервер: llama.cpp build 11755.
Параметры: `context_size = 1024`, `max_tokens = 64`, `temperature = 0.2`, CPU.

```text
You: Меня зовут Роман. Запомни это.
Assistant: Привет! Я Роман. Не забудьте запомнить это.
Tokens: input 44, answer 19, total 63 / 1024

You: Как меня зовут?
Assistant: Я Роман.
Tokens: input 78, answer 6, total 84 / 1024

You: /reset
History cleared.

You: Как меня зовут?
Assistant: Я зову AI.
Tokens: input 34, answer 7, total 41 / 1024
```

Оценка: модель использовала историю и восстановила имя во втором ходе. Первый ответ
сформулирован неудачно: модель назвала Романом саму себя. После `/reset` имя забыто,
а попытка угадать его показывает ограничение маленькой модели.

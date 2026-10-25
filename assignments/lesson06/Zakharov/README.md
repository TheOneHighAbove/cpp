# Local Model Chat

C++17 CLI-клиент для многоходового диалога с локальной моделью через OpenAI-совместимый
HTTP API `llama-server`. Клиент хранит историю в `std::vector<Message>`, считает токены до
генерации, сокращает старую историю и печатает статистику `usage`.

## Lesson 06: работа coding agent

Проект из Lesson 05 был проанализирован, собран и протестирован учебным агентом
из `assignments/lesson06/agent.py`. Единственное принятое изменение кода — ключ `/EHsc`
для MSVC. Он устраняет предупреждение `C4530` об обработке исключений.

Полная хронология tool calls, найденные проблемы, принятые и отклонённые
предложения, а также ответы на вопросы задания приведены в
[`AGENT_REPORT.md`](AGENT_REPORT.md).

## Проверенное окружение

- Windows 10 Pro 22H2, build 19045, x64;
- Intel Core i7-12700K;
- 31,8 ГБ RAM;
- NVIDIA GeForce RTX 4060 и Intel UHD Graphics 770;
- инференс выполнен только на CPU (`-ngl 0`, 2 потока), GPU не использовался;
- `llama-server` 0.7.0-dev, build 11755, commit `704366d76`;
- [Qwen2.5-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF),
  `qwen2.5-0.5b-instruct-q4_k_m.gguf`, Q4_K_M, 491 400 032 байта.

Веса, `llama-server`, его исходники и сборочные файлы в Git не добавляются.

## Запуск llama-server

Модель и сервер нужно скачать по инструкции урока. Из каталога `llama.cpp` на Windows:

```powershell
.\llama-server.exe -m .\models\qwen2.5-0.5b-instruct-q4_k_m.gguf --alias local-model -c 1024 -ngl 0 -t 2 -np 1 --host 127.0.0.1 --port 8080
```

Проверка готовности:

```powershell
curl.exe -i http://127.0.0.1:8080/health
```

Ожидается HTTP 200 и `{"status":"ok"}`. Веб-интерфейс доступен по
[http://127.0.0.1:8080](http://127.0.0.1:8080). Для проверки можно ввести
`Ответь одним словом: сколько будет 2 + 2?`. Та же проверка через HTTP сохранена в
[`examples/http-request.json`](examples/http-request.json), фактический ответ — в
[`examples/http-response.json`](examples/http-response.json): модель ответила `Два.`.

```powershell
curl.exe --fail-with-body --max-time 180 http://127.0.0.1:8080/v1/chat/completions -H "Content-Type: application/json" --data-binary "@examples/http-request.json"
```

## Сборка и тесты

Нужны CMake 3.20+, Git, компилятор C++17 и интернет при первой конфигурации. CMake через
`FetchContent` загружает закреплённые `nlohmann_json v3.11.3` и HTTP-библиотеку
`cpp-httplib v0.18.3`. HTTPS клиенту не нужен: сервер слушает loopback HTTP-адрес.

Из `assignments/lesson06/Zakharov/`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Запуск клиента

В Windows с Visual Studio:

```powershell
.\build\Debug\model_chat.exe config.example.json
```

В Linux/macOS или с Ninja/Make:

```bash
./build/model_chat config.example.json
```

Команда `/reset` очищает историю, но сохраняет системную реплику; `/exit` завершает клиент.

`config.example.json` задаёт адрес, псевдоним модели, контекст 1024, `max_tokens = 64`,
температуру и тайм-ауты. Тайм-аут соединения — 5 с, конечный тайм-аут каждого HTTP-запроса —
180 с. Значение `context_size` должно совпадать с `-c` сервера.

## Контроль контекста

Перед генерацией клиент отправляет тот же JSON на `/v1/chat/completions/input_tokens`. Если
`input_tokens + 64 > 1024`, он удаляет самую старую завершённую пару `user`–`assistant` и считает
снова. Системное сообщение и текущий ввод не удаляются. Если не помещается даже текущий
ввод, генерация не запускается. При любой ошибке незавершённая реплика `user` откатывается.

Фактическая проверка удаления пары и отказа для длинного ввода записана в
[`examples/context-control.md`](examples/context-control.md).

## Примеры и оценка

Три фактических реплики есть в [`examples/chat-session.md`](examples/chat-session.md):

1. Сообщение имени — модель ответила неловко и смешала роли, но имя попало в историю.
2. Вопрос об имени — память сработала, ответ по смыслу верный.
3. Тот же вопрос после `/reset` — имя забыто, но модель неудачно выдумала ответ `AI`.

Модель 0.5B достаточна для демонстрации истории и API, но её ответы нельзя считать надёжными.

## Ошибки и структура

Клиент завершается с кодом 1 при неверной конфигурации, отсутствии соединения, тайм-ауте,
неуспешном HTTP-статусе, неверном JSON, ошибке структуры `input_tokens`, `choices` или `usage`, а также при
переполнении контекста.

```text
include/config.hpp             проверка конфигурации
include/http_json_client.hpp   HTTP POST и проверка статуса/JSON
include/chat.hpp               запросы, разбор ответа, история и токены
src/main.cpp                   цикл CLI и команды
tests/model_chat_test.cpp      офлайн-тесты без модели
examples/                      фактические запросы, ответы и пограничные случаи
```

Библиотека `model_chat_core` содержит всю логику; `model_chat` и `model_chat_test` ссылаются на одну и ту же цель.

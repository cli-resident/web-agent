# Обработчики задач в TaskHandler

Этот документ описывает, как в проекте реализована обработка заданий на уровне `TaskHandler` (`include/task_handler.h`, `src/task_handler.cpp`).

## Общий маршрут обработки

Входной метод: `TaskHandler::process(const TaskInfo& task)`.

Маршрутизация по `task.task_code`:
- `TIMEOUT` -> `handleTimeout(task)`
- `CONF` -> `handleConf(task)`
- `FILE` -> `handleFile(task)`
- `TASK`, `EXEC`, `CMD` -> `handleNetworkTask(task)`
- любое другое значение -> результат с ошибкой `Unknown task_code`

После выполнения обработчик обычно вызывает `sendResult(...)`, который отправляет результат в API `/wa_result/` через `HttpClient`.

---

## 1) TIMEOUT

Метод: `handleTimeout(const TaskInfo& task)`.

Что делает:
1. Считывает `task.options` как целое число (`std::stoi`).
2. Проверяет, что число больше 0.
3. Присваивает `cfg_.poll_interval_sec = new_interval`.
4. Сохраняет конфиг на диск (`cfg_.save()`).
5. Отправляет `result_code=0` и сообщение об успешной смене интервала.

Ошибки:
- если парсинг не удался или значение некорректное (`<=0`) -> `result_code=-2`.

Итог:
- динамически меняет интервал опроса сервера и делает изменение персистентным.

---

## 2) CONF

Метод: `handleConf(const TaskInfo& task)`.

Что делает сейчас:
- отправляет успешный ответ с текстом `CONF task not implemented yet`.

Итог:
- это заглушка под будущую поддержку удаленного обновления конфигурации.

---

## 3) FILE

Метод: `handleFile(const TaskInfo& task)`.

Что делает:
1. Открывает файл `test_payload.json` в рабочей директории.
2. Пытается распарсить `task.options` как JSON.
3. Если JSON валиден -> сохраняет pretty-print (`dump(2)`).
4. Если JSON невалиден -> сохраняет исходную строку как есть.
5. Отправляет `result_code=0` и сообщение `FILE task saved payload to test_payload.json`.

Ошибки:
- если файл не открылся или произошло исключение -> `result_code=-3`.

Итог:
- задача служит для надежного сохранения входного payload локально.

---

## 4) TASK / EXEC / CMD (единый путь: сетевая диагностика)

Метод: `handleNetworkTask(const TaskInfo& task)`.

Важно:
- в текущей реализации `TASK`, `EXEC`, `CMD` обрабатываются одинаково как netdiag-задача.

### Формат входных параметров

`task.options` ожидается как JSON:

```json
{
  "targets": [
    {"url": "https://example.com/health", "timeout_sec": 8},
    {"url": "https://example.org/status"}
  ],
  "timeout_sec": 8
}
```

Поддержка полей:
- `targets` — обязательный массив;
- `url` (или `host`) — адрес проверки;
- `timeout_sec` — таймаут цели;
- корневой `timeout_sec` — дефолт для целей без собственного таймаута.

### Шаги выполнения

1. Валидирует `task.options` и наличие `targets`.
2. Создает отчёт:
   - `session_id`
   - `generated_at`
   - массив `targets`
3. Для каждой цели вызывает `probeTarget(url, timeout_sec)`.
4. В `probeTarget` выполняет `cpr::Get` с SSL verify и таймаутом.
5. Для каждой цели фиксирует:
   - `url`
   - `success`
   - `latency_ms`
   - `status_code`
   - `bytes`
   - `error`
   - `timestamp`
6. Если валидных целей нет -> ошибка `result_code=-2`.
7. Сохраняет JSON-отчёт в файл `results/netdiag_<session_id>.json`.
8. Отправляет отчёт во внешний AI (`buildAiSummary`).
9. Сохраняет summary в `results/netdiag_<session_id>_summary.txt`.
10. Отправляет `sendResult(..., code=0, message=<summary>, files=[json, txt])`.

### Логика статуса цели

Упрощенно:
- `2xx..3xx` и без transport error -> `success=true`;
- network ошибка или `>=400` -> `success=false` + текст ошибки.

---

## 5) Работа с AI summary

Метод: `buildAiSummary(const std::string& report_json)`.

Что делает:
1. Формирует chat-completions запрос:
   - endpoint: `https://api.mistral.ai/v1/chat/completions`
   - model: `mistral-small-latest`
   - messages: system prompt + JSON-отчёт
2. Отправляет `POST` с `Authorization: Bearer <key>`.
3. На успехе извлекает `choices[0].message.content`.
4. Если ответ неожиданного формата или ошибка сети/HTTP:
   - пишет warning в лог;
   - возвращает fallback через `buildFallbackSummary`.

Fallback:
- если JSON отчёта валиден: компактная сводка по каждой цели;
- если невалиден: короткий текст о недоступности AI.

Примечание по безопасности:
- в текущем коде API key и модель заданы константами;
- для production это нужно вынести в конфигурацию/секреты.

---

## 6) Формирование и отправка результата

Метод: `sendResult(...)`.

Что отправляется:
- `session_id`
- `result_code`
- `message` (обычно AI summary или fallback)
- список файлов (JSON-отчёт, summary; для FILE может быть без артефактов)

Поведение:
- `TaskHandler` вызывает `HttpClient::sendResult`;
- по `resp.code==0` пишет успешный лог;
- иначе warning;
- исключения ловятся, пишется error.

---

## 7) Коды ошибок в TaskHandler

Используемые коды:
- `0` — успех;
- `-1` — неизвестный `task_code`;
- `-2` — ошибка валидации/парсинга входных параметров;
- `-3` — ошибка обработки `FILE`.

---

## 8) Артефакты на диске

Файлы, которые может создавать `TaskHandler`:
- `test_payload.json` — для `FILE`;
- `results/netdiag_<session_id>.json` — технический отчёт netdiag;
- `results/netdiag_<session_id>_summary.txt` — AI/fallback сводка.

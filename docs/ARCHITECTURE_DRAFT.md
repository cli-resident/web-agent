# Черновик архитектуры WEB-AGENT

**Статус:** Черновик (ЛР №1)
**Версия:** 0.1

---

## Обзор

WEB-AGENT — однопроцессный многопоточный фоновый агент. Основной поток занимается циклом опроса сервера, задания выполняются в пуле потоков. Весь доступ к сети централизован в `HttpClient`, конфигурация неизменна после загрузки, логирование потокобезопасно.

Архитектура намеренно плоская: 4 модуля без глубокой иерархии наследования. Цель — простота и тестируемость.

---

## Диаграмма компонентов

```mermaid
graph TB
    subgraph "web_agent executable"
        MAIN["main.cpp\n(точка входа)"]
    end

    subgraph "agent_lib (static library)"
        CFG["Config\n(config.h/cpp)\nЗагрузка и валидация\nconifg.json"]
        LOG["Logger\n(logger.h/cpp)\nОбёртка над spdlog\nПотокобезопасная"]
        HTTP["HttpClient\n(http_client.h/cpp)\nHTTP-взаимодействие\nчерез CPR"]
        AGT["Agent\n(agent.h/cpp)\nОсновной цикл опроса\nУправление задачами"]
    end

    subgraph "Внешние зависимости"
        CPR["cpr\n(HTTP-клиент)"]
        JSON["nlohmann/json\n(JSON парсинг)"]
        SPDLOG["spdlog\n(Логирование)"]
        GTEST["Google Test\n(Тестирование)"]
    end

    subgraph "Файловая система"
        CFGFILE["config.json"]
        LOGFILE["agent.log"]
        TASKDIR["tasks/"]
        RESDIR["results/"]
    end

    subgraph "Удалённый сервер"
        SERVER["https://xdev.arkcom.ru:9999\n/api/wa_reg/\n/api/wa_task/\n/api/wa_result/"]
    end

    MAIN --> CFG
    MAIN --> LOG
    MAIN --> AGT
    AGT --> HTTP
    AGT --> LOG
    HTTP --> CPR
    HTTP --> JSON
    CFG --> JSON
    LOG --> SPDLOG
    CFG --> CFGFILE
    LOG --> LOGFILE
    AGT --> TASKDIR
    AGT --> RESDIR
    HTTP --> SERVER
```

---

## Модули

| Модуль | Файлы | Назначение | Статус (ЛР №1) |
|---|---|---|---|
| Config | `include/config.h`, `src/config.cpp` | Загрузка и валидация конфигурации из JSON | Реализован |
| Logger | `include/logger.h`, `src/logger.cpp` | Потокобезопасное логирование (spdlog) | Реализован |
| HttpClient | `include/http_client.h`, `src/http_client.cpp` | HTTP-запросы к API сервера (cpr) | Заглушка |
| Agent | `include/agent.h`, `src/agent.cpp` | Цикл опроса, управление заданиями | Заглушка |
| main | `src/main.cpp` | Точка входа, разбор аргументов | Рабочий |

---

## Планируемый поток выполнения

```mermaid
sequenceDiagram
    participant M as main()
    participant CFG as Config
    participant LOG as Logger
    participant AGT as Agent
    participant HTTP as HttpClient
    participant SRV as Server

    M->>CFG: Config::load("config.json")
    CFG-->>M: cfg
    M->>LOG: Logger::init(log_file, log_level)
    M->>AGT: Agent(cfg)
    M->>AGT: agent.init()
    AGT->>HTTP: registerAgent()
    HTTP->>SRV: POST /api/wa_reg/ {uid, descr}
    SRV-->>HTTP: {code:0, access_code:"..."}
    HTTP-->>AGT: RegResponse
    AGT-->>M: true

    M->>AGT: agent.run()

    loop Poll Loop (каждые poll_interval_sec)
        AGT->>HTTP: requestTask()
        HTTP->>SRV: POST /api/wa_task/ {uid, access_code}
        SRV-->>HTTP: {code:1, task_code:"CMD", session_id:"..."}
        HTTP-->>AGT: TaskInfo

        alt Есть задание (code==1)
            AGT->>AGT: handleTask(task) [в отдельном потоке]
            AGT->>HTTP: sendResult(session_id, 0, "OK", files)
            HTTP->>SRV: POST /api/wa_result/ multipart
            SRV-->>HTTP: {code:0, msg:"accepted"}
        else Ожидание (code==0 / status==WAIT)
            AGT->>AGT: sleep(poll_interval_sec)
        end
    end
```

---

## Зависимости

| Библиотека | Версия | Лицензия | Назначение | Подключение |
|---|---|---|---|---|
| nlohmann/json | 3.11.3 | MIT | JSON сериализация/десериализация | FetchContent |
| spdlog | 1.13.0 | MIT | Высокопроизводительное логирование | FetchContent |
| cpr | 1.10.5 | MIT | HTTP-клиент (обёртка над libcurl) | FetchContent |
| Google Test | 1.14.0 | BSD-3 | Unit/интеграционное тестирование | FetchContent |

---

## Структура потоков

```
Процесс web_agent
├── Главный поток
│   ├── Config::load()
│   ├── Logger::init()
│   ├── Agent::init() → HTTP registerAgent
│   └── Agent::run() → запускает poll_thread
├── poll_thread (Agent::pollLoop)
│   ├── HTTP requestTask() → ждёт задание
│   └── При получении задания → запускает task_thread
└── task_thread[0..N] (Agent::handleTask) — макс. max_parallel_tasks
    ├── Запуск процесса / выполнение команды
    ├── Ожидание завершения
    └── HTTP sendResult() → отправляет результат
```

---

## TODO для ЛР №2

- [ ] Реализовать `HttpClient::registerAgent()` через cpr (POST + JSON)
- [ ] Реализовать `HttpClient::requestTask()` через cpr
- [ ] Реализовать `HttpClient::sendResult()` с multipart/form-data
- [ ] Добавить retry-логику с exponential backoff в HttpClient
- [ ] Добавить таймауты (connect 5с, request 10с, upload 30с)
- [ ] Реализовать `Agent::pollLoop()` с sleep и graceful shutdown
- [ ] Реализовать `Agent::handleTask()` для типов CMD и EXEC
- [ ] Добавить пул потоков с лимитом `max_parallel_tasks`
- [ ] Добавить обработку сигналов SIGINT/SIGTERM
- [ ] Написать unit-тесты для HttpClient (mock-сервер)
- [ ] Написать интеграционные тесты (регистрация на реальном сервере)

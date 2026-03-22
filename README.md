# WEB-AGENT

Кроссплатформенный фоновый HTTP-клиент для удалённого управления задачами.

## Описание проекта

WEB-AGENT — это автономный фоновый агент, который:

1. Регистрируется на сервере и получает `access_code`
2. Периодически опрашивает сервер в поиске новых заданий
3. Выполняет задания: запуск программ, выполнение команд, передача файлов
4. Отправляет результаты на сервер (multipart/form-data)
5. Логирует все операции в файл и stdout

**API сервера:** `https://xdev.arkcom.ru:9999`

## Статус

**ЛР №1 — Инициализация проекта**

| Компонент | Статус |
|---|---|
| Config (загрузка config.json) | Реализован |
| Logger (spdlog) | Реализован |
| HttpClient (HTTP через cpr) | Заглушка (ЛР №3) |
| Agent (цикл опроса) | Заглушка (ЛР №3) |
| Тесты среды и конфига | Реализованы |

## Требования

| Инструмент | Версия |
|---|---|
| Компилятор | GCC 10+ / Clang 12+ / MSVC 2019+ (поддержка C++17) |
| CMake | 3.16+ |
| Git | 2.x |
| libcurl | Системная (Linux/macOS) или bundled (Windows через cpr) |

## Быстрый старт

### Linux / macOS

```bash
git clone <repo-url> web-agent
cd web-agent

# Настроить конфигурацию
cp config.json.example config.json  # или отредактировать config.json

# Сборка
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DWA_BUILD_TESTS=ON
cmake --build build -j$(nproc)

# Запуск
./build/web_agent --config config.json
```

### Windows

```powershell
git clone <repo-url> web-agent
cd web-agent

# Сборка
cmake -B build -DWA_BUILD_TESTS=ON
cmake --build build --config Debug

# Запуск
.\build\Debug\web_agent.exe --config config.json
```

## Конфигурация

Файл `config.json` в корне проекта:

| Поле | Тип | Обязательное | По умолчанию | Описание |
|---|---|---|---|---|
| `uid` | string | **Да** | — | Уникальный ID агента |
| `descr` | string | Нет | `"web-agent"` | Описание агента |
| `server_url` | string | **Да** | — | Base URL сервера |
| `poll_interval_sec` | int | Нет | `10` | Интервал опроса (сек) |
| `task_directory` | string | Нет | `"./tasks"` | Директория заданий |
| `result_directory` | string | Нет | `"./results"` | Директория результатов |
| `log_file` | string | Нет | `"./agent.log"` | Путь к файлу лога |
| `log_level` | string | Нет | `"info"` | Уровень: debug/info/warn/error |
| `access_code` | string | Нет | `""` | Предварительно выданный токен; если заполнен, регистрация пропускается |
| `max_parallel_tasks` | int | Нет | `4` | Макс. параллельных заданий |
| `retry_count` | int | Нет | `3` | Кол-во повторных попыток |
| `retry_delay_sec` | int | Нет | `5` | Начальная задержка retry (сек) |

Если `access_code` пустой, агент сначала зарегистрируется, получит токен и сохранит его в `config.json`.

## Сборка

```bash
# Debug (с тестами)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DWA_BUILD_TESTS=ON
cmake --build build

# Release (без тестов)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DWA_BUILD_TESTS=OFF
cmake --build build
```

## Запуск тестов

```bash
cd build
ctest --output-on-failure
# или напрямую:
./wa_tests
```

## Структура проекта

```
web-agent/
├── CMakeLists.txt              # Корневой файл сборки
├── cmake/
│   └── Dependencies.cmake      # FetchContent зависимостей
├── include/                    # Публичные заголовки
│   ├── config.h                # Конфигурация агента
│   ├── logger.h                # Логгер (обёртка spdlog)
│   ├── http_client.h           # HTTP-клиент
│   └── agent.h                 # Основной класс агента
├── src/                        # Реализации
│   ├── config.cpp              # Загрузка config.json (реализован)
│   ├── logger.cpp              # Инициализация spdlog (реализован)
│   ├── http_client.cpp         # HTTP-запросы (заглушка)
│   ├── agent.cpp               # Цикл опроса (заглушка)
│   └── main.cpp                # Точка входа
├── tests/
│   ├── CMakeLists.txt          # Сборка тестов
│   ├── test_config.cpp         # Тесты модуля Config (6 тест-кейсов)
│   ├── test_environment.cpp    # Тесты среды разработки (5 тест-кейсов)
│   └── test_repository.cpp     # Тесты структуры репозитория (5 тест-кейсов)
├── docs/
│   ├── TZ.md                   # Техническое задание
│   └── ARCHITECTURE_DRAFT.md  # Черновик архитектуры
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions CI (3 платформы)
├── config.json                 # Пример конфигурации
├── .gitignore
└── README.md
```

## Команда

| Роль | Имя |
|---|---|
| Тимлид | Васильев Иван |
| Проектировщик | Титов Владислав |
| Разработчик 1 | Решетов Максим |
| Разработчик 2 | Следнев Данила |
| Тестировщик | Шестаков Максим |
| Технический писатель | Зверев Петр |

## Лицензия

MIT License. See [LICENSE](LICENSE) for details.

<div align="center">

<img src="logo.png" alt="FPS Booster" width="128"/>

# ⚡ MacTFPS
**Geode mod для Geometry Dash 2.2+**

Убирает лимит кадров, разгоняет физику и навсегда вырубает V-Sync — в том числе на Apple Silicon.

[![Geode](https://img.shields.io/badge/Geode-4.2.0-orange?style=flat-square&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PC9zdmc+)](https://geode-sdk.org)
[![GD Version](https://img.shields.io/badge/GD-2.2074-yellow?style=flat-square)](https://store.steampowered.com/app/322170)
[![Platform](https://img.shields.io/badge/platform-Win%20%7C%20Mac%20%7C%20Android-blue?style=flat-square)]()
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)

</div>

---

## 🚀 Возможности

| Функция | Описание |
|---|---|
| 🎯 **FPS Cap** | От 1 до 1000 FPS или полный безлимит |
| 🔁 **TPS Multiplier** | До 8× физических тиков (240 → 1920 TPS) |
| 🖥️ **V-Sync Bypass** | Отключает вертикальную синхронизацию на уровне драйвера |
| 🍎 **Apple Silicon** | Специальный обход через CGL на M1/M2/M3 |
| 📊 **FPS Counter** | Оверлей в любом углу экрана |

---

## 🍎 V-Sync на Apple Silicon

GD на macOS по умолчанию залочен на частоту дисплея (60/120 Гц) через Metal/OpenGL swap interval. Этот мод отключает блокировку напрямую через **CGL** (`CGLSetParameter` → `kCGLCPSwapInterval = 0`), что работает на всех чипах семейства Apple Silicon (M1, M2, M3 и их Pro/Max/Ultra вариантах) через слой совместимости OpenGL.

Опция **"Keep V-Sync Disabled"** переприменяет это каждый кадр, чтобы движок не смог вернуть VSync самостоятельно.

---

## ⚙️ Настройки

Все параметры доступны прямо в меню Geode → этот мод.

```
Target FPS          [0 – 1000]   0 = безлимит, по умолчанию 240
TPS Multiplier      [1× – 8×]    множитель физических тиков
Disable V-Sync      [вкл/выкл]   отключить VSync при запуске
Keep V-Sync Disabled[вкл/выкл]   блокировать VSync каждый кадр (macOS)
Show FPS Counter    [вкл/выкл]   показывать счётчик FPS
FPS Counter Position             Top Left / Top Right / Bottom Left / Bottom Right
```

---

## 📦 Установка

### Через Geode (рекомендуется)
1. Открой Geode → Download
2. Найди **FPS & TPS Booster**
3. Нажми Install

### Вручную
1. Скачай `.geode`-файл из [Releases](../../releases/latest)
2. Положи в папку `<GD>/geode/mods/`
3. Запусти игру

---

## 🔨 Сборка из исходников

**Требования:**
- [Geode SDK](https://docs.geode-sdk.org/getting-started/) (4.2.0+)
- CMake 3.21+
- Clang / MSVC / Apple Clang

```bash
git clone https://github.com/yourname/fps-booster
cd fps-booster

# Укажи путь к Geode SDK (если не задан глобально)
export GEODE_SDK=/path/to/geode-sdk

cmake -B build
cmake --build build --config RelWithDebInfo
```

Готовый `.geode`-файл появится в папке `build/`.

---

## 🧠 Как это работает

### FPS
Хук на `CCDirector::setAnimationInterval` и `drawScene` перехватывает все попытки движка изменить интервал кадра и заменяет его значением из настроек. Применяется также через хук `AppDelegate::applicationDidFinishLaunching` — сразу после старта Cocos2d.

### TPS
Хук на `PlayLayer::update` дробит один визуальный кадр на N равных физических подшагов с пропорционально уменьшенным `dt`. Скорость игры не меняется, зато коллизии и физика становятся точнее при высоком FPS.

```
Обычно:  [frame dt=1/60] → update(dt)
2× TPS:  [frame dt=1/60] → update(dt/2) → update(dt/2)
4× TPS:  [frame dt=1/60] → update(dt/4) × 4
```

### V-Sync (macOS)
```cpp
CGLContextObj ctx = CGLGetCurrentContext();
GLint zero = 0;
CGLSetParameter(ctx, kCGLCPSwapInterval, &zero);
```
Это нижний уровень управления OpenGL на macOS — ниже уже только kernel extension. Работает поверх Metal через слой совместимости, который GD использует на Apple Silicon.

---

## 🐛 Известные проблемы

- При очень высоком TPS Multiplier (8×) на слабых устройствах может просесть FPS — это нормально, снизьте множитель
- Android: V-Sync bypass не реализован (зависит от EGL, в разработке)
- Редко: FPS-счётчик может пропасть при смене сцены — решается отключением и повторным включением в настройках

---

## 📄 Лицензия

MIT — делай что хочешь, но укажи авторство.

---

<div align="center">
Сделано с ❤️ и ненавистью к 60 FPS
</div>
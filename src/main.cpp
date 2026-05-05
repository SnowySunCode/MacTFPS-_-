#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/AppDelegate.hpp>

// ── macOS-specific V-Sync headers ─────────────────────────────────────────
#ifdef GEODE_IS_MACOS
  #include <OpenGL/OpenGL.h>
  #include <OpenGL/CGLTypes.h>
  #include <OpenGL/CGLCurrent.h>
#endif
// ──────────────────────────────────────────────────────────────────────────

using namespace geode::prelude;

// ═════════════════════════════════════════════════════════════════════════════
//  FPS overlay
// ═════════════════════════════════════════════════════════════════════════════

static CCLabelBMFont* s_fpsLabel     = nullptr;
static float          s_fpsTimer     = 0.f;
static int            s_fpsCount     = 0;
static float          s_displayedFPS = 0.f;

static void ensureFPSLabel() {
    bool show = Mod::get()->getSettingValue<bool>("show-fps-counter");

    if (!show) {
        if (s_fpsLabel) {
            s_fpsLabel->removeFromParent();
            s_fpsLabel = nullptr;
        }
        return;
    }

    auto* scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;

    if (!s_fpsLabel || !s_fpsLabel->getParent()) {
        s_fpsLabel = CCLabelBMFont::create("FPS: --", "bigFont.fnt");
        if (!s_fpsLabel) return;
        s_fpsLabel->setScale(0.45f);
        s_fpsLabel->setOpacity(220);
        s_fpsLabel->setZOrder(999);
        scene->addChild(s_fpsLabel);
    }

    // Позиция — верхний левый угол
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    const float pad = 8.f;
    float w = s_fpsLabel->getContentWidth()  * 0.45f;
    float h = s_fpsLabel->getContentHeight() * 0.45f;
    s_fpsLabel->setPosition({ pad + w * 0.5f, winSize.height - pad - h * 0.5f });
}

// ═════════════════════════════════════════════════════════════════════════════
//  V-Sync bypass (macOS / Apple Silicon)
// ═════════════════════════════════════════════════════════════════════════════

#ifdef GEODE_IS_MACOS
static void disableVSync() {
    CGLContextObj ctx = CGLGetCurrentContext();
    if (ctx) {
        GLint zero = 0;
        CGLSetParameter(ctx, kCGLCPSwapInterval, &zero);
    }
}
#endif

// ═════════════════════════════════════════════════════════════════════════════
//  Утилита: применить нужный интервал кадра
// ═════════════════════════════════════════════════════════════════════════════

static void applyFPS() {
    int64_t fps = Mod::get()->getSettingValue<int64_t>("target-fps");
    if (fps <= 0)
        CCDirector::sharedDirector()->setAnimationInterval(0.00025); // ~4000 FPS потолок
    else
        CCDirector::sharedDirector()->setAnimationInterval(1.0 / static_cast<double>(fps));
}

// ═════════════════════════════════════════════════════════════════════════════
//  CCDirector hook – FPS cap + V-Sync bypass каждый кадр
// ═════════════════════════════════════════════════════════════════════════════

class $modify(FPSDirector, CCDirector) {

    // Перехватываем любую попытку игры сменить интервал
    void setAnimationInterval(double interval) {
        applyFPS();
    }

    void drawScene() {

#ifdef GEODE_IS_MACOS
        if (Mod::get()->getSettingValue<bool>("disable-vsync") &&
            Mod::get()->getSettingValue<bool>("vsync-persistent")) {
            disableVSync();
        }
#endif

        CCDirector::drawScene();

        // ── FPS счётчик ────────────────────────────────────────────────────
        float dt = CCDirector::sharedDirector()->getDeltaTime();
        if (dt <= 0.f) return;

        s_fpsTimer += dt;
        s_fpsCount++;

        if (s_fpsTimer >= 0.25f) {
            s_displayedFPS = static_cast<float>(s_fpsCount) / s_fpsTimer;
            s_fpsTimer     = 0.f;
            s_fpsCount     = 0;

            ensureFPSLabel();
            if (s_fpsLabel) {
                s_fpsLabel->setString(
                    fmt::format("FPS: {:.0f}", s_displayedFPS).c_str()
                );
            }
        } else {
            ensureFPSLabel();
        }
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  PlayLayer hook – TPS multiplier
//
//  GD 2.2 работает на фиксированных 240 тиках/сек.
//  Мы дробим один визуальный кадр на N равных физических подшагов,
//  пропорционально уменьшая dt — скорость игры не меняется.
// ═════════════════════════════════════════════════════════════════════════════

class $modify(TPSPlayLayer, PlayLayer) {

    void update(float dt) {
        int mult = static_cast<int>(
            Mod::get()->getSettingValue<int64_t>("tps-multiplier")
        );

        if (mult <= 1) {
            PlayLayer::update(dt);
            return;
        }

        float subDt = dt / static_cast<float>(mult);
        for (int i = 0; i < mult; ++i) {
            PlayLayer::update(subDt);
        }
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  AppDelegate hook – применяем настройки сразу при старте
// ═════════════════════════════════════════════════════════════════════════════

class $modify(FPSAppDelegate, AppDelegate) {

    bool applicationDidFinishLaunching() {
        bool result = AppDelegate::applicationDidFinishLaunching();

        applyFPS();

#ifdef GEODE_IS_MACOS
        if (Mod::get()->getSettingValue<bool>("disable-vsync")) {
            disableVSync();
        }
#endif

        return result;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  Мод загружен – слушаем изменения настроек в реальном времени
// ═════════════════════════════════════════════════════════════════════════════

$on_mod(Loaded) {

    listenForSettingChanges<int64_t>("target-fps", [](int64_t fps) {
        applyFPS();
        log::info("[MacTFPS] Target FPS → {}", fps <= 0 ? "Unlimited" : std::to_string(fps));
    });

    listenForSettingChanges<int64_t>("tps-multiplier", [](int64_t mult) {
        log::info("[MacTFPS] TPS multiplier → {}x ({}  TPS)", mult, mult * 240);
    });

#ifdef GEODE_IS_MACOS
    listenForSettingChanges<bool>("disable-vsync", [](bool enabled) {
        if (enabled) disableVSync();
        log::info("[MacTFPS] V-Sync bypass: {}", enabled ? "ON" : "OFF");
    });
#endif

    listenForSettingChanges<bool>("show-fps-counter", [](bool enabled) {
        if (!enabled && s_fpsLabel) {
            s_fpsLabel->removeFromParent();
            s_fpsLabel = nullptr;
        }
        log::info("[MacTFPS] FPS counter: {}", enabled ? "ON" : "OFF");
    });

    log::info("[MacTFPS] Mod loaded! FPS={}, TPS={}x",
        Mod::get()->getSettingValue<int64_t>("target-fps"),
        Mod::get()->getSettingValue<int64_t>("tps-multiplier")
    );
}
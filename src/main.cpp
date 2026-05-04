#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <Geode/ui/GeodeUI.hpp>

// ── macOS-specific V-Sync headers ──────────────────────────────────────────
#ifdef GEODE_IS_MACOS
  #include <OpenGL/OpenGL.h>    // CGLGetCurrentContext, CGLSetParameter
  #include <OpenGL/CGLTypes.h>
  #include <OpenGL/CGLCurrent.h>
#endif
// ───────────────────────────────────────────────────────────────────────────

using namespace geode::prelude;

// ══════════════════════════════════════════════════════════════════════════════
//  Helper – FPS overlay label
// ══════════════════════════════════════════════════════════════════════════════

static CCLabelBMFont* s_fpsLabel     = nullptr;
static float          s_fpsTimer     = 0.f;
static int            s_fpsCount     = 0;
static float          s_displayedFPS = 0.f;

static void ensureFPSLabel() {
    if (!Mod::get()->getSettingValue<bool>("show-fps-counter")) {
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
        s_fpsLabel->setScale(0.45f);
        s_fpsLabel->setOpacity(220);
        s_fpsLabel->setZOrder(999);
        scene->addChild(s_fpsLabel);
    }

    // Position
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    const float pad = 6.f;
    auto pos = Mod::get()->getSettingValue<std::string>("fps-counter-position");

    if (pos == "Top Left")
        s_fpsLabel->setPosition({ pad + s_fpsLabel->getContentWidth() * 0.45f * 0.5f,
                                   winSize.height - pad - s_fpsLabel->getContentHeight() * 0.45f * 0.5f });
    else if (pos == "Top Right")
        s_fpsLabel->setPosition({ winSize.width - pad - s_fpsLabel->getContentWidth() * 0.45f * 0.5f,
                                   winSize.height - pad - s_fpsLabel->getContentHeight() * 0.45f * 0.5f });
    else if (pos == "Bottom Left")
        s_fpsLabel->setPosition({ pad + s_fpsLabel->getContentWidth() * 0.45f * 0.5f,
                                   pad + s_fpsLabel->getContentHeight() * 0.45f * 0.5f });
    else  // Bottom Right
        s_fpsLabel->setPosition({ winSize.width - pad - s_fpsLabel->getContentWidth() * 0.45f * 0.5f,
                                   pad + s_fpsLabel->getContentHeight() * 0.45f * 0.5f });
}

// ══════════════════════════════════════════════════════════════════════════════
//  V-Sync bypass (macOS / Apple Silicon)
// ══════════════════════════════════════════════════════════════════════════════

#ifdef GEODE_IS_MACOS
static void disableVSync() {
    // CGL path – works on both Intel OpenGL and the compatibility layer on M1/M2/M3
    CGLContextObj ctx = CGLGetCurrentContext();
    if (ctx) {
        GLint swapInterval = 0;
        CGLSetParameter(ctx, kCGLCPSwapInterval, &swapInterval);
    }
}
#endif

// ══════════════════════════════════════════════════════════════════════════════
//  CCDirector hook – FPS cap + V-Sync bypass per-frame
// ══════════════════════════════════════════════════════════════════════════════

class $modify(FPSDirector, CCDirector) {

    // Called by Cocos2d every time it wants to change the animation interval.
    // We intercept and enforce our own target.
    void setAnimationInterval(double interval) {
        auto fps = Mod::get()->getSettingValue<int64_t>("target-fps");

        if (fps <= 0) {
            // Unlimited – use a very small interval (≈ 4000 FPS cap ceiling)
            CCDirector::setAnimationInterval(0.00025);
        } else {
            CCDirector::setAnimationInterval(1.0 / static_cast<double>(fps));
        }
    }

    // Main render loop – runs every frame.
    void drawScene() {

// ── macOS: persistently kill V-Sync ────────────────────────────────────────
#ifdef GEODE_IS_MACOS
        if (Mod::get()->getSettingValue<bool>("disable-vsync") &&
            Mod::get()->getSettingValue<bool>("vsync-persistent")) {
            disableVSync();
        }
#endif
// ───────────────────────────────────────────────────────────────────────────

        CCDirector::drawScene();

        // ── FPS counter update ──────────────────────────────────────────────
        float dt = CCDirector::sharedDirector()->getDeltaTime();
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
            ensureFPSLabel();  // keep label parented after scene changes
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════════
//  PlayLayer hook – TPS (physics ticks) multiplier
//
//  GD 2.2 runs physics at a fixed 240 Hz step.  We call the physics updater
//  N extra times per visual frame with a proportionally scaled dt so that
//  the simulation stays correct while running at a higher tick rate.
// ══════════════════════════════════════════════════════════════════════════════

class $modify(TPSPlayLayer, PlayLayer) {

    void update(float dt) {
        int mult = static_cast<int>(
            Mod::get()->getSettingValue<int64_t>("tps-multiplier")
        );

        if (mult <= 1) {
            PlayLayer::update(dt);
            return;
        }

        // Split the frame into `mult` equal sub-steps.
        // The first call uses the actual Cocos dt; remaining calls use
        // a fixed physics step to avoid cascade errors.
        float subDt = dt / static_cast<float>(mult);

        for (int i = 0; i < mult; ++i) {
            PlayLayer::update(subDt);
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════════
//  AppDelegate hook – disable V-Sync once at startup (non-persistent path)
// ══════════════════════════════════════════════════════════════════════════════

class $modify(FPSAppDelegate, AppDelegate) {
    bool applicationDidFinishLaunching() {
        bool result = AppDelegate::applicationDidFinishLaunching();

        // Apply target FPS immediately after the Cocos2d director starts.
        auto fps = Mod::get()->getSettingValue<int64_t>("target-fps");
        if (fps <= 0)
            CCDirector::sharedDirector()->setAnimationInterval(0.00025);
        else
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / static_cast<double>(fps));

#ifdef GEODE_IS_MACOS
        if (Mod::get()->getSettingValue<bool>("disable-vsync")) {
            disableVSync();
        }
#endif

        return result;
    }
};

// ══════════════════════════════════════════════════════════════════════════════
//  Mod lifecycle – listen for setting changes at runtime
// ══════════════════════════════════════════════════════════════════════════════

$on_mod(Loaded) {
    // Re-apply FPS cap when the user changes "target-fps" in the mod settings.
    Mod::get()->addCustomSetting<int64_t>("target-fps", 240);

    listenForSettingChanges<int64_t>("target-fps", [](int64_t fps) {
        if (fps <= 0)
            CCDirector::sharedDirector()->setAnimationInterval(0.00025);
        else
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / static_cast<double>(fps));

        log::info("[FPS Booster] Target FPS changed → {}", fps <= 0 ? "Unlimited" : std::to_string(fps));
    });

#ifdef GEODE_IS_MACOS
    listenForSettingChanges<bool>("disable-vsync", [](bool enabled) {
        if (enabled) disableVSync();
        log::info("[FPS Booster] V-Sync bypass: {}", enabled ? "ON" : "OFF");
    });
#endif

    log::info("[FPS Booster] Mod loaded successfully.");
}
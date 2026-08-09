#pragma once

#include "configuration.h"

#ifdef MESHTASTIC_INCLUDE_NICHE_GRAPHICS

// ============================================================================
// InkHUD2 - New Architecture
// ============================================================================
#ifdef USE_INKHUD2

#include "graphics/niche/Drivers/EInk/GDEY0154D67.h"
#include "graphics/niche/InkHUD2/Setup.h"
#include "graphics/niche/Inputs/TwoButton.h"

void setupNicheGraphics()
{
    using namespace NicheGraphics;

    Serial.println(F("[NicheGfx] setupNicheGraphics() start (ThinkNode M1)"));

    // Initialize SPI for e-ink
    // Pins come from variant.h (PIN_SPI1_*)
    SPI1.begin();

    // Initialize e-ink driver
    // Same panel as T-Echo: GDEY0154D67, 1.54" 200x200
    Drivers::EInk* driver = new Drivers::GDEY0154D67;
    driver->begin(&SPI1, PIN_EINK_DC, PIN_EINK_CS, PIN_EINK_BUSY, PIN_EINK_RES);

    // Configure InkHUD2
    InkHUD2::Config config;

    // Front light: PIN_EINK_EN is the ON/OFF gate, the rotary knob sets BRIGHTNESS.
    //
    // Verified on hardware 2026-07-18. Two separate controls, don't confuse them:
    //   * PIN_EINK_EN  — powers the light. HIGH = lit, LOW = off.
    //   * rotary knob on the power switch — dims it, but does NOT reach full-off
    //     (at minimum the light still glows, so leaving the pin HIGH drains the battery).
    // Elecrow's own manual matches this split: the knob "adjusts the backlight LED
    // brightness level", while a Function-button double-click toggles the light on/off.
    //
    // So the light is driven automatically instead of costing a button: it comes on with
    // any input and goes out after a period of inactivity (like a phone screen). The knob
    // still sets how bright it is while lit, and nothing is left burning when the device
    // is idle in a pocket.
    config.backlightPin = PIN_EINK_EN;
    config.hasBacklight = true;
    config.backlightIdleOffMs = 30000; // dark after 30 s idle; any button wakes it

    // Buttons — BOTH are physical on the M1 (unlike T-Echo's capacitive aux).
    // NOTE: M1's variant.h defines no BUTTON_PIN, so TwoButton::getUserButtonPin()
    // would return 0xFF. Wire the main button explicitly.
    //   PIN_BUTTON2 = "Page Turn Button" (main)  -> short=select, long=back
    //   PIN_BUTTON1 = "Function Button"  (aux)   -> front light (hold/latch/off)
    // Observed on device: PIN_BUTTON2 is the TOP button (circle icon), PIN_BUTTON1 is
    // the BOTTOM one (triangle). This matches the Elecrow manual's naming, but if you
    // prefer the triangle as the main/select button, just swap these two lines.
    // ⚠️ Note PIN_BUTTON2 (P1.07) is also declared as PIN_SPI1_MISO in variant.h —
    // harmless in practice (the e-ink panel is write-only, MISO is never read).
    // Elecrow manual: https://www.elecrow.com/download/product/CIL12901M/ThinkNode-M1_User_Manual.pdf
    config.mainButtonPin = PIN_BUTTON2;
    config.mainButtonDebounce = 75;
    config.mainButtonLongPress = 400;

    config.auxButtonPin = PIN_BUTTON1;
    config.hasAuxButton = true;
    config.auxButtonDebounce = 50;
    config.auxButtonLongPress = 500; // physical button — no need for T-Echo's 5s touch workaround

    // Display rotation: 0. ✅ VERIFIED on hardware 2026-07-18.
    // Cross-checked two ways: (a) upstream's own M1 path uses setRotation(4) ≡ 0
    // (EInkDisplay2.cpp), (b) on-device test — with defaultRotation=3 the picture was
    // wrong and a +90 menu offset fixed it, i.e. (1 + 3) % 4 = 0 is the true value.
    // NOTE: the menu value is an OFFSET added to this default, persisted in
    // /prefs/inkhud2.dat. Anyone who hand-corrected rotation on an older build must
    // reset that offset back to 0, otherwise the correction gets applied twice.
    config.defaultRotation = 0;

    // Initialize InkHUD2
    InkHUD2::setup(driver, config);

    Serial.println(F("[NicheGfx] setupNicheGraphics() complete"));
}

// ============================================================================
// InkHUD (Original Architecture) — upstream
// ============================================================================
#else

// InkHUD-specific components
// ---------------------------
#include "graphics/niche/InkHUD/InkHUD.h"

// Applets
#include "graphics/niche/InkHUD/Applets/User/AllMessage/AllMessageApplet.h"
#include "graphics/niche/InkHUD/Applets/User/DM/DMApplet.h"
#include "graphics/niche/InkHUD/Applets/User/FavoritesMap/FavoritesMapApplet.h"
#include "graphics/niche/InkHUD/Applets/User/Heard/HeardApplet.h"
#include "graphics/niche/InkHUD/Applets/User/Positions/PositionsApplet.h"
#include "graphics/niche/InkHUD/Applets/User/RecentsList/RecentsListApplet.h"
#include "graphics/niche/InkHUD/Applets/User/ThreadedMessage/ThreadedMessageApplet.h"

// Shared NicheGraphics components
// --------------------------------
#include "graphics/niche/Drivers/Backlight/LatchingBacklight.h"
#include "graphics/niche/Drivers/EInk/GDEY0154D67.h"
#include "graphics/niche/Inputs/TwoButton.h"

// Button feedback
#include "buzz.h"

void setupNicheGraphics()
{
    using namespace NicheGraphics;

    // SPI
    // -----------------------------

    // For NRF52 platforms, SPI pins are defined in variant.h
    SPI1.begin();

    // E-Ink Driver
    // -----------------------------

    Drivers::EInk *driver = new Drivers::GDEY0154D67;
    driver->begin(&SPI1, PIN_EINK_DC, PIN_EINK_CS, PIN_EINK_BUSY, PIN_EINK_RES);

    // InkHUD
    // ----------------------------

    InkHUD::InkHUD *inkhud = InkHUD::InkHUD::getInstance();

    // Set the E-Ink driver
    inkhud->setDriver(driver);

    // Set how many FAST updates per FULL update
    // Set how unhealthy additional FAST updates beyond this number are
    // Todo: observe the display's performance in-person and adjust accordingly.
    // Currently set to the values given by Elecrow for EInkDynamicDisplay.
    inkhud->setDisplayResilience(10, 1.5);

    // Select fonts
    InkHUD::Applet::fontLarge = FREESANS_12PT_WIN1252;
    InkHUD::Applet::fontMedium = FREESANS_9PT_WIN1252;
    InkHUD::Applet::fontSmall = FREESANS_6PT_WIN1252;

    // Customize default settings
    inkhud->persistence->settings.userTiles.maxCount = 2;              // Two applets side-by-side
    inkhud->persistence->settings.optionalFeatures.batteryIcon = true; // Device definitely has a battery

    // Setup backlight controller
    // Note: button is attached further down
    Drivers::LatchingBacklight *backlight = Drivers::LatchingBacklight::getInstance();
    backlight->setPin(PIN_EINK_EN);

    // Pick applets
    // Note: order of applets determines priority of "auto-show" feature
    inkhud->addApplet("All Messages", new InkHUD::AllMessageApplet, true, true);      // Activated, autoshown
    inkhud->addApplet("DMs", new InkHUD::DMApplet);                                   // -
    inkhud->addApplet("Channel 0", new InkHUD::ThreadedMessageApplet(0));             // -
    inkhud->addApplet("Channel 1", new InkHUD::ThreadedMessageApplet(1));             // -
    inkhud->addApplet("Positions", new InkHUD::PositionsApplet, true);                // Activated
    inkhud->addApplet("Recents List", new InkHUD::RecentsListApplet);                 // -
    inkhud->addApplet("Heard", new InkHUD::HeardApplet, true, false, 0);              // Activated, no autoshow, default on tile 0
    inkhud->addApplet("Favorites Map", new InkHUD::FavoritesMapApplet, false, false); // -

    // Start running InkHUD
    inkhud->begin();

    // Buttons
    // --------------------------

    Inputs::TwoButton *buttons = Inputs::TwoButton::getInstance(); // Shared NicheGraphics component

    // Elecrow diagram: https://www.elecrow.com/download/product/CIL12901M/ThinkNode-M1_User_Manual.pdf

    // #0: Main User Button
    // Labeled "Page Turn Button" by manual
    buttons->setWiring(0, PIN_BUTTON2);
    buttons->setTiming(0, 50, 500); // Todo: confirm 50ms is adequate debounce
    buttons->setHandlerShortPress(0, [inkhud]() { inkhud->shortpress(); });
    buttons->setHandlerLongPress(0, [inkhud]() { inkhud->longpress(); });

    // #1: Aux Button
    // Labeled "Function Button" by manual
    // Todo: additional features
    buttons->setWiring(1, PIN_BUTTON1);
    buttons->setTiming(1, 50, 500); // 500ms before latch
    buttons->setHandlerDown(1, [backlight]() { backlight->peek(); });
    buttons->setHandlerLongPress(1, [backlight]() {
        backlight->latch();
        playBoop();
    });
    buttons->setHandlerShortPress(1, [backlight]() {
        backlight->off();
        playChirp();
    });

    // Begin handling button events
    buttons->start();
}

#endif // USE_INKHUD2

#endif // MESHTASTIC_INCLUDE_NICHE_GRAPHICS

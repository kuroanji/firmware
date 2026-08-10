#pragma once

#include "graphics/niche/Drivers/EInk/EInk.h"

namespace InkHUD2 {

/**
 * Device-specific configuration for InkHUD2
 * Set these values in nicheGraphics.h before calling setup()
 */
struct Config {
    // Backlight pin (-1 = no backlight)
    int8_t backlightPin = -1;

    // Button pins (-1 = not used)
    int8_t mainButtonPin = -1;      // Primary button (short=select, long=back)
    int8_t auxButtonPin = -1;       // Auxiliary button (e.g., touch for backlight)

    // Button timings (ms)
    uint16_t mainButtonDebounce = 75;
    uint16_t mainButtonLongPress = 400;
    uint16_t auxButtonDebounce = 50;
    uint16_t auxButtonLongPress = 5000;

    // Display rotation (0-3)
    uint8_t defaultRotation = 3;

    // Features
    bool hasBacklight = false;
    bool hasAuxButton = false;

    // Display maintenance: FULL refresh after idle to clear ghosting
    // Set to 0 to disable, or milliseconds of idle time before FULL refresh
    uint32_t idleFullRefreshMs = 0;  // Default: disabled

    // Backlight auto-off: turn the light off after this many ms without user input
    // (any input turns it back on). Intended for devices whose light has no hardware
    // off position, where leaving it lit would drain the battery. 0 = disabled.
    uint32_t backlightIdleOffMs = 0;  // Default: disabled
};

/**
 * Initialize InkHUD2 with the given e-ink driver and configuration.
 * This sets up all modules, menus, buttons, and events.
 *
 * @param driver Initialized e-ink driver
 * @param config Device-specific configuration
 */
void setup(NicheGraphics::Drivers::EInk* driver, const Config& config);

// Per-channel + DM alert toggles (index 0-7 = channels, 8 = DM). Read by Events when a message arrives.
extern bool alertsEnabled[9];

} // namespace InkHUD2

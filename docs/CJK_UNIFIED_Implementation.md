# CJK UNIFIED Mode Implementation for InkHUD

## Overview

This implementation adds full CJK (Chinese, Japanese, Korean) support to InkHUD using a unified bitmap font approach. All characters (ASCII, Cyrillic, Japanese) are rendered through a single CJK bitmap font, eliminating the need for legacy GFX fonts.

## Features

- **Full Joyo Kanji Support**: All 2136 Joyo kanji characters
- **Complete Kana**: Full Hiragana and Katakana blocks
- **Cyrillic**: Russian + Ukrainian alphabets (74 characters)
- **ASCII**: Full printable ASCII (95 characters)
- **Symbols**: Essential symbols including ♪, ♫, ★, ☆, ♥, arrows, etc.
- **Proportional Spacing**: Per-glyph xAdvance for optimal readability

## Architecture

### UNIFIED Mode

When `INKHUD_UNIFIED_ONLY` is defined, all text rendering uses the CJK bitmap font:

```cpp
// In platformio.ini
build_flags =
  -DINKHUD_UNIFIED_ONLY
```

This mode:
- Excludes legacy FreeSans GFX fonts (saves flash space)
- Routes ALL characters through the bitmap renderer
- Uses escape sequences for UTF-8 encoding

### Font Structure

```cpp
struct CJKFont {
    const uint8_t *bitmap;      // Packed 1-bit glyph data
    const CJKGlyph *glyphs;     // Glyph lookup table
    uint16_t glyphCount;        // Number of glyphs
    uint8_t width;              // Native glyph width (18px)
    uint8_t height;             // Native glyph height (18px)
    uint8_t xAdvance;           // Default advance width
    int8_t yOffset;             // Baseline offset
};

struct CJKGlyph {
    uint32_t codepoint;         // Unicode codepoint
    uint32_t bitmapOffset;      // Offset into bitmap array
    uint8_t xAdvance;           // Per-glyph advance width
};
```

### Text Encoding

UTF-8 characters are encoded as 3-byte escape sequences:
```
ESC (0x1B) + (high_byte + 1) + (low_byte + 1)
```

The +1 offset avoids null bytes that would terminate C strings.

## Font Generation

### Prerequisites

```bash
pip install pillow
```

### Fonts Used

- **JetBrains Mono NL ExtraLight**: Latin, Cyrillic characters (monospace, clean)
- **Noto Sans JP Regular**: Japanese characters, CJK symbols

### Running the Generator

```bash
python3 generate_kanaka_font.py
```

### Configuration

Edit `generate_kanaka_font.py` to customize:

```python
CELL_SIZE = 18          # Output glyph size in pixels
RENDER_SIZE = 25        # Render size (higher = better quality)

# Character width overrides for proportional spacing
CHAR_WIDTH_OVERRIDES = {
    '.': 0.40,   # Narrow punctuation
    ' ': 0.45,   # Space
    '0': 0.55,   # Digits
    # ...
}

# Codepoint aliases (map new forms to old)
CODEPOINT_ALIASES = {
    0x20B9F: 0x53F1,  # 𠮟 -> 叱
}
```

### Character Sets Included

| Category | Range | Count |
|----------|-------|-------|
| ASCII | U+0020-U+007E | 95 |
| Cyrillic | U+0400-U+04FF (subset) | 74 |
| Symbols | Hand-picked | ~30 |
| CJK Punctuation | U+3000-U+303F | 64 |
| Hiragana | U+3040-U+309F | 96 |
| Katakana | U+30A0-U+30FF | 96 |
| Joyo Kanji | From file | 2136 |

## Files Modified

### Core Files

- `src/graphics/niche/InkHUD/Applet.cpp` - Text rendering with UNIFIED mode
- `src/graphics/niche/InkHUD/Applet.h` - UNIFIED mode declarations
- `src/graphics/niche/InkHUD/AppletFont.cpp` - UTF-8 decoding, UNIFIED constructor
- `src/graphics/niche/InkHUD/AppletFont.h` - Encoding enum, CJK font pointer
- `src/graphics/niche/InkHUD/WindowManager.cpp` - Font initialization

### Font Files

- `src/graphics/niche/Fonts/CJK/CJKFont.h` - Font structures and lookup function
- `src/graphics/niche/Fonts/CJK/UnifiedFont18px.h` - Generated font data (~135KB)

### Platform Config

- `variants/nrf52840/t-echo-plus/platformio.ini` - Build flags, lib_ignore
- `variants/nrf52840/t-echo-plus/nicheGraphics.h` - Font macros

## Build Configuration

### platformio.ini

```ini
[env:t-echo-plus-inkhud]
build_flags =
  ${nrf52840_base.build_flags}
  ${inkhud.build_flags}
  -DINKHUD_UNIFIED_ONLY    ; Use only CJK bitmap font

lib_ignore =
  Adafruit GFX Library     ; Not needed with GFX_Root
  GxEPD2                   ; InkHUD has own e-ink drivers
```

### Flash Usage

- Without UNIFIED: ~700KB
- With UNIFIED (2136 kanji): ~787KB
- Flash capacity: 815KB (96.5% used)

## Text Alignment

### Horizontal Alignment

All alignments compensate for left padding in glyph rendering (~1.44px at native size):

```cpp
// Padding = 2px at render_size(25), scaled to cell_size(18) = 2*18/25 = 1.44px
int16_t paddingCompensation = 0;
if (currentFont.gfxFont == nullptr && currentFont.cjkFont != nullptr) {
    paddingCompensation = (int16_t)(1.44f * currentFont.cjkScale + 0.5f);
}

switch (ha) {
case LEFT:
    cursorX = x - textOffsetX - paddingCompensation;
    break;
case CENTER:
    cursorX = (x - textOffsetX) - (textWidth / 2) - paddingCompensation;
    break;
case RIGHT:
    cursorX = (x - textOffsetX) - textWidth - paddingCompensation;
    break;
}
```

### MIDDLE Alignment

Uses actual glyph height for vertical centering:

```cpp
case MIDDLE:
    if (currentFont.gfxFont == nullptr && currentFont.cjkFont != nullptr) {
        uint8_t glyphHeight = (uint8_t)(currentFont.cjkFont->height * currentFont.cjkScale + 0.5f);
        cursorY = y + (glyphHeight / 2);
    }
    break;
```

## Map Applet Enhancements

### Safe Area

Markers are clipped to avoid overlapping UI elements:

```cpp
constexpr int16_t marginTop = 22;     // Battery icon, notifications
constexpr int16_t marginBottom = 18;  // Scale bar + label
constexpr int16_t marginLeft = 10;    // Vertical scale bar
constexpr int16_t marginRight = 10;   // Uniform padding
```

### Scale Bar Label

Combined format: `H:3.2 km | V:3.2 km`

## Troubleshooting

### Character Not Rendering

1. Check if codepoint is in `get_all_codepoints()`
2. Verify font selection in `get_font_for_codepoint()`
3. Check glyph exists: `grep "0xXXXX" UnifiedFont18px.h`

### Flash Overflow

Reduce character set:
- Remove Katakana Phonetic Extensions
- Limit Joyo kanji to top 1500-2000
- Remove unused symbol ranges

### Alignment Issues

- CENTER: Check `paddingCompensation` calculation
- MIDDLE: Verify `glyphHeight` calculation for bitmap fonts

## License

Same as Meshtastic firmware (GPL-3.0)

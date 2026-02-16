#ifdef MESHTASTIC_INCLUDE_INKHUD

/*

    Wrapper class for an AdafruitGFX font
    Pre-calculates some font dimension info which InkHUD uses repeatedly
    Re-encodes UTF-8 characters to suit extended ASCII AdafruitGFX fonts

*/

#pragma once

#include "configuration.h"

#include <GFX.h> // GFXRoot drawing lib
#include "graphics/niche/Fonts/CJK/CJKFont.h"

namespace NicheGraphics::InkHUD
{

// An AdafruitGFX font, bundled with precalculated dimensions which are used frequently by InkHUD
class AppletFont
{
  public:
    enum Encoding {
        ASCII,
        WINDOWS_1250,
        WINDOWS_1251,
        WINDOWS_1252,
        WINDOWS_1253,
        JAPANESE,
    };

    AppletFont();
    AppletFont(const GFXfont &adafruitGFXFont, Encoding encoding = ASCII, int8_t paddingTop = 0, int8_t paddingBottom = 0);
    AppletFont(const GFXfont &adafruitGFXFont, Encoding encoding, int8_t paddingTop, int8_t paddingBottom,
               const NicheGraphics::CJKFont *cjkFont, float cjkScale = 1.0f);

    uint8_t lineHeight();
    uint8_t heightAboveCursor();
    uint8_t heightBelowCursor();
    uint8_t widthBetweenWords(); // Width of the space character

    std::string decodeUTF8(std::string encoded);

    const GFXfont *gfxFont = NULL; // Default value: in-built AdafruitGFX font
    const NicheGraphics::CJKFont *cjkFont = nullptr;
    float cjkScale = 1.0f;

  private:
    uint32_t toUtf32(std::string utf8);
    char applyEncoding(std::string utf8);

    uint8_t height = 8;          // Default value: in-built AdafruitGFX font
    uint8_t ascenderHeight = 0;  // Default value: in-built AdafruitGFX font
    uint8_t descenderHeight = 8; // Default value: in-built AdafruitGFX font
    uint8_t spaceCharWidth = 8;  // Default value: in-built AdafruitGFX font

    Encoding encoding = ASCII;
};

} // namespace NicheGraphics::InkHUD

// Macros for InkHUD's standard fonts
// --------------------------------------

// Use these once only, passing them to InkHUD::Applet::fontLarge and InkHUD::Applet:fontSmall
// Line padding has been adjusted manually, to compensate for a few *extra tall* diacritics

// Central European
#include "graphics/niche/Fonts/FreeSans12pt_Win1250.h"
#include "graphics/niche/Fonts/FreeSans6pt_Win1250.h"
#include "graphics/niche/Fonts/FreeSans9pt_Win1250.h"
#define FREESANS_12PT_WIN1250 InkHUD::AppletFont(FreeSans12pt_Win1250, InkHUD::AppletFont::WINDOWS_1250, -3, 1)
#define FREESANS_9PT_WIN1250 InkHUD::AppletFont(FreeSans9pt_Win1250, InkHUD::AppletFont::WINDOWS_1250, -1, -1)
#define FREESANS_6PT_WIN1250 InkHUD::AppletFont(FreeSans6pt_Win1250, InkHUD::AppletFont::WINDOWS_1250, -1, -2)

// Cyrillic
#include "graphics/niche/Fonts/FreeSans12pt_Win1251.h"
#include "graphics/niche/Fonts/FreeSans6pt_Win1251.h"
#include "graphics/niche/Fonts/FreeSans9pt_Win1251.h"
#define FREESANS_12PT_WIN1251 InkHUD::AppletFont(FreeSans12pt_Win1251, InkHUD::AppletFont::WINDOWS_1251, -3, 1)
#define FREESANS_9PT_WIN1251 InkHUD::AppletFont(FreeSans9pt_Win1251, InkHUD::AppletFont::WINDOWS_1251, -2, -1)
#define FREESANS_6PT_WIN1251 InkHUD::AppletFont(FreeSans6pt_Win1251, InkHUD::AppletFont::WINDOWS_1251, -1, -2)

// Western European
#include "graphics/niche/Fonts/FreeSans12pt_Win1252.h"
#include "graphics/niche/Fonts/FreeSans6pt_Win1252.h"
#include "graphics/niche/Fonts/FreeSans9pt_Win1252.h"
#define FREESANS_12PT_WIN1252 InkHUD::AppletFont(FreeSans12pt_Win1252, InkHUD::AppletFont::WINDOWS_1252, -3, 1)
#define FREESANS_9PT_WIN1252 InkHUD::AppletFont(FreeSans9pt_Win1252, InkHUD::AppletFont::WINDOWS_1252, -2, -1)
#define FREESANS_6PT_WIN1252 InkHUD::AppletFont(FreeSans6pt_Win1252, InkHUD::AppletFont::WINDOWS_1252, -1, -2)

// Greek
#include "graphics/niche/Fonts/FreeSans12pt_Win1253.h"
#include "graphics/niche/Fonts/FreeSans6pt_Win1253.h"
#include "graphics/niche/Fonts/FreeSans9pt_Win1253.h"
#define FREESANS_12PT_WIN1253 InkHUD::AppletFont(FreeSans12pt_Win1253, InkHUD::AppletFont::WINDOWS_1253, -3, 1)
#define FREESANS_9PT_WIN1253 InkHUD::AppletFont(FreeSans9pt_Win1253, InkHUD::AppletFont::WINDOWS_1253, -2, -1)
#define FREESANS_6PT_WIN1253 InkHUD::AppletFont(FreeSans6pt_Win1253, InkHUD::AppletFont::WINDOWS_1253, -1, -2)

// Japanese (CJK) - uses Win1251 base font with CJK bitmap overlay
#include "graphics/niche/Fonts/CJK/CJKFont_DotGothic18px.h"
#define FREESANS_12PT_JP InkHUD::AppletFont(FreeSans12pt_Win1251, InkHUD::AppletFont::JAPANESE, -3, 1, &CJKFontDotGothic18px, 1.0f)
#define FREESANS_9PT_JP InkHUD::AppletFont(FreeSans9pt_Win1251, InkHUD::AppletFont::JAPANESE, -2, -1, &CJKFontDotGothic18px, 0.72f)
#define FREESANS_6PT_JP InkHUD::AppletFont(FreeSans6pt_Win1251, InkHUD::AppletFont::JAPANESE, -1, -2, &CJKFontDotGothic18px, 0.67f)

#endif

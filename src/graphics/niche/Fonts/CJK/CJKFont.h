#pragma once

#include <stdint.h>
#include <Arduino.h>

namespace NicheGraphics
{

// One glyph entry in a CJK font — sorted by codepoint for binary search
struct CJKGlyph {
    uint16_t codepoint;    // Unicode codepoint
    uint32_t bitmapOffset; // Byte offset into bitmap data
};

// Fixed-width CJK bitmap font stored in PROGMEM
struct CJKFont {
    const uint8_t *bitmap;    // Glyph bitmap data (packed, 1 bit per pixel, row-major)
    const CJKGlyph *glyphs;   // Glyph table sorted by codepoint
    uint16_t glyphCount;       // Number of glyphs in table
    uint8_t width;             // Fixed glyph width in pixels
    uint8_t height;            // Fixed glyph height in pixels
    uint8_t xAdvance;          // Horizontal cursor advance (typically = width + 1)
    int8_t yOffset;            // Y offset from baseline (negative = above)
};

// Huffman-compressed glyph entry — stores bit offset and length
struct CJKGlyphHuff {
    uint16_t codepoint;    // Unicode codepoint
    uint32_t bitOffset;    // Bit offset into compressed bitmap data
    uint16_t bitLength;    // Length in bits
};

// Huffman-compressed CJK bitmap font
struct CJKFontHuff {
    const uint8_t *bitmap;       // Huffman-compressed bitmap data
    const CJKGlyphHuff *glyphs;  // Glyph table sorted by codepoint
    const uint8_t *huffSymbols;  // Symbol table: 256 × (byte_val, code_len, code_hi, code_lo)
    uint16_t glyphCount;         // Number of glyphs
    uint8_t width;               // Fixed glyph width in pixels
    uint8_t height;              // Fixed glyph height in pixels
    uint8_t xAdvance;            // Horizontal cursor advance
    int8_t yOffset;              // Y offset from baseline
    uint8_t maxCodeLen;          // Maximum Huffman code length in bits
};

// Binary search for Huffman font
inline int16_t cjkLookupHuff(const CJKFontHuff *font, uint16_t codepoint)
{
    if (!font || !font->glyphs || font->glyphCount == 0)
        return -1;

    int16_t lo = 0;
    int16_t hi = font->glyphCount - 1;

    while (lo <= hi) {
        int16_t mid = lo + (hi - lo) / 2;
        uint16_t midCp = pgm_read_word(&font->glyphs[mid].codepoint);

        if (midCp == codepoint)
            return mid;
        else if (midCp < codepoint)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    return -1;
}

// Binary search for a Unicode codepoint in a CJK font
// Returns glyph index if found, -1 if not found
inline int16_t cjkLookup(const CJKFont *font, uint16_t codepoint)
{
    if (!font || !font->glyphs || font->glyphCount == 0)
        return -1;

    int16_t lo = 0;
    int16_t hi = font->glyphCount - 1;

    while (lo <= hi) {
        int16_t mid = lo + (hi - lo) / 2;
        uint16_t midCp = pgm_read_word(&font->glyphs[mid].codepoint);

        if (midCp == codepoint)
            return mid;
        else if (midCp < codepoint)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    return -1;
}

} // namespace NicheGraphics

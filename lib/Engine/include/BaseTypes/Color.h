//
// Created by James Miller on 2/3/2026.
//

#pragma once
#include <cstdlib>

#include "MacroDefs.h"

#pragma pack(push, 1)

typedef struct Color32
{
    uint8 r, g, b, a;
} Color32;

typedef struct Color64
{
    uint16 r, g, b, a;
} Color64;

typedef struct Color128
{
    uint32 r, g, b, a;
} Color128;

typedef struct Color24
{
    uint8 r, g, b;
} Color24;

typedef struct Color48
{
    uint16 r, g, b;
} Color48;

typedef struct ColorR8
{
    uint8 r;
} ColorR8;

typedef struct ColorR16
{
    uint16 r;
} ColorR16;

typedef struct FColorR32
{
    float32 r;
} FColorR32;

typedef struct FColor
{
    float32 r, g, b, a;
} FColor;

#pragma pack(pop)

namespace RGBA
{
    static constexpr Color32 Black = {0, 0, 0, 255};
    static constexpr Color32 White = {255, 255, 255, 255};
    static constexpr Color32 Red = {255, 0, 0, 255};
    static constexpr Color32 Green = {0, 255, 0, 255};
    static constexpr Color32 Blue = {0, 0, 255, 255};
    static constexpr Color32 Yellow = {255, 255, 0, 255};
    static constexpr Color32 Cyan = {0, 255, 255, 255};
    static constexpr Color32 Magenta = {255, 0, 255, 255};
    static constexpr Color32 Grey = {128, 128, 128, 255};
}

namespace RGBA64
{
    static constexpr Color64 Black = {0, 0, 0, 65535};
    static constexpr Color64 White = {65535, 65535, 65535, 65535};
    static constexpr Color64 Red = {65535, 0, 0, 65535};
    static constexpr Color64 Green = {0, 65535, 0, 65535};
    static constexpr Color64 Blue = {0, 0, 65535, 65535};
    static constexpr Color64 Yellow = {65535, 65535, 0, 65535};
    static constexpr Color64 Cyan = {0, 65535, 65535, 65535};
    static constexpr Color64 Magenta = {65535, 0, 65535, 65535};
    static constexpr Color64 Grey = {32768, 32768, 32768, 65535};
}

namespace fRGBA
{
    static constexpr FColor Black = {0.0f, 0.0f, 0.0f, 1.0f};
    static constexpr FColor White = {1.0f, 1.0f, 1.0f, 1.0f};
    static constexpr FColor Red = {1.0f, 0.0f, 0.0f, 1.0f};
    static constexpr FColor Green = {0.0f, 1.0f, 0.0f, 1.0f};
    static constexpr FColor Blue = {0.0f, 0.0f, 1.0f, 1.0f};
    static constexpr FColor Yellow = {1.0f, 1.0f, 0.0f, 1.0f};
    static constexpr FColor Cyan = {0.0f, 1.0f, 1.0f, 1.0f};
    static constexpr FColor Magenta = {1.0f, 0.0f, 1.0f, 1.0f};
    static constexpr FColor Grey = {0.5f, 0.5f, 0.5f, 1.0f};
}

typedef union uColor
{
    Color32 color32{};
    Color64 color64;
    Color128 color128;
    Color24 color24;
    Color48 color48;
    ColorR8 colorR8;
    ColorR16 colorR16;
    FColorR32 fColorR32;
    FColor fColor;
} uColor;

enum class EColorFormat
{
    RGBA,
    RGBA64,
    RGBA128,
    RGB24,
    RGB48,
    R8,
    R16,
    fR32,
    fRGBA
};

typedef struct Color
{
private:
    uColor Value;
    EColorFormat Format = EColorFormat::RGBA;

public:
    Color() = default;
    Color(uint8 _r, uint8 _g, uint8 _b, uint8 _a);
    explicit Color(Color32 color);
    explicit Color(FColor color);
    explicit Color(Color64 color);
    Color(const Color& other);
    ~Color() = default;

    Color32 RGBA() const;
    Color64 RGBA64() const;
    FColor fRGBA() const;

    void SetColor(const Color32& color);
    void SetColor(const Color64& color);
    void SetColor(const Color128& color);
    void SetColor(const Color24& color);
    void SetColor(const Color48& color);
    void SetColor(const ColorR8& color);
    void SetColor(const ColorR16& color);
    void SetColor(const FColorR32& color);
    void SetColor(const FColor& color);
} Color;

namespace Colors
{
    static const auto Grey = Color{ RGBA::Grey };
    static const auto Black = Color{ RGBA::Black };
    static const auto White = Color{ RGBA::White };
    static const auto Red = Color{ RGBA::Red };
    static const auto Green = Color{ RGBA::Green };
    static const auto Blue = Color{ RGBA::Blue };
    static const auto Yellow = Color{ RGBA::Yellow };
    static const auto Cyan = Color{ RGBA::Cyan };
    static const auto Magenta = Color{ RGBA::Magenta };
    static const auto Grey64 = Color{ RGBA64::Grey };
    static const auto Black64 = Color{ RGBA64::Black };
    static const auto White64 = Color{ RGBA64::White };
    static const auto Red64 = Color{ RGBA64::Red };
    static const auto Green64 = Color{ RGBA64::Green };
    static const auto Blue64 = Color{ RGBA64::Blue };
    static const auto Yellow64 = Color{ RGBA64::Yellow };
    static const auto Cyan64 = Color{ RGBA64::Cyan };
    static const auto Magenta64 = Color{ RGBA64::Magenta };
    static const auto Greyf = Color{ fRGBA::Grey };
    static const auto Blackf = Color{ fRGBA::Black };
    static const auto Whitef = Color{ fRGBA::White };
    static const auto Redf = Color{ fRGBA::Red };
    static const auto Greenf = Color{ fRGBA::Green };
    static const auto Bluef = Color{ fRGBA::Blue };
    static const auto Yellowf = Color{ fRGBA::Yellow };
    static const auto Cyanf = Color{ fRGBA::Cyan };
    static const auto Magentaf = Color{ fRGBA::Magenta };
    static auto RandomColor()
    {
        uint8 r = static_cast<uint8>(rand() % 256);
        uint8 g = static_cast<uint8>(rand() % 256);
        uint8 b = static_cast<uint8>(rand() % 256);
        return Color{ r, g, b, 255 };
    }
}
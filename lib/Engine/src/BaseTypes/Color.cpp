//
// Created by James Miller on 2/3/2026.
//

#include "MacroDefs.h"
#include "BaseTypes/Color.h"

Color32 ConvertToRGBA(const ColorR8& colorR8)
{
    uint8 r = colorR8.r;
    return {r, r, r, 255}; // Grayscale with full opacity
}

Color32 ConvertToRGB(const ColorR16& colorR16)
{
    uint8 r = static_cast<uint8>(colorR16.r >> 8); // Convert 16-bit to 8-bit
    return {r, r, r, 255}; // Grayscale with full opacity
}

Color32 ConvertToRGBA(const Color48& color48)
{
    uint8 r = static_cast<uint8>(color48.r >> 8); // Convert 16-bit to 8-bit
    uint8 g = static_cast<uint8>(color48.g >> 8);
    uint8 b = static_cast<uint8>(color48.b >> 8);
    return {r, g, b, 255}; // Full opacity
}

Color32 ConvertToRGBA(const FColorR32& fColorR32)
{
    uint8 r = static_cast<uint8>(fColorR32.r * 255.0f); // Convert float to 8-bit
    return {r, r, r, 255}; // Grayscale with full opacity
}

Color32 ConvertToRGBA(const FColor& fColor)
{
    uint8 r = static_cast<uint8>(fColor.r * 255.0f);
    uint8 g = static_cast<uint8>(fColor.g * 255.0f);
    uint8 b = static_cast<uint8>(fColor.b * 255.0f);
    uint8 a = static_cast<uint8>(fColor.a * 255.0f);
    return {r, g, b, a};
}

Color32 ConvertToRGBA(const Color24& color24)
{
    return {color24.r, color24.g, color24.b, 255}; // Full opacity
}

Color32 ConvertToRGBA(const Color128& color128)
{
    uint8 r = static_cast<uint8>(color128.r >> 24); // Convert 32-bit to 8-bit
    uint8 g = static_cast<uint8>(color128.g >> 24);
    uint8 b = static_cast<uint8>(color128.b >> 24);
    uint8 a = static_cast<uint8>(color128.a >> 24);
    return {r, g, b, a};
}

Color32 ConvertToRGBA(const Color64& color64)
{
    uint8 r = static_cast<uint8>(color64.r >> 8); // Convert 16-bit to 8-bit
    uint8 g = static_cast<uint8>(color64.g >> 8);
    uint8 b = static_cast<uint8>(color64.b >> 8);
    uint8 a = static_cast<uint8>(color64.a >> 8);
    return {r, g, b, a};
}

Color32 ConvertToRGB(const Color48& color48)
{
    uint8 r = static_cast<uint8>(color48.r);
    uint8 g = static_cast<uint8>(color48.g);
    uint8 b = static_cast<uint8>(color48.b);
    return {r, g, b, 255}; // Full opacity
}

Color64 ConvertToRGBA64(const ColorR8& colorR8)
{
    uint16 r = static_cast<uint16>(colorR8.r) << 8; // Convert 8-bit to 16-bit
    return {r, r, r, 65535}; // Grayscale with full opacity
}

Color64 ConvertToRGBA64(const Color32& color32)
{
    uint16 r = static_cast<uint16>(color32.r) << 8; // Convert 8-bit to 16-bit
    uint16 g = static_cast<uint16>(color32.g) << 8;
    uint16 b = static_cast<uint16>(color32.b) << 8;
    uint16 a = static_cast<uint16>(color32.a) << 8;
    return {r, g, b, a};
}

Color64 ConvertToRGBA64(const Color128& color128)
{
    uint16 r = static_cast<uint16>(color128.r) << 8;
    uint16 g = static_cast<uint16>(color128.g) << 8;
    uint16 b = static_cast<uint16>(color128.b) << 8;
    uint16 a = static_cast<uint16>(color128.a) << 8;
    return {r, g, b, a};
}

Color64 ConvertToRGBA64(const Color24& color24)
{
    uint16 r = static_cast<uint16>(color24.r) << 8; // Convert 8-bit to 16-bit
    uint16 g = static_cast<uint16>(color24.g) << 8;
    uint16 b = static_cast<uint16>(color24.b) << 8;
    return {r, g, b, 65535}; // Full opacity
}

Color64 ConvertToRGBA64(const ColorR16& colorR16)
{
    uint16 r = colorR16.r;
    return {r, r, r, 65535}; // Grayscale with full opacity
}

Color64 ConvertToRGBA64(const Color48& color48)
{
    return {color48.r, color48.g, color48.b, 255}; // Full opacity
}

Color64 ConvertToRGBA64(const FColorR32& fColorR32)
{
    uint16 r = static_cast<uint16>(fColorR32.r * 255.0f);
    return {r, r, r, 65535}; // Grayscale with full opacity
}

Color64 ConvertToRGBA64(const FColor& fColor)
{
    uint16 r = static_cast<uint16>(fColor.r * 255.0f);
    uint16 g = static_cast<uint16>(fColor.g * 255.0f);
    uint16 b = static_cast<uint16>(fColor.b * 255.0f);
    uint16 a = static_cast<uint16>(fColor.a * 255.0f);
    return {r, g, b, a};
}

FColor ConvertTofRGBA(const Color32& color32)
{
    float32 r = static_cast<float32>(color32.r) / 255.0f;
    float32 g = static_cast<float32>(color32.g) / 255.0f;
    float32 b = static_cast<float32>(color32.b) / 255.0f;
    float32 a = static_cast<float32>(color32.a) / 255.0f;
    return {r, g, b, a};
}

FColor ConvertTofRGBA(const Color64& color64)
{
    float32 r = static_cast<float32>(color64.r) / 65535.0f;
    float32 g = static_cast<float32>(color64.g) / 65535.0f;
    float32 b = static_cast<float32>(color64.b) / 65535.0f;
    float32 a = static_cast<float32>(color64.a) / 65535.0f;
    return {r, g, b, a};
}

FColor ConvertTofRGBA(const Color128& color128)
{
    float32 r = static_cast<float32>(color128.r) / 4294967295.0f;
    float32 g = static_cast<float32>(color128.g) / 4294967295.0f;
    float32 b = static_cast<float32>(color128.b) / 4294967295.0f;
    float32 a = static_cast<float32>(color128.a) / 4294967295.0f;
    return {r, g, b, a};
}

FColor ConvertTofRGBA(const Color24& color24)
{
    float32 r = static_cast<float32>(color24.r) / 255.0f;
    float32 g = static_cast<float32>(color24.g) / 255.0f;
    float32 b = static_cast<float32>(color24.b) / 255.0f;
    return {r, g, b, 1.0f};
}

FColor CovnertTofRGBA(const Color48& color48)
{
    float32 r = static_cast<float32>(color48.r) / 255.0f;
    float32 g = static_cast<float32>(color48.g) / 255.0f;
    float32 b = static_cast<float32>(color48.b) / 255.0f;
    return {r, g, b, 1.0f};
}

FColor ConvertTofRGBA(const ColorR8& colorR8)
{
    float32 r = static_cast<float32>(colorR8.r) / 255.0f;
    return {r, r, r, 1.0f};
}

FColor ConvertTofRGBA(const ColorR16& colorR16)
{
    float32 r = static_cast<float32>(colorR16.r) / 65535.0f;
    return {r, r, r, 1.0f};
}

FColor ConvertTofRGBA(const FColorR32& fColorR32)
{
    const auto r = fColorR32.r;
    return {r, r, r, 1.0f};
}

Color::Color(uint8 _r, uint8 _g, uint8 _b, uint8 _a)
{
    SetColor(Color32(_r, _g, _b, _a));
}

Color::Color(Color32 color)
{
    SetColor(color);
}

Color::Color(FColor color)
{
    SetColor(color);
}

Color::Color(Color64 color)
{
    SetColor(color);
}

Color::Color(const Color& other)
{
    Format = other.Format;
    Value = other.Value;
}

Color32 Color::RGBA() const
{
    switch (Format)
    {
    case EColorFormat::RGBA:
        return Value.color32;
    case EColorFormat::R8:
        return ConvertToRGBA(Value.colorR8);
    case EColorFormat::RGB24:
        return ConvertToRGBA(Value.color24);
    case EColorFormat::RGBA64:
        return ConvertToRGBA(Value.color64);
    case EColorFormat::RGBA128:
        return ConvertToRGBA(Value.color128);
    case EColorFormat::RGB48:
        return ConvertToRGBA(Value.color48);
    case EColorFormat::R16:
        return ConvertToRGB(Value.colorR16);
    case EColorFormat::fR32:
        return ConvertToRGBA(Value.fColorR32);
    case EColorFormat::fRGBA:
        return ConvertToRGBA(Value.fColor);
    default:
        return {0, 0, 0, 255}; // Default to opaque black for unsupported formats
    }
}

Color64 Color::RGBA64() const
{
    switch (Format)
    {
        case EColorFormat::RGBA:
            return ConvertToRGBA64(Value.color32);
        case EColorFormat::R8:
            return ConvertToRGBA64(Value.colorR8);
        case EColorFormat::RGB24:
            return ConvertToRGBA64(Value.color24);
        case EColorFormat::RGBA64:
            return Value.color64;
        case EColorFormat::RGBA128:
            return ConvertToRGBA64(Value.color128);
        case EColorFormat::RGB48:
            return ConvertToRGBA64(Value.color48);
        case EColorFormat::R16:
            return ConvertToRGBA64(Value.colorR16);
        case EColorFormat::fR32:
            return ConvertToRGBA64(Value.fColorR32);
        case EColorFormat::fRGBA:
            return ConvertToRGBA64(Value.fColor);
        default:
            return {0, 0, 0, 65535}; // Default to opaque black for unsupported formats
    }
}

FColor Color::fRGBA() const
{
    switch (Format)
    {
        case EColorFormat::RGBA:
            return ConvertTofRGBA(Value.color32);
        case EColorFormat::R8:
            return ConvertTofRGBA(Value.colorR8);
        case EColorFormat::RGB24:
            return ConvertTofRGBA(Value.color24);
        case EColorFormat::RGBA64:
            return ConvertTofRGBA(Value.color64);
        case EColorFormat::RGBA128:
            return ConvertTofRGBA(Value.color128);
        case EColorFormat::RGB48:
            return CovnertTofRGBA(Value.color48);
        case EColorFormat::R16:
            return ConvertTofRGBA(Value.colorR16);
        case EColorFormat::fR32:
            return ConvertTofRGBA(Value.fColorR32);
        case EColorFormat::fRGBA:
            return Value.fColor;
        default:
            return {0.0f, 0.0f, 0.0f, 1.0f}; // Default to opaque black for unsupported formats
    }
}

void Color::SetColor(const Color32& color)
{
    Value.color32 = color;
    Format = EColorFormat::RGBA;
}

void Color::SetColor(const Color64& color)
{
    Value.color64 = color;
    Format = EColorFormat::RGBA64;
}

void Color::SetColor(const Color128& color)
{
    Value.color128 = color;
    Format = EColorFormat::RGBA128;
}

void Color::SetColor(const Color24& color)
{
    Value.color24 = color;
    Format = EColorFormat::RGB24;
}

void Color::SetColor(const Color48& color)
{
    Value.color48 = color;
    Format = EColorFormat::RGB48;
}

void Color::SetColor(const ColorR8& color)
{
    Value.colorR8 = color;
    Format = EColorFormat::R8;
}

void Color::SetColor(const ColorR16& color)
{
    Value.colorR16 = color;
    Format = EColorFormat::R16;
}

void Color::SetColor(const FColorR32& color)
{
    Value.fColorR32 = color;
    Format = EColorFormat::fR32;
}

void Color::SetColor(const FColor& color)
{
    Value.fColor = color;
    Format = EColorFormat::fRGBA;
}

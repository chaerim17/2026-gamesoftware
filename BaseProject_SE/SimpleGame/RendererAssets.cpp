#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Renderer.h"
#include "GameStorage.h"
#include <windows.h>
#include <unordered_map>
#include <cmath>
#include <iostream>

namespace
{
    std::wstring Wide(const std::string& text)
    {
        if (text.empty())
            return {};

        int count = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);

        if (count <= 0)
            return L"?";

        std::wstring result(count, L' ');
        MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), &result[0], count);

        return result;
    }
}

struct Renderer::FontCache
{
    struct Glyph
    {
        int x, y;
        float advance;
    };

    HDC dc = nullptr;
    HFONT face = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ oldFace = nullptr, oldBitmap = nullptr;
    void* pixels = nullptr;
    std::unordered_map<wchar_t, Glyph> glyphs;
    static constexpr int Cell = 48, Size = 2048, Columns = Size / Cell;

    Glyph Get(wchar_t ch, GLuint texture)
    {
        auto found = glyphs.find(ch);

        if (found != glyphs.end())
            return found->second;

        // The prototype uses fewer than 500 glyphs; retain a visible fallback if expanded.
        if (glyphs.size() >= Columns * Columns)
        {
            auto q = glyphs.find(L'?');

            return q == glyphs.end() ? Glyph{0, 0, 24} : q->second;
        }

        PatBlt(dc, 0, 0, Cell, Cell, BLACKNESS);
        TextOutW(dc, 2, 0, &ch, 1);
        GdiFlush();
        SIZE extent = {};
        GetTextExtentPoint32W(dc, &ch, 1, &extent);
        int index = static_cast<int>(glyphs.size());
        Glyph g = {(index % Columns) * Cell, (index / Columns) * Cell, static_cast<float>(extent.cx)};
        unsigned char mask[Cell * Cell];
        const unsigned char* bgra = static_cast<const unsigned char*>(pixels);

        for (int i = 0; i < Cell * Cell; ++i)
            mask[i] = bgra[i * 4 + 1];
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, g.x, g.y, Cell, Cell, GL_RED, GL_UNSIGNED_BYTE, mask);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glActiveTexture(GL_TEXTURE0);
        glyphs.emplace(ch, g);

        return g;
    }
};

bool Renderer::CreateAssets()
{
    font = new FontCache();
    font->dc = CreateCompatibleDC(nullptr);

    if (!font->dc)
        return false;

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = FontCache::Cell;
    info.bmiHeader.biHeight = -FontCache::Cell;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    font->bitmap = CreateDIBSection(font->dc, &info, DIB_RGB_COLORS, &font->pixels, nullptr, 0);
    font->face = CreateFontW(-32,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH,
        L"Malgun Gothic");

    if (!font->bitmap || !font->face || !font->pixels)
    {
        std::cerr << "한글 글꼴 생성 실패\n";

        return false;
    }

    font->oldBitmap = SelectObject(font->dc, font->bitmap);
    font->oldFace = SelectObject(font->dc, font->face);
    SetBkMode(font->dc, TRANSPARENT);
    SetTextColor(font->dc, RGB(255, 255, 255));
    glGenTextures(1, &fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    std::vector<unsigned char> empty(FontCache::Size * FontCache::Size, 0);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_R8, FontCache::Size, FontCache::Size, 0, GL_RED, GL_UNSIGNED_BYTE, empty.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    font->Get(L'?', fontTex);
    BakeCharacters();

    return sprites != 0 && fontTex != 0 && CreateModels();
}

void Renderer::ReleaseAssets()
{
    if (font)
    {
        if (font->dc)
        {
            if (font->oldFace)
                SelectObject(font->dc, font->oldFace);

            if (font->oldBitmap)
                SelectObject(font->dc, font->oldBitmap);
        }

        if (font->face)
            DeleteObject(font->face);

        if (font->bitmap)
            DeleteObject(font->bitmap);

        if (font->dc)
            DeleteDC(font->dc);
        delete font;
        font = nullptr;
    }

    if (fontTex)
        glDeleteTextures(1, &fontTex);

    if (sprites)
        glDeleteTextures(1, &sprites);
}

float Renderer::TextWidth(const std::string& text, float scale)
{
    if (!font)
        return 0;

    float width = 0, line = 0;

    for (wchar_t ch : Wide(text))
    {
        if (ch == L'\n')
        {
            if (line > width)
                width = line;
            line = 0;
        }
        else
            line += font->Get(ch, fontTex).advance * scale * .375f;
    }

    return line > width ? line : width;
}

void Renderer::Text(float x, float y, const std::string& text, Color color, float scale, float maxWidth)
{
    if (!font)
        return;

    float start = x, factor = scale * .375f, cell = FontCache::Cell * factor;

    for (wchar_t ch : Wide(text))
    {
        if (ch == L'\n')
        {
            x = start;
            y += scale * 17.f;
            continue;
        }

        FontCache::Glyph g = font->Get(ch, fontTex);

        if (maxWidth > 0 && x > start && x - start + g.advance * factor > maxWidth)
        {
            x = start;
            y += scale * 17.f;
        }

        float u = g.x / 2048.f, v = g.y / 2048.f;
        SurfaceQuad({x, y},
            {x + cell, y},
            {x + cell, y + cell},
            {x, y + cell},
            {u, v},
            {u + 48 / 2048.f, v},
            {u + 48 / 2048.f, v + 48 / 2048.f},
            {u, v + 48 / 2048.f},
            Material::Font,
            color);
        x += g.advance * factor;
    }
}

void Renderer::BakeCharacters()
{
    constexpr uint32_t Version = 1;
    constexpr size_t PixelBytes = 1536 * 768 * 4;
    std::vector<unsigned char> pixels;

    if (GameStorage::Read(L"characters.cache", Version, PixelBytes, pixels) && pixels.size() == PixelBytes)
    {
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &sprites);
        glBindTexture(GL_TEXTURE_2D, sprites);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1536, 768, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        std::cout << "캐릭터 애니메이션 캐시 불러오기 완료\n";
        return;
    }

    GLuint atlasFbo = 0;

    if (!Target(atlasFbo, sprites, 1536, 768, GL_RGBA8))
    {
        if (atlasFbo)
            glDeleteFramebuffers(1, &atlasFbo);

        if (sprites)
            glDeleteTextures(1, &sprites);
        sprites = 0;
        return;
    }

    // Keep the attached texture unbound while generating it to avoid texture feedback.
    GLuint atlas = sprites;
    sprites = 0;
    glViewport(0, 0, 1536, 768);
    canvasW = 1536;
    canvasH = 768;
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    const Color cloth[6] = {{.32f, .39f, .40f},
        {.52f, .36f, .24f},
        {.32f, .44f, .31f},
        {.55f, .29f, .25f},
        {.33f, .44f, .48f},
        {.55f, .49f, .33f}};
    const Color skin(.75f, .57f, .39f), dark(.16f, .17f, .15f), leather(.29f, .22f, .15f),
        steel(.58f, .65f, .64f);

    for (int variant = 0; variant < 6; ++variant)
        for (int dir = 0; dir < 4; ++dir)
            for (int frame = 0; frame < 8; ++frame)
            {
                float ox = (variant % 3) * 512.f + frame * 64.f, oy = (variant / 3) * 384.f + dir * 96.f;
                float x = ox + 32, y = oy + 86, cycle = frame * 6.2831853f / 8;
                float stride = std::sin(cycle) * 5, bob = std::fabs(std::sin(cycle)) * 1.5f;
                bool back = dir == 3, side = dir == 1 || dir == 2;
                float facing = dir == 1 ? -1.f : 1.f;
                Color tunic = cloth[variant];
                // Cloak/backpack, segmented legs, knees and boots.
                if (variant == 0)
                    Quad({x - 10, y - 53 - bob},
                        {x + 9, y - 53 - bob},
                        {x + 15 + stride * .3f, y - 18},
                        {x - 16 + stride * .3f, y - 18},
                        {.57f, .23f, .17f});

                for (int leg = 0; leg < 2; ++leg)
                {
                    float offset = (leg ? 1.f : -1.f) * (side ? 3.f : 6.f), step = leg ? stride : -stride;
                    Line({x + offset, y - 25}, {x + offset + step * .5f, y - 13}, 6, dark);
                    Line({x + offset + step * .5f, y - 13}, {x + offset + step, y - 3}, 5, leather);
                    Rect(x + offset + step - 4, y - 5, side ? 10.f : 8.f, 5, dark);
                    Line({x + offset + step - 2, y - 6},
                        {x + offset + step + 2, y - 6},
                        2,
                        {.45f, .35f, .24f});
                }

                float bodyWidth = side ? 9.f : 12.f;
                Quad({x - bodyWidth, y - 53 - bob},
                    {x + bodyWidth, y - 53 - bob},
                    {x + bodyWidth + 2, y - 24},
                    {x - bodyWidth - 2, y - 24},
                    tunic);
                Quad({x - bodyWidth, y - 52 - bob},
                    {x - 1, y - 51 - bob},
                    {x - 2, y - 25},
                    {x - bodyWidth - 2, y - 24},
                    {tunic.r * .75f, tunic.g * .75f, tunic.b * .75f});

                if (variant == 0 && !back)
                {
                    for (int row = 0; row < 5; ++row)
                        for (int col = 0; col < 5; ++col)
                            Ellipse(x - 8 + col * 4.f, y - 48 + row * 3.f, 1, 1, {.62f, .66f, .61f}, 6);
                }

                if (variant == 1 || variant == 5)
                    Quad({x - 8, y - 43}, {x + 8, y - 43}, {x + 10, y - 22}, {x - 10, y - 22}, leather);
                Rect(x - bodyWidth - 1, y - 31, bodyWidth * 2 + 2, 4, leather);
                Rect(x - 2, y - 31, 4, 4, {.72f, .55f, .29f});
                Line({x - 7, y - 52}, {x + 7, y - 32}, 2, {.57f, .43f, .29f});

                for (int arm = 0; arm < 2; ++arm)
                {
                    float sign = arm ? 1.f : -1.f, swing = sign * stride * .6f;
                    Line({x + sign * (bodyWidth - 1), y - 49 - bob},
                        {x + sign * (bodyWidth + 4), y - 39 + swing},
                        6,
                        tunic);
                    Line({x + sign * (bodyWidth + 4), y - 39 + swing},
                        {x + sign * (bodyWidth + 3), y - 29 + swing},
                        4,
                        leather);
                    Ellipse(x + sign * (bodyWidth + 3), y - 28 + swing, 3, 3, skin, 8);
                }

                // Neck, jaw, nose, hair and directional facial detail.
                Rect(x - 3, y - 57 - bob, 6, 7, skin);
                Ellipse(x, y - 64 - bob, side ? 8.f : 9.f, 10, skin, 12);
                Ellipse(x - 1, y - 69 - bob, 9, 6, {.23f, .20f, .16f}, 10);

                if (back)
                    Ellipse(x, y - 63 - bob, 9, 9, {.24f, .21f, .17f}, 12);
                else if (side)
                {
                    Rect(x + facing * 7, y - 64 - bob, 3, 4, skin);
                    Rect(x + facing * 4, y - 66 - bob, 2, 2, dark);
                }
                else
                {
                    Rect(x - 4, y - 65 - bob, 2, 2, dark);
                    Rect(x + 3, y - 65 - bob, 2, 2, dark);
                    Line({x - 2, y - 59 - bob}, {x + 2, y - 59 - bob}, 1, leather);
                }

                if (variant == 1)
                {
                    Ellipse(x, y - 69 - bob, 12, 3, {.60f, .47f, .28f}, 12);
                    Rect(x - 6, y - 77 - bob, 12, 7, {.53f, .41f, .25f});
                }

                if (variant == 4)
                {
                    Ellipse(x, y - 70 - bob, 10, 6, steel, 12);
                    Rect(x - 10, y - 70 - bob, 20, 3, {.36f, .43f, .44f});
                }

                if (variant == 2 || variant == 3)
                    Line({x - 8, y - 53 - bob}, {x + 8, y - 53 - bob}, 4, {.65f, .43f, .27f});

                if (variant == 0)
                {
                    Line({x + 17, y - 17}, {x + 22, y - 49}, 3, steel);
                    Line({x + 14, y - 27}, {x + 23, y - 25}, 3, {.73f, .57f, .30f});
                    Ellipse(x - 15, y - 39, 8, 11, {.34f, .27f, .19f}, 12);
                    Ellipse(x - 15, y - 39, 3, 4, steel, 10);
                }

                if (back && variant != 0)
                {
                    Rect(x - 8, y - 49, 16, 18, leather);
                    Rect(x - 9, y - 50, 18, 4, {.49f, .37f, .25f});
                }
            }

    Flush();
    sprites = atlas;
    pixels.resize(PixelBytes);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprites);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    GameStorage::Write(L"characters.cache", Version, pixels);
    std::cout << "캐릭터 애니메이션 생성 완료\n";
    canvasW = Width;
    canvasH = Height;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &atlasFbo);
}

void Renderer::Character(Point p, int appearance, int direction, int frame)
{
    int variant = appearance % 6;
    frame = frame % 8;
    direction = direction % 4;
    float x = (variant % 3) * 512.f + frame * 64.f, y = (variant / 3) * 384.f + direction * 96.f;
    SurfaceQuad({p.x - 32, p.y - 86},
        {p.x + 32, p.y - 86},
        {p.x + 32, p.y + 10},
        {p.x - 32, p.y + 10},
        {x / 1536.f, 1 - y / 768.f},
        {(x + 64) / 1536.f, 1 - y / 768.f},
        {(x + 64) / 1536.f, 1 - (y + 96) / 768.f},
        {x / 1536.f, 1 - (y + 96) / 768.f},
        Material::Sprite,
        {1, 1, 1});
}

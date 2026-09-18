#include "stdafx.h"
#include "Renderer.h"
#include "GameStorage.h"
#include <cmath>
#include <cstring>
#include <iostream>

bool Renderer::CreateModels()
{
    static_assert(sizeof(Vertex) == 9 * sizeof(float), "Update the model cache format when Vertex changes.");
    constexpr uint32_t Version = 1;
    std::vector<unsigned char> bytes;
    bool loaded = GameStorage::Read(L"models.cache", Version, 8 * 1024 * 1024, bytes);
    size_t offset = 0;

    if (loaded)
    {
        for (auto& model : models)
        {
            uint32_t count = 0;

            if (offset + sizeof(count) > bytes.size())
            {
                loaded = false;
                break;
            }

            std::memcpy(&count, bytes.data() + offset, sizeof(count));
            offset += sizeof(count);

            if (count == 0 || count > 100000 || count % 3 != 0 ||
                offset + count * sizeof(Vertex) > bytes.size())
            {
                loaded = false;
                break;
            }

            model.resize(count);
            std::memcpy(model.data(), bytes.data() + offset, count * sizeof(Vertex));
            offset += count * sizeof(Vertex);

            for (const Vertex& vertex : model)
            {
                const float values[] = {vertex.x,
                    vertex.y,
                    vertex.r,
                    vertex.g,
                    vertex.b,
                    vertex.a,
                    vertex.u,
                    vertex.v,
                    vertex.kind};

                for (float value : values)
                {
                    if (!std::isfinite(value) || std::fabs(value) > 4096)
                    {
                        loaded = false;
                    }
                }

                if (vertex.kind < 0 || vertex.kind > 7 || std::floor(vertex.kind) != vertex.kind)
                {
                    loaded = false;
                }
            }
        }

        loaded = loaded && offset == bytes.size();
    }

    if (loaded)
    {
        std::cout << "모델 캐시 불러오기 완료\n";
        return true;
    }

    bytes.clear();

    for (size_t i = 0; i < models.size(); ++i)
    {
        vertices.clear();
        BuildModel(static_cast<ModelKind>(i));
        models[i] = vertices;
        uint32_t count = static_cast<uint32_t>(vertices.size());
        size_t start = bytes.size();
        bytes.resize(start + sizeof(count) + count * sizeof(Vertex));
        std::memcpy(bytes.data() + start, &count, sizeof(count));
        std::memcpy(bytes.data() + start + sizeof(count), vertices.data(), count * sizeof(Vertex));
    }

    vertices.clear();
    GameStorage::Write(L"models.cache", Version, bytes);
    std::cout << "기본 도형 모델 생성 완료\n";
    return true;
}

void Renderer::BuildModel(ModelKind kind)
{
    const Color wood(.35f, .26f, .16f);
    const Color stone(.40f, .43f, .35f);

    if (kind == ModelKind::Pine)
    {
        Point p = {0, 0};
        float s = 1;
        float alpha = 1;
        SurfaceQuad({p.x - 5 * s, p.y},
            {p.x + 5 * s, p.y},
            {p.x + 4 * s, p.y - 58 * s},
            {p.x - 4 * s, p.y - 58 * s},
            {0, 0},
            {1, 0},
            {1, 3},
            {0, 3},
            Material::Wood,
            {.29f, .23f, .16f, alpha});

        for (int layer = 0; layer < 4; ++layer)
        {
            float base = p.y - (23 + layer * 24) * s, span = (43 - layer * 8) * s,
                  top = base - (57 - layer * 3) * s;

            for (int tooth = 0; tooth < 7; ++tooth)
            {
                float t = tooth / 6.f, xx = p.x - span + t * span * 2;
                float notch = std::sin(tooth * 4.3f + layer) * 5 * s;
                Color c = tooth < 3 ? Color(.12f, .23f, .17f, alpha) : Color(.20f, .31f, .20f, alpha);
                Triangle({p.x, top},
                    {xx - span * .22f, base + notch},
                    {xx + span * .20f, base + 7 * s + notch},
                    c);
            }

            for (int branch = 0; branch < 7; ++branch)
            {
                float t = (branch + 1) / 8.f;
                float xx = p.x + (branch % 2 ? 1 : -1) * span * t, yy = top + (base - top) * t;
                Line({xx, yy},
                    {xx + (branch % 2 ? 7 : -7) * s, yy + 7 * s},
                    1.2f * s,
                    {.39f, .43f, .22f, alpha * .65f});
            }
        }
    }
    else if (kind == ModelKind::Rock)
    {
        Ellipse(0, -6, 24, 14, stone, 7);
        Triangle({-21, -8}, {-6, -22}, {9, -12}, {.54f, .53f, .40f});
        Triangle({9, -12}, {20, -6}, {7, 4}, {.30f, .34f, .29f});
    }
    else if (kind == ModelKind::Boar || kind == ModelKind::Wolf || kind == ModelKind::Deer)
    {
        bool deer = kind == ModelKind::Deer;
        bool boar = kind == ModelKind::Boar;
        float y = deer ? -29.f : boar ? -17.f : -21.f;
        Color coat = deer   ? Color(.52f, .36f, .22f)
                     : boar ? Color(.28f, .25f, .21f)
                            : Color(.43f, .46f, .43f);
        Color dark(coat.r * .63f, coat.g * .63f, coat.b * .63f);

        for (int i = 0; i < 4; ++i)
        {
            float x = (i < 2 ? -12.f : 11.f) + i % 2 * 3;
            Line({x, y + 5}, {x, -9}, deer ? 3.f : 4.f, dark);
            Line({x, -9}, {x + 1, 0}, 3, dark);
        }

        Ellipse(0, y, boar ? 23.f : 21.f, boar ? 13.f : 10.f, coat, 16);
        Ellipse(-3, y + 4, 16, 5, dark, 12);
        float hy = y - (deer ? 16.f : 2.f);
        Line({14, y}, {23, hy}, deer ? 10.f : 13.f, coat);
        Ellipse(23, hy, boar ? 10.f : 8.f, deer ? 9.f : 7.f, coat, 12);
        Ellipse(30, hy + 3, 6, 4, dark, 10);
        Ellipse(27, hy - 3, 1.3f, 1.3f, {.07f, .08f, .06f}, 6);
        Triangle({19, hy - 5}, {17, hy - 15}, {24, hy - 8}, dark);
        Triangle({25, hy - 6}, {28, hy - 14}, {30, hy - 5}, coat);
        Line({-18, y}, {-31, y + (deer ? -3.f : 8.f)}, boar ? 3.f : 5.f, dark);

        if (deer)
        {
            for (int sign : {-1, 1})
            {
                Line({23 + sign * 4.f, hy - 7}, {23 + sign * 9.f, hy - 24}, 2, {.68f, .59f, .42f});
                Line({23 + sign * 7.f, hy - 18}, {23 + sign * 14.f, hy - 21}, 2, {.68f, .59f, .42f});
            }
        }

        if (boar)
        {
            Line({31, hy + 5}, {34, hy - 1}, 2, {.84f, .79f, .60f});

            for (int i = 0; i < 8; ++i)
            {
                Line({-14 + i * 4.f, y - 10}, {-12 + i * 4.f, y - 15}, 2, dark);
            }
        }
    }
    else if (kind == ModelKind::Supply)
    {
        Quad({-10, -16}, {0, -10}, {11, -16}, {0, -22}, {.65f, .49f, .26f});
        Quad({-10, -4}, {0, 2}, {0, -10}, {-10, -16}, wood);
        Quad({0, 2}, {11, -4}, {11, -16}, {0, -10}, {.51f, .36f, .21f});
        Line({-8, -12}, {8, -6}, 3, {.77f, .66f, .38f});
    }
    else if (kind == ModelKind::Cabin)
    {
        auto project = [](Point p, float height = 0)
        {
            return Point{(p.x - p.y) * 28, (p.x + p.y) * 15 - height};
        };

        float s = 1.5f;
        Point c = {0, 0};
        Point a = project({c.x - s, c.y - s}), b = project({c.x + s, c.y - s}),
              d = project({c.x - s, c.y + s}), e = project({c.x + s, c.y + s});
        Point center = project(c);
        float alpha = 1;

        auto tint = [alpha](float red, float green, float blue)
        {
            return Color(red, green, blue, alpha);
        };
        Point at = {a.x, a.y - 57}, bt = {b.x, b.y - 57}, dt = {d.x, d.y - 57}, et = {e.x, e.y - 57};
        SurfaceQuad(d, e, et, dt, {0, 2}, {3, 2}, {3, 0}, {0, 0}, Material::Wood, tint(.42f, .30f, .20f));
        SurfaceQuad(e, b, bt, et, {0, 2}, {3, 2}, {3, 0}, {0, 0}, Material::Wood, tint(.54f, .40f, .25f));
        SurfaceQuad(d,
            e,
            {e.x, e.y - 13},
            {d.x, d.y - 13},
            {0, 0},
            {3, 0},
            {3, 1},
            {0, 1},
            Material::Stone,
            tint(.45f, .44f, .34f));
        SurfaceQuad(e,
            b,
            {b.x, b.y - 13},
            {e.x, e.y - 13},
            {0, 0},
            {3, 0},
            {3, 1},
            {0, 1},
            Material::Stone,
            tint(.50f, .47f, .35f));

        for (int i = 1; i < 5; ++i)
        {
            float z = i * 10.f;
            Line({d.x, d.y - z}, {e.x, e.y - z}, 2, tint(.27f, .22f, .16f));
            Line({e.x, e.y - z}, {b.x, b.y - z}, 2, tint(.34f, .26f, .17f));
        }

        Line(d, dt, 6, tint(.27f, .22f, .16f));
        Line(e, et, 6, tint(.31f, .23f, .15f));
        Line(b, bt, 6, tint(.40f, .29f, .18f));
        Point peak = {center.x, center.y - 122};
        SurfaceTriangle(at, dt, peak, {0, 1}, {1, 1}, {.5f, 0}, Material::Thatch, tint(.37f, .29f, .18f));
        SurfaceTriangle(dt, et, peak, {0, 1}, {1, 1}, {.5f, 0}, Material::Thatch, tint(.58f, .43f, .25f));
        SurfaceTriangle(et, bt, peak, {0, 1}, {1, 1}, {.5f, 0}, Material::Thatch, tint(.70f, .53f, .30f));
        Line(dt, et, 6, tint(.34f, .27f, .16f));
        Line(et, bt, 6, tint(.43f, .31f, .18f));

        for (int i = 1; i < 12; ++i)
        {
            float t = i / 12.f;
            Line(peak, {dt.x + (et.x - dt.x) * t, dt.y + (et.y - dt.y) * t}, 1, tint(.42f, .32f, .18f));
        }

        Point door = project({c.x, c.y + s});
        Quad({door.x - 12, door.y - 6},
            {door.x + 12, door.y + 6},
            {door.x + 12, door.y - 29},
            {door.x - 12, door.y - 41},
            tint(.17f, .18f, .13f));
        Line({door.x - 12, door.y - 7}, {door.x - 12, door.y - 42}, 3, tint(.59f, .43f, .26f));
        Point window = project({c.x + s, c.y - .35f}, 30);
        Quad({window.x - 10, window.y + 4},
            {window.x + 10, window.y - 5},
            {window.x + 10, window.y - 21},
            {window.x - 10, window.y - 12},
            tint(1.2f, .75f, .29f));
        Line({window.x, window.y}, {window.x, window.y - 16}, 2, tint(.33f, .25f, .17f));
        // Storage pots and firewood give the settlement a pre-industrial silhouette.
        Point jar = project({c.x - s - .35f, c.y + s});
        Ellipse(jar.x, jar.y - 8, 9, 11, tint(.52f, .30f, .19f), 12);
        Ellipse(jar.x, jar.y - 17, 5, 3, tint(.24f, .21f, .15f), 12);

        for (int i = 0; i < 4; ++i)
            Line({b.x + 3, b.y - i * 4.f}, {b.x + 22, b.y - i * 4.f - 7}, 4, tint(.39f, .28f, .16f));
    }
    else if (kind == ModelKind::Camp)
    {
        for (int i = 0; i < 10; ++i)
        {
            float a = i * 6.2831853f / 10;
            Ellipse(std::cos(a) * 19, std::sin(a) * 9, 6, 4, stone, 7);
        }

        for (int i = 0; i < 4; ++i)
        {
            Line({-13 + i * 4.f, 4}, {9 - i * 5.f, -7}, 5, wood);
        }
    }
}

void Renderer::Model(
    ModelKind kind, Point base, float scale, float opacity, float gait, int facing, float flash)
{
    bool animal = kind == ModelKind::Boar || kind == ModelKind::Wolf || kind == ModelKind::Deer;

    for (Vertex vertex : models[static_cast<size_t>(kind)])
    {
        if (animal && vertex.y > -14)
        {
            vertex.x += std::sin(gait + (vertex.x < 0 ? 3.14159f : 0)) * 2.5f;
        }

        vertex.x = base.x + vertex.x * scale * facing;
        vertex.y = base.y + vertex.y * scale;
        vertex.a *= opacity;
        vertex.r += (1.f - vertex.r) * flash;
        vertex.g += (1.f - vertex.g) * flash;
        vertex.b += (1.f - vertex.b) * flash;
        vertices.push_back(vertex);
    }
}

void Renderer::Flame(Point base, float scale)
{
    SurfaceQuad({base.x - 42 * scale, base.y - 95 * scale},
        {base.x + 42 * scale, base.y - 95 * scale},
        {base.x + 42 * scale, base.y + 10 * scale},
        {base.x - 42 * scale, base.y + 10 * scale},
        {-1, 0},
        {1, 0},
        {1, 1},
        {-1, 1},
        Material::Flame,
        {1, 1, 1});
}

bool Renderer::ToCanvas(int x, int y, Point& output) const
{
    int top = windowHeight - viewportY - viewportH;

    if (viewportW <= 0 || viewportH <= 0 || x < viewportX || y < top || x >= viewportX + viewportW ||
        y >= top + viewportH)
    {
        return false;
    }

    output = {(x - viewportX) * Width / viewportW, (y - top) * Height / viewportH};
    return true;
}

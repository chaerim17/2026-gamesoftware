#include "stdafx.h"
#include "Tutorial.h"
#include <cmath>

bool Tutorial::Visible(Vec p, float margin) const
{
    Point s = Project(p);

    return s.x > -margin && s.x < 1280 + margin && s.y > -margin && s.y < 800 + margin;
}

void Tutorial::WorldEllipse(Renderer& r, Vec c, float rx, float ry, Color col, Material material)
{
    if (!Visible(c, (rx + ry) * 28 + 100))
        return;

    for (int i = 0; i < 48; ++i)
    {
        float a = i * 6.2831853f / 48, b = (i + 1) * 6.2831853f / 48;
        Vec p = {c.x + std::cos(a) * rx, c.y + std::sin(a) * ry},
            q = {c.x + std::cos(b) * rx, c.y + std::sin(b) * ry};
        r.SurfaceTriangle(
            Project(c), Project(p), Project(q), {c.x, c.y}, {p.x, p.y}, {q.x, q.y}, material, col);
    }
}

void Tutorial::Path(Renderer& r, Vec a, Vec b, float width)
{
    float d = Distance(a, b);

    if (d < .01f)
        return;

    float nx = -(b.y - a.y) / d * width, ny = (b.x - a.x) / d * width;
    Vec p = {a.x + nx, a.y + ny}, q = {b.x + nx, b.y + ny}, s = {b.x - nx, b.y - ny},
        t = {a.x - nx, a.y - ny};
    r.SurfaceQuad(Project(p),
        Project(q),
        Project(s),
        Project(t),
        {p.x, p.y},
        {q.x, q.y},
        {s.x, s.y},
        {t.x, t.y},
        Material::Dirt,
        {.46f, .38f, .25f});
    WorldEllipse(r, a, width, width, {.46f, .38f, .25f}, Material::Dirt);
    WorldEllipse(r, b, width, width, {.46f, .38f, .25f}, Material::Dirt);
}

void Tutorial::Ground(Renderer& r)
{
    r.Rect(0, 0, 1280, 800, {.13f, .20f, .19f});

    for (int x = -37; x < 38; ++x)
        for (int y = -37; y < 32; ++y)
        {
            Vec a = {static_cast<float>(x), static_cast<float>(y)};

            if (!Visible(a, 65))
                continue;

            Vec b = {x + 1.f, a.y}, c = {x + 1.f, y + 1.f}, d = {a.x, y + 1.f};
            r.SurfaceQuad(Project(a),
                Project(b),
                Project(c),
                Project(d),
                {a.x, a.y},
                {b.x, b.y},
                {c.x, c.y},
                {d.x, d.y},
                Material::Grass,
                {.26f, .32f, .19f});
        }

    WorldEllipse(r, {0, 5}, 14, 14, {.37f, .34f, .23f}, Material::Dirt);
    Path(r, {-2, 4}, {-2, 0}, 1.2f);
    Path(r, {-2, 0}, {-1, -10}, .9f);
    Path(r, {-1, -10}, gate, .85f);
    Path(r, {-1, -10}, clue, .75f);
    Path(r, clue, {-2, 0}, .65f);
    Path(r, {-1, -10}, {-20, -23}, .62f);
    Path(r, {0, 8}, {-19, 21}, .8f);

    for (const House& h : houses)
        Path(r, {h.pos.x, h.pos.y + h.half + .6f}, {0, 5}, .7f);
    WorldEllipse(r, {0, 2}, 4.1f, 3.8f, {.48f, .45f, .34f}, Material::Stone);
    // Layered wet sand and translucent shallows transition to animated deep water.
    WorldEllipse(r, lake, 8.3f, 10.1f, {.47f, .44f, .29f}, Material::Dirt);
    WorldEllipse(r, lake, 7.7f, 9.5f, {.34f, .43f, .33f}, Material::Dirt);
    WorldEllipse(r, lake, 7.4f, 9.2f, {.20f, .43f, .41f}, Material::Water);

    for (int i = 0; i < 9; ++i)
        WorldEllipse(r,
            {lake.x + .2f * i, lake.y - .10f * i},
            7.1f - i * .43f,
            8.9f - i * .48f,
            {.08f, .26f, .30f, .10f},
            Material::Water);

    for (int i = 0; i < 64; ++i)
    {
        float angle = i * 6.2831853f / 64, phase = animation * .8f + i * .17f;
        float radius = 1 + std::sin(phase) * .008f;
        Vec p = {lake.x + std::cos(angle) * 7.32f * radius, lake.y + std::sin(angle) * 9.12f * radius};
        Vec q = {lake.x + std::cos(angle + .04f) * 7.32f * radius,
            lake.y + std::sin(angle + .04f) * 9.12f * radius};
        r.Line(Project(p), Project(q), 1.2f, {.66f, .77f, .62f, .17f + .12f * std::sin(phase)});
    }

    for (int i = 0; i < 55; ++i)
    {
        float a = i * 2.39996f;
        Vec p = {lake.x + std::cos(a) * (1 + (i % 6) * .8f), lake.y + std::sin(a) * (1 + (i % 7) * .9f)};
        Point s = Project(p);
        float k = .12f + .10f * std::sin(animation * 1.3f + i);
        r.Line({s.x - 7, s.y}, {s.x + 10 + std::sin(animation + i) * 5, s.y}, 1.3f, {1.4f, 1.5f, 1.2f, k});
    }

    // Pebbles and grass are deterministic world details, culled before geometry work.
    for (int i = 0; i < 1500; ++i)
    {
        Vec v = {-33 + ((i * 73) % 660) * .1f, -33 + ((i * 127) % 600) * .1f};

        if (!Visible(v, 30))
            continue;

        float lx = (v.x - lake.x) / 8.f, ly = (v.y - lake.y) / 9.8f;

        if (lx * lx + ly * ly < 1)
            continue;

        if (std::fabs(v.x) < 12 && v.y > -4 && v.y < 18)
            continue;

        Point p = Project(v);

        if (i % 7 == 0)
        {
            r.Ellipse(p.x, p.y, 3 + i % 3, 2, {.37f, .37f, .29f}, 6);
            r.Line({p.x - 2, p.y - 1}, {p.x + 1, p.y - 2}, 1, {.52f, .49f, .35f});
        }
        else
        {
            float sway = std::sin(animation * 1.5f + i) * 1.5f;
            r.Line(p, {p.x - 3 + sway, p.y - 6}, 1, {.39f, .44f, .23f});
            r.Line(p, {p.x + 3 + sway, p.y - 5}, 1, {.46f, .48f, .25f});
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        Point p = Project({-.1f, -3.f - i * 2.6f});
        r.Line(p, {p.x, p.y - 25}, 3, {.28f, .20f, .15f});
        float flutter = std::sin(animation * 3 + i) * 3;
        r.Triangle({p.x, p.y - 26}, {p.x + 17, p.y - 22 + flutter}, {p.x, p.y - 15}, {.72f, .30f, .20f});
    }

    for (int i = -34; i <= 34; i += 2)
    {
        for (Vec v : {Vec{static_cast<float>(i), 28.5f}, Vec{static_cast<float>(i), -34.5f}})
        {
            if (!Visible(v, 40))
                continue;

            Point p = Project(v);
            r.Ellipse(p.x, p.y - 5, 14, 9, {.34f, .36f, .29f}, 6);
        }
    }

    for (int i = -34; i <= 28; i += 2)
        for (float x : {-34.5f, 34.5f})
        {
            Vec v = {x, static_cast<float>(i)};

            if (!Visible(v, 40))
                continue;

            Point p = Project(v);
            r.Ellipse(p.x, p.y - 5, 14, 9, {.34f, .36f, .29f}, 6);
        }
}

void Tutorial::DrawShadows(Renderer& r)
{
    r.BeginShadows();

    for (const Tree& t : trees)
        if (Visible(t.pos, 240))
        {
            Point p = Project(t.pos);
            r.SoftShadow(p, 13 * t.size, 7 * t.size, 70 * t.size, .8f);
            r.SoftShadow({p.x + 32 * t.size, p.y + 10 * t.size}, 25 * t.size, 13 * t.size, 40 * t.size, .65f);
        }

    for (const House& h : houses)
        if (Visible(h.pos, 230))
        {
            Point p = Project(h.pos);
            r.SoftShadow(p, h.half * 39, h.half * 18, 80, .8f);
            Point a = Project({h.pos.x - h.half, h.pos.y + h.half}),
                  b = Project({h.pos.x + h.half, h.pos.y + h.half});
            r.Quad(a, b, {b.x + 54, b.y + 20}, {a.x + 54, a.y + 20}, {0, 0, 0, .24f});
        }

    for (const Npc& n : npcs)
        if (Visible(n.pos))
            r.SoftShadow(Project(n.pos), 10, 5, 25, .9f);
    r.SoftShadow(Project(player), 11, 5, 28, .95f);

    for (const Animal& a : animals)
        if (Visible(a.pos))
            r.SoftShadow(Project(a.pos), 19, 6, 23, .8f);
    r.EndShadows();
}

void Tutorial::DrawTree(Renderer& r, const Tree& tree)
{
    if (!Visible(tree.pos))
        return;

    Point p = Project(tree.pos), pp = Project(player);
    float s = tree.size;
    float alpha = (pp.y < p.y && pp.y > p.y - 140 * s && std::fabs(pp.x - p.x) < 47 * s) ? .32f : 1.f;
    r.SurfaceQuad({p.x - 5 * s, p.y},
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
            r.Triangle(
                {p.x, top}, {xx - span * .22f, base + notch}, {xx + span * .20f, base + 7 * s + notch}, c);
        }

        for (int branch = 0; branch < 7; ++branch)
        {
            float t = (branch + 1) / 8.f;
            float xx = p.x + (branch % 2 ? 1 : -1) * span * t, yy = top + (base - top) * t;
            r.Line({xx, yy},
                {xx + (branch % 2 ? 7 : -7) * s, yy + 7 * s},
                1.2f * s,
                {.39f, .43f, .22f, alpha * .65f});
        }
    }
}

void Tutorial::DrawHouse(Renderer& r, const House& h)
{
    if (!Visible(h.pos, 200))
        return;

    float s = h.half;
    Vec c = h.pos;
    Point a = Project({c.x - s, c.y - s}), b = Project({c.x + s, c.y - s}), d = Project({c.x - s, c.y + s}),
          e = Project({c.x + s, c.y + s}), pp = Project(player);
    Point center = Project(c);
    float alpha = 1;

    if (pp.y < center.y + 2 * s * 15 && pp.y > center.y - 125 && std::fabs(pp.x - center.x) < 2 * s * 28)
        alpha = .40f;
    auto tint = [alpha](float red, float green, float blue)
    {
        return Color(red, green, blue, alpha);
    };
    Point at = {a.x, a.y - 57}, bt = {b.x, b.y - 57}, dt = {d.x, d.y - 57}, et = {e.x, e.y - 57};
    r.SurfaceQuad(d, e, et, dt, {0, 2}, {3, 2}, {3, 0}, {0, 0}, Material::Wood, tint(.42f, .30f, .20f));
    r.SurfaceQuad(e, b, bt, et, {0, 2}, {3, 2}, {3, 0}, {0, 0}, Material::Wood, tint(.54f, .40f, .25f));
    r.SurfaceQuad(d,
        e,
        {e.x, e.y - 13},
        {d.x, d.y - 13},
        {0, 0},
        {3, 0},
        {3, 1},
        {0, 1},
        Material::Stone,
        tint(.45f, .44f, .34f));
    r.SurfaceQuad(e,
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
        r.Line({d.x, d.y - z}, {e.x, e.y - z}, 2, tint(.27f, .22f, .16f));
        r.Line({e.x, e.y - z}, {b.x, b.y - z}, 2, tint(.34f, .26f, .17f));
    }

    r.Line(d, dt, 6, tint(.27f, .22f, .16f));
    r.Line(e, et, 6, tint(.31f, .23f, .15f));
    r.Line(b, bt, 6, tint(.40f, .29f, .18f));
    Point peak = {center.x, center.y - 122};
    r.SurfaceTriangle(at, dt, peak, {0, 1}, {1, 1}, {.5f, 0}, Material::Thatch, tint(.37f, .29f, .18f));
    r.SurfaceTriangle(dt, et, peak, {0, 1}, {1, 1}, {.5f, 0}, Material::Thatch, tint(.58f, .43f, .25f));
    r.SurfaceTriangle(et, bt, peak, {0, 1}, {1, 1}, {.5f, 0}, Material::Thatch, tint(.70f, .53f, .30f));
    r.Line(dt, et, 6, tint(.34f, .27f, .16f));
    r.Line(et, bt, 6, tint(.43f, .31f, .18f));

    for (int i = 1; i < 12; ++i)
    {
        float t = i / 12.f;
        r.Line(peak, {dt.x + (et.x - dt.x) * t, dt.y + (et.y - dt.y) * t}, 1, tint(.42f, .32f, .18f));
    }

    Point door = Project({c.x, c.y + s});
    r.Quad({door.x - 12, door.y - 6},
        {door.x + 12, door.y + 6},
        {door.x + 12, door.y - 29},
        {door.x - 12, door.y - 41},
        tint(.17f, .18f, .13f));
    r.Line({door.x - 12, door.y - 7}, {door.x - 12, door.y - 42}, 3, tint(.59f, .43f, .26f));
    Point window = Project({c.x + s, c.y - .35f}, 30);
    r.Quad({window.x - 10, window.y + 4},
        {window.x + 10, window.y - 5},
        {window.x + 10, window.y - 21},
        {window.x - 10, window.y - 12},
        tint(1.2f, .75f, .29f));
    r.Line({window.x, window.y}, {window.x, window.y - 16}, 2, tint(.33f, .25f, .17f));
    // Storage pots and firewood give the settlement a pre-industrial silhouette.
    Point jar = Project({c.x - s - .35f, c.y + s});
    r.Ellipse(jar.x, jar.y - 8, 9, 11, tint(.52f, .30f, .19f), 12);
    r.Ellipse(jar.x, jar.y - 17, 5, 3, tint(.24f, .21f, .15f), 12);

    for (int i = 0; i < 4; ++i)
        r.Line({b.x + 3, b.y - i * 4.f}, {b.x + 22, b.y - i * 4.f - 7}, 4, tint(.39f, .28f, .16f));
}

void Tutorial::DrawPerson(Renderer& r, Vec p, Color, bool isPlayer, float phase, int appearance)
{
    if (!Visible(p))
        return;

    int direction = isPlayer ? facing : npcs[appearance].direction;
    int frame = isPlayer ? (moving ? static_cast<int>(steps) % 8 : 0)
                         : (npcs[appearance].walking ? static_cast<int>(npcs[appearance].gait) % 8 : 0);
    r.Character(Project(p), isPlayer ? 0 : 1 + appearance % 5, direction, frame);
}

void Tutorial::DrawFire(Renderer& r)
{
    Point p = Project(fire);

    if (!Visible(fire))
        return;

    for (int i = 6; i >= 1; --i)
        r.Ellipse(p.x, p.y, 12 + i * 13.f, 5 + i * 6.f, {1.5f, .65f, .15f, .025f});

    for (int i = 0; i < 10; ++i)
    {
        float a = i * 6.2831853f / 10;
        r.Ellipse(p.x + std::cos(a) * 19, p.y + std::sin(a) * 9, 6, 4, {.37f, .36f, .28f}, 7);
    }

    for (int i = 0; i < 4; ++i)
        r.Line({p.x - 13 + i * 4.f, p.y + 4}, {p.x + 9 - i * 5.f, p.y - 7}, 5, {.22f, .16f, .10f});

    for (int i = 0; i < 7; ++i)
    {
        float x = p.x - 9 + i * 3.f, height = 20 + std::sin(animation * 7 + i * 1.7f) * 8 + i % 3 * 5;
        r.Triangle({x - 5, p.y},
            {x + std::sin(animation * 5 + i) * 6, p.y - height},
            {x + 6, p.y},
            {2.4f, .43f + i * .07f, .045f, .60f});
        r.Triangle({x - 2, p.y}, {x + 2, p.y - height * .62f}, {x + 3, p.y}, {3.0f, 1.6f, .28f, .85f});
    }

    for (int i = 0; i < 30; ++i)
    {
        float t = std::fmod(animation * .34f + i * .137f, 1.f);
        r.Ellipse(p.x + std::sin(t * 8 + i) * 12 + t * 16,
            p.y - 13 - t * 87,
            1.2f,
            2,
            {2.8f, 1.1f, .14f, (1 - t) * .7f},
            6);
    }

    for (int i = 0; i < 10; ++i)
    {
        float t = std::fmod(animation * .14f + i * .1f, 1.f);
        r.Ellipse(p.x + std::sin(t * 5 + i) * 9 + t * 38,
            p.y - 37 - t * 100,
            5 + t * 16,
            4 + t * 9,
            {.59f, .61f, .53f, (1 - t) * .065f});
    }
}

const char* Tutorial::AnimalName(int kind)
{
    return kind == 0 ? "숲사슴" : kind == 1 ? "멧돼지" : "회색늑대";
}

void Tutorial::UpdateAnimals(float dt)
{
    // Wildlife is observational in this tutorial: alert/flee behavior without combat locks.
    if (!dialogue.empty())
        return;

    for (Animal& a : animals)
    {
        float distance = Distance(player, a.pos), homeDistance = Distance(a.pos, a.home);
        Vec direction = {std::cos(animation * .27f + a.seed), std::sin(animation * .21f + a.seed)};
        float speed = .4f;

        if (distance < (a.kind == 0 ? 5.f : 3.5f))
        {
            direction = {a.pos.x - player.x, a.pos.y - player.y};
            speed = a.kind == 0 ? 2.7f : 1.7f;
        }
        else if (a.kind == 2 && distance < 6 && distance > 4.5f)
        {
            direction = {player.x - a.pos.x, player.y - a.pos.y};
            speed = .65f;
        }

        if (homeDistance > 7)
        {
            direction = {a.home.x - a.pos.x, a.home.y - a.pos.y};
            speed = 1.2f;
        }

        float length = Distance(direction, {0, 0});

        if (length < .001f)
            continue;

        Vec old = a.pos, next = {a.pos.x + direction.x / length * speed * dt, a.pos.y};

        if (Walkable(next))
            a.pos = next;
        next = {a.pos.x, a.pos.y + direction.y / length * speed * dt};

        if (Walkable(next))
            a.pos = next;

        if (Distance(a.pos, old) > .001f)
        {
            a.gait += dt * speed * 5;
            a.facing = direction.x - direction.y < 0 ? -1 : 1;
        }
        else
            a.seed += dt * 2;
    }
}

void Tutorial::DrawAnimal(Renderer& r, const Animal& a)
{
    if (!Visible(a.pos))
        return;

    Point p = Project(a.pos);
    float side = static_cast<float>(a.facing), stride = std::sin(a.gait) * 4;
    bool deer = a.kind == 0, boar = a.kind == 1;
    float bodyY = deer ? 29.f : boar ? 17.f : 21.f;
    Color coat = deer ? Color(.52f, .36f, .22f) : boar ? Color(.28f, .25f, .21f) : Color(.43f, .46f, .43f);
    Color dark(coat.r * .63f, coat.g * .63f, coat.b * .63f);

    for (int i = 0; i < 4; ++i)
    {
        float x = p.x + (i < 2 ? -12.f : 11.f) + (i % 2) * 3, step = i % 2 ? stride : -stride;
        r.Line({x, p.y - bodyY + 5}, {x + step * .5f, p.y - 9}, deer ? 3.f : 4.f, dark);
        r.Line({x + step * .5f, p.y - 9}, {x + step, p.y}, 3, dark);
    }

    r.Ellipse(p.x, p.y - bodyY, boar ? 23.f : 21.f, boar ? 13.f : 10.f, coat, 16);
    r.Ellipse(p.x - 3, p.y - bodyY + 4, 16, 5, dark, 12);
    float hx = p.x + side * 23, hy = p.y - bodyY - (deer ? 16.f : 2.f);
    r.Line({p.x + side * 14, p.y - bodyY}, {hx, hy}, deer ? 10.f : 13.f, coat);
    r.Ellipse(hx, hy, boar ? 10.f : 8.f, deer ? 9.f : 7.f, coat, 12);
    r.Ellipse(hx + side * 7, hy + 3, 6, 4, dark, 10);
    r.Ellipse(hx + side * 4, hy - 3, 1.3f, 1.3f, {.07f, .08f, .06f}, 6);
    r.Triangle({hx - 4, hy - 5}, {hx - 6, hy - 15}, {hx + 1, hy - 8}, dark);
    r.Triangle({hx + 2, hy - 6}, {hx + 5, hy - 14}, {hx + 7, hy - 5}, coat);
    r.Line({p.x - side * 18, p.y - bodyY},
        {p.x - side * 31, p.y - bodyY + (deer ? -3.f : 8.f) + stride * .3f},
        boar ? 3.f : 5.f,
        dark);

    if (deer)
    {
        for (int k = -1; k <= 1; k += 2)
        {
            r.Line({hx + k * 4.f, hy - 7}, {hx + k * 9.f, hy - 24}, 2, {.68f, .59f, .42f});
            r.Line({hx + k * 7.f, hy - 18}, {hx + k * 14.f, hy - 21}, 2, {.68f, .59f, .42f});
        }
    }

    if (boar)
    {
        r.Line({hx + side * 8, hy + 5}, {hx + side * 11, hy - 1}, 2, {.84f, .79f, .60f});

        for (int i = 0; i < 8; ++i)
            r.Line({p.x - 14 + i * 4.f, p.y - bodyY - 10}, {p.x - 12 + i * 4.f, p.y - bodyY - 15}, 2, dark);
    }
}

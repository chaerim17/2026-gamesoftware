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

    // Water glints and waves are animated in the material shader.

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
    {
        return;
    }

    Point p = Project(tree.pos);
    Point hero = Project(player);
    float size = tree.size;
    bool occludes = hero.y < p.y && hero.y > p.y - 140 * size && std::fabs(hero.x - p.x) < 47 * size;
    r.Model(ModelKind::Pine, p, size, occludes ? .32f : 1.f);
}

void Tutorial::DrawHouse(Renderer& r, const House& h)
{
    if (!Visible(h.pos, 230))
    {
        return;
    }

    Point p = Project(h.pos);
    Point hero = Project(player);
    float size = h.half / 1.5f;
    bool occludes =
        hero.y < p.y + h.half * 30 && hero.y > p.y - 125 * size && std::fabs(hero.x - p.x) < h.half * 56;
    r.Model(ModelKind::Cabin, p, size, occludes ? .40f : 1.f);
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
    if (!Visible(fire))
    {
        return;
    }

    Point p = Project(fire);
    r.Model(ModelKind::Camp, p);
    r.Flame(p);
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
    {
        return;
    }

    ModelKind kind = a.kind == 0 ? ModelKind::Deer : a.kind == 1 ? ModelKind::Boar : ModelKind::Wolf;
    r.Model(kind, Project(a.pos), 1, 1, a.gait, a.facing);
}

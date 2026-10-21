#include "stdafx.h"
#include "LevelMap.h"
#include <random>
#include <queue>
#include <cmath>
#include <cassert>
#include <algorithm>

bool LevelMap::Open(int index) const
{
    return index >= 0 && index < Count && (cells[index] == Tile::Grass || cells[index] == Tile::Dirt);
}

int LevelMap::Index(Point p) const
{
    int x = static_cast<int>(std::floor(p.x + Width * .5f));
    int y = static_cast<int>(std::floor(p.y + Height * .5f));

    if (x < 0 || x >= Width || y < 0 || y >= Height)
    {
        return -1;
    }

    return y * Width + x;
}

Point LevelMap::Center(int index) const
{
    return {index % Width - Width * .5f + .5f, index / Width - Height * .5f + .5f};
}

Point LevelMap::Camp() const
{
    return {-100.5f, 82.5f};
}

LevelMap::Tile LevelMap::At(int index) const
{
    return cells[index];
}

const std::vector<int>& LevelMap::OpenCells() const
{
    return openCells;
}

bool LevelMap::CanOccupy(Point p) const
{
    // A 0.44-unit actor fits every one-cell-wide, four-connected corridor.
    for (float dx : {-.22f, .22f})
    {
        for (float dy : {-.22f, .22f})
        {
            if (!Open(Index({p.x + dx, p.y + dy})))
            {
                return false;
            }
        }
    }

    return true;
}

bool LevelMap::ClearLine(Point from, Point to) const
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    int steps = static_cast<int>(std::sqrt(dx * dx + dy * dy) * 10) + 1;

    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;

        if (!CanOccupy({from.x + dx * t, from.y + dy * t}))
        {
            return false;
        }
    }

    return true;
}

std::vector<int> LevelMap::Distances(int start) const
{
    std::vector<int> result(Count, -1);

    if (!Open(start))
    {
        return result;
    }

    std::queue<int> queue;
    result[start] = 0;
    queue.push(start);

    while (!queue.empty())
    {
        int current = queue.front();
        queue.pop();
        int x = current % Width;
        int y = current / Width;

        for (int offset : {-1, 1, -Width, Width})
        {
            if ((offset == -1 && x == 0) || (offset == 1 && x == Width - 1) || (offset == -Width && y == 0) ||
                (offset == Width && y == Height - 1))
            {
                continue;
            }

            int next = current + offset;

            if (Open(next) && result[next] == -1)
            {
                result[next] = result[current] + 1;
                queue.push(next);
            }
        }
    }

    return result;
}

Point LevelMap::NextStep(Point from, Point target, const std::vector<int>& field) const
{
    int index = Index(from);

    if (!Open(index) || field[index] < 0)
    {
        return from;
    }

    if (index == Index(target))
    {
        return target;
    }

    // Visit a tile's center before turning into the next corridor.
    Point center = Center(index);

    if (!ClearLine(from, center))
    {
        return from;
    }

    int best = index;

    for (int offset : {-1, 1, -Width, Width})
    {
        int next = index + offset;

        if ((offset == -1 && index % Width == 0) || (offset == 1 && index % Width == Width - 1))
        {
            continue;
        }

        if (Open(next) && field[next] >= 0 && field[next] < field[best])
        {
            best = next;
        }
    }

    Point destination = Center(best);
    return ClearLine(from, destination) ? destination : center;
}

bool LevelMap::IsConnected() const
{
    auto distances = Distances(Index(Camp()));

    for (int i = 0; i < Count; ++i)
    {
        if (Open(i) && distances[i] < 0)
        {
            return false;
        }
    }

    return true;
}

void LevelMap::Generate(uint32_t seed)
{
    std::mt19937 random(seed);
    cells.fill(Tile::Grass);

    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            int roll = static_cast<int>(random() % 100);
            cells[y * Width + x] = x == 0 || y == 0 || x == Width - 1 || y == Height - 1 ? Tile::Rock
                                   : roll < 14                                           ? Tile::Tree
                                   : roll < 19                                           ? Tile::Rock
                                                                                         : Tile::Grass;
        }
    }

    for (int pond = 0; pond < 24; ++pond)
    {
        int cx = 7 + random() % (Width - 14);
        int cy = 7 + random() % (Height - 14);
        float rx = 3.f + random() % 8;
        float ry = 3.f + random() % 8;

        for (int y = std::max(1, cy - 11); y < std::min(Height - 1, cy + 12); ++y)
        {
            for (int x = std::max(1, cx - 11); x < std::min(Width - 1, cx + 12); ++x)
            {
                float dx = (x - cx) / rx;
                float dy = (y - cy) / ry;

                if (dx * dx + dy * dy < 1)
                {
                    cells[y * Width + x] = Tile::Water;
                }
            }
        }
    }

    for (int y = Index(Camp()) / Width - 4; y <= Index(Camp()) / Width + 4; ++y)
    {
        for (int x = Index(Camp()) % Width - 4; x <= Index(Camp()) % Width + 4; ++x)
        {
            cells[y * Width + x] = Tile::Dirt;
        }
    }

    villages = {{"refugee_camp", "불씨 피난민 캠프", "부족의 불씨·휴식·소식 수집", Camp(), false},
        {"homeland_ruins", "옛 고향의 폐허", "피난 흔적·가족 행방 조사", {-204.5f, 168.5f}, true},
        {"road_market", "길목 교역마을 (임시)", "교역·호위·산적 위협", {-12.5f, -2.5f}, false},
        {"other_refugees", "갈림길 정착지 (임시)", "다른 피난민·물자 교섭", {110.5f, 56.5f}, false},
        {"first_fire_village",
            "첫 불의 계곡 주민 마을",
            "기존 주민의 삶·터전·정착 갈등",
            {206.5f, -180.5f},
            false}};
    buildings.clear();
    auto paint = [&](int x, int y)
    {
        if (x > 0 && x < Width - 1 && y > 0 && y < Height - 1)
            cells[y * Width + x] = Tile::Dirt;
    };
    // All settlements have clear plazas and three-cell-wide land routes.
    for (const auto& village : villages)
    {
        int cx = Index(village.center) % Width, cy = Index(village.center) / Width;
        for (int y = cy - 7; y <= cy + 7; ++y)
            for (int x = cx - 7; x <= cx + 7; ++x)
                paint(x, y);
    }
    const int journey[] = {1, 0, 2, 3, 4};
    for (int step = 0; step < 4; ++step)
    {
        int source = Index(villages[journey[step]].center),
            destination = Index(villages[journey[step + 1]].center);
        int x = source % Width, y = source / Width, tx = destination % Width, ty = destination / Width;
        while (x != tx)
        {
            for (int dy = -1; dy <= 1; ++dy)
                paint(x, y + dy);
            x += x < tx ? 1 : -1;
        }
        while (y != ty)
        {
            for (int dx = -1; dx <= 1; ++dx)
                paint(x + dx, y);
            y += y < ty ? 1 : -1;
        }
    }
    for (size_t i = 0; i < villages.size(); ++i)
    {
        const auto& village = villages[i];
        for (Point offset : {Point{-5.f, -5.f}, Point{4.f, -5.f}, Point{-5.f, 4.f}, Point{4.f, 4.f}})
        {
            Point position = {village.center.x + offset.x, village.center.y + offset.y};
            int cell = Index(position);
            buildings.push_back({position, static_cast<int>(i)});
            // Visual cabins occupy the same two-by-two base in collision data.
            cells[cell] = cells[cell + 1] = cells[cell + Width] = cells[cell + Width + 1] = Tile::Rock;
        }
    }

    std::vector<bool> buildingFootprint(Count, false);
    for (const auto& building : buildings)
    {
        int cell = Index(building.position);
        buildingFootprint[cell] = buildingFootprint[cell + 1] = buildingFootprint[cell + Width] =
            buildingFootprint[cell + Width + 1] = true;
    }
    // Connect disconnected land components using one visited mask.
    auto reached = Distances(Index(Camp()));
    int root = Index(Camp()), rootX = root % Width, rootY = root / Width;
    for (int isolated = 0; isolated < Count; ++isolated)
    {
        if (!Open(isolated) || reached[isolated] >= 0)
            continue;
        int x = isolated % Width, y = isolated / Width;
        while (x != rootX)
        {
            if (!buildingFootprint[y * Width + x])
                cells[y * Width + x] = Tile::Dirt;
            x += x < rootX ? 1 : -1;
        }
        while (y != rootY)
        {
            if (!buildingFootprint[y * Width + x])
                cells[y * Width + x] = Tile::Dirt;
            y += y < rootY ? 1 : -1;
        }
        std::queue<int> pending;
        pending.push(isolated);
        reached[isolated] = 0;
        while (!pending.empty())
        {
            int cell = pending.front();
            pending.pop();
            for (int next : {cell - 1, cell + 1, cell - Width, cell + Width})
            {
                if (next < 0 || next >= Count || !Open(next) || reached[next] >= 0)
                    continue;
                if (std::abs(next % Width - cell % Width) + std::abs(next / Width - cell / Width) != 1)
                    continue;
                reached[next] = 0;
                pending.push(next);
            }
        }
    }

    assert(IsConnected());
    openCells.clear();

    for (int i = 0; i < Count; ++i)
    {
        if (Open(i))
        {
            openCells.push_back(i);
        }
    }
}

const std::vector<LevelMap::Village>& LevelMap::Villages() const
{
    return villages;
}

const std::vector<LevelMap::Building>& LevelMap::Buildings() const
{
    return buildings;
}

int LevelMap::VillageAt(Point position) const
{
    for (size_t i = 0; i < villages.size(); ++i)
    {
        const auto& center = villages[i].center;
        if (std::fabs(position.x - center.x) <= 8 && std::fabs(position.y - center.y) <= 8)
            return static_cast<int>(i);
    }
    return -1;
}

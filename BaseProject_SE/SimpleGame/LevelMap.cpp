#include "stdafx.h"
#include "LevelMap.h"
#include <random>
#include <queue>
#include <cmath>
#include <cassert>

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
    return Center((Height / 2) * Width + Width / 2);
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

std::array<int, LevelMap::Count> LevelMap::Distances(int start) const
{
    std::array<int, Count> result;
    result.fill(-1);

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

Point LevelMap::NextStep(Point from, Point target, const std::array<int, Count>& field) const
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

    for (int pond = 0; pond < 5; ++pond)
    {
        int cx = 7 + random() % (Width - 14);
        int cy = 7 + random() % (Height - 14);
        float rx = 2.f + random() % 4;
        float ry = 2.f + random() % 4;

        for (int y = 1; y < Height - 1; ++y)
        {
            for (int x = 1; x < Width - 1; ++x)
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

    for (int y = Height / 2 - 4; y <= Height / 2 + 4; ++y)
    {
        for (int x = Width / 2 - 4; x <= Width / 2 + 4; ++x)
        {
            cells[y * Width + x] = Tile::Dirt;
        }
    }

    // Join every isolated land component to the camp with a traversable land bridge.
    while (true)
    {
        auto reachable = Distances(Index(Camp()));
        int isolated = -1;

        for (int i = 0; i < Count; ++i)
        {
            if (Open(i) && reachable[i] < 0)
            {
                isolated = i;
                break;
            }
        }

        if (isolated < 0)
        {
            break;
        }

        int x = isolated % Width;
        int y = isolated / Width;

        while (x != Width / 2)
        {
            cells[y * Width + x] = Tile::Dirt;
            x += x < Width / 2 ? 1 : -1;
        }

        while (y != Height / 2)
        {
            cells[y * Width + x] = Tile::Dirt;
            y += y < Height / 2 ? 1 : -1;
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

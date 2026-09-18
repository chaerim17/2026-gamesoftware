#include "stdafx.h"
#include "PlayerProgress.h"
#include "GameStorage.h"
#include <cstring>

namespace
{
    int Requirement(int level)
    {
        return 100 + 50 * (level - 1);
    }
}

int PlayerProgress::Level() const
{
    int level = 1;
    int remaining = totalExperience;

    while (level < 50 && remaining >= Requirement(level))
    {
        remaining -= Requirement(level);
        ++level;
    }

    return level;
}

int PlayerProgress::ExperienceInLevel() const
{
    int remaining = totalExperience;

    for (int level = 1; level < Level(); ++level)
    {
        remaining -= Requirement(level);
    }

    return remaining;
}

int PlayerProgress::NextExperience() const
{
    return Requirement(Level());
}

int PlayerProgress::AvailablePoints() const
{
    return (Level() - 1) * 3 - strength - vitality - toughness;
}

int PlayerProgress::MaxHealth() const
{
    return 100 + (Level() - 1) * 20 + vitality * 10;
}

int PlayerProgress::Attack() const
{
    return 18 + (Level() - 1) * 4 + strength * 2;
}

int PlayerProgress::Defense() const
{
    return 3 + (Level() - 1) + toughness;
}

int PlayerProgress::AddExperience(int amount)
{
    int before = Level();

    if (amount > 0 && amount <= 10000)
    {
        totalExperience = totalExperience > 10000000 - amount ? 10000000 : totalExperience + amount;
    }

    return Level() - before;
}

bool PlayerProgress::SpendPoint(int attribute)
{
    if (AvailablePoints() <= 0 || attribute < 0 || attribute > 2)
    {
        return false;
    }

    if (attribute == 0)
    {
        ++strength;
    }
    else if (attribute == 1)
    {
        ++vitality;
    }
    else
    {
        ++toughness;
    }

    return true;
}

bool PlayerProgress::Save() const
{
    uint32_t values[] = {static_cast<uint32_t>(totalExperience),
        static_cast<uint32_t>(strength),
        static_cast<uint32_t>(vitality),
        static_cast<uint32_t>(toughness),
        static_cast<uint32_t>(herbs),
        static_cast<uint32_t>(hides),
        static_cast<uint32_t>(kills),
        static_cast<uint32_t>(collected)};
    std::vector<unsigned char> bytes(sizeof(values));
    std::memcpy(bytes.data(), values, sizeof(values));

    return GameStorage::Write(L"progress.save", 1, bytes);
}

bool PlayerProgress::Load()
{
    std::vector<unsigned char> bytes;

    if (!GameStorage::Read(L"progress.save", 1, 32, bytes) || bytes.size() != 32)
    {
        return false;
    }

    uint32_t values[8] = {};
    std::memcpy(values, bytes.data(), sizeof(values));

    if (values[0] > 10000000 || values[1] > 147 || values[2] > 147 || values[3] > 147 || values[4] > 999 ||
        values[5] > 999999 || values[6] > 999999 || values[7] > 999999)
    {
        return false;
    }

    PlayerProgress candidate;
    candidate.totalExperience = values[0];
    candidate.strength = values[1];
    candidate.vitality = values[2];
    candidate.toughness = values[3];
    candidate.herbs = values[4];
    candidate.hides = values[5];
    candidate.kills = values[6];
    candidate.collected = values[7];

    if (candidate.AvailablePoints() < 0)
    {
        return false;
    }

    *this = candidate;
    return true;
}

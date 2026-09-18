#pragma once

class PlayerProgress
{
  public:
    bool Load();
    bool Save() const;
    int AddExperience(int amount);
    bool SpendPoint(int attribute);
    int Level() const;
    int ExperienceInLevel() const;
    int NextExperience() const;
    int AvailablePoints() const;
    int MaxHealth() const;
    int Attack() const;
    int Defense() const;

    int totalExperience = 0;
    int strength = 0;
    int vitality = 0;
    int toughness = 0;
    int herbs = 3;
    int hides = 0;
    int kills = 0;
    int collected = 0;
};

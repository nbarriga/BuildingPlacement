#pragma once

#include "Common.h"
#include "Player.h"

namespace BuildingPlacement {
class Player_Goal : public Player
{
protected:
    Position _goal;
public:
    Player_Goal (const IDType & playerID, const Position& goal);
    const Position& getGoal() const { return _goal;};
    void setGoal(const Position& goal) { _goal=goal;};
};
}

#pragma once

#include "Common.h"
#include "Player.h"

namespace BuildingPlacement {
/*----------------------------------------------------------------------
 | Attack HighestDPS Player No Overkill
 |----------------------------------------------------------------------
 | Chooses an action with following priority:
 | 1) If it can attack, ATTACK highest DPS/HP enemy unit to overkill
 | 2) If it cannot attack:
 |    a) If it is in range to attack an enemy, WAIT until attack
 |    b) If it is not in range of enemy, MOVE towards closest
 `----------------------------------------------------------------------*/
class Player_Defend : public Player
{
    Position _goal;
public:
    static const std::string modelString;
	Player_Defend (const IDType & playerID, const Position& goal);
	void getMoves(const GameState & state, const MoveArray & moves, std::vector<UnitAction> & moveVec);
	IDType getType() { return PlayerModels::Defend; }
	const Position& getGoal() const { return _goal;};
};
}

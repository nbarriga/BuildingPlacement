#pragma once

#include "Common.h"
#include "Player_Goal.h"

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
class Player_Assault : public Player_Goal
{
//    static const int N=10;
//    Position _lastMyPos[N][Constants::Max_Units];
public:
    static const std::string modelString;
	Player_Assault (const IDType & playerID, const Position& goal);
	void getMoves(const GameState & state, const MoveArray & moves, std::vector<UnitAction> & moveVec);
	IDType getType() { return PlayerModels::Assault; }
//	void reset(){
//	    for(int i=0;i<N;i++){
//	        std::fill(_lastMyPos[i], _lastMyPos[i]+Constants::Max_Units, Position(0,0));
//	    }
//	}
};
}

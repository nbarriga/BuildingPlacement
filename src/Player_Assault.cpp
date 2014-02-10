#include "Player_Assault.h"

namespace BuildingPlacement {
const std::string Player_Assault::modelString = "Assault";

Player_Assault::Player_Assault (const IDType & playerID, const Position& goal):_goal(goal){
	_playerID=playerID;

}

void Player_Assault::getMoves(const GameState & state, const MoveArray & moves, std::vector<UnitAction> & moveVec)
{
    moveVec.clear();
	IDType enemy(state.getEnemy(_playerID));

	Array<int, Constants::Max_Units> hpRemaining;

	for (IDType u(0); u<state.numUnits(enemy); ++u)
	{
		hpRemaining[u] = state.getUnit(enemy,u).currentHP();
	}

	for (IDType u(0); u<moves.numUnits(); ++u)
	{
		bool foundUnitAction						(false);
		size_t actionMoveIndex					(0);
		double actionHighestDPS					(0);
		size_t closestMoveIndex					(0);
		int closestMoveDist		(std::numeric_limits<int>::max());
		
		const SparCraft::Unit & ourUnit				(state.getUnit(_playerID, u));

//		size_t mdist	= state.getMap().getDistanceToGoal(ourUnit.pos());
//		std::cout<<"Unit: "<<ourUnit.name()<<" dist to goal: "<<mdist<<
//				" pos: "<<ourUnit.x()<<" "<<ourUnit.y()<<
//				" range: "<<ourUnit.range()<<
//				" size:"<<ourUnit.type().dimensionLeft()<<" "<<ourUnit.type().dimensionRight()<<std::endl;
		for (size_t m(0); m<moves.numMoves(u); ++m)
		{
			const UnitAction move						(moves.getMove(u, m));
				
			if ((move.type() == UnitActionTypes::ATTACK) && (hpRemaining[move._moveIndex] > 0))
			{
				const SparCraft::Unit & target				(state.getUnit(state.getEnemy(move.player()), move._moveIndex));
				if(target.type().canAttack()){
					double dpsHPValue =				(target.dpf() / hpRemaining[move._moveIndex]);
					//				std::cout<<"Attack: "<<target.name()<<std::endl;
					if (dpsHPValue > actionHighestDPS)
					{
						actionHighestDPS = dpsHPValue;
						actionMoveIndex = m;
						foundUnitAction = true;
					}

					if (move._moveIndex >= state.numUnits(enemy))
					{
//						int e = enemy;
//						int pl = _playerID;
						printf("wtf\n");
					}
				}else{
					if(actionHighestDPS==0){
						actionHighestDPS = 0.01;
						actionMoveIndex = m;
						foundUnitAction = true;
					}
				}
			}
			else if (move.type() == UnitActionTypes::HEAL)
			{
				const SparCraft::Unit & target				(state.getUnit(move.player(), move._moveIndex));
				double dpsHPValue =				(target.dpf() / hpRemaining[move._moveIndex]);
//				std::cout<<"Heal: "<<target.name()<<std::endl;
				if (dpsHPValue >= actionHighestDPS)
				{
					actionHighestDPS = dpsHPValue;
					actionMoveIndex = m;
					foundUnitAction = true;
				}
			}
			else if (move.type() == UnitActionTypes::RELOAD)
			{
//				std::cout<<"Reload"<<std::endl;
				const boost::optional<const Unit&> & closestUnitOpt = state.getClosestEnemyUnitOpt(_playerID, u);
				if (closestUnitOpt.is_initialized()&&
						ourUnit.canAttackTarget(closestUnitOpt.get(), state.getTime()))
				{
					closestMoveIndex = m;
					break;
				}
			}
			else if (move.type() == UnitActionTypes::MOVE)
			{
				Position ourDest=move.pos();
				int dist(std::numeric_limits<int>::max());
				if(ourUnit.canHeal()){//medic
					const boost::optional<const Unit&> & closestWoundedOpt	=state.getClosestOurWoundedUnitOpt(_playerID, u);
					if(closestWoundedOpt.is_initialized()&&
							closestWoundedOpt.get().previousAction().type()!=UnitActionTypes::MOVE)
					{
						//this slows down things considerably
						dist = state.getMap().getDistance(ourDest,closestWoundedOpt.get().pos());
					}
					else if(closestWoundedOpt.is_initialized()&&
							state.getMap().canWalkStraight(ourDest,closestWoundedOpt.get().pos(), ourUnit.range()))
					{
						dist = sqrt(closestWoundedOpt.get().getDistanceSqToPosition(ourDest, state.getTime()));
					}
					else
					{
						dist = state.getMap().getDistance(ourDest,_goal);//walk towards goal?
					}
				}else{//not a medic
					const boost::optional<const Unit&> & closestBuildingOpt	=state.getClosestEnemyBuildingOpt(_playerID, u);
					if(closestBuildingOpt.is_initialized()){
						int distBuilding = state.getMap().getDistance(ourDest,closestBuildingOpt.get().pos());
						int distGoal = state.getMap().getDistance(ourDest,_goal);
						dist = ((distBuilding>0 && distBuilding<distGoal) ||
								closestBuildingOpt.get().canAttackTarget(ourUnit, state.getTime()))?distBuilding:distGoal;
					}else{//no enemy buildings alive
						const boost::optional<const Unit&> & closestEnemyOpt	=state.getClosestEnemyUnitOpt(_playerID, u);
						if(closestEnemyOpt.is_initialized()&&
								closestEnemyOpt.get().canAttackTarget(ourUnit, state.getTime())){//move towards unit that can hurt us
							if(state.getMap().canWalkStraight(ourDest,closestEnemyOpt.get().pos(), ourUnit.range())){
								dist = sqrt(closestEnemyOpt.get().getDistanceSqToPosition(ourDest, state.getTime()));
							}else{//no direct line towards enemy, lets go to goal
								dist = state.getMap().getDistance(ourDest,_goal);//walk towards goal?
							}

						}else{//closest enemy can't hurt us, move towards goal
							dist = state.getMap().getDistance(ourDest,_goal);
						}
					}
				}
				if (dist < closestMoveDist)
				{
					closestMoveDist = dist;
					closestMoveIndex = m;
				}
			}
		}

		size_t bestMoveIndex(foundUnitAction ? actionMoveIndex : closestMoveIndex);

		UnitAction theMove(moves.getMove(u, actionMoveIndex));
		if (theMove.type() == UnitActionTypes::ATTACK)
		{
			hpRemaining[theMove.index()] -= state.getUnit(_playerID, theMove.unit()).damage();
		}
			
		moveVec.push_back(moves.getMove(u, bestMoveIndex));
	}
}
}

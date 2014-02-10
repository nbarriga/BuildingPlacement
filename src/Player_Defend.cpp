#include "Player_Defend.h"

namespace BuildingPlacement {

const std::string Player_Defend::modelString = "Defend";

Player_Defend::Player_Defend (const IDType & playerID, const Position& goal):_goal(goal){
	_playerID=playerID;
}

void Player_Defend::getMoves(const GameState & state, const MoveArray & moves, std::vector<UnitAction> & moveVec)
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
		bool foundUnitMove						(false);
		size_t actionMoveIndex					(0);
		double actionHighestDPS					(0);
		size_t closestMoveIndex					(0);
		unsigned long long closestMoveDist		(std::numeric_limits<unsigned long long>::max());
		
		const SparCraft::Unit & ourUnit				(state.getUnit(_playerID, u));

//		size_t mdist	= state.getMap().getDistanceToGoal(ourUnit.pos());
//		std::cout<<"Unit: "<<ourUnit.name()<<" dist to goal: "<<mdist<<
//						" pos: "<<ourUnit.x()<<" "<<ourUnit.y()<<
//						" range: "<<ourUnit.range()<<
//						" size:"<<ourUnit.type().dimensionLeft()<<" "<<ourUnit.type().dimensionRight()<<std::endl;
		for (size_t m(0); m<moves.numMoves(u); ++m)
		{
			const UnitAction move						(moves.getMove(u, m));
				
			if ((move.type() == UnitActionTypes::ATTACK) && (hpRemaining[move._moveIndex] > 0))
			{
				const SparCraft::Unit & target				(state.getUnit(state.getEnemy(move.player()), move._moveIndex));
				double dpsHPValue =				(target.dpf() / hpRemaining[move._moveIndex]);

				if (dpsHPValue > actionHighestDPS)
				{
					actionHighestDPS = dpsHPValue;
					actionMoveIndex = m;
					foundUnitAction = true;
				}

                if (move._moveIndex >= state.numUnits(enemy))
                {
//                    int e = enemy;
//                    int pl = _playerID;
                    printf("wtf\n");
                }
			}
			else if (move.type() == UnitActionTypes::HEAL)
			{
				const SparCraft::Unit & target				(state.getUnit(move.player(), move._moveIndex));
				double dpsHPValue =				(target.dpf() / hpRemaining[move._moveIndex]);

				if (dpsHPValue > actionHighestDPS)
				{
					actionHighestDPS = dpsHPValue;
					actionMoveIndex = m;
					foundUnitAction = true;
				}
			}
			else if (move.type() == UnitActionTypes::RELOAD)
			{
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
				size_t dist=INT_MAX;
				const boost::optional<const Unit&> & closestUnitOpt = state.getClosestEnemyUnitOpt(_playerID, u);

				if (closestUnitOpt.is_initialized()&&
						closestUnitOpt.get().previousAction().type()!=UnitActionTypes::MOVE)
				{
					//this slows down things considerably
					dist = state.getMap().getDistance(ourDest,closestUnitOpt.get().pos());
				}
				else if (closestUnitOpt.is_initialized()&&
						state.getMap().canWalkStraight(ourDest,closestUnitOpt.get().pos(), ourUnit.range()))
				{
					dist = sqrt(closestUnitOpt.get().getDistanceSqToPosition(ourDest, state.getTime()));
				}
				else
				{
					const boost::optional<const Unit&> & closestDamagedBuildingOpt=state.getClosestOurDamagedBuildingOpt(_playerID, u);
					if(closestDamagedBuildingOpt.is_initialized()){
						dist = state.getMap().getDistance(ourDest,closestDamagedBuildingOpt.get().pos());
					}else{
						const boost::optional<const Unit&> & closestOurBuildingOpt	= state.getClosestOurBuildingOpt(_playerID, u);
						if(closestOurBuildingOpt.is_initialized()){
							dist = state.getMap().getDistance(ourDest,closestOurBuildingOpt.get().pos());
							if(dist<1){//no path to nearest building?
								//stay put
								std::cerr<<"eh?";
							}
						}
					}
				}

				if (dist < closestMoveDist)
				{
					closestMoveDist = dist;
					closestMoveIndex = m;
					foundUnitMove=true;
				}

			}
		}



		UnitAction theMove(moves.getMove(u, actionMoveIndex));
		if (theMove.type() == UnitActionTypes::ATTACK)
		{
			hpRemaining[theMove.index()] -= state.getUnit(_playerID, theMove.unit()).damage();
		}
//		size_t bestMoveIndex(foundUnitAction ? actionMoveIndex : closestMoveIndex);
		if(foundUnitAction){
			moveVec.push_back(moves.getMove(u, actionMoveIndex));
		}else if(foundUnitMove){
			moveVec.push_back(moves.getMove(u, closestMoveIndex));
		}else{
			moveVec.push_back(UnitAction(u, _playerID, UnitActionTypes::PASS, 0));
		}
	}
}
}

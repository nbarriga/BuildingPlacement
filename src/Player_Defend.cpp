#include "Player_Defend.h"

namespace BuildingPlacement {

const std::string Player_Defend::modelString = "Defend";

Player_Defend::Player_Defend (const IDType & playerID, const Position& goal):Player_Goal(playerID,goal){
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
			    if(ourUnit.canHeal()){//medic
			        const boost::optional<const Unit&> & closestWoundedOpt  =state.getClosestOurWoundedUnitOpt(_playerID, u);
			        int d(std::numeric_limits<int>::max()) ;
			        if(closestWoundedOpt.is_initialized()&&
			                state.getMap().canWalkStraight(ourDest,closestWoundedOpt.get().pos(), ourUnit.range()))
			        {
			            d = sqrt(closestWoundedOpt.get().getDistanceSqToPosition(ourDest, state.getTime()));
			        }
			        else if(closestWoundedOpt.is_initialized()&&
			                closestWoundedOpt.get().previousAction().type()!=UnitActionTypes::MOVE)
			        {
			            //this slows down things considerably
			            d = state.getMap().getDistance(ourDest,closestWoundedOpt.get().pos());
			        }else{
			            //closest wounded unit is moving
			        }
			        if(d<10*TILE_SIZE){
			            dist=d;
			        }
			        else
			        {
			            dist = state.getMap().getDistance(ourDest,_goal);
			        }
			    }else{//not a medic
			        const boost::optional<const Unit&> & closestEnemyOpt = state.getClosestEnemyUnitOpt(_playerID, u);

			        if (closestEnemyOpt.is_initialized()&&
			                //closestEnemyOpt->previousAction().type()!=UnitActionTypes::MOVE&&
			                !state.getAliveUnitsInCircleIDs(_playerID,closestEnemyOpt->currentPosition(state.getTime()),closestEnemyOpt->range()*2).empty()){
			                dist = state.getMap().getDistance(ourDest,closestEnemyOpt->currentPosition(state.getTime()));
			        }else{
			            const boost::optional<const Unit&> & closestDamagedBuildingOpt=state.getClosestOurDamagedBuildingOpt(_playerID, u);
			            if(closestDamagedBuildingOpt.is_initialized()){
			                int d = state.getMap().getDistance(ourDest,closestDamagedBuildingOpt.get().pos());
			               // if(d<10*TILE_SIZE){
			                    dist=d;
			               // }else{
			               //     dist = state.getMap().getDistance(ourDest,_goal);
			               // }
			            }else{
			                dist = state.getMap().getDistance(ourDest,_goal);
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

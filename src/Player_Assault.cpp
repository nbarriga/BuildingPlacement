#include "Player_Assault.h"

namespace BuildingPlacement {
const std::string Player_Assault::modelString = "Assault";

Player_Assault::Player_Assault (const IDType & playerID, const Position& goal):Player_Goal(playerID,goal){

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
		bool foundUnitMove                        (false);
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
			    const SparCraft::Unit & target              (state.getUnit(state.getEnemy(move.player()), move._moveIndex));

			    if(target.damage()>0 || !state.getClosestEnemyThreatOpt(_playerID, u).is_initialized()){
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
//			    bool repeat=false;
//			    for(int i=0;i<N;i++){
//			        if(ourDest==_lastMyPos[i][u]){
//			            repeat=true;
//			        }
//			    }
//			    if(repeat){
//			        dist=std::numeric_limits<int>::max();
//			    }else{

			        if(ourUnit.canHeal()){//medic
			            const boost::optional<const Unit&> & closestWoundedOpt  =state.getClosestOurWoundedUnitOpt(_playerID, u);
			            if(closestWoundedOpt.is_initialized()&&
			                    state.getMap().canWalkStraight(ourDest,closestWoundedOpt.get().pos(), ourUnit.range()))
			            {
			                dist = sqrt(closestWoundedOpt.get().getDistanceSqToPosition(ourDest, state.getTime()));
			            }
			            else if(closestWoundedOpt.is_initialized()&&
			                    closestWoundedOpt.get().previousAction().type()!=UnitActionTypes::MOVE)
			            {
			                //this slows down things considerably
			                dist = state.getMap().getDistance(ourDest,closestWoundedOpt.get().pos());
			            }
			            else
			            {
			                //dist = state.getMap().getDistanceToGoal(ourDest);//walk towards goal?
			                dist = state.getMap().getDistance(ourDest,_goal);
			            }
			        }else{//not a medic
			            const boost::optional<const Unit&> & closestEnemyOpt=state.getClosestEnemyThreatOpt(_playerID, u);
			            if(closestEnemyOpt.is_initialized()){
			                dist = state.getMap().getDistance(ourDest,closestEnemyOpt->currentPosition(state.getTime()));
			            }else {
//			                std::vector<IDType> ids=state.getAliveUnitIDs(_playerID);
//			                int ourDist=state.getMap().getDistance(ourUnit.currentPosition(state.getTime()),_goal);
//			                int max=std::numeric_limits<int>::min(),min=std::numeric_limits<int>::max();
//			                BOOST_FOREACH(const IDType &id,ids){
//			                    int dd=state.getMap().getDistance(state.getUnitByID(_playerID,id).currentPosition(state.getTime()),_goal);
//			                    if(dd>max){
//			                        max=dd;
//			                    }
//			                    if(dd<min){
//			                        min=dd;
//			                    }
//			                }
			                //			            std::cout<<min<<" "<<max<<" "<<ourDist<<std::endl;
//			                if((max-min)> 16 && (ourDist-min)< 4){
//			                    dist=std::numeric_limits<int>::max();
//			                }else{
			                    const boost::optional<const Unit&> & closestEnemyOpt=state.getClosestEnemyUnitOpt(_playerID, u);
			                    if(closestEnemyOpt.is_initialized()&&
			                            closestEnemyOpt->previousAction().type()!=UnitActionTypes::MOVE){//move towards unit that can hurt us
			                        //			                if(state.getMap().canWalkStraight(ourDest,closestEnemyOpt->pos(), ourUnit.range())){
			                        //			                    dist = sqrt(closestEnemyOpt.get().getDistanceSqToPosition(ourDest, state.getTime()));
			                        //			                }else{
			                        dist = state.getMap().getDistance(ourDest,closestEnemyOpt->currentPosition(state.getTime()));

			                        //			                }

			                    }else{//closest enemy can't hurt us, move towards goal
			                        //			                if(state.getMap().canWalkStraight(ourDest,_goal,0)){
			                        //			                    dist = ourDest.getDistance(_goal);
			                        //			                }else{
			                        dist = state.getMap().getDistance(ourDest,_goal);
			                        //			                }
			                    }
//			                }
			            }
			        }
//			    }
				if (dist < closestMoveDist)
				{
					closestMoveDist = dist;
					closestMoveIndex = m;
					foundUnitMove = true;
				}
			}
		}

		if(foundUnitAction){
		    UnitAction theMove(moves.getMove(u, actionMoveIndex));
		    if (theMove.type() == UnitActionTypes::ATTACK)
		    {
//		        reset();
		        hpRemaining[theMove.index()] -= state.getUnit(_playerID, theMove.unit()).damage();
		    }

		    moveVec.push_back(moves.getMove(u, actionMoveIndex));
		}else if(foundUnitMove){
		    moveVec.push_back(moves.getMove(u, closestMoveIndex));
		}else{//pass
		    moveVec.push_back(UnitAction(u,_playerID,UnitActionTypes::PASS,0));
		}
//		for(int i=N-1;i>0;i--){
//		    _lastMyPos[i][u]=_lastMyPos[i-1][u];
//		}
//		_lastMyPos[0][u]=ourUnit.currentPosition(state.getTime());
	}
}
}

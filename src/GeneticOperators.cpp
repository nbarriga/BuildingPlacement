/*
 * GeneticOperators.cpp
 *
 *  Created on: Jul 26, 2013
 *      Author: nbarriga
 */

#include "GeneticOperators.h"
#include "Gene.h"
#include "Player_Assault.h"
#include "Player_Defend.h"
#include "Spiral.h"
#include "BuildingPlacementExperiment.h"

namespace BuildingPlacement {
std::vector<SparCraft::Unit> GeneticOperators::_fixedBuildings=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_buildings=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_defenders=std::vector<SparCraft::Unit>();
std::vector<std::vector<SparCraft::Unit> > GeneticOperators::_attackers=std::vector<std::vector<SparCraft::Unit> >();
std::vector<std::pair<Unit, TimeType> > GeneticOperators::_delayedDefenders=std::vector<std::pair<Unit, TimeType> >();
std::vector<std::vector<std::pair<Unit, TimeType> > > GeneticOperators::_delayedAttackers=std::vector<std::vector<std::pair<Unit, TimeType> > >();
Map* GeneticOperators::_map=NULL;
Display* GeneticOperators::_display=NULL;
boost::shared_ptr<Player_Assault> GeneticOperators::_assaultPlayer;
boost::shared_ptr<Player_Defend> GeneticOperators::_defendPlayer;
svv GeneticOperators::_expDesc=svv();

int GeneticOperators::_baseLeft, GeneticOperators::_baseRight, GeneticOperators::_baseTop, GeneticOperators::_baseBottom;
bool GeneticOperators::_seedOriginalPosition;

const int mutDistance=5;
const int placementRetries=50;
const int minBaseSize=20;
const int repairRadius=5;

float GeneticOperators::Objective(GAGenome &g) {
	std::cout<<"calling Objective\n";


	GAListGenome<Gene>& genome=(GAListGenome<Gene>&)g;
	std::cout<<"genome: "<<genome<<std::endl;
	int total_score=0;
	std::vector<int> scores;
	for(int wave=0;wave<_attackers.size();wave++){
	    GameState state;
	    state.checkCollisions=true;
	    state.setMap(*_map);
	    for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings.begin();
	            it!=_fixedBuildings.end();it++){
	        assert(it->player()==_defendPlayer->ID());
	        //		std::cout<<"defender unit: "<<it->type().getName()<<std::endl;
	        state.addUnit(*it);
	    }
	    for(int i=0;i<genome.size();i++){
	        Gene *gene=genome[i];
	        //		std::cout<<"building unit: "<<gene->getType().getName()<<" "<<gene->getType().dimensionLeft()<<" "
	        //		        <<gene->getType().dimensionRight()<<" "<<gene->getType().dimensionUp()<<" "<<gene->getType().dimensionDown()<<std::endl;


	        Unit unit(gene->getType(),
	                gene->getCenterPos(),
	                0,
	                _defendPlayer->ID(),
	                _buildings[i].currentHP(),
	                gene->getType() == BWAPI::UnitTypes::Terran_Medic ? Constants::Starting_Energy : 0,
	                        _buildings[i].nextMoveActionTime(),
	                        _buildings[i].nextAttackActionTime());
	        state.addUnit(unit);
	        //		state.addUnit(gene->getType(),_defendPlayer->ID(),gene->getCenterPos());
	    }
	    for(std::vector<SparCraft::Unit>::const_iterator it=_defenders.begin();
	            it!=_defenders.end();it++){
	        assert(it->player()==_defendPlayer->ID());
	        //		std::cout<<"defender unit: "<<it->type().getName()<<std::endl;
	        state.addUnitClosestLegalPos(*it);
	    }
	    for(std::vector<SparCraft::Unit>::const_iterator it=_attackers[wave].begin();
	            it!=_attackers[wave].end();it++){
	        assert(it->player()==_assaultPlayer->ID());
	        //		std::cout<<"attacker unit: "<<it->type().getName()<<std::endl;
	        state.addUnitClosestLegalPos(*it);
	    }
	    //todo: check that defenders and attackers are placed in legal locations, otherwise move them

	    boost::shared_ptr<Player> p1,p2;
	    if(_assaultPlayer->ID()==0){
	        p1=_assaultPlayer;
	        p2=_defendPlayer;
	    }else{
	        p2=_assaultPlayer;
	        p1=_defendPlayer;
	    }

	    std::vector<std::pair<Unit, TimeType> > delayedUnits(
	            _delayedAttackers[wave].size()+_delayedDefenders.size());
	    std::merge(_delayedAttackers[wave].begin(),_delayedAttackers[wave].end(),
	            _delayedDefenders.begin(),_delayedDefenders.end(),
	            delayedUnits.begin(), BuildingPlacementExperiment::Comparison());

	    Game game(state, p1, p2, 20000, delayedUnits);

#ifdef USING_VISUALIZATION_LIBRARIES
	    if (_display!=NULL)
	    {
	        game.disp = _display;
	        _display->SetExpDesc(_expDesc);

	    }
#endif

	    // play the game to the end
	    game.play();
	    int score = evalBuildingPlacement(game.getState());
	    std::cout<<"score: "<<score<<std::endl;
	    total_score+=score;
	    scores.push_back(score);
	}
	std::cout<<"total score: "<<total_score<<std::endl;
//	return total_score/_attackers.size();
	return *std::min_element(scores.begin(),scores.end());
}

bool GeneticOperators::goalReached(const GameState& state){
      for (IDType u(0); u<state.numUnits(_assaultPlayer->ID()); ++u){
          const Unit & unit(state.getUnit(_assaultPlayer->ID(), u));
          if(_map->getDistance(unit.pos(),_assaultPlayer->getGoal())<TILE_SIZE/4){
              return true;
          }
      }
  return false;
}

ScoreType GeneticOperators::unitScore(const Unit& unit){
    ScoreType hpPercent=(unit.currentHP()*100)/(unit.maxHP());
    ScoreType cost=unit.type().mineralPrice()+1.5f*unit.type().gasPrice();
    ScoreType val=cost*hpPercent;
    if(unit.type().isWorker()){
        val*=2;
    }else if(unit.type()==BWAPI::UnitTypes::Protoss_Pylon){
        val*=3;
    }
    return val;
}
ScoreType GeneticOperators::unitScore(const GameState& state,
        IDType player){

    std::vector<IDType> units=state.getAliveUnitIDs(player);
    ScoreType score=0;
    BOOST_FOREACH(const IDType &id,units){
        const Unit &u=state.getUnitByID(player,id);
        score+=unitScore(u);
//        std::cout<<hpPercent<<" "<<cost<<" "<<val<<std::endl;
    }

    return score;
}
ScoreType GeneticOperators::evalBuildingPlacement(const GameState& state){

//    std::cout<<"att"<<std::endl;
    ScoreType attValue=unitScore(state,_assaultPlayer->ID());
//    std::cout<<"def"<<std::endl;
    ScoreType defValue=unitScore(state,_defendPlayer->ID());

    const ScoreType constant = 10000000;

  if(state.playerDead(_assaultPlayer->ID())){//attacker defeated, count how many we have left
      std::cout<<"Attacker destroyed"<<std::endl;
      return defValue+constant;
//  }else if(goalReached(state)){//enemy reached goal,
//      std::cout<<"Attacker reached goal"<<std::endl;
//      return /*defValue*/-attValue+1000000;
  }else if(state.playerDead(_defendPlayer->ID())){//defender destroyed, count how many he has left
      std::cout<<"Defender destroyed"<<std::endl;
      return constant-attValue;
  }else{//simulation time exhausted
	  std::cerr<<"Simulation timeout, something wrong!!!!"<<std::endl;
      System::FatalError("Simulation timeout");
//      return defValue-attValue+2000000;
  }
}
bool GeneticOperators::defenderWon(const GameState& state){
    if(state.playerDead(_assaultPlayer->ID())){
        return true;
//    }else if(goalReached(state)){
//        return false;
    }else if(state.playerDead(_defendPlayer->ID())){
        return false;
    }else{//simulation time exhausted
        System::FatalError("Simulation timeout");
    }
}
void GeneticOperators::Initializer(GAGenome& g)//todo: better initializer
{
	std::cout<<"calling Initializer\n";
	GAListGenome<Gene>& genome = (GAListGenome<Gene>&)g;


	if(_seedOriginalPosition){
	    while(genome.head()) genome.destroy(); // destroy any pre-existing list
	    for(std::vector<SparCraft::Unit>::const_iterator it=_buildings.begin();
	                   it!=_buildings.end();it++){
	        Gene gene(it->type(),BWAPI::TilePosition((it->pos().x()-it->type().dimensionLeft())/TILE_SIZE,(it->pos().y()-it->type().dimensionUp())/TILE_SIZE));
	        genome.insert(gene,GAListBASE::TAIL);
	    }
	    if(!isLegal(genome)||!isPowered(genome)){
	        std::cout<<"Repairing setup from file"<<std::endl;
	        if(!GeneticOperators::repair(genome)){
	            System::FatalError("Couldn't repair at initializer");
	        }
	    }
	    _seedOriginalPosition=false;
	}else{
	    do{
	        while(genome.head()) genome.destroy(); // destroy any pre-existing list
	        bool needsRepair=false;
	        for(std::vector<SparCraft::Unit>::const_iterator it=_buildings.begin();
	                it!=_buildings.end();it++){


	            BWAPI::TilePosition pos(_defendPlayer->getGoal().x()/TILE_SIZE,_defendPlayer->getGoal().y()/TILE_SIZE);
	            Gene gene(it->type(),pos);
	            //	        BWAPI::TilePosition offset(0,0);
	            int n=placementRetries;
	            do{
	                do{
	                    //	                gene.undo(offset);
	                    //	                offset=BWAPI::TilePosition(GARandomInt(-mutDistance,mutDistance),GARandomInt(-mutDistance,mutDistance));
	                    //	                gene.move(offset);
	                    gene.setPosition(BWAPI::TilePosition(
	                            GARandomInt(_baseLeft,_baseRight),
	                            GARandomInt(_baseTop,_baseBottom)));

	                }while(!_map->canBuildHere(gene.getType(),gene.getCenterPos()));

	                if(isLegal(genome,gene)){
	                    if(!gene.getType().requiresPsi()||isPowered(genome,gene)){
	                        genome.insert(gene,GAListBASE::TAIL);
	                        break;
	                    }
	                }
	                n--;
	            }while(n>0);
	            if(n==0){//if we reached the max amount of tries, add it anyway and try to repair later
	                genome.insert(gene,GAListBASE::TAIL);
	                needsRepair=true;
	                std::cout<<"Max amount of retries for initial location failed, will try to repair\n";
	            }

	            //		std::cout<<"building added"<<std::endl;
	        }
	        if(needsRepair||!isLegal(genome)){
	            if(!GeneticOperators::repair(genome)){
	                System::FatalError("Couldn't repair at initializer");
	            }
	        }
	    }while(!isLegal(genome));
	}
	//	Mutator(genome,0.5,20);
}

void GeneticOperators::configure(
        const std::vector<SparCraft::Unit>& fixedBuildings,
        const std::vector<SparCraft::Unit>& buildings,
        const std::vector<SparCraft::Unit>& defenders,
        const std::vector<std::vector<SparCraft::Unit> > attackers,
        const std::vector<std::pair<Unit, TimeType> > delayedDefenders,
        const std::vector<std::vector<std::pair<Unit, TimeType> > > delayedAttackers,
        Map* map,
        Display* display,
        PlayerPtr p1,
        PlayerPtr p2,
		 svv expDesc) {
    assert(attackers.size()==delayedAttackers.size());//same amount of attack waves
	_fixedBuildings=fixedBuildings;
	_buildings=buildings;
	_defenders=defenders;
	_attackers=attackers;
	_delayedAttackers=delayedAttackers;
	_delayedDefenders=delayedDefenders;
	_map=map;
	_display=display;
	if(p1->getType()==PlayerModels::Assault){
		_assaultPlayer=boost::dynamic_pointer_cast<Player_Assault>(p1);
		_defendPlayer=boost::dynamic_pointer_cast<Player_Defend>(p2);
	}else{
		_assaultPlayer=boost::dynamic_pointer_cast<Player_Assault>(p2);
		_defendPlayer=boost::dynamic_pointer_cast<Player_Defend>(p1);
	}
	_expDesc=expDesc;

//	Position average;
//	BOOST_FOREACH(const Unit &u,_fixedBuildings){
//	    average+=u.pos();
//	}
//	BOOST_FOREACH(const Unit &u,_buildings){
//	    average+=u.pos();
//	}
//	average.scale(1.0f/(_fixedBuildings.size()+_buildings.size()));
//
	_baseLeft=std::numeric_limits<int>::max();
	_baseRight=std::numeric_limits<int>::min();
    _baseTop=std::numeric_limits<int>::max();
    _baseBottom=std::numeric_limits<int>::min();
    BOOST_FOREACH(const Unit &u,_fixedBuildings){
        _baseLeft=std::min(_baseLeft,Map::floorDiv((u.x()-u.type().dimensionLeft()),TILE_SIZE));
        _baseRight=std::max(_baseRight,Map::ceilDiv((u.x()+u.type().dimensionRight()),TILE_SIZE));
        _baseTop=std::min(_baseTop,Map::floorDiv((u.y()-u.type().dimensionUp()),TILE_SIZE));
        _baseBottom=std::max(_baseBottom,Map::ceilDiv((u.y()+u.type().dimensionDown()),TILE_SIZE));
    }
    BOOST_FOREACH(const Unit &u,_buildings){
        _baseLeft=std::min(_baseLeft,Map::floorDiv((u.x()-u.type().dimensionLeft()),TILE_SIZE));
        _baseRight=std::max(_baseRight,Map::ceilDiv((u.x()+u.type().dimensionRight()),TILE_SIZE));
        _baseTop=std::min(_baseTop,Map::floorDiv((u.y()-u.type().dimensionUp()),TILE_SIZE));
        _baseBottom=std::max(_baseBottom,Map::ceilDiv((u.y()+u.type().dimensionDown()),TILE_SIZE));
    }

    int width=_baseRight-_baseLeft;
    int height=_baseBottom-_baseTop;
    std::cout<<"Original base size: "<<width<<"X"<<height<<std::endl;

    if(width<minBaseSize){
        int diff=minBaseSize-width;
        _baseLeft-=Map::floorDiv(diff,2);
        _baseRight+=Map::ceilDiv(diff,2);
    }
    if(height<minBaseSize){
        int diff=minBaseSize-height;
        _baseTop-=Map::floorDiv(diff,2);
        _baseBottom+=Map::ceilDiv(diff,2);
    }
//    width=_baseRight-_baseLeft;
//    height=_baseBottom-_baseTop;
//    std::cout<<"Base size: "<<width<<"X"<<height<<std::endl;

    _seedOriginalPosition=true;
}
void GeneticOperators::configure(
            const std::vector<SparCraft::Unit>& fixedBuildings,
            const std::vector<SparCraft::Unit>& buildings,
            const std::vector<SparCraft::Unit>& defenders,
            const std::vector<SparCraft::Unit> attackers,
            const std::vector<std::pair<Unit, TimeType> > delayedDefenders,
            const std::vector<std::pair<Unit, TimeType> > delayedAttackers,
            Map* map,
            Display* display,
            PlayerPtr p1,
            PlayerPtr p2,
            svv expDesc){
    std::vector<std::vector<SparCraft::Unit> > m_attackers;
    m_attackers.push_back(attackers);
            std::vector<std::vector<std::pair<Unit, TimeType> > > m_delayedAttackers;
            m_delayedAttackers.push_back(delayedAttackers);

            configure(
                    fixedBuildings,
                    buildings,
                    defenders,
                    m_attackers,
                    delayedDefenders,
                    m_delayedAttackers,
                    map,
                    display,
                    p1,
                    p2,
                    expDesc);

}
int GeneticOperators::Mutator(GAGenome& g, float pmut){
	return Mutator(g,pmut,mutDistance);
}

bool GeneticOperators::moveIfLegal(GAListGenome<Gene>& genome, int pos,
		BWAPI::TilePosition& offset, bool checkPowered) {

    genome[pos]->move(offset);
    if(isLegal(genome)){
        if(checkPowered&&isPowered(genome)){
            return true;
        }else{
            genome[pos]->undo(offset);
            return false;
        }
    }else{
        genome[pos]->undo(offset);
        return false;
    }
}

int GeneticOperators::Mutator(GAGenome& g, float pmut, int maxJump)
{
	std::cout<<"calling Mutator\n";
	GAListGenome<Gene>& genome = (GAListGenome<Gene>&)g;


	if(pmut <= 0.0) return 0;


	int nMut = 0;

	for(int i=0; i<genome.size(); i++){
		if(GAFlipCoin(pmut)){
//			Gene* gene=genome[i];
//			BWAPI::TilePosition offset(GARandomInt(-maxJump,maxJump),GARandomInt(-maxJump,maxJump));
//			if(moveIfLegal(genome,i,offset)){
//				std::cout<<"mutating"<<std::endl;
//				nMut++;
//			}
			BWAPI::TilePosition offset;
			int n=50;
			do{
				offset=BWAPI::TilePosition(GARandomInt(-maxJump,maxJump),GARandomInt(-maxJump,maxJump));
				n--;
			}while(n>0&&!moveIfLegal(genome,i,offset, true));
			if(n>0){
				std::cout<<"mutating"<<std::endl;
				nMut++;
			}
		}
	}
	assert(isLegal(genome));
	if(nMut!=0) genome.swap(0,0);//_evaluated = gaFalse;
	return nMut;

}

bool GeneticOperators::repair(GAListGenome<Gene>& genome, int pos) {

    Spiral sp(0,0,TILE_SIZE);
    BWAPI::TilePosition offset;
    int iters=0,maxIters=repairRadius*repairRadius*4;
    do{
        iters++;
        offset=BWAPI::TilePosition(sp.getNext());
    }while(!moveIfLegal(genome,pos,offset,genome[pos]->getType()!=BWAPI::UnitTypes::Protoss_Pylon)&&iters<maxIters);

    if(iters==maxIters){
        std::cerr<<"repair failed (non fatal): "<<genome[pos]->getType().getName()<<std::endl;
        return false;
    }else{
        if(iters>1){
            std::cout<<"repaired\n";
        }
        return true;
    }
}

bool GeneticOperators::repair(GAListGenome<Gene>& genome) {
    bool legal=true;
    for(int i=0; i<genome.size(); i++){
//        std::cout<<"repairing building: "<<i<<" "<<*genome[i]<<std::endl;
        legal&=repair(genome,i);
    }

    if(legal){
        return true;
    }else{//if some failed, could still at the end be legal(moving pylon gets stuff unpowered, but then we fix those)
        return isLegal(genome)&&isPowered(genome);
    }
}

int GeneticOperators::UniformCrossover(const GAGenome& parent1, const GAGenome& parent2,
		GAGenome* child1, GAGenome* child2) {
	std::cout<<"calling Crossover\n";
	int children=0;
	if(child1!=NULL){
		child1->copy(parent1);
		children++;
	}
	if(child2!=NULL){
		child2->copy(parent2);
		children++;
	}
	if(children==2){
		bool repaired;
		do{
			GAListGenome<Gene>& c1 = *(GAListGenome<Gene>*)child1;
			GAListGenome<Gene>& c2 = *(GAListGenome<Gene>*)child2;
			for(int i=0; i<c1.size(); i++){
				if(GAFlipCoin(0.5)){
					Gene* temp1=c1[i];
					Gene* temp2=c2[i];
					temp1->move(temp2->getTilePos()-temp1->getTilePos());
					temp2->move(temp1->getTilePos()-temp2->getTilePos());
					std::cout<<"exchanging\n";
				}
			}
			if(!(repair(c1)&&repair(c2))){
				repaired=false;
				std::cerr<<"couldn't repair after crossover (not fatal)\n";
			}else{
				repaired=true;
				c1.swap(0,0);//_evaluated = gaFalse;
				c2.swap(0,0);//_evaluated = gaFalse;
			}
		}while(!repaired);
	}
	return children;
}

int GeneticOperators::AverageCrossover(const GAGenome& parent1, const GAGenome& parent2,
        GAGenome* child1, GAGenome* child2) {
    std::cout<<"calling Crossover\n";
    int children=0;
    if(child1!=NULL){
        child1->copy(parent1);
        children++;
    }
    if(child2!=NULL){
        child2->copy(parent2);
        children++;
    }
    if(children==2){
        bool repaired;
        do{
            GAListGenome<Gene>& c1 = *(GAListGenome<Gene>*)child1;
            GAListGenome<Gene>& c2 = *(GAListGenome<Gene>*)child2;
            for(int i=0; i<c1.size(); i++){
                if(GAFlipCoin(0.5)){
                    Gene* temp1=c1[i];
                    Gene* temp2=c2[i];
                    BWAPI::TilePosition pos1=temp1->getTilePos();
                    BWAPI::TilePosition pos2=temp2->getTilePos();
                    PositionType x1=(pos1.x()*2+pos2.x())/3;
                    PositionType y1=(pos1.y()*2+pos2.y())/3;
                    PositionType x2=(pos1.x()+pos2.x()*2)/3;
                    PositionType y2=(pos1.y()+pos2.y()*2)/3;
                    temp1->setPosition(BWAPI::TilePosition(x1,y1));
                    temp2->setPosition(BWAPI::TilePosition(x2,y2));
                    std::cout<<"averaging\n";
                }
            }
            if(!(repair(c1)&&repair(c2))){
                repaired=false;
                std::cerr<<"couldn't repair after crossover (not fatal)\n";
            }else{
                repaired=true;
                c1.swap(0,0);//_evaluated = gaFalse;
                c2.swap(0,0);//_evaluated = gaFalse;
            }
        }while(!repaired);
    }
    return children;
}

// The comparator returns a number in the interval [0,1] where 0 means that
// the two genomes are identical (zero diversity) and 1 means they are
// completely different (maximum diversity).
float
GeneticOperators::Comparator(const GAGenome& g1, const GAGenome& g2) {
	GAListGenome<Gene>& a = (GAListGenome<Gene> &)g1;
	GAListGenome<Gene>& b = (GAListGenome<Gene> &)g2;
	int diffs=0;
	for(int i=0;i<a.size();i++){
		if((*a[i])!=(*b[i])){
			diffs++;
		}
	}
  return diffs/(float)a.size();
}


bool GeneticOperators::isLegal(GAListGenome<Gene>& genome) {
    for(int i=0;i<genome.size();i++){
        if(_map->canBuildHere(genome[i]->getType(),genome[i]->getCenterPos())){
            for(int j=0; j<genome.size(); j++){
                if(i!=j && genome[i]->collides(*genome[j])){
                    return false;
                }
            }
        }else{
            return false;
        }
        for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings.begin();
                it!=_fixedBuildings.end();it++){
            if(genome[i]->collides(*it)){
                return false;
            }
        }
    }
    return true;

}


bool GeneticOperators::isLegal(GAListGenome<Gene>& genome,
        const Gene& newGene) {

    if(_map->canBuildHere(newGene.getType(),newGene.getCenterPos())){
        for(int j=0; j<genome.size(); j++){
            if(newGene.collides(*genome[j])){
                return false;
            }
        }
    }else{
        return false;
    }
    for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings.begin();
            it!=_fixedBuildings.end();it++){
        if(newGene.collides(*it)){
            return false;
        }
    }
    return true;
}

bool GeneticOperators::isPowered(GAListGenome<Gene>& genome,
        const Gene& gene) {
//    std::cout<<"ispowered? "<<gene<<"w:"<<gene.getType().tileWidth()<<" h:"<<gene.getType().tileHeight()<<std::endl;
    for(int x=gene.getTilePos().x();x<gene.getTilePos().x()+gene.getType().tileWidth();x++){
        for(int y=gene.getTilePos().y();y<gene.getTilePos().y()+gene.getType().tileHeight();y++){
//            std::cout<<x<<"-"<<y<<std::endl;
//            if(y>200){
//                System::FatalError("sdfgadsfg");
//            }
//            std::cout<<x*TILE_SIZE+TILE_SIZE/2<<"---"<<y*TILE_SIZE+TILE_SIZE/2<<std::endl;
            if(isPowered(genome,Position(x*TILE_SIZE+TILE_SIZE/2,y*TILE_SIZE+TILE_SIZE/2))){
                return true;
            }
        }
    }
    return false;
}
bool GeneticOperators::isPowered(GAListGenome<Gene>& genome, const SparCraft::Position &pos){
//    std::cout<<"inner ispowered? "<<pos<<std::endl;
    for(int i=0;i<genome.size();i++){
        if(genome[i]->getType()==BWAPI::UnitTypes::Protoss_Pylon){
            if(genome[i]->getCenterPos().getDistance(pos)<genome[i]->getType().sightRange()){
//                std::cout<<genome[i]->getCenterPos()<<" "<<pos<<std::endl;
                return true;
            }
        }
    }
    return false;
}

//for each building that requires PSI, check that it has it
bool GeneticOperators::isPowered(GAListGenome<Gene>& genome){
    for(int i=0;i<genome.size();i++){
        if(genome[i]->getType().requiresPsi()){
            if(!isPowered(genome,*genome[i])){
                return false;
            }
        }
    }
    return true;
}
}

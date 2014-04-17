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

namespace BuildingPlacement {
std::vector<SparCraft::Unit> GeneticOperators::_fixedBuildings=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_buildings=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_defenders=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_attackers=std::vector<SparCraft::Unit>();
std::vector<std::pair<Unit, TimeType> > GeneticOperators::_delayed=std::vector<std::pair<Unit, TimeType> >();
Map* GeneticOperators::_map=NULL;
Display* GeneticOperators::_display=NULL;
boost::shared_ptr<Player_Assault> GeneticOperators::_assaultPlayer;
boost::shared_ptr<Player_Defend> GeneticOperators::_defendPlayer;
svv GeneticOperators::_expDesc=svv();
const int mutDistance=20;
const int placementRetries=50;

float GeneticOperators::Objective(GAGenome &g) {
	std::cout<<"calling Objective\n";


	GAListGenome<Gene>& genome=(GAListGenome<Gene>&)g;
std::cout<<"genome: "<<genome<<std::endl;
	GameState state;
	state.checkCollisions=true;
	state.setMap(*_map);
	for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings.begin();
				it!=_fixedBuildings.end();it++){
			assert(it->player()==Players::Player_Two);
	//		std::cout<<"defender unit: "<<it->type().getName()<<std::endl;
			state.addUnit(*it);
	}
	for(int i=0;i<genome.size();i++){
		Gene *gene=genome[i];
//		std::cout<<"building unit: "<<gene->getType().getName()<<" "<<gene->getType().dimensionLeft()<<" "
//		        <<gene->getType().dimensionRight()<<" "<<gene->getType().dimensionUp()<<" "<<gene->getType().dimensionDown()<<std::endl;
		state.addUnit(gene->getType(),Players::Player_Two,gene->getCenterPos());
	}
	for(std::vector<SparCraft::Unit>::const_iterator it=_defenders.begin();
			it!=_defenders.end();it++){
		assert(it->player()==Players::Player_Two);
//		std::cout<<"defender unit: "<<it->type().getName()<<std::endl;
		state.addUnit(*it);
	}
	for(std::vector<SparCraft::Unit>::const_iterator it=_attackers.begin();
			it!=_attackers.end();it++){
		assert(it->player()==Players::Player_One);
//		std::cout<<"attacker unit: "<<it->type().getName()<<std::endl;
		state.addUnit(*it);
	}
//todo: check that defenders and attackers are placed in legal locations, otherwise move them

	Game game(state, _assaultPlayer, _defendPlayer, 20000, _delayed);
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
	return score;

}

bool GeneticOperators::goalReached(const GameState& state){
      for (IDType u(0); u<state.numUnits(_assaultPlayer->ID()); ++u){
          const Unit & unit(state.getUnit(_assaultPlayer->ID(), u));
          if(_map->getDistance(unit.pos(),_assaultPlayer->getGoal())<TILE_SIZE/2){
              return true;
          }
      }
  return false;
}
ScoreType GeneticOperators::unitScore(const GameState& state,
        const std::vector<IDType> &units,
        IDType player){

    ScoreType score=0;
    BOOST_FOREACH(const IDType &id,units){
        const Unit &u=state.getUnitByID(player,id);
        ScoreType hpPercent=(u.type().maxHitPoints()-u.currentHP())*100/u.type().maxHitPoints();
        ScoreType cost=u.type().mineralPrice()+u.type().gasPrice();
        ScoreType val=cost*hpPercent;
        if(u.type().isWorker()){
            val*=3;
        }else if(u.type()==BWAPI::UnitTypes::Protoss_Pylon){
            val*=3;
        }
        score+=val;
    }
    return score;
}
ScoreType GeneticOperators::evalBuildingPlacement(const GameState& state){

    const std::vector<IDType> &attackers=state.getAliveUnitIDs(_assaultPlayer->ID());
    const std::vector<IDType> &defenders=state.getAliveUnitIDs(_defendPlayer->ID());

    ScoreType attValue=unitScore(state,attackers,_assaultPlayer->ID());
    ScoreType defValue=unitScore(state,defenders,_defendPlayer->ID());


  if(state.playerDead(_assaultPlayer->ID())){//attacker defeated, count how many we have left
      return defValue+3000000;
  }else if(goalReached(state)){//enemy reached goal,
      return /*defValue*/-attValue+1000000;
  }else if(state.playerDead(_defendPlayer->ID())){//defender destroyed, count how many he has left
      return defValue-attValue+500000;
  }else{//simulation time exhausted
      System::FatalError("Simulation timeout");
      return defValue-attValue+2000000;
  }
}
void GeneticOperators::Initializer(GAGenome& g)//todo: better initializer
{
    static int firstTime=true;
	std::cout<<"calling Initializer\n";
	GAListGenome<Gene>& genome = (GAListGenome<Gene>&)g;
	while(genome.head()) genome.destroy(); // destroy any pre-existing list

	if(firstTime){
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
	    firstTime=false;
	}else{
	    bool needsRepair=false;
	    for(std::vector<SparCraft::Unit>::const_iterator it=_buildings.begin();
	            it!=_buildings.end();it++){


	        BWAPI::TilePosition pos(_defendPlayer->getGoal().x()/TILE_SIZE,_defendPlayer->getGoal().y()/TILE_SIZE);
	        Gene gene(it->type(),pos);
	        BWAPI::TilePosition offset(0,0);
	        int n=placementRetries;
	        do{
	            do{
	                gene.undo(offset);
	                offset=BWAPI::TilePosition(GARandomInt(-mutDistance,mutDistance),GARandomInt(-mutDistance,mutDistance));
	                gene.move(offset);
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
	    if(needsRepair){
	        if(!GeneticOperators::repair(genome)){
			System::FatalError("Couldn't repair at initializer");
		}
	}
	        }
//	Mutator(genome,0.5,20);
}

void GeneticOperators::configure(
        const std::vector<SparCraft::Unit>& fixedBuildings,
        const std::vector<SparCraft::Unit>& buildings,
        const std::vector<SparCraft::Unit>& defenders,
        const std::vector<SparCraft::Unit>& attackers,
        const std::vector<std::pair<Unit, TimeType> > &delayed,
        Map* map,
        Display* display,
        PlayerPtr assaultPlayer,
        PlayerPtr defendPlayer,
		 svv expDesc) {
	_fixedBuildings=fixedBuildings;
	_buildings=buildings;
	_defenders=defenders;
	_attackers=attackers;
	_delayed=delayed;
	_map=map;
	_display=display;
	_assaultPlayer=boost::dynamic_pointer_cast<Player_Assault>(assaultPlayer);
	_defendPlayer=boost::dynamic_pointer_cast<Player_Defend>(defendPlayer);
	_expDesc=expDesc;


    //todo: sort stuff so that pylons come first
}

int GeneticOperators::Mutator(GAGenome& g, float pmut){
	return Mutator(g,pmut,mutDistance);
}

bool GeneticOperators::moveIfLegal(GAListGenome<Gene>& genome, int pos,
		BWAPI::TilePosition& offset, bool checkPowered) {


	BWAPI::TilePosition newTilePos=genome[pos]->getTilePos()+offset;

	BWAPI::UnitType type=genome[pos]->getType();
	float x=newTilePos.x()+type.tileWidth()/2.0f;
	float y=newTilePos.y()+type.tileHeight()/2.0f;
	SparCraft::Position newPos(x*TILE_SIZE,y*TILE_SIZE);

	if(_map->canBuildHere(genome[pos]->getType(),newPos)){

		genome[pos]->move(offset);
		bool legal=true;
		for(int j=0; j<genome.size(); j++){
			if(pos!=j){
				if(genome[pos]->collides(*genome[j])){
					legal=false;
//					   std::cout<<"Trying new pos: "<<newPos<<std::endl;
//					std::cout<<"collides with "<<*genome[j]<<std::endl;
					break;
				}
			}
		}
		for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings.begin();
				it!=_fixedBuildings.end() && legal;it++){
			if(genome[pos]->collides(*it)){
				legal=false;
//				   std::cout<<"Trying new pos: "<<newPos<<std::endl;
//				std::cout<<"collides with "<<it->debugString()<<std::endl;
				break;
			}
		}
		if(checkPowered && legal){
			if(type.requiresPsi()&&!isPowered(genome,newPos)){
//			    std::cout<<"is not powered"<<std::endl;
				legal=false;
			}else if((type==BWAPI::UnitTypes::Protoss_Pylon)&&!isPowered(genome)){
//			    std::cout<<"is pylon and rest not powered"<<std::endl;
				legal=false;
			}
		}
		if(!legal){//undo
			genome[pos]->undo(offset);
		}
		return legal;
	}else{
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
	if(nMut!=0) genome.swap(0,0);//_evaluated = gaFalse;
	return nMut;

}

bool GeneticOperators::repair(GAListGenome<Gene>& genome, int pos) {
	int distance=0;//start with distance 0, to check if it is currently legal
	do{
		for(int a=-distance;a<=distance;a++){
			for(int b=-distance;b<=distance;b++){
				if(std::max(std::abs(a),std::abs(b))==distance){
					BWAPI::TilePosition offset(a,b);
					if(moveIfLegal(genome,pos,offset,genome[pos]->getType()!=BWAPI::UnitTypes::Protoss_Pylon)){
						if(distance>0){
							std::cout<<"repaired\n";
						}
						return true;
					}
				}
			}
		}
		distance++;
//		std::cout<<"dist: "<<distance<<std::endl;
	}while(distance<mutDistance*3);
	std::cerr<<"repair failed (non fatal): "<<genome[pos]->getType().getName()<<std::endl;
	return false;
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

int GeneticOperators::Crossover(const GAGenome& parent1, const GAGenome& parent2,
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

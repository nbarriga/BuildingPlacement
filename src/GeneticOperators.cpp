/*
 * GeneticOperators.cpp
 *
 *  Created on: Jul 26, 2013
 *      Author: nbarriga
 */

#include "GeneticOperators.h"
#include "Gene.h"

namespace SparCraft {
BWAPI::TilePosition GeneticOperators::_basePos;
std::vector<SparCraft::Unit> GeneticOperators::_fixedBuildings=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_buildings=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_defenders=std::vector<SparCraft::Unit>();
std::vector<SparCraft::Unit> GeneticOperators::_attackers=std::vector<SparCraft::Unit>();
Map* GeneticOperators::_map=NULL;
Display* GeneticOperators::_display=NULL;
svv GeneticOperators::_expDesc=svv();
const int mutDistance=20;

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
		BWAPI::TilePosition pos=gene->getTilePos();
		BWAPI::UnitType type=gene->getType();
		float x=pos.x()+type.tileWidth()/2.0f;
		float y=pos.y()+type.tileHeight()/2.0f;
//		std::cout<<"building unit: "<<gene->getType().getName()<<std::endl;
		state.addUnit(type,Players::Player_Two,SparCraft::Position(x*TILE_SIZE,y*TILE_SIZE));
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



	PlayerPtr ptr1(new Player_Assault(Players::Player_One));
	PlayerPtr ptr2(new Player_Defend(Players::Player_Two));
	Game game(state, ptr1, ptr2, 20000);
#ifdef USING_VISUALIZATION_LIBRARIES
	if (_display!=NULL)
	{
		game.disp = _display;
		_display->SetExpDesc(_expDesc);

	}
#endif

	// play the game to the end
	game.play();
	int score = game.getState().evalBuildingPlacement(Players::Player_One,Players::Player_Two);
	std::cout<<"score: "<<score<<std::endl;
	return score;

}

void GeneticOperators::Initializer(GAGenome& g)//todo: better initializer
{
	std::cout<<"calling Initializer\n";
	GAListGenome<Gene>& genome = (GAListGenome<Gene>&)g;
	while(genome.head()) genome.destroy(); // destroy any pre-existing list

	bool repair=false;
	for(std::vector<SparCraft::Unit>::const_iterator it=_buildings.begin();
			it!=_buildings.end();it++){

		BWAPI::TilePosition pos(_basePos);
		Gene gene(it->type(),pos);
		genome.insert(gene,GAListBASE::TAIL);

		BWAPI::TilePosition offset;
		int n=50;
		do{
			offset=BWAPI::TilePosition(GARandomInt(-mutDistance,mutDistance),GARandomInt(-mutDistance,mutDistance));
			n--;
		}while(n>0&&!moveIfLegal(genome,genome.size()-1,offset, true));
		if(n==0){
			repair=true;
			std::cout<<"Max amount of retries for initial location failed, will try to repair\n";
		}

	}
	if(repair){
		if(!GeneticOperators::repair(genome)){
			System::FatalError("Couldn't repair at initializer");
		}
	}
//	Mutator(genome,0.5,20);
}

void GeneticOperators::configure(BWAPI::TilePosition& basePos,
		std::vector<SparCraft::Unit>& fixedBuildings,
		std::vector<SparCraft::Unit>& buildings,
		std::vector<SparCraft::Unit>& defenders,
		std::vector<SparCraft::Unit>& attackers,
		Map* map,
		Display* display,
		 svv expDesc) {
	_basePos=basePos;
	_fixedBuildings=fixedBuildings;
	_buildings=buildings;
	_defenders=defenders;
	_attackers=attackers;
	_map=map;
	_display=display;
	_expDesc=expDesc;
}

int GeneticOperators::Mutator(GAGenome& g, float pmut){
	return Mutator(g,pmut,mutDistance);
}

bool GeneticOperators::moveIfLegal(GAListGenome<Gene>& genome, int pos,
		BWAPI::TilePosition& offset, bool checkPowered) {


	BWAPI::TilePosition newTilePos=genome[pos]->getTilePos()+offset;
//	if((newTilePos.x()<0)
//			|| (newTilePos.x()>=_map->getBuildTileWidth())
//			|| (newTilePos.y()<0)
//			|| (newTilePos.y()>=_map->getBuildTileHeight())){
//		return false;
//	}
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
					break;
				}
			}
		}
		for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings.begin();
				it!=_fixedBuildings.end() && legal;it++){
			if(genome[pos]->collides(*it)){
				legal=false;
				break;
			}
		}
		if(checkPowered && legal){
			if(type.requiresPsi()&&!isPowered(genome,pos,newPos)){
				legal=false;
			}else if((type==BWAPI::UnitTypes::Protoss_Pylon)&&!isPowered(genome)){
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

bool GeneticOperators::isPowered(GAListGenome<Gene>& genome, int except, const SparCraft::Position &pos){
	for(int i=0;i<genome.size();i++){
		if(i!=except){
			Gene *gene=genome[i];
			BWAPI::UnitType type=gene->getType();
			BWAPI::TilePosition tilePos=gene->getTilePos();
			float x=tilePos.x()+type.tileWidth()/2.0f;
			float y=tilePos.y()+type.tileHeight()/2.0f;
			SparCraft::Position genePos(x*TILE_SIZE,y*TILE_SIZE);
			if(type==BWAPI::UnitTypes::Protoss_Pylon){
//				std::cout<<pos.getString()<<" "<<genePos.getString()<<" "<<genePos.getDistance(pos)<<" "<<
//						type.sightRange()<<std::endl;
				if(genePos.getDistance(pos)<type.sightRange()){
					return true;
				}
			}
		}
	}
	return false;
}

//for each building that requires PSI, check that it has it
bool GeneticOperators::isPowered(GAListGenome<Gene>& genome){
	for(int i=0;i<genome.size();i++){
		Gene *gene=genome[i];
		BWAPI::UnitType type=gene->getType();
		BWAPI::TilePosition tilePos=gene->getTilePos();
		float x=tilePos.x()+type.tileWidth()/2.0f;
		float y=tilePos.y()+type.tileHeight()/2.0f;
		SparCraft::Position genePos(x*TILE_SIZE,y*TILE_SIZE);
		if(type.requiresPsi()){
			if(!isPowered(genome,i,genePos)){
				return false;
			}
		}
	}
	return true;
}

int GeneticOperators::Mutator(GAGenome& g, float pmut, int maxJump)
{
	std::cout<<"calling Mutator\n";
	GAListGenome<Gene>& genome = (GAListGenome<Gene>&)g;


	if(pmut <= 0.0) return 0;


	int nMut = 0;

	for(int i=0; i<genome.size(); i++){
		if(GAFlipCoin(pmut)){
			Gene* gene=genome[i];
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
	int a=0,b=0,distance=0;//start with distance 0, to check if it is currently legal
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
	}while(distance<mutDistance);
	std::cerr<<"repair failed (non fatal): "<<genome[pos]->getType().getName()<<std::endl;
	return false;
}

bool GeneticOperators::repair(GAListGenome<Gene>& genome) {
	bool repaired=true;
	for(int i=0; i<genome.size(); i++){
		repaired=repaired&&(repair(genome,i)||genome[i]->getType()==BWAPI::UnitTypes::Protoss_Pylon);
	}
	return repaired;
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
}

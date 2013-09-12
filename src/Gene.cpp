/*
 * Gene.cpp
 *
 *  Created on: Jul 26, 2013
 *      Author: nbarriga
 */

#include "Gene.h"
#include "ga/garandom.h"
#include "GeneticOperators.h"

namespace SparCraft {

Gene::Gene(BWAPI::UnitType type, BWAPI::TilePosition pos):
	_type(type),_pos(pos){
}

//Gene::Gene(SparCraft::Unit building):
//				_type(building.type()),
//				_pos(BWAPI::Position(building.pos().x()-building.type().dimensionLeft(),
//						building.pos().y()-building.type().dimensionUp())){
//	if(!building.type().isBuilding()){
//		System::FatalError("Only buildings are allowed");
//	}
//}

//Gene::Gene():
//				_type(BWAPI::UnitTypes::None),
//				_pos(BWAPI::TilePosition(GARandomInt(0,10),GARandomInt(0,10))){
//}

void Gene::move(BWAPI::TilePosition offset){
	_pos+=offset;
}

std::ostream& operator<< (std::ostream& stream, const Gene& building){
	stream<<"Type: "<<building._type.getName()<<", "<<
			building._pos.x() << ", " << building._pos.y() << ")"<<
			std::endl;
	return stream;
}

bool Gene::collides(const Gene& other) const{
	int otherLeft=other.getTilePos().x();
	int otherRight=otherLeft+other.getType().tileWidth()-1;
	int otherTop=other.getTilePos().y();
	int otherBottom=otherTop+other.getType().tileHeight()-1;

	int thisLeft=getTilePos().x();
	int thisRight=thisLeft+getType().tileWidth()-1;
	int thisTop=getTilePos().y();
	int thisBottom=thisTop+getType().tileHeight()-1;

	if(
			((thisLeft<=otherRight&&thisLeft>=otherLeft)||
					(thisRight>=otherLeft&&thisRight<=otherRight)
			)&&
			(
					(thisTop<=otherBottom&&thisTop>=otherTop)||
					(thisBottom>=otherTop&&thisBottom<=otherBottom)
			)){
		return true;
	}else{
		return false;
	}
}

bool Gene::collides(const SparCraft::Unit& other) const {
	if(!other.type().isBuilding()){
		System::FatalError("This method is only intended for buildings");
	}
	int otherLeft=(other.pos().x()-other.type().dimensionLeft())/TILE_SIZE;
	int otherRight=otherLeft+other.type().tileWidth()-1;
	int otherTop=(other.pos().y()-other.type().dimensionUp())/TILE_SIZE;
	int otherBottom=otherTop+other.type().tileHeight()-1;

	int thisLeft=getTilePos().x();
	int thisRight=thisLeft+getType().tileWidth()-1;
	int thisTop=getTilePos().y();
	int thisBottom=thisTop+getType().tileHeight()-1;
//	std::cout<<"checking "<<getType().getName()<<" at "<<getTilePos().x()<<" "<<getTilePos().y()<<std::endl;
//	std::cout<<otherLeft<<" "<<otherRight<<" "<<otherTop<<" "<<otherBottom<<std::endl;

	if(
			((thisLeft<=otherRight&&thisLeft>=otherLeft)||
					(thisRight>=otherLeft&&thisRight<=otherRight)
			)&&
			(
					(thisTop<=otherBottom&&thisTop>=otherTop)||
					(thisBottom>=otherTop&&thisBottom<=otherBottom)
			)){
		return true;
	}else{
		return false;
	}
}

bool operator== (Gene &b1, Gene &b2){
	return b1.getType()==b2.getType() && b1.getTilePos()==b2.getTilePos();
}

const BWAPI::UnitType Gene::getType() const {
	return _type;
}

const BWAPI::TilePosition Gene::getTilePos() const {
	return _pos;
}

void Gene::undo(BWAPI::TilePosition offset) {
	_pos-=offset;
}

bool operator!= (Gene &b1, Gene &b2){
	return !(b1==b2);
}

} /* namespace SparCraft */

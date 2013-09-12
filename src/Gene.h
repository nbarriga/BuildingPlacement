/*
 * Gene.h
 *
 *  Created on: Jul 26, 2013
 *      Author: nbarriga
 */

#ifndef GENE_H_
#define GENE_H_

#include "Common.h"
#include "Position.hpp"
#include "Unit.h"

namespace SparCraft {

class Gene{
	BWAPI::UnitType     _type;
	BWAPI::TilePosition            _pos;
public:
	const BWAPI::UnitType     getType() const;
	const BWAPI::TilePosition getTilePos() const;

//	Gene(SparCraft::Unit building);
	Gene(BWAPI::UnitType type,	BWAPI::TilePosition pos);
	//	Gene();
	void move(BWAPI::TilePosition offset);
	void undo(BWAPI::TilePosition offset);
	bool collides(const Gene &other) const;
	bool collides(const SparCraft::Unit &other) const;
	friend std::ostream& operator<< (std::ostream& stream, const Gene& building);
	friend bool operator== (Gene &b1, Gene &b2);
	friend bool operator!= (Gene &b1, Gene &b2);


};
} /* namespace SparCraft */
#endif /* GENE_H_ */

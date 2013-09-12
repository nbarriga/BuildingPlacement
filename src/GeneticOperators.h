/*
 * GeneticOperators.h
 *
 *  Created on: Jul 26, 2013
 *      Author: nbarriga
 */

#ifndef GENETICOPERATORS_H_
#define GENETICOPERATORS_H_

#include <ga/GAListGenome.h>
#include "Common.h"
#include "SparCraft.h"
#include "Gene.h"

namespace SparCraft {

class GeneticOperators {
	static bool repair(GAListGenome<Gene>& genome, int pos);
	static bool repair(GAListGenome<Gene>& genome);
	static bool moveIfLegal(GAListGenome<Gene>& genome, int pos, BWAPI::TilePosition& offset, bool checkPowered);
	static bool isPowered(GAListGenome<Gene>& genome, int except, const SparCraft::Position &pos);
	static bool isPowered(GAListGenome<Gene>& genome);
public:
	static BWAPI::TilePosition _basePos;
	static std::vector<SparCraft::Unit> _fixedBuildings;
	static std::vector<SparCraft::Unit> _buildings;
	static std::vector<SparCraft::Unit> _defenders;
	static std::vector<SparCraft::Unit> _attackers;
	static Map* _map;
	static Display* _display;
	static svv _expDesc;
	static void configure(BWAPI::TilePosition& basePos,
			std::vector<SparCraft::Unit>& fixedBuildings,
			 std::vector<SparCraft::Unit>& buildings,
			 std::vector<SparCraft::Unit>& defenders,
			 std::vector<SparCraft::Unit>& attackers,
			 Map* map,
			 Display* display,
			 svv expDesc);
	static float Objective(GAGenome &g);
	static void	Initializer(GAGenome& g);
	static int Mutator(GAGenome& g, float pmut);
	static int Mutator(GAGenome& g, float pmut, int maxJump);
	static int Crossover(const GAGenome&, const GAGenome&,
		      GAGenome*, GAGenome*);
	static float Comparator(const GAGenome&, const GAGenome&);
};

} /* namespace SparCraft */
#endif /* GENETICOPERATORS_H_ */

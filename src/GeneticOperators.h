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
#include "Player_Assault.h"
#include "Player_Defend.h"

namespace BuildingPlacement {

class GeneticOperators {
	static bool repair(GAListGenome<Gene>& genome, int pos);
	static bool repair(GAListGenome<Gene>& genome);
	static bool moveIfLegal(GAListGenome<Gene>& genome, int pos, BWAPI::TilePosition& offset, bool checkPowered);

	//check all pylons to see if one powers this position.
	static bool isPowered(GAListGenome<Gene>& genome, const SparCraft::Position &pos);
	//is newGene legal(doesn't collide with map, fixed buildings or genome)
	static bool isLegal(GAListGenome<Gene>& genome, const Gene& newGene);
	//is building in newGene powered?
	static bool isPowered(GAListGenome<Gene>& genome, const Gene& newGene);
	//check if genome is fully legal
    static bool isLegal(GAListGenome<Gene>& genome);
    //check if all buildings in genome are powered
	static bool isPowered(GAListGenome<Gene>& genome);
	static ScoreType unitScore(const GameState& state,
	        const std::vector<IDType> &units,
	        IDType player);
public:
	static std::vector<SparCraft::Unit> _fixedBuildings;
	static std::vector<SparCraft::Unit> _buildings;
	static std::vector<SparCraft::Unit> _defenders;
	static std::vector<SparCraft::Unit> _attackers;
	static std::vector<std::pair<Unit, TimeType> > _delayed;
	static Map* _map;
	static Display* _display;
	static svv _expDesc;
	static boost::shared_ptr<Player_Assault> _assaultPlayer;
	static boost::shared_ptr<Player_Defend > _defendPlayer;
	static void configure(
	        const std::vector<SparCraft::Unit>& fixedBuildings,
	        const std::vector<SparCraft::Unit>& buildings,
	        const std::vector<SparCraft::Unit>& defenders,
	        const std::vector<SparCraft::Unit>& attackers,
	        const std::vector<std::pair<Unit, TimeType> > &delayed,
	        Map* map,
	        Display* display,
	        PlayerPtr p1,
	        PlayerPtr p2,
	        svv expDesc);
	static float Objective(GAGenome &g);
	static void	Initializer(GAGenome& g);
	static int Mutator(GAGenome& g, float pmut);
	static int Mutator(GAGenome& g, float pmut, int maxJump);
	static int UniformCrossover(const GAGenome&, const GAGenome&,
		      GAGenome*, GAGenome*);
	static int AverageCrossover(const GAGenome&, const GAGenome&,
	              GAGenome*, GAGenome*);
	static float Comparator(const GAGenome&, const GAGenome&);
	static ScoreType evalBuildingPlacement(const GameState& state);
	static bool goalReached(const GameState& state);
};

} /* namespace SparCraft */
#endif /* GENETICOPERATORS_H_ */

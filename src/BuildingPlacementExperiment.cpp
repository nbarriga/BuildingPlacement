#include "BuildingPlacementExperiment.h"

#include <ga/GASimpleGA.h>
#include <ga/GAListGenome.h>
#include "Gene.h"
#include "GeneticOperators.h"

namespace SparCraft {
BuildingPlacementExperiment::BuildingPlacementExperiment(const std::string & configFile):
							SearchExperiment(configFile),_display(NULL){
	// TODO Auto-generated constructor stub

}

BuildingPlacementExperiment::~BuildingPlacementExperiment(){
	// TODO Auto-generated destructor stub
}


void BuildingPlacementExperiment::runExperiment(){
	int popsize  = 10;
	int ngen     = 10;
	float pmut   = 0.025;
	float pcross = 0.9;
	gaDefDivFlag=gaTrue;
	GARandomSeed(time(NULL));

	// set the map file for all states
	for (size_t state(0); state < states.size(); ++state)
	{
		states[state].setMap(*map);
	}

#ifdef USING_VISUALIZATION_LIBRARIES
	_display = NULL;
	if (showDisplay)
	{
		_display = new Display(map ? map->getBuildTileWidth() : 40, map ? map->getBuildTileHeight() : 22);
		_display->SetImageDir(imageDir);
		_display->OnStart();
		_display->LoadMapTexture(map, 19);
	}
#endif
	// for each player one player
	for (size_t p1Player(0); p1Player < players[0].size(); p1Player++)
	{
		// for each player two player
		for (size_t p2Player(0); p2Player < players[1].size(); p2Player++)
		{
			// for each state we care about
			for (size_t state(0); state < states.size(); ++state)
			{
				char buf[255];
				fprintf(stderr, "%s  ", configFileSmall.c_str());
				fprintf(stderr, "%5d %5d %5d %5d", (int)p1Player, (int)p2Player, (int)state, (int)states[state].numUnits(Players::Player_One));


				resultsPlayers[0].push_back(p1Player);
				resultsPlayers[1].push_back(p2Player);
				resultsStateNumber[p1Player][p2Player].push_back(state);
				resultsNumUnits[p1Player][p2Player].push_back(states[state].numUnits(Players::Player_One));

				// get player one
				PlayerPtr playerOne(players[0][p1Player]);

				// give it a new transposition table if it's an alpha beta player
				Player_AlphaBeta * p1AB = dynamic_cast<Player_AlphaBeta *>(playerOne.get());
				if (p1AB)
				{
					p1AB->setTranspositionTable(TTPtr(new TranspositionTable()));
				}

				// get player two
				PlayerPtr playerTwo(players[1][p2Player]);
				Player_AlphaBeta * p2AB = dynamic_cast<Player_AlphaBeta *>(playerTwo.get());
				if (p2AB)
				{
					p2AB->setTranspositionTable(TTPtr(new TranspositionTable()));
				}

				// Now create the GA and run it.  First we create a genome of the type that
				// we want to use in the GA.  The ga doesn't operate on this genome in the
				// optimization - it just uses it to clone a population of genomes.

				BWAPI::TilePosition goal(map->getGoal().x()/TILE_SIZE,map->getGoal().y()/TILE_SIZE);

				std::vector<Unit> fixedBuildings,buildings,attackers,defenders;
				int units=states[state].numUnits(Players::Player_One)+states[state].numUnits(Players::Player_Two);
				for(int i=0;i<units;i++){
					const Unit &unit=states[state].getUnitByID(i);
					if(unit.player()==Players::Player_One){//attacker
						attackers.push_back(unit);
					}else if(unit.player()==Players::Player_Two){//defender
						if(unit.type().isBuilding()){
							buildings.push_back(unit);
						}else{
							defenders.push_back(unit);
						}
					}else{
						System::FatalError("More than 2 players?");
					}
				}
				const SparCraft::Unit nexus(BWAPI::UnitTypes::Protoss_Nexus,
						Players::Player_Two,
						SparCraft::Position(TILE_SIZE*2+BWAPI::UnitTypes::Protoss_Nexus.tileWidth()/2.0f*TILE_SIZE,
								TILE_SIZE*55+BWAPI::UnitTypes::Protoss_Nexus.tileHeight()/2.0f*TILE_SIZE));
				fixedBuildings.push_back(nexus);

				std::cout<<"fixed buildings: "<<fixedBuildings.size()<<
						", buildings: "<<buildings.size()<<
						", defenders: "<<defenders.size()<<
						",attackers: "<<attackers.size()<<std::endl;



				GeneticOperators::configure(goal,
						fixedBuildings,
						buildings,
						defenders,
						attackers,
						map,_display,
						getExpDescription(0,0,0));

				GAListGenome<Gene> genome(GeneticOperators::Objective);

				// Now that we have the genome, we create the genetic algorithm and set
				// its parameters - number of generations, mutation probability, and crossover
				// probability.  And finally we tell it to evolve itself.
				genome.initializer(GeneticOperators::Initializer);
				genome.mutator(GeneticOperators::Mutator);
				genome.crossover(GeneticOperators::Crossover);
				genome.comparator(GeneticOperators::Comparator);



				GASimpleGA ga(genome);
				ga.populationSize(popsize);
				ga.nGenerations(ngen);
				ga.pMutation(pmut);
				ga.pCrossover(pcross);
//				GATournamentSelector sel;
//				ga.selector(sel);
				//				ga.elitist(gaTrue);

				ga.evolve();

				// Now we print out the best genome that the GA found.
				GAStatistics stats=ga.statistics();
				//				stats.recordDiversity(gaTrue);
				std::cout << "The GA found: "<<stats.bestIndividual().evaluate()<<"\n" <<
						stats.bestIndividual() << "\n";
				stats.write(std::cout);

			}
		}
	}





}
}


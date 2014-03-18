#include "BuildingPlacementExperiment.h"

#include <ga/GASimpleGA.h>
#include <ga/GAListGenome.h>
#include "Gene.h"
#include "GeneticOperators.h"
#include "Common.h"
#include "Player_Assault.h"
#include "Player_Defend.h"

namespace BuildingPlacement {
BuildingPlacementExperiment::BuildingPlacementExperiment(const std::string & configFile):
            SearchExperiment(),_display(NULL){
    init(configFile);
}

BuildingPlacementExperiment::~BuildingPlacementExperiment(){
	// TODO Auto-generated destructor stub
}

void BuildingPlacementExperiment::addPlayer(const std::string & line){

    std::istringstream iss(line);

    // the first number is the playerID

    std::string player;
    int playerID;
//    int playerModelID;
    std::string playerModelString;

    iss >> player;
    iss >> playerID;
    iss >> playerModelString;

    if (playerModelString.compare(Player_Assault::modelString)==0)
    {
        assert(playerID==0);
        playerStrings[playerID].push_back(playerModelString);
        int goalX, goalY;
        iss>>goalX;assert(goalX>=0);
        iss>>goalY;assert(goalY>=0);
        players[playerID].push_back(SparCraft::PlayerPtr(new Player_Assault(playerID,SparCraft::Position(goalX,goalY))));
    }else if (playerModelString.compare(Player_Defend::modelString)==0)
    {
        assert(playerID==1);
        playerStrings[playerID].push_back(playerModelString);
        int goalX, goalY;
        iss>>goalX;assert(goalX>=0);
        iss>>goalY;assert(goalY>=0);
        players[playerID].push_back(SparCraft::PlayerPtr(new Player_Defend(playerID,SparCraft::Position(goalX,goalY))));
    }else{
        SearchExperiment::addPlayer(line);
    }
}

void BuildingPlacementExperiment::runExperiment(){
	int popsize  = 10;
	int ngen     = 10;
	float pmut   = 0.025;
	float pcross = 0.9;
	gaDefDivFlag=gaTrue;
	GARandomSeed(time(NULL));

//	// set the map file for all states
//	for (size_t state(0); state < states.size(); ++state)
//	{
//		states[state].setMap(*map);
//	}

	if(!map){
	    map=new Map(64,64);
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
			for (size_t state(0); state < _fixedBuildings.size(); ++state)
			{
//				char buf[255];
//				fprintf(stderr, "%s  ", configFileSmall.c_str());
//				fprintf(stderr, "%5d %5d %5d %5d", (int)p1Player, (int)p2Player, (int)state, (int)states[state].numUnits(Players::Player_One));
//
//
//				resultsPlayers[0].push_back(p1Player);
//				resultsPlayers[1].push_back(p2Player);
//				resultsStateNumber[p1Player][p2Player].push_back(state);
//				resultsNumUnits[p1Player][p2Player].push_back(states[state].numUnits(Players::Player_One));

				// get player one
				PlayerPtr playerOne(players[0][p1Player]);

				// give it a new transposition table if it's an alpha beta player
				Player_AlphaBeta * p1AB = dynamic_cast<Player_AlphaBeta *>(playerOne.get());
				if (p1AB)
				{
					p1AB->setTranspositionTable(TTPtr(new SparCraft::TranspositionTable()));
				}

				// get player two
				PlayerPtr playerTwo(players[1][p2Player]);
				Player_AlphaBeta * p2AB = dynamic_cast<Player_AlphaBeta *>(playerTwo.get());
				if (p2AB)
				{
					p2AB->setTranspositionTable(TTPtr(new SparCraft::TranspositionTable()));
				}

				// Now create the GA and run it.  First we create a genome of the type that
				// we want to use in the GA.  The ga doesn't operate on this genome in the
				// optimization - it just uses it to clone a population of genomes.

//				std::vector<Unit> fixedBuildings,buildings,attackers,defenders;
//				int units=states[state].numUnits(Players::Player_One)+states[state].numUnits(Players::Player_Two);
//				for(int i=0;i<units;i++){
//					const Unit &unit=states[state].getUnitByID(i);
//					if(unit.player()==Players::Player_One){//attacker
//						attackers.push_back(unit);
//					}else if(unit.player()==Players::Player_Two){//defender
//						if(unit.type().isBuilding()){
//							buildings.push_back(unit);
//						}else{
//							defenders.push_back(unit);
//						}
//					}else{
//						System::FatalError("More than 2 players?");
//					}
//				}
//				const SparCraft::Unit nexus(BWAPI::UnitTypes::Protoss_Nexus,
//						Players::Player_Two,
//						SparCraft::Position(TILE_SIZE*2+BWAPI::UnitTypes::Protoss_Nexus.tileWidth()/2.0f*TILE_SIZE,
//								TILE_SIZE*55+BWAPI::UnitTypes::Protoss_Nexus.tileHeight()/2.0f*TILE_SIZE));
//				fixedBuildings.push_back(nexus);

				std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
						", buildings: "<<_buildings[state].size()<<
						", defenders: "<<_defenders[state].size()<<
						",attackers: "<<_attackers[state].size()<<std::endl;



				svv expDesc;
				expDesc.push_back(sv());
				GeneticOperators::configure(_fixedBuildings[state],
				        _buildings[state],
				        _defenders[state],
				        _attackers[state],
						map,_display,
						playerOne,
						playerTwo,
						expDesc);

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

void BuildingPlacementExperiment::addState(const std::string& line) {

    std::istringstream iss(line);

    // the first number is the playerID
    std::string state;
    std::string stateType;
    int numStates;

    iss >> state;
    iss >> stateType;
    iss >> numStates;
    if (strcmp(stateType.c_str(), "BaseAssaultStateDescriptionFile") == 0)
    {
        std::string filename;
        iss >> filename;

        for (int i(0); i<numStates; ++i)
        {
            parseBaseAssaultStateDescriptionFile(filename);
        }
    }
    else{
        SparCraft::System::FatalError("State type not supported");
        SearchExperiment::addState(line);
    }
}


void BuildingPlacementExperiment::parseBaseAssaultStateDescriptionFile(
        const std::string& fileName) {
    std::vector<std::string> lines = getLines(fileName);

     std::vector<Unit> fixedBuildings,buildings,attackers,defenders;

     for (size_t u(0); u<lines.size(); ++u)
     {
         std::stringstream iss(lines[u]);
         std::string unitType;
         int playerID,x,y,hp,time,hpFinal;
         bool fixed;

         iss >> unitType;
         iss >> playerID;
         iss >> x;
         iss >> y;
         iss >> hp;
         iss >> time;
         iss >> fixed;
         iss >> hpFinal;

         BWAPI::UnitType type(getUnitType(unitType));
         if(!type.isBuilding()&&fixed){
             SparCraft::System::FatalError("A non building unit cannot be fixed");
         }

         if(type==BWAPI::UnitTypes::Terran_Refinery){
             SparCraft::System::FatalError("Refinery not supported");
         }

         if(type.isBuilding()){
             int xoffset=(x-(type.tileWidth()*TILE_SIZE/2))%TILE_SIZE;
             int yoffset=(y-(type.tileHeight()*TILE_SIZE/2))%TILE_SIZE;
             if(xoffset!=0 || yoffset!=0){
                 std::cerr<<"Building '"<<unitType<<"' in wrong position "<<Position(x, y).getString()<<
                         ", closest legal is "<<Position(x-xoffset, y-yoffset).getString()<<
                         ". Please fix your configuration file."<<std::endl;
             }
         }

         Unit unit(type, Position(x, y), 0, playerID, hp,
                 type == BWAPI::UnitTypes::Terran_Medic ? Constants::Starting_Energy : 0, 0,0);


         if(playerID==0){//assault
             if(type.isBuilding()){
                 SparCraft::System::FatalError("Assault player cannot have buildings");
             }else{
                 attackers.push_back(unit);
             }
         }else{//defend
             if(type.isBuilding()){
                 if(fixed){
                     fixedBuildings.push_back(unit);
                 }else{
                     buildings.push_back(unit);
                 }
             }else{
                 defenders.push_back(unit);
             }
         }
     }

     _fixedBuildings.push_back(fixedBuildings);
     _buildings.push_back(buildings);
     _attackers.push_back(attackers);
     _defenders.push_back(defenders);
}

}

#include "BuildingPlacementExperiment.h"

#include <ga/GASimpleGA.h>
#include <ga/GAListGenome.h>
#include "Gene.h"
#include "GeneticOperators.h"
#include "Common.h"
#include "Player_Assault.h"
#include "Player_Defend.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace BuildingPlacement {
BuildingPlacementExperiment::BuildingPlacementExperiment(const std::string & configFile):BuildingPlacementExperiment(){
    init(configFile);
}

BuildingPlacementExperiment::BuildingPlacementExperiment():
        SearchExperiment(),
        _display(NULL),
        _assaultPlayer(std::numeric_limits<IDType>::max()),
        _defendPlayer(std::numeric_limits<IDType>::max()),
        _popSize(15),
        _genSize(40){
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
//        assert(playerID==0);
        playerStrings[playerID].push_back(playerModelString);
        if(_assaultPlayer!=std::numeric_limits<IDType>::max()){
        	System::FatalError("Please specify exactly one Assault and one Defend player");
        }
        _assaultPlayer=playerID;
//        int goalX, goalY;
//        iss>>goalX;assert(goalX>=0);
//        iss>>goalY;assert(goalY>=0);
        players[playerID].push_back(SparCraft::PlayerPtr(new Player_Assault(playerID,SparCraft::Position())));
    }else if (playerModelString.compare(Player_Defend::modelString)==0)
    {
//        assert(playerID==1);
        playerStrings[playerID].push_back(playerModelString);
        if(_defendPlayer!=std::numeric_limits<IDType>::max()){
        	System::FatalError("Please specify exactly one Assault and one Defend player");
        }
        _defendPlayer=playerID;
//        int goalX, goalY;
//        iss>>goalX;assert(goalX>=0);
//        iss>>goalY;assert(goalY>=0);
        players[playerID].push_back(SparCraft::PlayerPtr(new Player_Defend(playerID,SparCraft::Position())));
    }else{
        SearchExperiment::addPlayer(line);
    }
}
bool goalReached(const GameState& state, boost::shared_ptr<Player_Assault> player){
    for (IDType u(0); u<state.numUnits(player->ID()); ++u){
        const Unit & unit(state.getUnit(player->ID(), u));
        if(state.getMap().getDistance(unit.pos(),player->getGoal())<TILE_SIZE/2){
            return true;
        }
    }
    return false;
}

const Position& BuildingPlacementExperiment::getGoal(
        const std::vector<Unit>& fixedBuildings) const {

    BOOST_FOREACH(const Unit& u,fixedBuildings){
        if(u.type()==BWAPI::UnitTypes::Protoss_Nexus||
                u.type()==BWAPI::UnitTypes::Terran_Command_Center){
            return u.pos();
        }
    }
    System::FatalError("Didn't find a Nexus or Command Center in fixed buildings to use as a goal for player scripts");
}

void BuildingPlacementExperiment::runCross() {
    if(!map){
        System::FatalError("Need to set either MapFile or Map");
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

                for(size_t otherState(0); otherState < _fixedBuildings.size(); ++otherState){
                    if(state==otherState){
                        continue;
                    }

                    PlayerPtr playerOne(players[0][p1Player]);
                    PlayerPtr playerTwo(players[1][p2Player]);

                    setupPlayers(p1Player, p2Player, getGoal(_fixedBuildings[state]));

                    std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
                            ", buildings: "<<_buildings[state].size()<<
                            ", defenders: "<<_defenders[state].size()<<
                            ",attackers: "<<_attackers[otherState].size()<<
                            ", delayedAttackers: "<<_delayedAttackers[state].size()<<
                            ", delayedDefenders: "<<_delayedDefenders[state].size()<<std::endl;


                    GameState gameState;
                    gameState.checkCollisions=true;
                    gameState.setMap(*map);
                    for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings[state].begin();
                            it!=_fixedBuildings[state].end();it++){
                        assert(it->player()==_defendPlayer);
                        gameState.addUnit(*it);
                    }
                    for(std::vector<SparCraft::Unit>::const_iterator it=_buildings[state].begin();
                            it!=_buildings[state].end();it++){
                        assert(it->player()==_defendPlayer);
                        gameState.addUnit(*it);
                    }

                    for(std::vector<SparCraft::Unit>::const_iterator it=_defenders[state].begin();
                            it!=_defenders[state].end();it++){
                        assert(it->player()==_defendPlayer);
                        gameState.addUnitClosestLegalPos(*it);
                    }
                    for(std::vector<SparCraft::Unit>::const_iterator it=_attackers[otherState].begin();
                            it!=_attackers[otherState].end();it++){
                        assert(it->player()==_assaultPlayer);
                        gameState.addUnitClosestLegalPos(*it);
                    }


                    std::vector<std::pair<Unit, TimeType> > delayedUnits(
                            _delayedAttackers[otherState].size()+_delayedDefenders[state].size());
                    std::merge(_delayedAttackers[otherState].begin(),_delayedAttackers[otherState].end(),
                            _delayedDefenders[state].begin(),_delayedDefenders[state].end(),
                            delayedUnits.begin(), Comparison());
                    Game game(gameState, playerOne, playerTwo, 20000,delayedUnits);
#ifdef USING_VISUALIZATION_LIBRARIES
                    if (_display!=NULL)
                    {
                        game.disp = _display;
                        _display->SetExpDesc(getExpDescription(p1Player,p2Player,state));

                    }
#endif

                    // play the game to the end
                    game.play();


                    GeneticOperators::configure(_fixedBuildings[state],
                            _buildings[state],
                            _defenders[state],
                            _attackers[otherState],
                            _delayedDefenders[state],
                            _delayedAttackers[otherState],
                            map,_display,
                            playerOne,
                            playerTwo,
                            getExpDescription(p1Player,p2Player,state));
                    int score = GeneticOperators::evalBuildingPlacement(game.getState());
                    std::cout<<"score: "<<score<<std::endl;

                }

            }
        }
    }
}


void BuildingPlacementExperiment::runEvaluate() {
    if(!map){
        System::FatalError("Need to set either MapFile or Map");
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

                PlayerPtr playerOne(players[0][p1Player]);
                PlayerPtr playerTwo(players[1][p2Player]);

                setupPlayers(p1Player, p2Player, getGoal(_fixedBuildings[state]));

                std::cout<<"Evaluating battle "<<stateFileNames[state]<<std::endl;
                std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
                        ", buildings: "<<_buildings[state].size()<<
                        ", defenders: "<<_defenders[state].size()<<
                        ",attackers: "<<_attackers[state].size()<<
                        ", delayedAttackers: "<<_delayedAttackers[state].size()<<
                        ", delayedDefenders: "<<_delayedDefenders[state].size()<<std::endl;



                GameState afterState=runGame(map,
                        _fixedBuildings[state],
                        _buildings[state],
                        _defenders[state],
                        _attackers[state],
                        _delayedDefenders[state],
                        _delayedAttackers[state],
                        playerOne,
                        playerTwo);
                try{
                    int score = GeneticOperators::evalBuildingPlacement(afterState);
                    std::cout<<"score: "<<score<<std::endl;


                    ScoreType defenderScoreAfter,attackerScoreAfter;
                    attackerScoreAfter = GeneticOperators::unitScore(afterState,_assaultPlayer);
                    defenderScoreAfter = GeneticOperators::unitScore(afterState,_defendPlayer);

                    //========================FINISHED EVAL==========================================================




                    std::stringstream comments;
                    comments<<"Initial Score Defender: "<<_defenderScoreBefore[state]<<std::endl;
                    comments<<"Initial Score Attacker: "<<_attackerScoreBefore[state]<<std::endl;
                    comments<<"Final Score Defender: "<<defenderScoreAfter<<std::endl;
                    comments<<"Final Score Attacker: "<<attackerScoreAfter<<std::endl;
                    std::cout<<comments.str();
                }catch(int e){
                    std::cerr<<"Timeout at file: "<<stateFileNames[state]<<std::endl;
                }

            }
        }
    }

}
void BuildingPlacementExperiment::runBalance() {
    if(!map){
        System::FatalError("Need to set either MapFile or Map");
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

                PlayerPtr playerOne(players[0][p1Player]);
                PlayerPtr playerTwo(players[1][p2Player]);

                setupPlayers(p1Player, p2Player, getGoal(_fixedBuildings[state]));

                std::cout<<"Balancing battle "<<stateFileNames[state]<<std::endl;
                std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
                        ", buildings: "<<_buildings[state].size()<<
                        ", defenders: "<<_defenders[state].size()<<
                        ",attackers: "<<_attackers[state].size()<<
                        ", delayedAttackers: "<<_delayedAttackers[state].size()<<
                        ", delayedDefenders: "<<_delayedDefenders[state].size()<<std::endl;


                try{
                    GameState afterState=runGame(map,
                            _fixedBuildings[state],
                            _buildings[state],
                            _defenders[state],
                            _attackers[state],
                            _delayedDefenders[state],
                            _delayedAttackers[state],
                            playerOne,
                            playerTwo);
                    bool defWonFirst = GeneticOperators::defenderWon(afterState);
                    if(defWonFirst){
                        std::cout<<"Defender won"<<std::endl;
                    }else{
                        std::cout<<"Attacker won"<<std::endl;
                    }
                    bool defWon=defWonFirst;
                    do{
                        if(defWon){//add unit to attacker
                            assert(!_attackers[state].empty());
                            Unit unit(BWAPI::UnitTypes::Protoss_Zealot,  _attackers[state][0].pos(), 0,
                                    _attackers[state][0].player(),
                                    BWAPI::UnitTypes::Protoss_Zealot.maxHitPoints()+BWAPI::UnitTypes::Protoss_Zealot.maxShields(),
                                    0, 0, 0);
                            _attackers[state].push_back(unit);
                            std::cout<<"Added attacker"<<std::endl;
                        }else{//add unit to defender
                            assert(!_defenders[state].empty());
                            Unit unit(BWAPI::UnitTypes::Protoss_Zealot,  _defenders[state][0].pos(), 0,
                                    _defenders[state][0].player(),
                                    BWAPI::UnitTypes::Protoss_Zealot.maxHitPoints()+BWAPI::UnitTypes::Protoss_Zealot.maxShields(),
                                    0, 0, 0);
                            _defenders[state].push_back(unit);
                            std::cout<<"Added defender"<<std::endl;
                        }

                        afterState=runGame(map,
                                _fixedBuildings[state],
                                _buildings[state],
                                _defenders[state],
                                _attackers[state],
                                _delayedDefenders[state],
                                _delayedAttackers[state],
                                playerOne,
                                playerTwo);

                        defWon = GeneticOperators::defenderWon(afterState);
                        if(defWon){
                            std::cout<<"Defender won"<<std::endl;
                        }else{
                            std::cout<<"Attacker won"<<std::endl;
                        }
                    }while(defWonFirst==defWon);

                    if(!defWonFirst){
                        _defenders[state].pop_back();//delete last added defender, so that the game is won by attackers
                    }

                    saveBaseAssaultStateDescriptionFile(state,stateFileNames[state]+".balancedattackers",boost::optional<const GAStatistics&>(boost::none));

                    if(!defWonFirst){
                        _defenders[state].push_back(Unit(BWAPI::UnitTypes::Protoss_Zealot,  _defenders[state][0].pos(), 0,
                                _defenders[state][0].player(),
                                BWAPI::UnitTypes::Protoss_Zealot.maxHitPoints()+BWAPI::UnitTypes::Protoss_Zealot.maxShields(),
                                0, 0, 0));//add back last defender, so that the game is won by defenders
                    }else{
                        _attackers[state].pop_back();//delete last added attacker, so that the game is won by defenders
                    }

                    saveBaseAssaultStateDescriptionFile(state,stateFileNames[state]+".balanceddefenders",boost::optional<const GAStatistics&>(boost::none));

                }catch(int e){
                    std::cerr<<"Timeout at file: "<<stateFileNames[state]<<std::endl;
                }
            }
        }
    }

}

svv BuildingPlacementExperiment::getExpDescription(){
    svv expDesc;
    expDesc.push_back(sv());
    return expDesc;
}
svv BuildingPlacementExperiment::getExpDescription(const size_t& p1,
        const size_t& p2, const size_t& state) {
    svv expDesc;
    expDesc.push_back(sv());
    return expDesc;
}

GameState BuildingPlacementExperiment::runGame(Map *map
        , const std::vector<Unit> &fixedBuildings
        , const std::vector<Unit> &buildings
        , const std::vector<Unit> &defenders
        , const std::vector<Unit> &attackers
        , const std::vector<std::pair<Unit, TimeType> > &delayedDefenders
        , const std::vector<std::pair<Unit, TimeType> > &delayedAttackers
        , PlayerPtr playerOne
        , PlayerPtr playerTwo
){
//    std::vector<std::vector<Unit> > att;att.push_back(attackers);
//    std::vector<std::vector<Unit> > delAtt;delAtt.push_back(delayedAttackers);
//    return runGame(map,fixedBuildings,buildings,defenders, att, delayedDefenders, delAtt);
//}
//GameState BuildingPlacementExperiment::runGame(const Map &map
//        , const std::vector<Unit> &fixedBuildings
//        , const std::vector<Unit> &buildings
//        , const std::vector<Unit> &defenders
//        , const std::vector<std::vector<Unit> > &attackers
//        , const std::vector<Unit> &delayedDefenders
//        , const std::vector<std::vector<Unit> > &delayedAttackers
//        ){

    GameState gameState;
    gameState.checkCollisions=true;
    gameState.setMap(*map);
    for(std::vector<SparCraft::Unit>::const_iterator it=fixedBuildings.begin();
            it!=fixedBuildings.end();it++){
        assert(it->player()==_defendPlayer);
        gameState.addUnit(*it);
    }
    for(std::vector<SparCraft::Unit>::const_iterator it=buildings.begin();
            it!=buildings.end();it++){
        assert(it->player()==_defendPlayer);
        gameState.addUnit(*it);
    }

    for(std::vector<SparCraft::Unit>::const_iterator it=defenders.begin();
            it!=defenders.end();it++){
        assert(it->player()==_defendPlayer);
        gameState.addUnitClosestLegalPos(*it);
    }
    for(std::vector<SparCraft::Unit>::const_iterator it=attackers.begin();
            it!=attackers.end();it++){
        assert(it->player()==_assaultPlayer);
        gameState.addUnitClosestLegalPos(*it);
    }

    std::vector<std::pair<Unit, TimeType> > delayedUnits(
            delayedAttackers.size()+delayedDefenders.size());
    std::merge(delayedAttackers.begin(),delayedAttackers.end(),
            delayedDefenders.begin(),delayedDefenders.end(),
            delayedUnits.begin(), Comparison());
    Game game(gameState, playerOne, playerTwo, 20000,delayedUnits);
#ifdef USING_VISUALIZATION_LIBRARIES
    if (_display!=NULL)
    {
        game.disp = _display;
        _display->SetExpDesc(getExpDescription());

    }
#endif

    // play the game to the end
    game.play();


    GeneticOperators::configure(fixedBuildings,
            buildings,
            defenders,
            attackers,
            delayedDefenders,
            delayedAttackers,
            map,_display,
            playerOne,
            playerTwo,
            getExpDescription());

    return game.getState();
}

void BuildingPlacementExperiment::runOptimize(bool cross) {
    float pmut   = 0.05;
    float pcross = 0.9;
//    gaDefDivFlag=gaTrue;
    GARandomSeed(time(NULL));

    if(!map){
        System::FatalError("Need to set either MapFile or Map");
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
                PlayerPtr playerOne(players[0][p1Player]);
                PlayerPtr playerTwo(players[1][p2Player]);

                setupPlayers(p1Player, p2Player, getGoal(_fixedBuildings[state]));




                //==================================================================================
                //====================RUN EVAL FIRST================================================



               GameState afterState=runGame(map,
                       _fixedBuildings[state],
                       _buildings[state],
                       _defenders[state],
                       _attackers[state],
                       _delayedDefenders[state],
                       _delayedAttackers[state],
                       playerOne,
                       playerTwo);

                ScoreType attackerScoreAfter,defenderScoreAfter;
                attackerScoreAfter = GeneticOperators::unitScore(afterState,_assaultPlayer);
                defenderScoreAfter = GeneticOperators::unitScore(afterState,_defendPlayer);
                std::cout<<"Final Score Defender: "<<defenderScoreAfter<<std::endl;
                std::cout<<"Final Score Attacker: "<<attackerScoreAfter<<std::endl;
                //========================FINISHED EVAL==========================================================




                // Now create the GA and run it.  First we create a genome of the type that
                // we want to use in the GA.  The ga doesn't operate on this genome in the
                // optimization - it just uses it to clone a population of genomes.
                std::cout<<"Optimizing battle "<<stateFileNames[state]<<std::endl;
                if(!cross){

                    std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
                            ", buildings: "<<_buildings[state].size()<<
                            ", defenders: "<<_defenders[state].size()<<
                            ", attackers: "<<_attackers[state].size()<<
                            ", delayedAttackers: "<<_delayedAttackers[state].size()<<
                            ", delayedDefenders: "<<_delayedDefenders[state].size()<<std::endl;


                    GeneticOperators::configure(_fixedBuildings[state],
                            _buildings[state],
                            _defenders[state],
                            _attackers[state],
                            _delayedDefenders[state],
                            _delayedAttackers[state],
                            map,_display,
                            playerOne,
                            playerTwo,
                            getExpDescription(p1Player,p2Player,state));
                }else{//cross

                    //============================balancing waves towards attackers ==================================
                    std::vector<std::vector<Unit> > attackers;
                    for(int wave=0;wave<_attackers.size();wave++){
                        if(wave!=state){
                            attackers.push_back(_attackers[wave]);
                            GameState afterState=runGame(map,
                                    _fixedBuildings[state],
                                    _buildings[state],
                                    _defenders[state],
                                    attackers.back(),
                                    _delayedDefenders[state],
                                    _delayedAttackers[wave],
                                    playerOne,
                                    playerTwo);



                            bool defWon = GeneticOperators::defenderWon(afterState);
                            if(defWon){
                                std::cout<<"Defender won"<<std::endl;
                            }else{
                                std::cout<<"Attacker won"<<std::endl;
                            }
                            while(defWon){
                                assert(!attackers.back().empty());
                                Unit unit(BWAPI::UnitTypes::Protoss_Dragoon,  attackers.back()[0].pos(), 0,
                                        attackers.back()[0].player(),
                                        BWAPI::UnitTypes::Protoss_Dragoon.maxHitPoints()+BWAPI::UnitTypes::Protoss_Dragoon.maxShields(),
                                        0, 0, 0);
                                attackers.back().push_back(unit);
                                std::cout<<"Added attacker"<<std::endl;

                                afterState=runGame(map,
                                        _fixedBuildings[state],
                                        _buildings[state],
                                        _defenders[state],
                                        attackers.back(),
                                        _delayedDefenders[state],
                                        _delayedAttackers[wave],
                                        playerOne,
                                        playerTwo);

                                defWon = GeneticOperators::defenderWon(afterState);
                                if(defWon){
                                    std::cout<<"Defender won"<<std::endl;
                                }else{
                                    std::cout<<"Attacker won"<<std::endl;
                                }
                            }
                        }
                    }
                    //=====================balancing done========================================
                    std::vector<std::vector<std::pair<Unit, TimeType> > > delayedAttackers;
                    for(int wave=0;wave<_delayedAttackers.size();wave++){
                        if(wave!=state){
                            delayedAttackers.push_back(_delayedAttackers[wave]);
                        }
                    }
                    if(attackers.empty()){
                        continue;
                    }
                    GeneticOperators::configure(_fixedBuildings[state],
                            _buildings[state],
                            _defenders[state],
                            attackers,
                            _delayedDefenders[state],
                            delayedAttackers,
                            map,_display,
                            playerOne,
                            playerTwo,
                            getExpDescription(p1Player,p2Player,state));

                }
                GAListGenome<Gene> genome(GeneticOperators::Objective);

                // Now that we have the genome, we create the genetic algorithm and set
                // its parameters - number of generations, mutation probability, and crossover
                // probability.  And finally we tell it to evolve itself.
                genome.initializer(GeneticOperators::Initializer);
                genome.mutator(GeneticOperators::Mutator);
//                genome.crossover(GeneticOperators::AverageCrossover);
                genome.crossover(GeneticOperators::UniformCrossover);
                genome.comparator(GeneticOperators::Comparator);



                GASimpleGA ga(genome);
                ga.populationSize(_popSize);
                ga.nGenerations(_genSize);
                ga.pMutation(pmut);
                ga.pCrossover(pcross);
                ga.recordDiversity(gaTrue);
                //				GATournamentSelector sel;
                //				ga.selector(sel);
                //				ga.elitist(gaTrue);

                try{
                    boost::posix_time::ptime before = boost::posix_time::microsec_clock::local_time();

                    ga.evolve();

                    boost::posix_time::ptime after = boost::posix_time::microsec_clock::local_time();
                    std::cout<<"took "<<(after-before).total_milliseconds()<<" [ms] to evovle"<<std::endl;

                    // Now we print out the best genome that the GA found.
                    GAStatistics stats=ga.statistics();
                    //				stats.recordDiversity(gaTrue);
                    std::cout << "The GA found: "<<stats.bestIndividual().evaluate()<<"\n" <<
                            stats.bestIndividual() << "\n";
                    stats.write(std::cout);


                    //==================================================================================
                    //====================RUN EVAL AFTER================================================

                    std::vector<Unit> buildings;
                    GAListGenome<Gene>& g=(GAListGenome<Gene>&)stats.bestIndividual();
                    for(int i=0;i<g.size();i++){
                        Unit u=_buildings[state][i];
                        Unit unit(u.type(), g[i]->getCenterPos(), 0, u.player(), u.currentHP(),
                                u.currentEnergy(), u.nextMoveActionTime(), u.nextAttackActionTime());
                        buildings.push_back(unit);
                    }
                    GameState afterState=runGame(map,
                            _fixedBuildings[state],
                            buildings,
                            _defenders[state],
                            _attackers[state],
                            _delayedDefenders[state],
                            _delayedAttackers[state],
                            playerOne,
                            playerTwo);


                    ScoreType attackerScoreAfterOptimize,defenderScoreAfterOptimize;
                    attackerScoreAfterOptimize = GeneticOperators::unitScore(afterState,_assaultPlayer);
                    defenderScoreAfterOptimize = GeneticOperators::unitScore(afterState,_defendPlayer);

                    //========================FINISHED EVAL==========================================================




                    std::stringstream comments;
                    comments<<"Initial Score Defender: "<<_defenderScoreBefore[state]<<std::endl;
                    comments<<"Initial Score Attacker: "<<_attackerScoreBefore[state]<<std::endl;
                    comments<<"Final Score Defender: "<<defenderScoreAfter<<std::endl;
                    comments<<"Final Score Attacker: "<<attackerScoreAfter<<std::endl;
                    comments<<"Optimized Score Defender: "<<defenderScoreAfterOptimize<<std::endl;
                    comments<<"Optimized Score Attacker: "<<attackerScoreAfterOptimize<<std::endl;
                    comments<<"Time to Optimize: "<<(after-before).total_milliseconds()<<" [ms]"<<std::endl;
                    std::stringstream filename;
                    filename<<stateFileNames[state]<<".optimized."<<_popSize<<"x"<<_genSize;
                    saveBaseAssaultStateDescriptionFile(state,filename.str(),boost::optional<const GAStatistics&>(stats), comments.str());
                    std::cout<<comments.str();
                }catch(int e){
                    std::cerr<<"Timeout at file: "<<stateFileNames[state]<<std::endl;
                }

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
    std::vector<std::pair<Unit, TimeType> > delayedAttackers,delayedDefenders;

    int skippedBuildings=0;
    ScoreType attackerScoreBefore=0, defenderScoreBefore=0;

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


        if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Dark_Templar.getName())==0){
            //2 zealots
            std::stringstream extra;
            extra<<replace(BWAPI::UnitTypes::Protoss_Zealot.getName(),' ','_')<<" ";
            extra<<playerID<<" ";
            extra<<x<<" ";
            extra<<y<<" ";
            float hpRatio=(BWAPI::UnitTypes::Protoss_Zealot.maxHitPoints()+
                    BWAPI::UnitTypes::Protoss_Zealot.maxShields())/
                            (float)(BWAPI::UnitTypes::Protoss_Dark_Templar.maxHitPoints()+
                                    BWAPI::UnitTypes::Protoss_Dark_Templar.maxShields());
            extra<<hp*hpRatio<<" ";
            extra<<time<<" ";
            extra<<fixed<<" ";
            extra<<hpFinal*hpRatio<<" ";
            lines.push_back(extra.str());
            lines.push_back(extra.str());
            std::cout<<"Unsupported unit "<<unitType<<", replacing with 2 "<<BWAPI::UnitTypes::Protoss_Zealot.getName()<<std::endl;
            continue;
        }else if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Reaver.getName())==0){
            //2 dragoons
            std::stringstream extra;
            extra<<replace(BWAPI::UnitTypes::Protoss_Dragoon.getName(),' ','_')<<" ";
            extra<<playerID<<" ";
            extra<<x<<" ";
            extra<<y<<" ";
            float hpRatio=(BWAPI::UnitTypes::Protoss_Dragoon.maxHitPoints()+
                    BWAPI::UnitTypes::Protoss_Dragoon.maxShields())/
                            (float)(BWAPI::UnitTypes::Protoss_Reaver.maxHitPoints()+
                                    BWAPI::UnitTypes::Protoss_Reaver.maxShields());
            extra<<hp*hpRatio<<" ";
            extra<<time<<" ";
            extra<<fixed<<" ";
            extra<<hpFinal*hpRatio<<" ";
            lines.push_back(extra.str());
            lines.push_back(extra.str());
            std::cout<<"Unsupported unit "<<unitType<<", replacing with 2 "<<BWAPI::UnitTypes::Protoss_Dragoon.getName()<<std::endl;
            continue;
        }else if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_High_Templar.getName())==0){
            //1 dragoon
            std::stringstream extra;
            extra<<replace(BWAPI::UnitTypes::Protoss_Dragoon.getName(),' ','_')<<" ";
            extra<<playerID<<" ";
            extra<<x<<" ";
            extra<<y<<" ";
            float hpRatio=(BWAPI::UnitTypes::Protoss_Dragoon.maxHitPoints()+
                    BWAPI::UnitTypes::Protoss_Dragoon.maxShields())/
                            (float)(BWAPI::UnitTypes::Protoss_High_Templar.maxHitPoints()+
                                    BWAPI::UnitTypes::Protoss_High_Templar.maxShields());
            extra<<hp*hpRatio<<" ";
            extra<<time<<" ";
            extra<<fixed<<" ";
            extra<<hpFinal*hpRatio<<" ";
            lines.push_back(extra.str());
            std::cout<<"Unsupported unit "<<unitType<<", replacing with 1 "<<BWAPI::UnitTypes::Protoss_Zealot.getName()<<std::endl;
            continue;
        }else if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Archon.getName())==0){
            //1 dragoon
            std::stringstream extra;
            extra<<replace(BWAPI::UnitTypes::Protoss_Dragoon.getName(),' ','_')<<" ";
            extra<<playerID<<" ";
            extra<<x<<" ";
            extra<<y<<" ";
            float hpRatio=(BWAPI::UnitTypes::Protoss_Dragoon.maxHitPoints()+
                    BWAPI::UnitTypes::Protoss_Dragoon.maxShields())/
                            (float)(BWAPI::UnitTypes::Protoss_Archon.maxHitPoints()+
                                    BWAPI::UnitTypes::Protoss_Archon.maxShields());
            extra<<hp*hpRatio<<" ";
            extra<<time<<" ";
            extra<<fixed<<" ";
            extra<<hpFinal*hpRatio<<" ";
            lines.push_back(extra.str());
            //1 zealots
            extra.str("");
            extra<<replace(BWAPI::UnitTypes::Protoss_Zealot.getName(),' ','_')<<" ";
            extra<<playerID<<" ";
            extra<<x<<" ";
            extra<<y<<" ";
            hpRatio=(BWAPI::UnitTypes::Protoss_Zealot.maxHitPoints()+
                    BWAPI::UnitTypes::Protoss_Zealot.maxShields())/
                            (float)(BWAPI::UnitTypes::Protoss_Archon.maxHitPoints()+
                                    BWAPI::UnitTypes::Protoss_Archon.maxShields());
            extra<<hp*hpRatio<<" ";
            extra<<time<<" ";
            extra<<fixed<<" ";
            extra<<hpFinal*hpRatio<<" ";
            lines.push_back(extra.str());

            std::cout<<"Unsupported unit "<<unitType<<", replacing with 1 "<<
                    BWAPI::UnitTypes::Protoss_Zealot.getName()<<" and 1 "<<
                    BWAPI::UnitTypes::Protoss_Zealot.getName()<<std::endl;
            continue;
        }

        BWAPI::UnitType type;

        std::streambuf* oldCoutStreamBuf = std::cerr.rdbuf();
        std::ofstream filestr;
        filestr.open ("/dev/null");
        std::cerr.rdbuf( filestr.rdbuf() );
        try{
            type=getUnitType(unitType);

            // Restore old cout.
            std::cerr.rdbuf( oldCoutStreamBuf );
            filestr.close();
        }catch(int i){
            // Restore old cout.
            std::cerr.rdbuf( oldCoutStreamBuf );
            filestr.close();
            if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Shield_Battery.getName())==0){
                std::cerr<<"Unsupported unit type, skipping "<<unitType<<std::endl;
                continue;
            }else if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Scarab.getName())==0){
                std::cerr<<"Unsupported unit type, skipping "<<unitType<<std::endl;
                continue;
            }else if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Shuttle.getName())==0){
                std::cerr<<"Unsupported unit type, skipping "<<unitType<<std::endl;
                continue;
            }else if(replace(unitType,'_',' ').compare(BWAPI::UnitTypes::Protoss_Observer.getName())==0){
                std::cerr<<"Unsupported unit type, skipping "<<unitType<<std::endl;
                continue;
            }else{
                std::stringstream ss;
                ss<<"Unsupported unit type "<<unitType<<", aborting";
                SparCraft::System::FatalError(ss.str());
            }

        }

        if(!type.isBuilding()&&fixed){
            SparCraft::System::FatalError("A non building unit cannot be fixed");
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
                type == BWAPI::UnitTypes::Terran_Medic ? Constants::Starting_Energy : 0, time, time);

        if(playerID==_assaultPlayer){//assault
            if(type.isBuilding()){
                std::cerr<<"Assault player cannot have buildings, skipping "<<unitType<<std::endl;
//                SparCraft::System::FatalError("Assault player cannot have buildings");
            }else if(time>0){
                delayedAttackers.push_back(std::pair<Unit, TimeType>(unit,time));
            }else{
                attackers.push_back(unit);
            }
            attackerScoreBefore+=GeneticOperators::unitScore(unit);
        }else{//defend
            if(time>0){
                if(type.isBuilding()){
                    if(time>1000){
                        skippedBuildings++;
                        std::cerr<<"Cannot have delayed buildings by "<<time<<"[s], skipping "<<type.getName()<<std::endl;
                    }else{
                        std::cerr<<"Cannot have delayed buildings, but only "<<time<<"[s], so adding at beginning "<<type.getName()<<std::endl;
                        Unit unit(type, Position(x, y), 0, playerID, hp,
                                       type == BWAPI::UnitTypes::Terran_Medic ? Constants::Starting_Energy : 0, 0, 0);
                        buildings.push_back(unit);
                    }
                    //                    std::cerr<<"Cannot have delayed buildings, adding at beginning "<<type.getName()<<std::endl;
                    //                    buildings.push_back(unit);


                    //                    SparCraft::System::FatalError("Cannot have delayed buildings, aborting");
                }else{
                    delayedDefenders.push_back(std::pair<Unit, TimeType>(unit,time));
                }
            }else{
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
            defenderScoreBefore+=GeneticOperators::unitScore(unit);
        }
    }

    if(skippedBuildings>2){
        std::cerr<<"Skipped too many buildings, skipping battle"<<std::endl;
        return;
    }
    if((fixedBuildings.size()+buildings.size()) < 4){
        std::cerr<<"Too few buildings, skipping battle"<<std::endl;
        return;
    }
    _attackerScoreBefore.push_back(attackerScoreBefore);
    _defenderScoreBefore.push_back(defenderScoreBefore);

    _fixedBuildings.push_back(fixedBuildings);

    std::sort(buildings.begin(),buildings.end(),PylonsFirst());
    _buildings.push_back(buildings);

    _attackers.push_back(attackers);
    _defenders.push_back(defenders);

    std::sort(delayedAttackers.begin(),delayedAttackers.end(),Comparison());
    _delayedAttackers.push_back(delayedAttackers);

    std::sort(delayedDefenders.begin(),delayedDefenders.end(),Comparison());
    _delayedDefenders.push_back(delayedDefenders);


    stateFileNames.push_back(fileName);
}

void BuildingPlacementExperiment::unitToString(std::stringstream &ss, const Unit &unit, bool fixed){
    std::string name=unit.type().getName();
    std::replace( name.begin(), name.end(), ' ', '_');
    ss<<name<<" "<<unit.player()<<" "<<unit.x()<<" "<<unit.y()<<" ";
    ss<<unit.currentHP()<<" "<<std::min(unit.nextMoveActionTime(),unit.nextAttackActionTime())<<" ";
    ss<<fixed<<" "<<0<<std::endl;
}

void BuildingPlacementExperiment::unitsToString(std::stringstream &ss, const std::vector<Unit> &units, bool fixed){
    BOOST_FOREACH(const SparCraft::Unit &unit, units){
        unitToString(ss,unit,fixed);
    }

}

void BuildingPlacementExperiment::saveBaseAssaultStateDescriptionFile(int state, const std::string & fileName,
        const boost::optional<const GAStatistics &> stats,
        std::string comments){

    std::stringstream ss;
    ss<<"#Fixed buildings"<<std::endl;
    unitsToString(ss,_fixedBuildings[state],true);
    ss<<"#Attackers"<<std::endl;
    unitsToString(ss,_attackers[state]);
    ss<<"#Defenders"<<std::endl;
    unitsToString(ss,_defenders[state]);

    ss<<"#Delayed Attackers"<<std::endl;
    for(std::vector<std::pair<Unit, TimeType> >::const_iterator it=_delayedAttackers[state].begin();
                    it!=_delayedAttackers[state].end();it++){
        unitToString(ss,it->first);
    }
    ss<<"#Delayed Defenders"<<std::endl;
    for(std::vector<std::pair<Unit, TimeType> >::const_iterator it=_delayedDefenders[state].begin();
            it!=_delayedDefenders[state].end();it++){
        unitToString(ss,it->first);
    }

    ss<<"#Non-fixed Buildings"<<std::endl;
    //take buildings from GA or original array
    if(stats.is_initialized()){
        GAListGenome<Gene>& g=(GAListGenome<Gene>&)stats->bestIndividual();
        for(int i=0;i<g.size();i++){
            Unit u=_buildings[state][i];
            Unit unit(u.type(), g[i]->getCenterPos(), 0, u.player(), u.currentHP(),
                    u.currentEnergy(), u.nextMoveActionTime(), u.nextAttackActionTime());
            unitToString(ss,unit);
        }
    }else{
        unitsToString(ss,_buildings[state]);
    }

    //add comment about what was done to this file
    if(stats.is_initialized()){
        ss<<"#Optimized against attackers from"<<std::endl;
        for(int i=0;i<_buildings.size();i++){
            if(i!=state){
                ss<<"#"<<stateFileNames[i]<<std::endl;
            }
        }
    }else{
        ss<<"#Balanced base assault"<<std::endl;
    }
    std::ofstream file(fileName.c_str(), std::ofstream::out);

    file<<ss.str();

    //write GA stats if they exist
    if(stats.is_initialized()){
        std::stringstream statsStream;
        stats->write(statsStream);

        char buff[80];
        while(!statsStream.eof()){
            statsStream.getline(buff,80);
            file<<"#"<<buff<<std::endl;
        }
    }
    std::stringstream comm(comments);
    char buff[80];
    while(!comm.eof()){
        comm.getline(buff,80);
        file<<"#"<<buff<<std::endl;
    }
    file.close();

}
void BuildingPlacementExperiment::setupPlayers(size_t p1Player,
        size_t p2Player, const Position& goal) {

    // get player one
    PlayerPtr playerOne(players[0][p1Player]);

    // give it a new transposition table if it's an alpha beta player
    Player_AlphaBeta * p1AB = dynamic_cast<Player_AlphaBeta *>(playerOne.get());
    if (p1AB)
    {
        p1AB->setTranspositionTable(TTPtr(new SparCraft::TranspositionTable()));
    }else{
        Player_Goal * p1goal = dynamic_cast<Player_Goal *>(playerOne.get());
        if (p1goal)
        {
            p1goal->setGoal(goal);
        }
    }

    // get player two
    PlayerPtr playerTwo(players[1][p2Player]);
    Player_AlphaBeta * p2AB = dynamic_cast<Player_AlphaBeta *>(playerTwo.get());
    if (p2AB)
    {
        p2AB->setTranspositionTable(TTPtr(new SparCraft::TranspositionTable()));
    }else{
        Player_Goal * p2goal = dynamic_cast<Player_Goal *>(playerTwo.get());
        if (p2goal)
        {
            p2goal->setGoal(goal);
        }
    }
}

void BuildingPlacementExperiment::setDisplay(bool showDisplay,
        std::string dir) {
    imageDir=dir;
    this->showDisplay=showDisplay;
}

void BuildingPlacementExperiment::checkCol(bool check) {
    checkCollisions=check;
}

void BuildingPlacementExperiment::runDisplay() {
    if(!map){
        System::FatalError("Need to set either MapFile or Map");
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

                PlayerPtr playerOne(players[0][p1Player]);
                PlayerPtr playerTwo(players[1][p2Player]);

                setupPlayers(p1Player, p2Player, getGoal(_fixedBuildings[state]));
                std::cout<<"Displaying battle "<<stateFileNames[state]<<std::endl;
                std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
                        ", buildings: "<<_buildings[state].size()<<
                        ", defenders: "<<_defenders[state].size()<<
                        ",attackers: "<<_attackers[state].size()<<
                        ", delayedAttackers: "<<_delayedAttackers[state].size()<<
                        ", delayedDefenders: "<<_delayedDefenders[state].size()<<std::endl;



                GameState gameState;
                gameState.checkCollisions=true;
                gameState.setMap(*map);
                for(std::vector<SparCraft::Unit>::const_iterator it=_fixedBuildings[state].begin();
                        it!=_fixedBuildings[state].end();it++){
                    assert(it->player()==_defendPlayer);
                    gameState.addUnit(*it);
                }
                for(std::vector<SparCraft::Unit>::const_iterator it=_buildings[state].begin();
                        it!=_buildings[state].end();it++){
                    assert(it->player()==_defendPlayer);
                    gameState.addUnit(*it);
                }

                for(std::vector<SparCraft::Unit>::const_iterator it=_defenders[state].begin();
                        it!=_defenders[state].end();it++){
                    assert(it->player()==_defendPlayer);
                    gameState.addUnit(*it);
                }
                for(std::vector<SparCraft::Unit>::const_iterator it=_attackers[state].begin();
                        it!=_attackers[state].end();it++){
                    assert(it->player()==_assaultPlayer);
                    gameState.addUnit(*it);
                }

                std::vector<std::pair<Unit, TimeType> > delayedUnits(
                        _delayedAttackers[state].size()+_delayedDefenders[state].size());
                std::merge(_delayedAttackers[state].begin(),_delayedAttackers[state].end(),
                        _delayedDefenders[state].begin(),_delayedDefenders[state].end(),
                        delayedUnits.begin(), Comparison());
#ifdef USING_VISUALIZATION_LIBRARIES
                if (_display!=NULL)
                {
                    _display->SetExpDesc(getExpDescription(p1Player,p2Player,state));

                }
#endif

                _display->SetState(gameState);

                while(1){
                    _display->OnFrame();
                }



            }
        }
    }

}

}

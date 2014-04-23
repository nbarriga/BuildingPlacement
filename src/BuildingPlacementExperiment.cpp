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
                    SearchExperiment(),
                    _display(NULL),
                    _assaultPlayer(std::numeric_limits<IDType>::max()),
                    _defendPlayer(std::numeric_limits<IDType>::max()){
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
                        gameState.addUnit(*it);
                    }
                    for(std::vector<SparCraft::Unit>::const_iterator it=_attackers[otherState].begin();
                            it!=_attackers[otherState].end();it++){
                        assert(it->player()==_assaultPlayer);
                        gameState.addUnit(*it);
                    }


                    std::vector<std::pair<Unit, TimeType> > delayedUnits(
                            _delayedAttackers[otherState].size()+_delayedDefenders[state].size());
                    std::merge(_delayedAttackers[otherState].begin(),_delayedAttackers[otherState].end(),
                            _delayedDefenders[state].begin(),_delayedDefenders[state].end(),
                            delayedUnits.begin(), Comparison());
                    Game game(gameState, playerOne, playerTwo, 2000,delayedUnits);

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
                            delayedUnits,
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
                Game game(gameState, playerOne, playerTwo, 2000,delayedUnits);
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
                        _attackers[state],
                        delayedUnits,
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

svv BuildingPlacementExperiment::getExpDescription(const size_t& p1,
        const size_t& p2, const size_t& state) {
    svv expDesc;
    expDesc.push_back(sv());
    return expDesc;
}

void BuildingPlacementExperiment::runOptimize() {
    int popsize  = 5;
    int ngen     = 8;
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

                // Now create the GA and run it.  First we create a genome of the type that
                // we want to use in the GA.  The ga doesn't operate on this genome in the
                // optimization - it just uses it to clone a population of genomes.

                std::cout<<"fixed buildings: "<<_fixedBuildings[state].size()<<
                        ", buildings: "<<_buildings[state].size()<<
                        ", defenders: "<<_defenders[state].size()<<
                        ", attackers: "<<_attackers[state].size()<<
                        ", delayedAttackers: "<<_delayedAttackers[state].size()<<
                        ", delayedDefenders: "<<_delayedDefenders[state].size()<<std::endl;

                std::vector<std::pair<Unit, TimeType> > delayedUnits(
                        _delayedAttackers[state].size()+_delayedDefenders[state].size());
                std::merge(_delayedAttackers[state].begin(),_delayedAttackers[state].end(),
                        _delayedDefenders[state].begin(),_delayedDefenders[state].end(),
                        delayedUnits.begin(), Comparison());

                GeneticOperators::configure(_fixedBuildings[state],
                        _buildings[state],
                        _defenders[state],
                        _attackers[state],
                        delayedUnits,
                        map,_display,
                        playerOne,
                        playerTwo,
                        getExpDescription(p1Player,p2Player,state));

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
                ga.populationSize(popsize);
                ga.nGenerations(ngen);
                ga.pMutation(pmut);
                ga.pCrossover(pcross);
                ga.recordDiversity(gaTrue);
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


                saveBaseAssaultStateDescriptionFile(state,stateFileNames[state]+".optimized",stats);
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
            stateFileNames.push_back(filename);
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
                type == BWAPI::UnitTypes::Terran_Medic ? Constants::Starting_Energy : 0, time, time);


        if(playerID==_assaultPlayer){//assault
            if(type.isBuilding()){
                SparCraft::System::FatalError("Assault player cannot have buildings");
            }else if(time>0){
                delayedAttackers.push_back(std::pair<Unit, TimeType>(unit,time));
            }else{
                attackers.push_back(unit);
            }
        }else{//defend
            if(time>0){
                if(type.isBuilding()){
                    std::cerr<<"Cannot have delayed buildings, adding at beginning "<<type.getName()<<std::endl;
                    buildings.push_back(unit);
//                    SparCraft::System::FatalError("Cannot have delayed buildings");
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
        }
    }


    _fixedBuildings.push_back(fixedBuildings);

    std::sort(buildings.begin(),buildings.end(),PylonsFirst());
    _buildings.push_back(buildings);

    _attackers.push_back(attackers);
    _defenders.push_back(defenders);

    std::sort(delayedAttackers.begin(),delayedAttackers.end(),Comparison());
    _delayedAttackers.push_back(delayedAttackers);

    std::sort(delayedDefenders.begin(),delayedDefenders.end(),Comparison());
    _delayedDefenders.push_back(delayedDefenders);
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

void BuildingPlacementExperiment::saveBaseAssaultStateDescriptionFile(int state, const std::string & fileName, const GAStatistics &stats){

    std::stringstream ss;
    unitsToString(ss,_fixedBuildings[state],true);
    unitsToString(ss,_attackers[state]);
    unitsToString(ss,_defenders[state]);

    for(std::vector<std::pair<Unit, TimeType> >::const_iterator it=_delayedAttackers[state].begin();
                    it!=_delayedAttackers[state].end();it++){
        unitToString(ss,it->first);
    }
    for(std::vector<std::pair<Unit, TimeType> >::const_iterator it=_delayedDefenders[state].begin();
            it!=_delayedDefenders[state].end();it++){
        unitToString(ss,it->first);
    }


    GAListGenome<Gene>& g=(GAListGenome<Gene>&)stats.bestIndividual();
    for(int i=0;i<g.size();i++){
        Unit u=_buildings[state][i];
        Unit unit(u.type(), g[i]->getCenterPos(), 0, u.player(), u.currentHP(),
                u.currentEnergy(), u.nextMoveActionTime(), u.nextAttackActionTime());
        unitToString(ss,unit);
    }


    std::ofstream file(fileName.c_str(), std::ofstream::out);
    file<<ss.str();

    std::stringstream statsStream;
    stats.write(statsStream);

    char buff[80];
    while(!statsStream.eof()){
        statsStream.getline(buff,80);
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

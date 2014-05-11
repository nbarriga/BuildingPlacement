#pragma once

#include "SearchExperiment.h"
#include "Common.h"
#include <ga/GAStatistics.h>

namespace BuildingPlacement {
class BuildingPlacementExperiment:public SearchExperiment {


    struct PylonsFirst{
        bool isPylon(const Unit &u) const{
            return u.type()==BWAPI::UnitTypes::Protoss_Pylon;
        }
        bool operator() (const Unit &lhs, const Unit &rhs) const
        {
            if(isPylon(lhs)){
                if(isPylon(rhs)){
                    return false;
                }else{
                    return true;
                }
            }else if(isPylon(rhs)){
                return false;
            }else{
                if(lhs.type().requiresPsi()){
                    if(rhs.type().requiresPsi()){
                        return false;
                    }else{
                        return true;
                    }
                }else if(rhs.type().requiresPsi()){
                    return false;
                }else{
                    return false;
                }
            }
        }
    };

	Display *_display;
	std::vector<std::vector<Unit> > _fixedBuildings,_buildings,_attackers,_defenders;
	std::vector<std::vector<std::pair<Unit, TimeType> > > _delayedAttackers, _delayedDefenders;

	std::vector<ScoreType> _attackerScoreBefore, _defenderScoreBefore;

	std::vector<std::string> stateFileNames;

	void parseBaseAssaultStateDescriptionFile(const std::string & fileName);
	void saveBaseAssaultStateDescriptionFile(int state, const std::string & fileName, const boost::optional<const GAStatistics &> stats, std::string comments="");
	void unitsToString(std::stringstream &ss, const std::vector<Unit> &units, bool fixed=false);
	void unitToString(std::stringstream &ss, const Unit &unit, bool fixed=false);
	void setupPlayers(size_t p1Player, size_t p2Player, const Position& goal);
	const Position& getGoal(const std::vector<Unit> &fixedBuildings) const;
//	GameState runGame(const Map &map
//	        , const std::vector<Unit> &fixedBuildings
//	        , const std::vector<Unit> &buildings
//	        , const std::vector<Unit> &defenders
//	        , const std::vector<std::vector<Unit> > &attackers
//	        , const std::vector<Unit> &delayedDefenders
//	        , const std::vector<std::vector<Unit> > &delayedAttackers
//	        );
	GameState runGame(Map *map
	        , const std::vector<Unit> &fixedBuildings
	        , const std::vector<Unit> &buildings
	        , const std::vector<Unit> &defenders
	        , const std::vector<Unit> &attackers
	        , const std::vector<std::pair<Unit, TimeType> > &delayedDefenders
	        , const std::vector<std::pair<Unit, TimeType> > &delayedAttackers
	        , PlayerPtr playerOne
	        , PlayerPtr playerTwo
	);

	IDType _assaultPlayer,_defendPlayer;
    int _popSize, _genSize;
public:
	struct Comparison{
	    bool operator() (const std::pair<Unit, TimeType>& lhs, const std::pair<Unit, TimeType>&rhs) const
	    {
	        return (lhs.second>rhs.second);
	    }
	};
	void setDisplay(bool showDisplay, std::string dir);
	void checkCol(bool check);
	BuildingPlacementExperiment(const std::string & configFile);
	BuildingPlacementExperiment();
	virtual ~BuildingPlacementExperiment();
	virtual void addPlayer(const std::string & line);//override
	virtual void addState(const std::string & line);//override
	virtual svv getExpDescription(const size_t & p1, const size_t & p2, const size_t & state);
	svv getExpDescription();
	void runEvaluate();
	void runBalance();
	void runOptimize(bool cross=false);
	void runCross();
	void runDisplay();
	void setPop(int popSize){
	    _popSize=popSize;
	}
	void setGen(int genSize){
	    _genSize=genSize;
	}


};

}

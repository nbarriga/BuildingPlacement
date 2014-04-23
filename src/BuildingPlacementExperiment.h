#pragma once

#include "SearchExperiment.h"
#include "Common.h"
#include <ga/GAStatistics.h>

namespace BuildingPlacement {
class BuildingPlacementExperiment:public SearchExperiment {
    struct Comparison{
        bool operator() (const std::pair<Unit, TimeType>& lhs, const std::pair<Unit, TimeType>&rhs) const
        {
            return (lhs.second>rhs.second);
        }
    };

    struct PylonsFirst{
        bool isPylon(const Unit &u) const{
            return u.type()==BWAPI::UnitTypes::Protoss_Pylon;
        }
        bool operator() (const Unit &lhs, const Unit &rhs) const
        {
            if(isPylon(lhs)){
                return true;
            }else if(isPylon(rhs)){
                return false;
            }else{
                if(lhs.type().requiresPsi()){
                    return true;
                }else if(rhs.type().requiresPsi()){
                    return false;
                }else{
                    return true;
                }
            }
        }
    };

	Display *_display;
	std::vector<std::vector<Unit> > _fixedBuildings,_buildings,_attackers,_defenders;
	std::vector<std::vector<std::pair<Unit, TimeType> > > _delayedAttackers, _delayedDefenders;

	std::vector<std::string> stateFileNames;

	void parseBaseAssaultStateDescriptionFile(const std::string & fileName);
	void saveBaseAssaultStateDescriptionFile(int state, const std::string & fileName, const GAStatistics &stats);
	void unitsToString(std::stringstream &ss, const std::vector<Unit> &units, bool fixed=false);
	void unitToString(std::stringstream &ss, const Unit &unit, bool fixed=false);
	void setupPlayers(size_t p1Player, size_t p2Player, const Position& goal);
	const Position& getGoal(const std::vector<Unit> &fixedBuildings) const;

	IDType _assaultPlayer,_defendPlayer;
public:
	BuildingPlacementExperiment(const std::string & configFile);
	virtual ~BuildingPlacementExperiment();
	virtual void addPlayer(const std::string & line);//override
	virtual void addState(const std::string & line);//override
	virtual svv getExpDescription(const size_t & p1, const size_t & p2, const size_t & state);
	void runEvaluate();
	void runOptimize();
	void runCross();
	void runDisplay();
};

}

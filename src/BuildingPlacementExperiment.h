#pragma once

#include "SearchExperiment.h"
#include "Common.h"

namespace BuildingPlacement {
class BuildingPlacementExperiment:public SearchExperiment {
    struct Comparison{
        bool operator() (const std::pair<Unit, TimeType>& lhs, const std::pair<Unit, TimeType>&rhs) const
        {
            return (lhs.second>rhs.second);
        }
    };

	Display *_display;
	std::vector<std::vector<Unit> > _fixedBuildings,_buildings,_attackers,_defenders;
	std::vector<std::vector<std::pair<Unit, TimeType> > > _delayedAttackers, _delayedDefenders;
	void parseBaseAssaultStateDescriptionFile(const std::string & fileName);
	void setupPlayers(size_t p1Player, size_t p2Player, const Position& goal);
	const Position& getGoal(const std::vector<Unit> &fixedBuildings) const;
public:
	BuildingPlacementExperiment(const std::string & configFile);
	virtual ~BuildingPlacementExperiment();
	virtual void addPlayer(const std::string & line);//override
	virtual void addState(const std::string & line);//override
	virtual svv getExpDescription(const size_t & p1, const size_t & p2, const size_t & state);
	void runEvaluate();
	void runOptimize();
	void runCross();
};

}

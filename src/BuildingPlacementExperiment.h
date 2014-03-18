#pragma once

#include "SearchExperiment.h"
#include "Common.h"

namespace BuildingPlacement {
class BuildingPlacementExperiment:public SearchExperiment {
	Display *_display;
	std::vector<std::vector<Unit> > _fixedBuildings,_buildings,_attackers,_defenders;
	void parseBaseAssaultStateDescriptionFile(const std::string & fileName);
public:
	BuildingPlacementExperiment(const std::string & configFile);
	virtual ~BuildingPlacementExperiment();
	virtual void addPlayer(const std::string & line);//override
	virtual void addState(const std::string & line);//override
	void runExperiment();
};

}

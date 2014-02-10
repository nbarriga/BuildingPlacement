#pragma once

#include "SearchExperiment.h"
#include "Common.h"

namespace BuildingPlacement {
class BuildingPlacementExperiment:public SearchExperiment {
	Display *_display;
public:
	BuildingPlacementExperiment(const std::string & configFile);
	virtual ~BuildingPlacementExperiment();
	virtual void addPlayer(const std::string & line);//override
	void runExperiment();
};

}

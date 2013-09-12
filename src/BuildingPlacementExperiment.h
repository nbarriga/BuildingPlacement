#pragma once

#include "SearchExperiment.h"

namespace SparCraft {
class BuildingPlacementExperiment:public SearchExperiment {
	Display *_display;
public:
	BuildingPlacementExperiment(const std::string & configFile);
	virtual ~BuildingPlacementExperiment();
	void runExperiment();
};

}

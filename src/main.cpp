#include "SparCraft.h"
#include "BuildingPlacementExperiment.h"
#include <boost/program_options.hpp>
#include "Gene.h"

void myterminate () {
    static bool tried_throw = false;
    try {
        if (!tried_throw++) throw;
        std::cerr << "No active exception" << std::endl;
    }catch (const std::exception &ex) {
        std::cerr << std::endl<<"       "<< ex.what()<< std::endl<<std::endl;
        SparCraft::System::printStackTrace(4);
    }catch (...) {
        std::cerr << "Terminate handler called for an unknown exception" <<std::endl;
    }
    abort();  // forces abnormal termination
}

void test(){
    BuildingPlacement::Gene a(BWAPI::UnitTypes::Protoss_Photon_Cannon,BWAPI::TilePosition(115,44));
    BuildingPlacement::Gene b(BWAPI::UnitTypes::Protoss_Gateway,BWAPI::TilePosition(114,48));
    assert(a.collides(b));
}

int main(int argc, char *argv[])
{
	SparCraft::init();
	std::set_terminate (myterminate);

	try
	{
		if(argc>=2){
			std::string experimentArg, configArg, mapArg, imageDir;
			std::vector<std::string> baseArg, stateArg;
			int assault,defend;
			boost::program_options::options_description desc("Allowed options");
			desc.add_options()("help,h", "prints this help message")
                              ("test,t", "runs test function")
            		          ("experiment,e", boost::program_options::value<std::string>(&experimentArg)->default_value("evaluate"), "set experiment")
            		          ("config,c", boost::program_options::value<std::string>(&configArg), "config file")
            		          ("state,s", boost::program_options::value<std::vector<std::string> >(&stateArg), "state description file")
            		          ("base,b", boost::program_options::value<std::vector<std::string> >(&baseArg), "base assault state description file")
            		          ("map,m", boost::program_options::value<std::string>(&mapArg), "map file (replaces the one in config file)")
            		          ("nocollisions,n", "disable check for collision")
            		          ("display,d", "enable display")
            		          ("images,i", boost::program_options::value<std::string>(&imageDir)->default_value("../sparcraft/starcraft_images/"), "starcraft images directory")
            		          ("assault,a", boost::program_options::value<int>(&assault)->default_value(0), "assault player id(default 0)")
            		          ("defend,f", boost::program_options::value<int>(&defend)->default_value(1), "defend player id(default 1)")
            		          ;
			boost::program_options::positional_options_description pd;
			pd.add("base", -1);
			boost::program_options::variables_map vm;
			boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
			boost::program_options::notify(vm);

			if (vm.count("help")) {
				std::cout << desc << std::endl;
				return 1;
			}
			if (vm.count("test")) {
			    test();
			    return 1;
			}
			BuildingPlacement::BuildingPlacementExperiment exp;
			if(vm.count("config")!=1){//if no config file is passed, we need at least some options
			    assert(vm.count("base")>0);
			    assert(vm.count("map")>0);
//			    SparCraft::System::FatalError("Please provide experiment file");
			}else{
			    exp.init(configArg);
			}

			std::stringstream ss;
			ss<<"Player "<<assault<<" Assault";
			exp.addPlayer(ss.str());
			ss.str("");
			ss<<"Player "<<defend<<" Defend";
			exp.addPlayer(ss.str());
			if(vm.count("nocollisions")>0){
			    exp.checkCol(false);
			}
			if(vm.count("display")>0){
			    if(vm.count("images")>0){
			        exp.setDisplay(true,imageDir);
			    }else{
			        SparCraft::System::FatalError("Images directory needed");
			    }
			}

			if(vm.count("state")>0){
			    BOOST_FOREACH(const std::string &state,stateArg){
			        std::stringstream ss;
			        ss<<"State StateDescriptionFile 1 "<<state;
			        std::cout<<"Adding state file: "<<ss.str()<<std::endl;
			        exp.addState(ss.str());
			    }
			}
			if(vm.count("base")>0){
			    BOOST_FOREACH(const std::string &base,baseArg){
			        std::stringstream ss;
			        ss<<"State BaseAssaultStateDescriptionFile 1 "<<base;
                    std::cout<<"Adding state file: "<<ss.str()<<std::endl;
			        exp.addState(ss.str());
			    }
			}
            if(vm.count("map")>0){
                std::stringstream ss;
                SparCraft::Map *map = new SparCraft::Map();
                map->load(mapArg);
                exp.setMap(map);
            }
			if(experimentArg.compare("evaluate")==0){
			    exp.runEvaluate();
			}else if(experimentArg.compare("optimize")==0){
			    exp.runOptimize();
			}else if(experimentArg.compare("crossoptimize")==0){
                exp.runOptimize(true);
            }else if(experimentArg.compare("crossevaluate")==0){
			    exp.runCross();
            }else if(experimentArg.compare("display")==0){
                exp.runDisplay();
            }else{
				SparCraft::System::FatalError("Error parsing arguments");
			}
		}
		else
		{
			SparCraft::System::FatalError("Please provide at least experiment file as only argument");
		}
	}
	catch(int e)
	{
		if (e == SparCraft::System::SPARCRAFT_FATAL_ERROR)
		{
			std::cerr << "\nSparCraft FatalError Exception, Shutting Down\n\n";
		}
		else
		{
			std::cerr << "\nUnknown Exception, Shutting Down\n\n";
		}
	}

	return 0;
}

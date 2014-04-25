/*
 * Common.cpp
 *
 *  Created on: Apr 25, 2014
 *      Author: nbarriga
 */

#include "Common.h"


namespace BuildingPlacement{
std::string replace(const std::string& s, char old_value, char new_value){
    std::string newS=s;
    std::replace(newS.begin(),newS.end(),old_value,new_value);
    return newS;
}
}

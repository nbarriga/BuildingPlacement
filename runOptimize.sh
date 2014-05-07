#!/bin/bash
for dir in $(ls -d  $1/*/ | sed "s/\/$//g"); do 
    for subdir in $(ls -d  ${dir}/*/ | sed "s/\/$//g"); do 
	    ./BuildPlacement -m ${dir}_map.txt  ${subdir}/*.txt.balancedattackers -e crossoptimize;
	    ./BuildPlacement -m ${dir}_map.txt  ${subdir}/*.txt.balanceddefenders -e crossoptimize;
    done
done

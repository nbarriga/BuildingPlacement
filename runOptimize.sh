#!/bin/bash
declare -a options=('-p 15 -g 40' '-p 10 -g 20')
for opt in "${options[@]}";do
    for dir in $(ls -d  $1/*/ | sed "s/\/$//g"); do 
        for subdir in $(ls -d  ${dir}/*/ | sed "s/\/$//g"); do 
	        ./BuildPlacement ${opt} -m ${dir}_map.txt  ${subdir}/*.txt.balancedattackers -e crossoptimize;
	        ./BuildPlacement ${opt} -m ${dir}_map.txt  ${subdir}/*.txt.balanceddefenders -e crossoptimize;
        done
    done
done

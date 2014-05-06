#!/bin/bash
for dir in $(ls -d  $1/*/ | sed "s/\/$//g"); do 
	./BuildPlacement -m ${dir}_map.txt  ${dir}/*/*.txt.balanced -e crossoptimize;
done

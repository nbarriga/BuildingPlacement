#!/bin/bash
echo Human=0/bot=1, Att=0/Def=1 wins, Number of attack waves used to train, Initial Score Defender, Initial Score Attacker, Final Score Defender, Final Score Attacker, Optimized Score Defender, Optimized Score Attacker
dirs=(test_battles/ bot_battles/)
balance=(attackers defenders)
i=0
for dir in ${dirs[@]}; do
    j=0
    for bal in ${balance[@]}; do
        for file in $(find ${dir} -name *balanced${bal}.optimized); do
            #echo ${file}
            numBattles=$(cat ${file}|grep balanced|wc -l)
            echo $i $j ${numBattles} $(cat ${file}|grep Score|sed s/[^0-9]*//g)
        done
        j=$((j+1))
    done
    i=$((i+1))
done

#!/bin/bash

ds=sl;
. ./scripts/run.config

mkdir -p data
make clean
make "SET_CPU := 0"

algos=( SL_FRASER SL_HERLIHY_LB SL_HERLIHY_LF SL_OPTIK );

params_initial=(  1024 16384 65536 1024 16384 65536 );
params_update=(   40   40    40    40   40    40 );
params_workload=( 0    0     0     2    2     2 );

num_params=${#params_initial[*]};

. ./scripts/cores.config

num_tests_cores=$(echo "$cores" | wc -w);
duration_secs=$( echo "$duration/1000" | bc -l );
num_algorithms=${#algos[@]};

dur_tot=$( echo "$num_algorithms*$num_params*$num_tests_cores*$repetitions*$duration_secs" | bc -l );

printf "#> $num_algorithms algos, $num_params params, $num_tests_cores cores configurations, $repetitions reps of %.2f sec = %.2f sec\n" $duration_secs $dur_tot;
printf "#> = %.2f hours\n" $( echo "$dur_tot/3600" | bc -l );

#printf "   Continue? [Y/n] ";
#read cont;
#if [ "$cont" = "n" ]; then
#	exit;
#fi;

#cores=$cores_backup;

for ((i=0; i < $num_params; i++))
do
    initial=${params_initial[$i]};
    update=${params_update[$i]};
    range=$((2*$initial));

    workload=${params_workload[$i]};
    if [ "${workload}0" = "0" ] ; then
	workload=0;
    fi;

    #algos_w=( "${algos[@]/%/_$workload}" )
    #algos_progs=( "${algos[@]/#/./bin/test -a}" )
    #algos_str="${algos_w[@]}";
    algos_str="${algos[@]}";

    if [ $fixed_file_dat -ne 1 ] ; then
	out="$(hostname).${ds}.i${initial}.u${update}.w${workload}.dat"
    else
	out="data.${ds}.i${initial}.u${update}.w${workload}.dat"
    fi;

    echo "### params -i$initial -r$range -u$update / keep $keep of reps $repetitions of dur $duration" | tee data/$out;
    ./scripts/scalability_rep_simple.sh ./bin/test_search "$cores" $repetitions $keep "$algos_str" -d$duration -i$initial -r$range -u$update -w$workload \
				 | tee -a data/$out;
done;

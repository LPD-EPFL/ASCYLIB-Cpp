host_name=$(uname -n);

if [ $host_name = "parsasrv1.epfl.ch" ];
then
    run_script="./run"
    run_script_timeout="./run_timeout"
fi;

max_cores(){
if [ $1 = "maglite" ];
then
	echo 64;
elif [ $1 = "lpd48core" ];
then
	echo 48;
elif [ $1 = "parsasrv1.epfl.ch" ];
then
	echo 36;
elif [ $1 = "diassrv8" ];
then
	echo 160;
elif [ $1 = "lpdpc34" ];
then
	echo 8;
elif [ $1 = "lpdpc4" ];
then
	echo 8;
elif [ $1 = "lpdxeon2680" ];
then
	echo 40;
elif [ $1 = "ol-collab1" ];
then
	echo 256;
else
	echo `grep processor /proc/cpuinfo | wc -l`;
	#echo 48;
fi;

}

if [ $host_name = "maglite" ];
then
	max_cores=64;
elif [ $host_name = "lpd48core" ];
then
	max_cores=48;
elif [ $host_name = "parsasrv1.epfl.ch" ];
then
	max_cores=36;
elif [ $host_name = "diassrv8" ];
then
	max_cores=80;
elif [ $host_name = "lpdpc34" ];
then
	max_cores=8;
elif [ $host_name = "lpdpc4" ];
then
	max_cores=8;
elif [ $host_name = "lpdxeon2680" ];
then
	max_cores=40;
elif [ $host_name = "ol-collab1" ];
then
	max_cores=256;
else
	max_cores=48;
fi;


if [ "$cores" = "all" ];
then
    cores=$(seq 2 1 $max_cores);
elif [ "$cores" = "allover" ];
then
    cores="$(seq 2 1 $((max_cores-1))) $(seq $max_cores 2 $max_over)";
elif [ "$cores" = "two" ];
then
    cores=$(seq 2 2 $max_cores);
elif [ "$cores" = "three" ];
then
    cores=$(seq 3 3 $max_cores);
elif [ "$cores" = "four" ];
then
    cores=$(seq 4 4 $max_cores);
elif [ "$cores" = "five" ];
then
    cores=$(seq 5 5 $max_cores);
elif [ "$cores" = "socket" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=$(seq 8 8 64);
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores=$(seq 6 6 36);
    elif [ $host_name = "diassrv8" ];
    then
	cores=$(seq 10 10 $max_cores);
    elif [ $host_name = "lpdpc34" ];
    then
	cores=$(seq 2 1 8);
    elif [ $host_name = "lpdpc4" ];
    then
	cores=$(seq 2 1 8);
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores=$(seq 10 10 40);
    elif [ $host_name = "ol-collab1" ];
    then
	cores=$(seq 64 64 256);
    else
	cores=$(seq 6 6 48);
    fi;
elif [ "$cores" = "socketover" ];
then
    if [ $host_name = "lpdxeon2680" ];
    then
	cores=$(seq 10 10 60);
    else
	cores=$(seq 6 6 48);
    fi;
elif [ "$cores" = "ppopp" ];
then
    if [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 2 2 40) $(seq 44 4 60)";
    elif [ $host_name = "lpd48core" ];
    then
	cores="$(seq 2 2 48) $(seq 52 4 64)";
    elif [ $host_name = "lpdquad" ];
    then
	cores="$(seq 2 4 96) $(seq 104 8 164)";
    elif [ $host_name = "ol-collab1" ];
    then
	cores="2 $(seq 8 8 256) 288 320 352";
    elif [ $host_name = "diassrv8" ];
    then
	cores="2 $(seq 10 5 200)";
    else
        max=$(nproc);
	maxp4=$(($max+4));
	max15=$(echo "1.5*$max" | bc);
	max15i=$(printf "%.0f" $max15);
	if [ $maxp4 -le $max15i ];
	then 
	   cores="1 $(seq 2 2 $max) $(seq $maxp4 4 $max15)";
	else
	    cores="1 $(seq 2 2 $max) $max15i";
	fi;
    fi;
elif [ "$cores" = "ppoppfast" ];
then
    if [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 2 4 40) $(seq 45 5 60)";
    elif [ $host_name = "lpd48core" ];
    then
	cores="$(seq 2 4 48) $(seq 56 8 64)";
    elif [ $host_name = "ol-collab1" ];
    then
	cores="2 $(seq 16 16 256) 288 320 352";
    else
	cores=$(seq 6 6 48);
    fi;
elif [ "$cores" = "osdi" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=$(seq 1 4 64);
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores=$(seq 1 2 36);
    elif [ $host_name = "diassrv8" ];
    then
	cores=$(seq 1 4 80);
    elif [ $host_name = "lpdpc34" ];
    then
	cores=$(seq 1 1 8);
    elif [ $host_name = "lpdpc4" ];
    then
	cores=$(seq 1 1 8);
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores=$(seq 1 2 40);
    elif [ $host_name = "ol-collab1" ];
    then
	cores=$(seq 1 8 256);
    else
	cores=$(seq 1 2 48);
    fi;
elif [ "$cores" = "osdi_bar" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores="1 9 20 36"
    elif [ $host_name = "diassrv8" ];
    then
	cores="1 10 20 80"
    elif [ $host_name = "lpdpc34" ];
    then
	cores=
    elif [ $host_name = "lpdpc4" ];
    then
	cores=
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores="1 10 20 40"
    elif [ $host_name = "ol-collab1" ];
    then
	cores="1 20 64 256"
    else
	cores="1 6 20 48"
    fi;
elif [ "$cores" = "socketandone" ];
then
    if [ $host_name = "maglite" ];
    then
	cores_init=$(seq 8 8 64);
	cores="1 ${cores_init}";
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores_init=$(seq 6 6 36);
	cores="1 ${cores_init}";
    elif [ $host_name = "diassrv8" ];
    then
	cores_init=$(seq 10 10 80);
	cores="1 ${cores_init}";
    elif [ $host_name = "lpdpc34" ];
    then
	cores_init=$(seq 2 1 8);
	cores="1 ${cores_init}";
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores_init=$(seq 10 10 40);
	cores="1 ${cores_init}";
    elif [ $host_name = "ol-collab1" ];
    then
	cores_init=$(seq 32 32 256);
	cores="1 ${cores_init}";
    else
	cores_init=$(seq 6 6 48);
	cores="1 ${cores_init}";
    fi;
elif [ "$cores" = "insocket" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=$(seq 8 8 64);
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores=$(seq 6 6 36);
    elif [ $host_name = "diassrv8" ];
    then
	cores="$(seq 2 1 9) $(seq 10 10 80)";
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 2 1 9) $(seq 10 10 40)";
    elif [ $host_name = "lpdpc34" ];
    then
	cores=$(seq 2 1 8);
    else
	cores="2 3 4 5 $(seq 6 6 48)";
    fi;
elif [ "$cores" = "step" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=$(seq 7 1 10);
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores=$(seq 5 1 8);
    elif [ $host_name = "diassrv8" ];
    then
	cores=$(seq 9 1 11);
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores=$(seq 9 1 11);
    elif [ $host_name = "lpdpc34" ];
    then
	cores=$(seq 2 1 8);
    else
	cores=$(seq 5 1 8);
    fi;
elif [ "$cores" = "eight" ];
then
    cores=$(seq 8 8 $max_cores);
elif [ "$cores" = "ten" ];
then
    cores=$(seq 10 10 $max_cores);
elif [ "$cores" = "to256" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=$(seq 8 8 64);
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores=$(seq 6 6 36);
    elif [ $host_name = "diassrv8" ];
    then
	cores=$(seq 10 10 80);
    elif [ $host_name = "lpdpc34" ];
    then
	cores=$(seq 2 1 8);
    elif [ $host_name = "lpdpc4" ];
    then
	cores=$(seq 2 1 8);
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 4 4 40) 48 56 $(seq 64 16 256)";
    elif [ $host_name = "ol-collab1" ];
    then
	cores=$(seq 32 32 256);
    else
	cores=$(seq 6 6 48);
    fi;
elif [ "$cores" = "to256fast" ];
then
    if [ $host_name = "maglite" ];
    then
	cores=$(seq 8 8 64);
    elif [ $host_name = "parsasrv1.epfl.ch" ];
    then
	cores=$(seq 6 6 36);
    elif [ $host_name = "diassrv8" ];
    then
	cores=$(seq 10 10 80);
    elif [ $host_name = "lpdpc34" ];
    then
	cores=$(seq 2 1 8);
    elif [ $host_name = "lpdpc4" ];
    then
	cores=$(seq 2 1 8);
    elif [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 10 10 40) $(seq 64 64 256)";
    elif [ $host_name = "ol-collab1" ];
    then
	cores=$(seq 32 32 256);
    else
	cores=$(seq 6 6 48);
    fi;
elif [ "$cores" = "to64" ];
then
    if [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 2 2 40) $(seq 44 4 64)";
    fi;
elif [ "$cores" = "to64fast" ];
then
	cores="$(seq 4 4 64)";
elif [ "$cores" = "to64all" ];
then
    if [ $host_name = "lpdxeon2680" ];
    then
	cores="$(seq 2 1 64)";
    fi;
elif [ "$cores" = "twelve" ];
then
    cores=$(seq 2 1 12);
fi;

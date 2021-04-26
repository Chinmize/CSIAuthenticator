# !/bin/bash

channel=157
bandwidth=80
addr=""
filename="output.pcap"
num=""

while getopts ":m:b:c:n:o:" opt
do
    case $opt in
        m)
        addr=" -m $OPTARG"
        ;;
        b)
        bandwidth=$OPTARG
        ;;
        c)
        channel=$OPTARG
        ;;
        n)
        num=" -c $OPTARG"
        ;;
        o)
        filename=$OPTARG
        ;;
        ?)
        echo "error: 未知参数"
        exit 1;;
    esac
done

par=$(mcp -c $channel/$bandwidth -C 1 -N 1$addr)

sudo ifconfig wlan0 up

nexutil -Iwlan0 -s500 -b -l34 -v $par

sudo iw dev wlan0 interface add mon0 type monitor

sudo ip link set mon0 up

rm -f ./authmsq/*

rm -f ./model_file/*

touch ./authmsq/tmp1 ./authmsq/tmp2 ./authmsq/tmp3

# sudo tcpdump -i wlan0 dst port 5500 -vv -w $filename -c $num
sudo tcpdump -i wlan0 dst port 5500$num -X -l | ./csiauthenticator

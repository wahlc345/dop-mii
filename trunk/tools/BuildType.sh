#!/bin/bash

if [ ! -z "$1" ];
then
	if [ ! -s include/BuildType.h ];
	then
		echo "#define $1" > include/BuildType.h
	fi
else
	if [[ ! -f include/BuildType.h || -s include/BuildType.h ]];
	then
		cp /dev/null include/BuildType.h
	fi 
fi 

#!/bin/sh
# $Id: mirror.sh,v 1.1 1999/07/15 18:13:25 jens Exp $

# This script shows an example of how you can mirror lots of
# servers without lots of administrativ problems.

# where to find the binary
SPEGLA="./spegla"

# where we run the program from
MIRROR_DIR=. #"/chageme"

# name of the config file
CONF="mirrorconf"

# comment out if unwanted
CALCTIME=1 

# mirror
#	runs spegla for ther named section in the config file mirrorconf
#
mirror() {
	section=${1}
	if [ -z "$section" ]; then
		echo 1>&2 "mirror: no section"
		exit 2
	fi

	echo Updating $section
	$SPEGLA 											\
		--section=common --configfile=mirrorconf 		\
		--section=$section --configfile=mirrorconf 		\
		--lockfile=lock/$section						\
		--logfile=log/$section
}

printtime() {
	seconds=$1
	if [ -z "$seconds" ]; then
		seconds=0
	fi

	hours=`expr $seconds / 3600`
	seconds=`expr $seconds - $hours '*' 3600`
	minutes=`expr $seconds / 60`
	seconds=`expr $seconds - $minutes '*' 60`
	printf '%02d:%02d:%02d' $hours $minutes $seconds
}

if cd $MIRROR_DIR ; then
else
	exit 2
fi

if [ ! -z "$CALCTIME" ]; then
	starttime=`date '+%s'`
	startdate=`date`
fi


# Sections that are to be mirrored
##################################

mirror gnu
mirror bind
mirror tools
mirror host

##################################
if [ ! -z "$CALCTIME" ]; then
	endtime=`date '+%s'`
	echo update of mirrors started at $startdate
	echo update of mirrors ended at `date`
	echo -n "total amount of time "
	printtime `expr $endtime - $starttime`
	echo
fi

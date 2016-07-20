#!/bin/sh
#	$Id: spegla.sh,v 1.1 1999/10/19 21:15:45 jens Exp $
# Example script for running spegla 1.1 when you have many
# areas to mirror.
#
# The config file has one section "common" used for defaults that
# gets sourced by spegla first. After that will the section for the
# are to be mirrored get sourced

SPEGLA_BIN=./spegla
CONFIGFILE=spegla.conf

SPEGLA() {
for section in "$@"; do
	echo "Updating $section" && \
		$SPEGLA_BIN --section=common --configfile=$CONFIGFILE \
			--section=$section --configfile=$CONFIGFILE
	done
}

# If there are arguments treat them as areas to mirror.
# Don't do the usual areas.
if [ $# -gt 0 ]; then
	SPEGLA "$@"
	exit
fi

# Put the areas you want to mirror here
#SPEGLA lpmud exmh
#SPEGLA gnu bind tools host groupkit
#SPEGLA ncftp ncftpd vrwave



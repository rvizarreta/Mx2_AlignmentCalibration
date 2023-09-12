#!/bin/sh
#
# make_readNT.sh
#
# Reads ntuples and creates readNT file
#
# Arguments: data_processing_path version run1 subrun1 run2 subrun2
# Author: Renzo Vizarreta

##################################
# Make Playlist.txt file
##################################

cd $ROCKMUONCALIBRATIONROOT/scripts/alignment_2x2
python make_playlist.py $*

if [ ! -f playlist.txt ]
then
    echo 'Could not make playlist!'
#  exit 1
else
    echo '******************************'
    echo '***** Playlist Created! ******'
    echo '******************************'
fi

#########################################
# Reading NTuples and making ReadNT.root
#########################################

./ReadNT

if [ ! -f ReadNT.root ]
then
  echo 'Could not read ntuples!'
#  exit 1
else
    echo '*********************************'
    echo '***** ReadNT.root Created! ******'
    echo '*********************************'
fi

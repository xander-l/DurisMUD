#!/bin/bash

DATESTR_FULL=`date +%C%y.%m.%d-%H.%M.%S`
DATESTR=`date +%s`
#This is the oldest allowed directory name in Players/Backup
#  It is equivalent to 1 week ago (7days * 24hrs *60 mins * 60secs)
OLDDATES=`expr $DATESTR - 7 \* 24 \* 60 \* 60`

FILENAMES=`ls Players/Backup/`
FILENAMES_LENGTH=${#FILENAME}

#If there are any files to possibly remove...
if [[ `expr $FILENAMES_LENGTH \> 0` ]]; then
  #Look through them and remove them if they're old.
  for WORD in $FILENAMES; do
    if [[ $WORD < $OLDDATES ]]; then
      DATE=`date -d @${WORD}`
      echo "Removing old backup Players/Backup/$WORD = $DATE"
      rm -r Players/Backup/$WORD
    fi
  done
fi

echo "Creating backup directory: Players/Backup/$DATESTR = $DATESTR_FULL"
mkdir Players/Backup/$DATESTR

#We have to do this manually since there are other directories & files in Players/
mkdir Players/Backup/$DATESTR/a
  cp Players/a/* Players/Backup/$DATESTR/a 2>/dev/null
mkdir Players/Backup/$DATESTR/b
  cp Players/b/* Players/Backup/$DATESTR/b 2>/dev/null
mkdir Players/Backup/$DATESTR/c
  cp Players/c/* Players/Backup/$DATESTR/c 2>/dev/null
mkdir Players/Backup/$DATESTR/d
  cp Players/d/* Players/Backup/$DATESTR/d 2>/dev/null
mkdir Players/Backup/$DATESTR/e
  cp Players/e/* Players/Backup/$DATESTR/e 2>/dev/null
mkdir Players/Backup/$DATESTR/f
  cp Players/f/* Players/Backup/$DATESTR/f 2>/dev/null
mkdir Players/Backup/$DATESTR/g
  cp Players/g/* Players/Backup/$DATESTR/g 2>/dev/null
mkdir Players/Backup/$DATESTR/h
  cp Players/h/* Players/Backup/$DATESTR/h 2>/dev/null
mkdir Players/Backup/$DATESTR/i
  cp Players/i/* Players/Backup/$DATESTR/i 2>/dev/null
mkdir Players/Backup/$DATESTR/j
  cp Players/j/* Players/Backup/$DATESTR/j 2>/dev/null
mkdir Players/Backup/$DATESTR/k
  cp Players/k/* Players/Backup/$DATESTR/k 2>/dev/null
mkdir Players/Backup/$DATESTR/l
  cp Players/l/* Players/Backup/$DATESTR/l 2>/dev/null
mkdir Players/Backup/$DATESTR/m
  cp Players/m/* Players/Backup/$DATESTR/m 2>/dev/null
mkdir Players/Backup/$DATESTR/n
  cp Players/n/* Players/Backup/$DATESTR/n 2>/dev/null
mkdir Players/Backup/$DATESTR/o
  cp Players/o/* Players/Backup/$DATESTR/o 2>/dev/null
mkdir Players/Backup/$DATESTR/p
  cp Players/p/* Players/Backup/$DATESTR/p 2>/dev/null
mkdir Players/Backup/$DATESTR/q
  cp Players/q/* Players/Backup/$DATESTR/q 2>/dev/null
mkdir Players/Backup/$DATESTR/r
  cp Players/r/* Players/Backup/$DATESTR/r 2>/dev/null
mkdir Players/Backup/$DATESTR/s
  cp Players/s/* Players/Backup/$DATESTR/s 2>/dev/null
mkdir Players/Backup/$DATESTR/t
  cp Players/t/* Players/Backup/$DATESTR/t 2>/dev/null
mkdir Players/Backup/$DATESTR/u
  cp Players/u/* Players/Backup/$DATESTR/u 2>/dev/null
mkdir Players/Backup/$DATESTR/v
  cp Players/v/* Players/Backup/$DATESTR/v 2>/dev/null
mkdir Players/Backup/$DATESTR/w
  cp Players/w/* Players/Backup/$DATESTR/w 2>/dev/null
mkdir Players/Backup/$DATESTR/x
  cp Players/x/* Players/Backup/$DATESTR/x 2>/dev/null
mkdir Players/Backup/$DATESTR/y
  cp Players/y/* Players/Backup/$DATESTR/y 2>/dev/null
mkdir Players/Backup/$DATESTR/z
  cp Players/z/* Players/Backup/$DATESTR/z 2>/dev/null

#These just have a few files in the directory.
#Note: Ships is not in the Players/ directory, but we still want to back it up.
mkdir Players/Backup/$DATESTR/Ships
  cp Ships/* Players/Backup/$DATESTR/Ships 2>/dev/null
mkdir Players/Backup/$DATESTR/Corpses
  cp Players/Corpses/* Players/Backup/$DATESTR/Corpses 2>/dev/null
mkdir Players/Backup/$DATESTR/Justice
  cp Players/Justice/* Players/Backup/$DATESTR/Justice 2>/dev/null
mkdir Players/Backup/$DATESTR/SavedItems
  cp Players/SavedItems/* Players/Backup/$DATESTR/SavedItems 2>/dev/null
mkdir Players/Backup/$DATESTR/Kingdoms
  cp Players/Kingdoms/* Players/Backup/$DATESTR/Kingdoms 2>/dev/null
mkdir Players/Backup/$DATESTR/ShopKeepers
  cp Players/ShopKeepers/* Players/Backup/$DATESTR/ShopKeepers 2>/dev/null

#These have subdirectories... we want all of it.
mkdir Players/Backup/$DATESTR/Shapechange
  cp -r Players/Shapechange/* Players/Backup/$DATESTR/Shapechange 2>/dev/null
mkdir Players/Backup/$DATESTR/Tradeskills
  cp -r Players/Tradeskills/* Players/Backup/$DATESTR/Tradeskills 2>/dev/null
mkdir Players/Backup/$DATESTR/House
  cp -r Players/House/* Players/Backup/$DATESTR/House 2>/dev/null

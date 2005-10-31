#!/bin/sh

RESULT=53

ulimit -c unlimited

while [ $RESULT != 0 ]; do
  if [ $RESULT == 53 ]; then
    if [ -f src/dms_new ]; then
      mv src/dms_new dms
    fi
    if [ -f areas/tworld.wld ]; then
      mv areas/tworld.wld areas/world.wld
    fi
    if [ -f areas/tworld.obj ]; then
      mv areas/tworld.obj areas/world.obj
    fi
    if [ -f areas/tworld.zon ]; then
      mv areas/tworld.zon areas/world.zon
    fi
    if [ -f areas/tworld.shp ]; then
      mv areas/tworld.shp areas/world.shp
    fi
    if [ -f areas/tworld.qst ]; then
      mv areas/tworld.qst areas/world.qst
    fi
  fi

if [ -d logs/log ]; then
    LOGNAME=`date +%b%d-%H%M`
    mkdir logs/old-logs/$LOGNAME
    mv logs/log/* logs/old-logs/$LOGNAME
  fi

   echo "Coping news files"
   rm -rf /var/www/html/duris_files/*
   cp /duris/mud/lib/information/news /var/www/html/duris_files/
  bin/cvs2cl.pl -f lib/information/changelog.cvs src

  nm --demangle dms | grep " T " | sed -e 's/[(].*[)]//g' > lib/event_names
  ./dms 6666 > dms.out

  RESULT=${PIPESTATUS[0]}
done

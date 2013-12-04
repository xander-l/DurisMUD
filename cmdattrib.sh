#!/bin/bash

#Parses the file for attribs
parsefile ( )
{
  declare -i COUNT number
  declare -i COUNT1 number
  declare -i COUNT2 number
  declare -i FOUNDIT number
  FOUNDIT=0

  cat $FILENAME | 
  while read LINE;
  do
    if [[ $LINE = "$FUNCTIONNAME"* ]]; then
      # Count the damn brackets and do the math
      COUNT1=`grep -o "{" <<<"$LINE" | wc -l`
      COUNT2=`grep -o "}" <<<"$LINE" | wc -l`
      COUNT=$COUNT1-$COUNT2
      # FOUNDIT -> We're inside the function.
      FOUNDIT=1
    elif [[ $FOUNDIT = 1 ]]; then
      # Hunt for the GET_C_...( crap.
      ATTRIB=$ATTRIB grep -o "GET_C_..." <<<"$LINE"
      
      # Count the damn brackets and do the math
      COUNT1=`grep -o "{" <<<"$LINE" | wc -l`
      COUNT2=`grep -o "}" <<<"$LINE" | wc -l`
      COUNT+=$COUNT1-$COUNT2

      # if at end of function
      if [[ $COUNT = 0 ]]; then
        break
      fi
    fi
  done
}

#remove the old command_att. file.
rm -f command_attributes.txt

FUNCTIONNAME="void bash"
FILENAME=`grep -l "$FUNCTIONNAME" ./src/*.c`
echo $FILENAME $FUNCTIONNAME
parsefile
echo $ATTRIB

FUNCTIONNAME="void do_tackle"
FILENAME=`grep -l "$FUNCTIONNAME" ./src/*.c`
echo $FILENAME $FUNCTIONNAME
parsefile
echo $ATTRIB

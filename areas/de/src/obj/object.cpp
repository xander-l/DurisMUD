//
//  File: object.cpp     originally part of durisEdit
//
//  Usage: multitudes of functions for use with objects
//

/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../types.h"
#include "../fh.h"

#include "object.h"
#include "objhere.h"



extern objectType **g_objLookup;
extern uint g_lowestObjNumber, g_highestObjNumber, g_numbLookupEntries, g_numbObjTypes;
extern editableListNode *g_inventory;
extern bool g_madeChanges;
extern char *g_exitnames[];
extern room *g_currentRoom;


//
// noObjTypesExist
//

bool noObjTypesExist(void)
{
  return (getLowestObjNumber() > getHighestObjNumber());
}


//
// getObjShortName
//

const char *getObjShortName(const objectType *obj)
{
  if (!obj)
    return "(object type not in this .obj file)";
  else
    return obj->objShortName;
}


//
// findObj : Returns the address of the object node that has the requested
//           objNumber (if any)
//
//   objNumber : object number to return
//

objectType *findObj(const uint objNumber)
{
  if (objNumber >= g_numbLookupEntries) 
    return NULL;

  return g_objLookup[objNumber];
}


//
// objExists : Returns true if object with objNumber exists
//
//   objNumber : object number to seek
//

bool objExists(const uint objNumber)
{
  if (objNumber >= g_numbLookupEntries) 
    return false;

  return (g_objLookup[objNumber] != NULL);
}


//
// copyObjectType : copies all the info from an object type record into a new
//                  record and returns the address of the new record
//
//        *srcObj : source object type record
// incNumbObjects : if TRUE, increments the global g_numbObjTypes var
//

objectType *copyObjectType(const objectType *srcObj, const bool incNumbObjects)
{
  objectType *newObj;


 // make sure src exists

  if (!srcObj) 
    return NULL;

 // alloc mem for new rec

  newObj = new(std::nothrow) objectType;
  if (!newObj)
  {
    displayAllocError("objectType", "copyObjectType");

    return NULL;
  }

 // first, copy the simple stuff

  memcpy(newObj, srcObj, sizeof(objectType));

 // next, the not-so-simple stuff

  newObj->keywordListHead = copyStringNodes(srcObj->keywordListHead);

 // extra desc linked list

  newObj->extraDescHead = copyExtraDescs(srcObj->extraDescHead);

  if (incNumbObjects)
  {
    g_numbObjTypes++;
    
    createPrompt();
  }

 // return the address of the new object

  return newObj;
}


//
// compareObjectApply : FALSE no match TRUE match
//
//   *app1 : first record to compare
//   *app2 : second record
//

bool compareObjectApply(const objApplyRec *app1, const objApplyRec *app2)
{
  if (!memcmp(app1, app2, sizeof(objApplyRec))) 
    return true;
  else 
    return false;
}


//
// compareObjectType : compares almost all - returns FALSE if they don't
//                     match, TRUE if they do
//
//                     doesn't compare defaultObj var
//

bool compareObjectType(const objectType *obj1, const objectType *obj2)
{
  if (obj1 == obj2) 
    return true;

  if (!obj1 || !obj2) 
    return false;

 // check all object attributes

  if (strcmp(obj1->objShortName, obj2->objShortName)) 
    return false;
  if (strcmp(obj1->objLongName, obj2->objLongName)) 
    return false;

  if (obj1->objNumber != obj2->objNumber) 
    return false;

  if (obj1->extraBits != obj2->extraBits)
    return false;
  if (obj1->wearBits != obj2->wearBits)
    return false;
  if (obj1->extra2Bits != obj2->extra2Bits)
    return false;
  if (obj1->antiBits != obj2->antiBits)
    return false;
  if (obj1->anti2Bits != obj2->anti2Bits)
    return false;

  if (obj1->affect1Bits != obj2->affect1Bits)
    return false;
  if (obj1->affect2Bits != obj2->affect2Bits)
    return false;
  if (obj1->affect3Bits != obj2->affect3Bits)
    return false;
  if (obj1->affect4Bits != obj2->affect4Bits)
    return false;

  if (obj1->objType != obj2->objType) 
    return false;
  if (obj1->material != obj2->material) 
    return false;
  if (obj1->size != obj2->size) 
    return false;
  if (obj1->space != obj2->space) 
    return false;
  if (obj1->craftsmanship != obj2->craftsmanship) 
    return false;
  if (obj1->damResistBonus != obj2->damResistBonus) 
    return false;
  if (obj1->weight != obj2->weight) 
    return false;
  if (obj1->worth != obj2->worth) 
    return false;
  if (obj1->condition != obj2->condition) 
    return false;

  for (uint i = 0; i < NUMB_OBJ_APPLIES; i++)
  {
    if (!compareObjectApply(&(obj1->objApply[i]), &(obj2->objApply[i])))
      return false; 
  }

  if (memcmp(obj1->objValues, obj2->objValues, sizeof(int) * NUMB_OBJ_VALS))
    return false;

  if (obj1->trapBits != obj2->trapBits) 
    return false;
  if (obj1->trapDam != obj2->trapDam) 
    return false;
  if (obj1->trapCharge != obj2->trapCharge) 
    return false;
  if (obj1->trapLevel != obj2->trapLevel) 
    return false;

 // description and extra descs

  if (!compareStringNodesIgnoreCase(obj1->keywordListHead, obj2->keywordListHead))
    return false;

  if (!compareExtraDescs(obj1->extraDescHead, obj2->extraDescHead))
    return false;


  return true;
}


//
// getHighestObjNumber : Gets the highest object number
//

uint getHighestObjNumber(void)
{
  return g_highestObjNumber;
}


//
// getLowestObjNumber : Gets the lowest object number
//

uint getLowestObjNumber(void)
{
  return g_lowestObjNumber;
}


//
// getFirstFreeObjNumber : Starts at lowest obj number + 1, loops up to highest - 1,
//                         returning the first number with no object type
//

uint getFirstFreeObjNumber(void)
{
  for (uint i = g_lowestObjNumber + 1; i <= g_highestObjNumber - 1; i++)
  {
    if (!g_objLookup[i]) 
      return i;
  }

  return g_highestObjNumber + 1;
}


//
// checkAndFixRefstoObj : reset any object value refs (i.e. container key), room exit
//                        refs, quest and shop obj vnum refs from oldNumb to newNumb
//

void checkAndFixRefstoObj(const uint oldNumb, const uint newNumb)
{
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();
  const uint highMobNumb = getHighestMobNumber();


 // scan through object types and room exits

 // fix object field refs

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      objectType *obj = findObj(objNumb);

      for (uint i = 0; i < NUMB_OBJ_VALS; i++)
      {
        if ((fieldRefsObjNumb(obj->objType, i) && (obj->objValues[i] == oldNumb)))
        {
          obj->objValues[i] = newNumb;
        }
      }
    }
  }

 // fix keynumb refs

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      room *roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (roomPtr->exits[i] && roomPtr->exits[i]->keyNumb && (roomPtr->exits[i]->keyNumb == oldNumb))
          roomPtr->exits[i]->keyNumb = newNumb;
      }
    }
  }

 // fix quest refs

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (mob->questPtr)
      {
        questQuest *qst = mob->questPtr->questHead;

        while (qst)
        {
          questItem *item = qst->questPlayRecvHead;
          while (item)
          {
            if ((item->itemType == QUEST_RITEM_OBJ) && (item->itemVal == oldNumb))
            {
              item->itemVal = newNumb;
            }

            item = item->Next;
          }

          item = qst->questPlayGiveHead;
          while (item)
          {
            if ((item->itemType == QUEST_GITEM_OBJ) && (item->itemVal == oldNumb))
            {
              item->itemVal = newNumb;
            }

            item = item->Next;
          }

          qst = qst->Next;
        }
      }
    }
  }

 // fix shop refs

  for (uint mobNumb = getLowestMobNumber(); mobNumb <= highMobNumb; mobNumb++)
  {
    if (mobExists(mobNumb))
    {
      mobType *mob = findMob(mobNumb);

      if (mob->shopPtr)
      {
        for (uint i = 0; (i < MAX_NUMBSHOPITEMS) && mob->shopPtr->producedItemList[i]; i++)
        {
          if (mob->shopPtr->producedItemList[i] == oldNumb)
          {
            mob->shopPtr->producedItemList[i] = newNumb;
          }
        }
      }
    }
  }
}


//
// renumberObjs : Renumbers the objs so that there are no "gaps" - starts
//                at the first obj and simply renumbers from there
//

void renumberObjs(const bool renumberHead, const uint newHeadNumb)
{
  uint objNumb = getLowestObjNumber(), lastNumb, oldNumb;
  objectType *objPtr = findObj(objNumb);


 // basic technique - keep all old obj pointers in g_objLookup table until clearing them at the
 // very end so that checkAndFixRefstoObj()/etc calls work; similarly, do not reset low/high obj
 // number until the very end

 if (renumberHead)
  {
    oldNumb = objNumb;
    objPtr->objNumber = lastNumb = newHeadNumb;

    resetAllObjHere(objNumb, newHeadNumb);
    resetNumbLoaded(ENTITY_OBJECT, objNumb, newHeadNumb);

    checkAndFixRefstoObj(objNumb, newHeadNumb);

    g_objLookup[newHeadNumb] = objPtr;

    g_madeChanges = true;
  }
  else
  {
    objNumb = oldNumb = lastNumb = getLowestObjNumber();
  }

 // skip past first object

  objPtr = getNextObj(oldNumb);

 // remove gaps

  while (objPtr)
  {
    oldNumb = objNumb = objPtr->objNumber;

    if (objNumb != (lastNumb + 1))
    {
      objPtr->objNumber = objNumb = lastNumb + 1;

      resetAllObjHere(oldNumb, objNumb);
      resetNumbLoaded(ENTITY_OBJECT, oldNumb, objNumb);

      checkAndFixRefstoObj(oldNumb, objNumb);

      g_madeChanges = true;

      g_objLookup[objNumb] = objPtr;
      if (!renumberHead)
        g_objLookup[oldNumb] = NULL;
    }

    lastNumb = objPtr->objNumber;

    objPtr = getNextObj(oldNumb);
  }

  resetEntityPointersByNumb(true, false);

 // clear old range

  if (renumberHead)
  {
   // entire range was moved, assumes no overlap

    memset(g_objLookup + g_lowestObjNumber, 0, ((g_highestObjNumber - g_lowestObjNumber) + 1) * sizeof(objectType*));
  }
  else
  {
   // if the head vnum wasn't changed, then the most that happened was that the high vnum came down

    memset(g_objLookup + lastNumb + 1, 0, (g_highestObjNumber - lastNumb) * sizeof(objectType*));
  }

 // set new low/high

  if (renumberHead)
    g_lowestObjNumber = newHeadNumb;

  g_highestObjNumber = lastNumb;
}


//
// getPrevObj : find object right before objNumb, numerically
//

objectType *getPrevObj(const uint objNumb)
{
  uint i = objNumb - 1;

  if (objNumb <= getLowestObjNumber()) 
    return NULL;

  while (!g_objLookup[i]) 
    i--;

  return g_objLookup[i];
}


//
// getNextObj : find object right after objNumb, numerically
//

objectType *getNextObj(const uint objNumb)
{
  uint i = objNumb + 1;

  if (objNumb >= getHighestObjNumber()) 
    return NULL;

  while (!g_objLookup[i]) 
    i++;

  return g_objLookup[i];
}


//
// deleteObjsinInv : delete any carried items that have objPtr
//

void deleteObjsinInv(const objectType *objPtr)
{
  editableListNode *edit = g_inventory;
  

  if (!objPtr) 
    return;

  while (edit)
  {
    editableListNode *next = edit->Next;

    if ((edit->entityType == ENTITY_OBJECT) && (((objectHere *)(edit->entityPtr))->objectPtr == objPtr))
      deleteEditableinList(&g_inventory, edit);

    edit = next;
  }
}


//
// updateInvKeywordsObj : update keywords of any items in inventory that match objPtr
//

void updateInvKeywordsObj(const objectType *objPtr)
{
  editableListNode *edit = g_inventory;


  if (!objPtr) 
    return;

  while (edit)
  {
    if ((edit->entityType == ENTITY_OBJECT) && (((objectHere *)(edit->entityPtr))->objectPtr == objPtr))
    {
      edit->keywordListHead = objPtr->keywordListHead;
    }

    edit = edit->Next;
  }
}


//
// resetLowHighObj : reset low and high obj type numb to actual
//

void resetLowHighObj(void)
{
  uint high = 0, low = g_numbLookupEntries;

  for (uint i = 0; i < g_numbLookupEntries; i++)
  {
    if (g_objLookup[i])
    {
      if (i > high) 
        high = i;

      if (i < low) 
        low = i;
    }
  }

  g_lowestObjNumber = low;
  g_highestObjNumber = high;
}


//
// getMatchingObj : snags the first matching object type based on character
//                  string - if numeric, looks for vnum, if non-numeric, scans
//                  keyword list.  checks current room obj list first, then
//                  entire object type list
//

objectType *getMatchingObj(const char *strn)
{
  uint vnum;
  bool isVnum;
  const uint highObjNumb = getHighestObjNumber();


  if (strnumer(strn))
  {
    isVnum = true;
    vnum = strtoul(strn, NULL, 10);
  }
  else 
  {
    isVnum = false;
  }

  objectHere *objHere = g_currentRoom->objectHead;

  while (objHere)
  {
    if (objHere->objectPtr)
    {
      if (!isVnum)
      {
        if (scanKeyword(strn, objHere->objectPtr->keywordListHead))
          return objHere->objectPtr;
      }
      else
      {
        if (objHere->objectNumb == vnum)
          return objHere->objectPtr;
      }
    }

    objHere = objHere->Next;
  }
  
  if (isVnum)
    return findObj(vnum);

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      objectType *obj = findObj(objNumb);

      if (scanKeyword(strn, obj->keywordListHead)) 
        return obj;
    }
  }

  return NULL;
}


//
// showKeyUsed : list all objects of type ITEM_KEY or with the keyword 'key' if no arg, or show where object
//               is used as key if arg
//

void showKeyUsed(const char *args)
{
  objectType *obj;
  room *roomPtr;
  uint vnum;
  size_t lines = 0;
  char outStrn[512];
  bool foundKey = false;
  const uint highRoomNumb = getHighestRoomNumber();
  const uint highObjNumb = getHighestObjNumber();


  if (strlen(args) == 0)
  {
    if (noObjTypesExist())
    {
      _outtext("\nThere are no object types.\n\n");

      return;
    }

    _outtext("\n\n");

    lines += 2;

    for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
    {
      if (objExists(objNumb))
      {
        obj = findObj(objNumb);

        if ((obj->objType == ITEM_KEY) || scanKeyword("KEY", obj->keywordListHead))
        {
          foundKey = true;

          sprintf(outStrn, "%s&n (#%u)\n", obj->objShortName, objNumb);

          if (checkPause(outStrn, lines))
            return;
        }
      }
    }

    if (!foundKey) 
      checkPause("There are no items of type key or with the keyword 'key'.\n", lines);

    checkPause("\n", lines);

    return;
  }

 // allow looking for objects that don't exist in this zone

  if (strnumer(args))
  {
    vnum = strtoul(args, NULL, 10);

    obj = findObj(vnum);
  }
  else
  {
    for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
    {
      if (objExists(objNumb))
      {
        obj = findObj(objNumb);

        if (scanKeyword(args, obj->keywordListHead))
        {
          vnum = objNumb;
          break;
        }
      }
    }

    if (!obj)
    {
      _outtext("\nNo object could be found that matches that keyword.\n\n");
      return;
    }
  }

  if (obj && (obj->objType != ITEM_KEY))
  {
    sprintf(outStrn, "\n&+WNOTE:&n '%s&n' is not of item type KEY.\n\n", obj->objShortName);
    if (checkPause(outStrn, lines))
      return;
  }

 // find the thingies that need this key and display them

  sprintf(outStrn, "\n\n'%s&n' (#%u) is needed for -\n\n",
          getObjShortName(obj), vnum);
  if (checkPause(outStrn, lines))
    return;

 // first, room exits

  for (uint roomNumb = getLowestRoomNumber(); roomNumb <= highRoomNumb; roomNumb++)
  {
    if (roomExists(roomNumb))
    {
      roomPtr = findRoom(roomNumb);

      for (uint i = 0; i < NUMB_EXITS; i++)
      {
        if (exitNeedsKey(roomPtr->exits[i], vnum))
        {
          sprintf(outStrn, "  &+C%s&n exit of &+groom #&+c%u&n, '%s&n'\n",
                  g_exitnames[i], roomPtr->roomNumber, roomPtr->roomName);

          if (checkPause(outStrn, lines)) 
            return;

          foundKey = true;
        }
      }
    }
  }

 // then, objects (containers)

  for (uint objNumb = getLowestObjNumber(); objNumb <= highObjNumb; objNumb++)
  {
    if (objExists(objNumb))
    {
      obj = findObj(objNumb);

      if ((obj->objType == ITEM_CONTAINER) && (obj->objValues[2] == vnum))
      {
        sprintf(outStrn, "  &+gobject #&+c%u&n, '%s&n' (&+ycontainer&n)\n",
                objNumb, obj->objShortName);

        if (checkPause(outStrn, lines)) 
          return;

        foundKey = true;
      }
    }
  }

  if (!foundKey) 
    _outtext("That key is not used on any doors or containers.\n");

  _outtext("\n");
}

# Makefile for the DE for Imms.  This includes the 'God' info incluing
#  item values and hunting for OP equipment.  Also, it compiles with
#  gdb enabled so crashes can be explored.

MALLOC_DEBUG = n
SHELL = /bin/sh

CC	= gcc
DEBUG	= -ggdb3

ifdef USE_SHARED
LDFLAGS += -rdynamic
endif
OPTS    = -O2
CCFLAGS = -D__UNIX__ -Wno-deprecated -D_DE_ -DGODMODE=1
CFLAGS = -D__UNIX__ -D_DE_ -DGODMODE=1
INCLUDES = -I./
LIBS    = -lncurses -lstdc++

# files that would be compiled as dynamic libs if USE_SHARED was defined
#    make sure that they are NOT included in ANY form in OBJS!!!
# The makefile will deal with compiling SH_LIBS as normal files 
# if USE_SHARED isn't defined

SH_LIBS 	=

MALLOC_OBJS	= 
OBJS		=

OPT_OBJS	= \
	command/alias.o \
	command/check.o \
	command/clone.o \
	command/command.o \
	command/copydesc.o \
	command/create.o \
	command/crtedit.o \
	command/delete.o \
	command/editent.o \
	command/list.o \
	command/load.o \
	command/look.o \
	command/maincomm.o \
	command/misccomm.o \
	command/mountfol.o \
	command/purge.o \
	command/setcomm.o \
	command/setrand.o \
	command/stat.o \
	command/var.o \
	edesc/crtdesc.o \
	edesc/edesc.o \
	edesc/editdesc.o \
      edesc/readexd.o \
      edesc/writeexd.o \
	inv/drop.o \
	inv/equip.o \
	inv/inv.o \
	inv/put.o \
	inv/take.o \
	inv/unequip.o \
	misc/config.o \
	misc/default.o \
	misc/editable.o \
	misc/editchck.o \
	misc/editdisp.o \
	misc/editichk.o \
	misc/editmchk.o \
	misc/editochk.o \
	misc/editrchk.o \
	misc/flags.o \
	misc/getstrn.o \
	misc/history.o \
	misc/keywords.o \
	misc/loaded.o \
	misc/master.o \
	misc/menu.o \
	misc/misc.o \
	misc/mudcomm.o \
	misc/strings.o \
	misc/strnnode.o \
	mob/crtmob.o \
	mob/crtmobhu.o \
	mob/crtmobtu.o \
	mob/delmobhu.o \
	mob/delmobt.o \
	mob/delmobtu.o \
	mob/dispmob.o \
	mob/editmms2.o \
	mob/editmms3.o \
	mob/editmmsc.o \
	mob/editmob.o \
	mob/editmobu.o \
	mob/mob.o \
	mob/mobhere.o \
	mob/mobtypes.o \
	mob/mobu.o \
	mob/readmob.o \
	mob/writemob.o \
	obj/crtobj.o \
	obj/crtobjhu.o \
	obj/crtobjtu.o \
	obj/delobjh.o \
	obj/delobjhu.o \
	obj/delobjt.o \
	obj/delobjtu.o \
	obj/dispobj.o \
	obj/editobj.o \
	obj/editobju.o \
	obj/editoexd.o \
	obj/editoms2.o \
	obj/editomsc.o \
	obj/editotrp.o \
	obj/object.o \
	obj/objectu.o \
	obj/objhere.o \
	obj/objtypes.o \
	obj/readobj.o \
	obj/writeobj.o \
	quest/delqstu.o \
	quest/editqst.o \
	quest/editqsti.o \
	quest/editqstm.o \
	quest/editqstq.o \
	quest/editqstu.o \
	quest/qsttypes.o \
	quest/quest.o \
	quest/readqst.o \
	quest/writeqst.o \
	room/crtexit.o \
	room/crtrexit.o \
	room/crtroom.o \
	room/crtroomu.o \
	room/delexitu.o \
	room/delroomu.o \
	room/disproom.o \
	room/editexit.o \
	room/editrexd.o \
	room/editrext.o \
	room/editrmsc.o \
	room/editroom.o \
	room/editsector.o \
	room/exit.o \
	room/readwld.o \
	room/room.o \
	room/roomtype.o \
	room/roomu.o \
	room/writewld.o \
	set/readset.o \
	set/writeset.o \
	shop/delshpu.o \
	shop/editshp.o \
	shop/editshpb.o \
	shop/editshpc.o \
	shop/editshpm.o \
	shop/editshpr.o \
	shop/editshps.o \
	shop/editshpt.o \
	shop/editshpu.o \
	shop/readshp.o \
	shop/shop.o \
	shop/writeshp.o \
	zone/dispzone.o \
	zone/editzmsc.o \
	zone/editzone.o \
	zone/readzon.o \
	zone/writezon.o \
	zone/zone.o \
	zone/zonetype.o \
	cmddef.o \
	de.o \
	dispflag.o \
	display.o \
	dispmisc.o \
	editflag.o \
	editval.o \
	flagdef.o \
	init.o \
	interp.o \
	listdef.o \
	map.o \
	menudef.o \
	readfile.o \
	template.o \
	unixgraph.o \
	vardef.o


# those below are .c files shared with dms
C_OBJS = \
	common.o \
	skills.o
	
#******************************************************************************
#******************************************************************************
#
# Don't mess around below this point.  Everything past here are makefile rules,
# and NOT dependency lists that need to be changed

PROGS 	= de

DMS_OBJS = $(OBJS)


ifndef USE_SHARED
DMS_OBJS += $(SH_LIBS)
else
PROGS += $(SH_LIBS:.o=.so)
endif

SRCS	= $(OBJS:.o=.cpp) $(OPT_OBJS:.o=.cpp) $(SH_LIBS:.o=.cpp) $(MALLOC_OBJS:.o=.cpp)

all: $(PROGS)

de : $(DMS_OBJS) $(OPT_OBJS) $(MALLOC_OBJS) $(C_OBJS)
	$(CC) $(LDFLAGS) -o de $(DMS_OBJS) $(OPT_OBJS) $(C_OBJS) $(MALLOC_OBJS) $(LIBS) $(DEBUG)

$(DMS_OBJS): %.o: %.cpp
	$(CC) $(CCFLAGS) $(DEBUG) $(INCLUDES) -c $< -o $@

$(OPT_OBJS): %.o: %.cpp
	$(CC) $(CCFLAGS) $(DEBUG) $(INCLUDES) $(OPTS) -c $< -o $@

$(C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDES) $(OPTS) -c $< -o $@

ifeq ($(MALLOC_DEBUG),y)

$(MALLOC_OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDES) -DDEBUG -c $< -o $@
else

$(MALLOC_OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) $(DEBUG) $(OPTS) $(INCLUDES) -c $< -o $@
endif



$(SH_LIBS:.o=.so): %.so: %.cpp
	$(CC) $(CFLAGS) $(DEBUG) -fpic -c $< -o $*.o
	ld -shared -o $@ /usr/lib/crtbeginS.o $*.o /usr/lib/crtendS.o

#depend:
#	makedepend -m -w110 -- $(CFLAGS) -- $(SRCS)

depend .depend:
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDES) -MM $(SRCS) > .depend

clean:
	rm $(OBJS) $(OPT_OBJS) $(C_OBJS) de .depend

include .depend

# DO NOT DELETE THIS LINE -- make depend depends on it.

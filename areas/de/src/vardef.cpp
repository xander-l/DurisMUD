//
//  File: vardef.cpp     originally part of durisEdit
//
//  Usage: inline functions to obtain internal variable values,
//         respecting hardwired defaults
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

#include "fh.h"
#include "types.h"

#include "vardef.h"
#include "system.h"

extern variable *g_varHead;

//
// getScreenHeightVal
//

uint getScreenHeightVal(void)
{
  return getVarNumbUnsigned(g_varHead, VAR_SCREENHEIGHT_NAME, DEF_SCREENHEIGHT_VAL);
}


//
// getScreenWidthVal
//

uint getScreenWidthVal(void)
{
  return getVarNumbUnsigned(g_varHead, VAR_SCREENWIDTH_NAME, DEF_SCREENWIDTH_VAL);
}


//
// getIsMapZoneVal
//

bool getIsMapZoneVal(void)
{
  return getVarBoolVal(g_varHead, VAR_ISMAPZONE_NAME, DEF_ISMAPZONE_VAL);
}


//
// getSaveHowOftenVal
//

uint getSaveHowOftenVal(void)
{
  return getVarNumbUnsigned(g_varHead, VAR_SAVEHOWOFTEN_NAME, DEF_SAVEHOWOFTEN_VAL);
}


//
// getEditorName
//

const char *getEditorName(void)
{
  return getVarStrn(g_varHead, VAR_TEXTEDIT_NAME, DEF_TEXTEDIT_VAL);
}


//
// getVnumCheckVal
//

bool getVnumCheckVal(void)
{
  return getVarBoolVal(g_varHead, VAR_VNUMCHECK_NAME, DEF_VNUMCHECK_VAL);
}


//
// getObjAffectVal
//

bool getObjAffectVal(void)
{
  return getVarBoolVal(g_varHead, VAR_OBJAFFECT_NAME, DEF_OBJAFFECT_VAL);
}


//
// getCheckLimitsVal
//

bool getCheckLimitsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKLIMITS_NAME, DEF_CHECKLIMITS_VAL);
}


//
// getCheckZoneFlagsVal
//

bool getCheckZoneFlagsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKZONEFLAGS_NAME, DEF_CHECKZONEFLAGS_VAL);
}


//
// getShowObjContentsVal
//

bool getShowObjContentsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWCONTENTS_NAME, DEF_SHOWCONTENTS_VAL);
}


//
// getShowObjFlagsVal
//

bool getShowObjFlagsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWOBJFLAGS_NAME, DEF_SHOWOBJFLAGS_VAL);
}


//
// getShowMobFlagsVal
//

bool getShowMobFlagsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWMOBFLAGS_NAME, DEF_SHOWMOBFLAGS_VAL);
}


//
// getShowMobPosVal
//

bool getShowMobPosVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWMOBPOS_NAME, DEF_SHOWMOBPOS_VAL);
}


//
// getShowObjVnumVal
//

bool getShowObjVnumVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWOBJVNUM_NAME, DEF_SHOWOBJVNUM_VAL);
}


//
// getShowMobVnumVal
//

bool getShowMobVnumVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWMOBVNUM_NAME, DEF_SHOWMOBVNUM_VAL);
}


//
// getShowMobRideFollowVal
//

bool getShowMobRideFollowVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWMOBRIDEFOLLOW_NAME, DEF_SHOWMOBRIDEFOLLOW_VAL);
}


//
// getShowRoomExtraVal
//

bool getShowRoomExtraVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWROOMEXTRA_NAME, DEF_SHOWROOMEXTRA_VAL);
}


//
// getShowRoomVnumVal
//

bool getShowRoomVnumVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWROOMVNUM_NAME, DEF_SHOWROOMVNUM_VAL);
}


//
// getShowPricesAdjustedVal
//

bool getShowPricesAdjustedVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWPRICESADJUSTED_NAME, DEF_SHOWPRICESADJUSTED_VAL);
}


//
// getInterpColorVal
//

bool getInterpColorVal(void)
{
  return getVarBoolVal(g_varHead, VAR_INTERPCOLOR_NAME, DEF_INTERPCOLOR_VAL);
}


//
// getShowColorVal
//

bool getShowColorVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWCOLOR_NAME, DEF_SHOWCOLOR_VAL);
}


//
// getMenuPromptName
//

const char *getMenuPromptName(void)
{
  return getVarStrn(g_varHead, VAR_MENUPROMPT_NAME, DEF_MENUPROMPT_VAL);
}


//
// getMainPromptStrn
//

const char *getMainPromptStrn(void)
{
  return getVarStrn(g_varHead, VAR_MAINPROMPT_NAME, DEF_MAINPROMPT_VAL);
}


//
// getWalkCreateVal
//

bool getWalkCreateVal(void)
{
  return getVarBoolVal(g_varHead, VAR_WALKCREATE_NAME, DEF_WALKCREATE_VAL);
}


//
// getShowExitDestVal
//

bool getShowExitDestVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWEXITDEST_NAME, DEF_SHOWEXITDEST_VAL);
}


//
// getShowExitFlagsVal
//

bool getShowExitFlagsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWEXITFLAGS_NAME, DEF_SHOWEXITFLAGS_VAL);
}


//
// getRoomStartVal
//

uint getRoomStartVal(void)
{
  return getVarNumbUnsigned(g_varHead, VAR_STARTROOM_NAME, DEF_STARTROOM_VAL);
}


//
// getRoomStartActiveVal
//

bool getStartRoomActiveVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SROOMACTIVE_NAME, DEF_SROOMACTIVE_VAL);
}


//
// getShowMenuInfoVal
//

bool getShowMenuInfoVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SHOWMENUINFO_NAME, DEF_SHOWMENUINFO_VAL);
}


//
// getMainZoneNameStrn
//

const char *getMainZoneNameStrn(void)
{
  return getVarStrn(g_varHead, VAR_MAINZONENAME_NAME, DEF_MAINZONENAME_VAL);
}


//
// getLastCommandStrn
//

const char *getLastCommandStrn(void)
{
  return getVarStrn(g_varHead, VAR_LASTCOMMAND_NAME, DEF_LASTCOMMAND_VAL);
}


//
// getCheckSaveVal
//

bool getCheckSaveVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKSAVE_NAME, DEF_CHECKSAVE_VAL);
}


//
// getCheckLoneRoomVal
//

bool getCheckLoneRoomVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKLONEROOM_NAME, DEF_CHECKLONEROOM_VAL);
}


//
// getCheckMissingKeysVal
//

bool getCheckMissingKeysVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKMISSINGKEYS_NAME, DEF_CHECKMISSINGKEYS_VAL);
}


//
// getCheckLoadedVal
//

bool getCheckLoadedVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKLOADED_NAME, DEF_CHECKLOADED_VAL);
}


//
// getCheckRoomVal
//

bool getCheckRoomVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKROOM_NAME, DEF_CHECKROOM_VAL);
}


//
// getCheckExitVal
//

bool getCheckExitVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKEXIT_NAME, DEF_CHECKEXIT_VAL);
}


//
// getCheckExitDescVal
//

bool getCheckExitDescVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKEXITDESC_NAME, DEF_CHECKEXITDESC_VAL);
}


//
// getCheckObjVal
//

bool getCheckObjVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKOBJ_NAME, DEF_CHECKOBJ_VAL);
}


//
// getCheckObjValuesVal
//

bool getCheckObjValuesVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKOBJVAL_NAME, DEF_CHECKOBJVAL_VAL);
}


//
// getCheckObjDescVal
//

bool getCheckObjDescVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKOBJDESC_NAME, DEF_CHECKOBJDESC_VAL);
}


//
// getCheckMobDescVal
//

bool getCheckMobDescVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKMOBDESC_NAME, DEF_CHECKMOBDESC_VAL);
}


//
// getCheckMobVal
//

bool getCheckMobVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKMOB_NAME, DEF_CHECKMOB_VAL);
}


//
// getCheckEdescVal
//

bool getCheckEdescVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKEDESC_NAME, DEF_CHECKEDESC_VAL);
}


//
// getNoSaveonCheckErrVal
//

bool getNoSaveonCheckErrVal(void)
{
  return getVarBoolVal(g_varHead, VAR_NOSAVEONCHECKERR_NAME, DEF_NOSAVEONCHECKERR_VAL);
}


//
// getPauseCheckScreenfulVal
//

bool getPauseCheckScreenfulVal(void)
{
  return getVarBoolVal(g_varHead, VAR_PAUSECHECKSCREENFUL_NAME, DEF_PAUSECHECKSCREENFUL_VAL);
}


//
// getCheckFlagsVal
//

bool getCheckFlagsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKFLAGS_NAME, DEF_CHECKFLAGS_VAL);
}


//
// getSaveCheckLogVal
//

bool getSaveCheckLogVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SAVECHECKLOG_NAME, DEF_SAVECHECKLOG_VAL);
}


//
// getEditUneditableFlagsVal
//

bool getEditUneditableFlagsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_EDITUNEDITABLEFLAGS_NAME, DEF_EDITUNEDITABLEFLAGS_VAL);
}


//
// getAllMobs
//

bool getAllMobsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_GETALLMOBS_NAME, DEF_GETALLMOBS_VAL);
}


//
// getIgnoreZoneS
//

bool getIgnoreZoneSVal(void)
{
  return getVarBoolVal(g_varHead, VAR_IGNOREZONES_NAME, DEF_IGNOREZONES_VAL);
}


//
// getCheckMobClassEq
//

bool getCheckMobClassEqVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKMOBCLASSEQ_NAME, DEF_CHECKMOBCLASSEQ_VAL);
}


//
// getCheckMobRaceEq
//

bool getCheckMobRaceEqVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKMOBRACEEQ_NAME, DEF_CHECKMOBRACEEQ_VAL);
}


//
// getCheckMobSexEq
//

bool getCheckMobSexEqVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKMOBSEXEQ_NAME, DEF_CHECKMOBSEXEQ_VAL);
}


//
// getCheckObjMaterial(void)
//

bool getCheckObjMaterialVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKOBJMATERIAL_NAME, DEF_CHECKOBJMATERIAL_VAL);
}


//
// getCheckGuildStuffVal(void)
//

bool getCheckGuildStuffVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKGUILDSTUFF_NAME, DEF_CHECKGUILDSTUFF_VAL);
}


//
// getCheckZoneVal
//

bool getCheckZoneVal(void)
{
  return getVarBoolVal(g_varHead, VAR_CHECKZONE_NAME, DEF_CHECKZONE_VAL);
}


//
// getNegDestOutofZoneVal(void)
//

bool getNegDestOutofZoneVal(void)
{
  return getVarBoolVal(g_varHead, VAR_NEGDESTOUTOFZONE_NAME, DEF_NEGDESTOUTOFZONE_VAL);
}


//
// getSaveEveryXCommandsVal(void)
//

bool getSaveEveryXCommandsVal(void)
{
  return getVarBoolVal(g_varHead, VAR_SAVEEVERYXCOMMANDS_NAME, DEF_SAVEEVERYXCOMMANDS_VAL);
}

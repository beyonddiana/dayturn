/** 
 * @file RRInterface.cpp
 * @author Marine Kelley
 * @brief Implementation of the RLV features
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llavatarnamecache.h"
#include "llcamera.h"
#include "lldrawpoolalpha.h"
//#include "llfloaterenvsettings.h"
#include "llfloatereditsky.h"
#include "llfloaterimnearbychat.h"
#include "llfloatermap.h"
#include "llfloaterpostprocess.h"
#include "llfloaterreg.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersidepanelcontainer.h"
//#include "llfloaterwater.h"
//#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llgroupactions.h"
#include "llhudtext.h"
#include "llinventoryfunctions.h"
#include "llmoveview.h"
#include "llnavigationbar.h"
//#include "llnearbychat.h"
//#include "llnearbychatbar.h"
#include "llnotifications.h"
#include "llpanelmaininventory.h"
#include "llpaneloutfitedit.h"
#include "llpaneltopinfobar.h"
#include "llregionhandle.h"
#include "llrendersphere.h"
#include "llselectmgr.h"
#include "llspeakers.h"
#include "llstartup.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
//#include "llinventoryview.h"
#include "lltoolmgr.h"
#include "lltracker.h"
//#include "llurlsimstring.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llwaterparammanager.h"
#include "llwlparammanager.h"
#include "llinventorybridge.h"
#include "llviewerdisplay.h"
#include "llviewerjoystick.h"
#include "llviewerregion.h"
#include "llviewermessage.h"
#include "llviewerparcelmgr.h"
#include "llworldmapmessage.h"
#include "pipeline.h"

#include <boost/algorithm/string.hpp>

#include <cstdlib>

#include "RRInterface.h"

// Global and static variables initialization.
BOOL gRRenabled = TRUE;
BOOL RRInterface::sRRNoSetEnv = FALSE;
BOOL RRInterface::sRestrainedLoveDebug = FALSE;
BOOL RRInterface::sCanOoc = TRUE;
std::string RRInterface::sRecvimMessage = "The Resident you messaged is prevented from reading your instant messages at the moment, please try again later.";
std::string RRInterface::sSendimMessage = "*** IM blocked by sender's viewer";
std::string RRInterface::sBlacklist = "";
F32 RRInterface::sLastAvatarZOffsetCommit = 1.f;
F32 RRInterface::sLastOutfitChange = -1000.f;
U32 RRInterface::mCamDistNbGradients = 40;
BOOL RRInterface::sRenderLimitRenderedThisFrame = FALSE;


#if !defined(max)
#define max(a, b)	((a) > (b) ? (a) : (b))
#endif

// --
// Local functions
std::string dumpList2String (std::deque<std::string> list, std::string sep, int size = -1)
{
	bool found_one = false;
	if (size < 0) size = (int)list.size();
	std::string res = "";
	for (int i = 0; i < (int)list.size() && i < size; ++i) {
		if (found_one) res += sep;
		found_one = true;
		res += list[i];
	}
	return res;
}

int match (std::deque<std::string> list, std::string str, bool& exact_match)
{
	// does str contain list[0]/list[1]/.../list[n] ?
	// yes => return the size of the list
	// no  => try again after removing the last element
	// return 0 if never found
	// Exception : if str starts with a "~" character, the match must be exact
	// exact_match is an output, set to true when strict matching is found, false otherwise.
	unsigned int size = list.size();
	std::string dump;
	exact_match = false;
	while (size > 0) {
		dump = dumpList2String (list, "/", (int)size);
		if (str == dump) {
			exact_match = true;
			return (int)size;
		}
		else if (str != "" && str[0] == '~') {
			return 0;
		}
		else if (str.find (dump) != -1) {
			return (int)size;
		}
		size--;
	}
	return 0;
}

std::deque<std::string> getSubList (std::deque<std::string> list, int min, int max = -1)
{
	if (min < 0) min = 0;
	if (max < 0) max = list.size() - 1;
	std::deque<std::string> res;
	for (int i = min; i <= max; ++i) {
		res.push_back (list[i]);
	}
	return res;
}

bool findMultiple (std::deque<std::string> list, std::string str)
{
	// returns true if all the tokens in list are contained into str
	unsigned int size = list.size();
	for (unsigned int i = 0; i < size; i++) {
		if (str.find (list[i]) == -1) return false;
	}
	return true;
}

void setVisibleAll(std::string floater_name, BOOL visible)
{
	// Use this to hide or show all floaters bearing this name
	U32 count = 0;
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList(floater_name);
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
	{
		LLFloater* iv = dynamic_cast<LLFloater*>(*iter++);
		if (iv)
		{
			count++;
			iv->setVisible(visible);
		}
	}
}

void refreshCachedVariable (std::string var)
{
	// Call this function when adding/removing a restriction only, i.e. in this file
	// Test the cached variables in the code of the viewer itself
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (!avatar) return;

	BOOL contained = gAgent.mRRInterface.contains (var);
	if (var == "detach" || var.find ("detach:") == 0 || var.find ("addattach") == 0 || var.find ("remattach") == 0) {
		contained = gAgent.mRRInterface.contains("detach")
		|| gAgent.mRRInterface.containsSubstr("detach:")
		|| gAgent.mRRInterface.containsSubstr("addattach")
		|| gAgent.mRRInterface.containsSubstr("remattach");
		gAgent.mRRInterface.mContainsDetach = contained;
		gAgent.mRRInterface.mHasLockedHuds = gAgent.mRRInterface.hasLockedHuds();
		if (gAgent.mRRInterface.mHasLockedHuds) {
			// To force the viewer to render the HUDs again, just in case
			LLPipeline::sShowHUDAttachments = TRUE;
		}
	}
	else if (var == "showinv")				gAgent.mRRInterface.mContainsShowinv = contained;
	else if (var == "unsit")				gAgent.mRRInterface.mContainsUnsit = contained;
	else if (var == "standtp") 			    gAgent.mRRInterface.mContainsStandtp = contained;
	else if (var == "interact")				gAgent.mRRInterface.mContainsInteract = contained;
	else if (var == "showworldmap")			gAgent.mRRInterface.mContainsShowworldmap = contained;
	else if (var == "showminimap")			gAgent.mRRInterface.mContainsShowminimap = contained;
	else if (var == "showloc")				gAgent.mRRInterface.mContainsShowloc = contained;
	else if (var == "shownames")			gAgent.mRRInterface.mContainsShownames = contained;
	else if (var == "shownametags")			gAgent.mRRInterface.mContainsShownametags = contained;
	else if (var == "setenv")				gAgent.mRRInterface.mContainsSetenv = contained;
	else if (var == "setdebug")				gAgent.mRRInterface.mContainsSetdebug = contained;
	else if (var == "fly")					gAgent.mRRInterface.mContainsFly = contained;
	else if (var == "edit")					gAgent.mRRInterface.mContainsEdit = (gAgent.mRRInterface.containsWithoutException ("edit")); // || gAgent.mRRInterface.containsSubstr ("editobj"));
	else if (var == "rez")					gAgent.mRRInterface.mContainsRez = contained;
	else if (var == "showhovertextall")		gAgent.mRRInterface.mContainsShowhovertextall = contained;
	else if (var == "showhovertexthud")		gAgent.mRRInterface.mContainsShowhovertexthud = contained;
	else if (var == "showhovertextworld")	gAgent.mRRInterface.mContainsShowhovertextworld = contained;
	else if (var == "defaultwear")			gAgent.mRRInterface.mContainsDefaultwear = contained;
	else if (var == "permissive")			gAgent.mRRInterface.mContainsPermissive = contained;
	else if (var == "temprun")					gAgent.mRRInterface.mContainsRun = contained;
	else if (var == "alwaysrun")				gAgent.mRRInterface.mContainsAlwaysRun = contained;
	//else if (var == "moveup")					gAgent.mRRInterface.mContainsMoveUp = contained;
	//else if (var == "movedown")				gAgent.mRRInterface.mContainsMoveDown = contained;
	//else if (var == "moveleft")			gAgent.mRRInterface.mContainsMoveStrafeLeft = contained;
	//else if (var == "moveright")			gAgent.mRRInterface.mContainsMoveStrafeRight = contained;
	//else if (var == "moveforward")				gAgent.mRRInterface.mContainsMoveForward = contained;
	//else if (var == "movebackward")			gAgent.mRRInterface.mContainsMoveBackward = contained;
	//else if (var == "moveturnup")				gAgent.mRRInterface.mContainsMoveTurnUp = contained;
	//else if (var == "moveturndown")			gAgent.mRRInterface.mContainsMoveTurnDown = contained;
	//else if (var == "moveturnleft")			gAgent.mRRInterface.mContainsMoveTurnLeft = contained;
	//else if (var == "moveturnright")			gAgent.mRRInterface.mContainsMoveTurnRight = contained;

	gAgent.mRRInterface.mContainsCamTextures = (gAgent.mRRInterface.containsSubstr("camtextures") || gAgent.mRRInterface.containsSubstr("setcam_textures"));

	if (var == "showinv") {
		if (gAgent.mRRInterface.mContainsShowinv) {
//			LLSideTray::getInstance()->childSetVisible("panel_main_inventory", false);
			LLFloaterReg::hideInstance("panel_main_inventory", LLSD());
			setVisibleAll("inventory", FALSE);
			LLPanelOutfitEdit* panel_outfit_edit = dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfit_edit"));
			if (NULL != panel_outfit_edit) {
				panel_outfit_edit->showAddWearablesPanel(false);
			}
			//LLFloaterInventory::hideAll(); // close all the secondary inventory floaters
//			LLBottomTray::getInstance()->childSetEnabled("inventory_btn", false);
		}
		else {
//			setVisibleAll("inventory", TRUE);
//			LLSideTray::getInstance()->childSetVisible("panel_main_inventory", true);
//			LLBottomTray::getInstance()->childSetEnabled("inventory_btn", true);
		}
	}

	else if (var == "rez") {
		bool parcel_allows_build = LLToolMgr::getInstance()->canEdit();
		if (parcel_allows_build) { // if we can't edit in this parcel, no need to refresh the button anyway
			if (gAgent.mRRInterface.mContainsRez) {
//				LLBottomTray::getInstance()->childSetEnabled("build_btn", false);
			}
			else {
//				LLBottomTray::getInstance()->childSetEnabled("build_btn", true);
			}
		}
	}

	else if (var == "shownames" || var == "shownametags") {
		if (gAgent.mRRInterface.mContainsShownames || gAgent.mRRInterface.mContainsShownametags) {
//			LLSideTray::getInstance()->childSetVisible("nearby_panel", false);
//			LLSideTray::getInstance()->childSetVisible("recent_panel", false);
			LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
			if (panel) {
				panel->childSetVisible("avatar_list", false);
			}
		}
		else {
//			LLSideTray::getInstance()->childSetVisible("nearby_panel", true);
//			LLSideTray::getInstance()->childSetVisible("recent_panel", true);
			LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
			if (panel) {
				panel->childSetVisible("avatar_list", true);
			}
		}
		LLLocalSpeakerMgr::getInstance()->updateSpeakerList();
	}

	else if (var == "showminimap") {
		if (gAgent.mRRInterface.mContainsShowminimap) {
			LLFloaterMap::getInstance()->setVisible (false);
			LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
			if (panel) {
				panel->childSetVisible("Net Map", false);
			}
//			LLBottomTray::getInstance()->childSetEnabled("mini_map_btn", false);
//			LLSideTray::getInstance()->childSetVisible("Net Map", false);

		}
		else {
			if (!gAgent.mRRInterface.mContainsShowloc) {
//				LLBottomTray::getInstance()->childSetEnabled("mini_map_btn", true);
			}
			LLPanel* panel = LLFloaterSidePanelContainer::getPanel("people", "panel_people");
			if (panel) {
				panel->childSetVisible("Net Map", true);
			}
//			LLSideTray::getInstance()->childSetVisible("Net Map", true);
		}
	}

	else if (var == "showworldmap") {
		if (gAgent.mRRInterface.mContainsShowworldmap) {
			LLFloaterWorldMap::getInstance()->setVisible (false);
//			LLBottomTray::getInstance()->childSetEnabled("world_map_btn", false);
		}
		else {
			if (!gAgent.mRRInterface.mContainsShowloc) {
//				LLBottomTray::getInstance()->childSetEnabled("world_map_btn", true);
			}
		}
	}

	else if (var == "showloc") {
		LLNavigationBar::getInstance()->refresh();
		if (gAgent.mRRInterface.mContainsShowloc) {
			if (LLPanelTopInfoBar::getInstance()->getVisible()) {
				toggle_show_mini_location_panel(LLSD(false));
			}
			LLFloaterWorldMap::getInstance()->setVisible (false);
			setVisibleAll("panel_places", FALSE);
			gSavedSettings.setBOOL ("ShowMiniLocationPanel", FALSE);
		}
		else {
			LLPanelTopInfoBar::getInstance()->update();
			if (!gAgent.mRRInterface.mContainsShowworldmap) {
			}
		}
	}

	// Here we need to explicitely refresh the Stand and Stop Flying buttons because there is no Refresh function
	else if (var == "unsit" || var == "fly") {
		if (avatar->isSitting()) {
			if (gAgent.mRRInterface.mContainsUnsit) LLFloaterMove::setSittingMode(FALSE);
			else LLFloaterMove::setSittingMode(TRUE);
		}
		else if (gAgent.getFlying()) {
			if (gAgent.mRRInterface.mContainsFly) LLFloaterMove::setFlyingMode(FALSE);
			else LLFloaterMove::setFlyingMode(TRUE);
		}
	}
	else if (var == "temprun") {
		if (gAgent.mRRInterface.mContainsRun) {
			if (gAgent.getRunning()) {
				if (gAgent.getAlwaysRun()) gAgent.clearAlwaysRun();
				gAgent.clearRunning();
				gAgent.sendWalkRun(gAgent.getRunning());
			}
		}
	}
	else if (var == "alwaysrun") {
		if (gAgent.mRRInterface.mContainsAlwaysRun) {
			if (gAgent.getAlwaysRun()) {
				if (gAgent.getRunning()) gAgent.clearRunning();
				gAgent.clearAlwaysRun();
				gAgent.sendWalkRun(gAgent.getRunning());
			}
		}
	}
	else if (var.find ("camtextures") == 0 || var.find ("setcam_textures") == 0) {
		// silly hack, but we need to force all textures in world to be updated
		S32 i;
		for (i=0; i<gObjectList.getNumObjects(); ++i) {
			LLViewerObject* object = gObjectList.getObject(i);
			if (object) {
				object->setSelected(FALSE);
			}
		}

		// Is there a uuid specified ?
		int ind = var.find(":");
		if (ind != -1) {
			std::string uuid_str = var.substr(ind + 1);
			LLUUID uuid;
			if (uuid.set(uuid_str)) {
				//if (gAgent.mRRInterface.mCamTexturesCustom != LLViewerFetchedTexture::sDefaultImagep) {
				//	delete (gAgent.mRRInterface.mCamTexturesCustom);
				//}
				gAgent.mRRInterface.mCamTexturesCustom = LLViewerTextureManager::getFetchedTexture(uuid, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
			}
		}
		else {
			//if (gAgent.mRRInterface.mCamTexturesCustom != LLViewerFetchedTexture::sDefaultImagep) {
			//	delete (gAgent.mRRInterface.mCamTexturesCustom);
			//}
			gAgent.mRRInterface.mCamTexturesCustom = LLViewerFetchedTexture::sDefaultImagep;
		}
	}
	else if (var == "camunlock" || var == "setcam_unlock") {
		gAgentCamera.resetView(TRUE, TRUE);
	}
	//else if (var.find ("camzoommax") == 0 || var.find ("camzoommin") == 0) {
	//	LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
	//	gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
	//}

	//else if (var.find("setcam_fovmin") == 0 || var.find("setcam_fovmax") == 0) {
	//	LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
	//	gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
	//}

	if (gAgent.mRRInterface.contains("tplm")
	|| gAgent.mRRInterface.contains("tploc")
	|| gAgent.mRRInterface.contains("tplure")
	|| (gAgent.mRRInterface.mContainsUnsit && gAgentAvatarp->mIsSitting)
	) {
		gAgent.mRRInterface.mContainsTp = TRUE;
	}
	else {
		gAgent.mRRInterface.mContainsTp = FALSE;
	}

}

void updateAllHudTexts ()
{
	LLHUDText::TextObjectIterator text_it;
	
	for (text_it = LLHUDText::sTextObjects.begin(); 
		text_it != LLHUDText::sTextObjects.end(); 
		++text_it)
	{
		LLHUDText *hudText = *text_it;
		if (hudText && hudText->mLastMessageText != "") {
			// do not update the floating names of the avatars around
			LLViewerObject* obj = hudText->getSourceObject();
			if (obj && !obj->isAvatar()) {
				hudText->setString(hudText->mLastMessageText);
			}
		}
	}
}

void updateOneHudText (LLUUID uuid)
{
	LLViewerObject* obj = gObjectList.findObject(uuid);
	if (obj) {
		if (obj->mText.notNull()) {
			LLHUDText *hudText = obj->mText.get();
			if (hudText && hudText->mLastMessageText != "") {
				hudText->setString(hudText->mLastMessageText);
			}
		}
	}
}

// --





RRInterface::RRInterface():
	mInventoryFetched(FALSE)
	, mAllowCancelTp(TRUE)
	, mSitTargetId()
	, mLastLoadedPreset()
	, mReattaching(FALSE)
	, mReattachTimeout(FALSE)
	, mSnappingBackToLastStandingLocation(FALSE)
	, mUserUpdateAttachmentsUpdatesAll(FALSE)
	, mUserUpdateAttachmentsCalledFromScript(FALSE)
	, mScriptsEnabledOnce(FALSE)
	, mHasLockedHuds(FALSE)
	, mParcelLandingType(LLParcel::L_DIRECT)
	, mContainsDetach(FALSE)
	, mContainsShowinv(FALSE)
	, mContainsUnsit(FALSE)
	, mContainsInteract(FALSE)
	, mContainsShowworldmap(FALSE)
	, mContainsShowminimap(FALSE)
	, mContainsShowloc(FALSE)
	, mContainsShownames(FALSE)
	, mContainsShownametags(FALSE)
	, mContainsSetenv(FALSE)
	, mContainsSetdebug(FALSE)
	, mContainsFly(FALSE)
	, mContainsEdit(FALSE)
	, mContainsRez(FALSE)
	, mContainsShowhovertextall(FALSE)
	, mContainsShowhovertexthud(FALSE)
	, mContainsShowhovertextworld(FALSE)
	, mContainsDefaultwear(FALSE)
	, mContainsPermissive(FALSE)
	, mContainsRun(FALSE)
	, mContainsAlwaysRun(FALSE)
	, mContainsTp(FALSE)
	, mHandleNoStrip(TRUE)
	, mContainsCamTextures(FALSE)
	, mUserUpdateAttachmentsFirstCall(TRUE)
	, mUserUpdateAttachmentsCalledManually(FALSE)
	, mCamDistDrawFromJoint(NULL)
	, mGarbageCollectorCalledOnce(FALSE)
	//, mContainsMoveUp(FALSE)
	//, mContainsMoveDown(FALSE)
	//, mContainsMoveForward(FALSE)
	//, mContainsMoveBackward(FALSE)
	//, mContainsMoveTurnUp(FALSE)
	//, mContainsMoveTurnDown(FALSE)
	//, mContainsMoveTurnLeft(FALSE)
	//, mContainsMoveTurnRight(FALSE)
	//, mContainsMoveStrafeLeft(FALSE)
	//, mContainsMoveStrafeRight(FALSE)
	, mLaunchTimestamp(LLDate::now().secondsSinceEpoch())
	, mCamTexturesCustom(LLViewerFetchedTexture::sDefaultImagep)
{
	mAllowedGetDebug.clear();
	mAllowedGetDebug.push_back("AvatarSex"); // 0 female, 1 male
	mAllowedGetDebug.push_back("RenderResolutionDivisor"); // simulate blur, default is 1
	mAllowedGetDebug.push_back("RestrainedLoveForbidGiveToRLV");
	mAllowedGetDebug.push_back("RestrainedLoveNoSetEnv");
	mAllowedGetDebug.push_back("WindLightUseAtmosShaders");

	mAllowedSetDebug.clear();
	mAllowedSetDebug.push_back("AvatarSex"); // 0 female, 1 male
	mAllowedSetDebug.push_back("RenderResolutionDivisor"); // simulate blur, default is 1

	mParcelName = "";

	mAssetsToReattach.clear();

	mJustDetached.uuid.setNull();
	mJustDetached.attachpt = "";
	mJustReattached.uuid.setNull();
	mJustReattached.attachpt = "";

	sLastAvatarZOffsetCommit = 1.f; // So a first shape update will be done shortly after the viewer has started
	sLastOutfitChange = -1000.f;
	updateCameraLimits();
	updateLimits();

	// Calling gSavedSettings here crashes the viewer when compiled with VS2005.
	// OK under Linux. Moved this initialization to llstartup.cpp as a consequence.
	// sRestrainedLoveDebug = gSavedSettings.getBOOL("RestrainedLoveDebug");
}

RRInterface::~RRInterface()
{
}

std::string RRInterface::getVersion ()
{
	return RR_VIEWER_NAME " viewer v" RR_VERSION " (" RR_SLV_VERSION")"; // there is no '+' between the string and the macro
//	return RR_VIEWER_NAME" viewer v"RR_VERSION" ("+LLViewerInfo::getVersion()+")";
}

std::string RRInterface::getVersion2 ()
{
	return RR_VIEWER_NAME_NEW " viewer v" RR_VERSION " (" RR_SLV_VERSION")"; // there is no '+' between the string and the macro
//	return RR_VIEWER_NAME_NEW" viewer v"RR_VERSION" ("+LLViewerInfo::getVersion()+")";
}

std::string RRInterface::getVersionNum ()
{
	std::string res = RR_VERSION_NUM;
	if (sBlacklist != "") {
		res += ","+sBlacklist;
	}
	return res;
}

std::string RRInterface::getFirstName (std::string fullName)
{
	int ind = fullName.find (" ");
	if (ind != -1) return fullName.substr (0, ind);
	ind = fullName.find (".");
	if (ind != -1) return fullName.substr (0, ind);
	else return fullName;
}

std::string RRInterface::getLastName (std::string fullName)
{
	int ind = fullName.find (" ");
	if (ind != -1) return fullName.substr (ind+1);
	ind = fullName.find (".");
	if (ind != -1) return fullName.substr (ind+1);
	else return fullName;
}

BOOL RRInterface::isAllowed (LLUUID object_uuid, std::string action, BOOL log_it)
{
	BOOL debug = sRestrainedLoveDebug && log_it;
	if (debug) {
		LL_INFOS() << object_uuid.asString() << "      " << action << LL_ENDL;
	}
	RRMAP::iterator it = mSpecialObjectBehaviours.find (object_uuid.asString());
	while (it != mSpecialObjectBehaviours.end() &&
			it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	{
		if (debug) {
			LL_INFOS() << "  checking " << it->second << LL_ENDL;
		}
		if (it->second == action) {
			if (debug) {
				LL_INFOS() << "  => forbidden. " << LL_ENDL;
			}
			return FALSE;
		}
		it++;
	}
	if (debug) {
		LL_INFOS() << "  => allowed. " << LL_ENDL;
	}
	return TRUE;
}

BOOL RRInterface::contains (std::string action)
{
	RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
	LLStringUtil::toLower(action);
//	LL_INFOS() << "looking for " << action << LL_ENDL;
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second == action) {
//			LL_INFOS() << "found " << it->second << LL_ENDL;
			return TRUE;
		}
		it++;
	}
	return FALSE;
}

BOOL RRInterface::containsSubstr (std::string action)
{
	RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
	LLStringUtil::toLower(action);
//	LL_INFOS() << "looking for " << action << LL_ENDL;
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second.find (action) != -1) {
//			LL_INFOS() << "found " << it->second << LL_ENDL;
			return TRUE;
		}
		it++;
	}
	return FALSE;
}

F32 RRInterface::getMax (std::string action, F32 dflt /*= EXTREMUM*/)
{
	// action can be a list of behavs separated by commas
	LLStringUtil::toLower(action);
	action = "," + action + ",";
	F32 res = -EXTREMUM;
	F32 tmp;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;
	BOOL found_one = FALSE;
	for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
		command = it->second;
		LLStringUtil::toLower(command);
		if (parseCommand (command+"=n", behav, option, param)) {
			if (action.find ("," + behav + ",") != -1) {
				tmp = atof (option.c_str());
				if (option == "") {
					tmp = 1.5;
				}
				if (tmp > res) {
					res = tmp;
					found_one = TRUE;
				}
			}
		}
	}
	if (!found_one) {
		return dflt;
	}
	return res;
}

F32 RRInterface::getMin (std::string action, F32 dflt /*= -EXTREMUM*/)
{
	// action can be a list of behavs separated by commas
	LLStringUtil::toLower(action);
	action = "," + action + ",";
	F32 res = EXTREMUM;
	F32 tmp;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;
	BOOL found_one = FALSE;
	for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
		command = it->second;
		LLStringUtil::toLower(command);
		if (parseCommand (command+"=n", behav, option, param)) {
			if (action.find("," + behav + ",") != -1) {
				tmp = atof(option.c_str());
				if (option == "") {
					tmp = 1.5;
				}
				if (tmp < res) {
					res = tmp;
					found_one = TRUE;
				}
			}
		}
	}
	if (!found_one) {
		return dflt;
	}
	return res;
}

LLColor3 RRInterface::getMixedColors (std::string action, LLColor3 dflt /*= LLColor3::black*/)
{
	// action can be a list of behavs separated by commas
	LLStringUtil::toLower(action);
	action = "," + action + ",";
	LLColor3 res = dflt;
	LLColor3 tmp;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;
	int nb_found = 0;
	F32 h, s, l;
	F32 total_h = 0.f, total_s = 0.f, total_l = 0.f;
	std::deque<std::string> tokens;
	for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
		command = it->second;
		LLStringUtil::toLower(command);
		if (parseCommand (command+"=n", behav, option, param)) {
			if (action.find("," + behav + ",") != -1) {
				tokens = parse(option, ";");
				tmp.mV[0] = atof (tokens[0].c_str());
				tmp.mV[1] = atof (tokens[1].c_str());
				tmp.mV[2] = atof (tokens[2].c_str());
				tmp.calcHSL (&h, &s, &l);
				total_h += h;
				total_s += s;
				total_l += l;
				nb_found++;
			}
		}
	}
	if (nb_found > 0) {
		total_h /= (F32)nb_found;
		total_s /= (F32)nb_found;
		total_l /= (F32)nb_found;
		res.setHSL (total_h, total_s, total_l);
	}
	return res;
}

BOOL RRInterface::containsWithoutException (std::string action, std::string except /* = "" */)
{
	// action is a restriction like @sendim, which can accept exceptions (@sendim:except_uuid=add)
	// action_sec is the same action, with "_sec" appended (like @sendim_sec)
	
	LLStringUtil::toLower(action);
	std::string action_sec = action + "_sec";
	LLUUID uuid;
	
	// 1. If except is empty, behave like contains(), but looking for both action and action_sec
	if (except == "") {
		return (contains (action) || contains (action_sec));
	}

	// 2. For each action_sec, if we don't find an exception tied to the same object, return TRUE
	// if @permissive is set, then even action needs the exception to be tied to the same object, not just action_sec
	// (@permissive restrains the scope of all the exceptions to their own objects)
	RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second == action_sec 
		|| it->second == action && mContainsPermissive) {
			uuid.set (it->first);
			if (isAllowed (uuid, action+":"+except, FALSE) && isAllowed (uuid, action_sec+":"+except, FALSE)) { // we use isAllowed because we need to check the object, but it really means "does not contain"
				return TRUE;
			}
		}
		it++;
	}
	
	// 3. If we didn't return yet, but the map contains action, just look for except_uuid without regard to its object, if none is found return TRUE
	if (contains (action)) {
		if (!contains (action+":"+except) && !contains (action_sec+":"+except)) {
			return TRUE;
		}
	}
	
	// 4. Finally return FALSE if we didn't find anything
	return FALSE;
}

bool RRInterface::isFolderLocked(LLInventoryCategory* cat)
{
//	const LLFolderType::EType folder_type = cat->getPreferredType();
//	if (LLFolderType::lookupIsProtectedType(folder_type)) return false;

	bool shared = isUnderRlvShare(cat);
	if (!shared && contains("unsharedwear")) return true;
	if (shared && contains("sharedwear")) return true;
	if (isFolderLockedWithoutException(cat, "attach") != FolderLock_unlocked) return true;
	if (isFolderLockedWithoutException(cat, "detach") != FolderLock_unlocked) return true;
	return false;
}

FolderLock RRInterface::isFolderLockedWithoutException (LLInventoryCategory* cat, std::string attach_or_detach)
{
	if (cat == NULL) return FolderLock_unlocked;

	if (sRestrainedLoveDebug) {
		LL_INFOS() << "isFolderLockedWithoutException(" << cat->getName() << ", " << attach_or_detach << ")" << LL_ENDL;
	}
	// For each object that is locking this folder, check whether it also issues exceptions to this lock
	std::deque<std::string> commands_list;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;
	std::string this_command;
	std::string this_behav;
	std::string this_option;
	std::string this_param;
	bool this_object_locks;
	FolderLock current_lock = FolderLock_unlocked;
	for (RRMAP::iterator it = mSpecialObjectBehaviours.begin (); it != mSpecialObjectBehaviours.end(); ++it) {
		LLUUID uuid = LLUUID(it->first);
		command = it->second;
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "command = " << command << LL_ENDL;
		}
		// param will always be equal to "n" in this case since we added it to command, but we don't care about this here
		// Attention, an option must absolutely be specified here (there must be a ":" character), or we wouldn't be able to tell "detachthis" from "detachthis:"
		// and both have different meanings
		if (command.find (":") != -1 && parseCommand (command+"=n", behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
		{
			// find whether this object has issued a "{attach|detach}[all]this" command on a folder that is either this one, or a parent
			this_object_locks = false;
			if (behav == attach_or_detach+"this") {
				if (getCategoryUnderRlvShare(option) == cat) {
					this_object_locks = true;
				}
			}
			else if (behav == attach_or_detach+"allthis") {
				if (isUnderFolder(getCategoryUnderRlvShare(option), cat)) {
					this_object_locks = true;
				}
			}

			// This object has issued such a command, check whether it has issued an exception to it as well
			if (this_object_locks) {
				commands_list = getListOfRestrictions(uuid);
				FolderLock this_lock = isFolderLockedWithoutExceptionAux(cat, attach_or_detach, commands_list);
				if (this_lock == FolderLock_locked_without_except) return FolderLock_locked_without_except;
				else current_lock = this_lock;
				if (sRestrainedLoveDebug) {
					LL_INFOS() << "this_lock=" << this_lock << LL_ENDL;
				}
			}
		}
	}

	// Finally, return unlocked since we didn't find any lock on this folder
	return current_lock;
}

FolderLock RRInterface::isFolderLockedWithoutExceptionAux (LLInventoryCategory* cat, std::string attach_or_detach, std::deque<std::string> list_of_restrictions)
{
	// list_of_restrictions contains the list of restrictions issued by one particular object, at least one is supposed to be a "{attach|detach}[all]this"
	// For each folder from cat up to the root folder, check :
	// - if we are on cat and we find "{attach|detach}this_except", there is an exception, keep looking up
	// - if we are on cat and we find "{attach|detach}this", there is no exception, return FolderLock_locked_without_except
	// - if we are on a parent and we find "{attach|detach}allthis_except", there is an exception, keep looking up
	// - if we are on a parent and we find "{attach|detach}allthis", if we found an exception return FolderLock_locked_with_except, else return FolderLock_locked_without_except
	// - finally, if we are on the root, return FolderLocked_unlocked (whether there was an exception or not)
	if (cat == NULL) {
		return FolderLock_unlocked;
	}

	if (sRestrainedLoveDebug) {
		LL_INFOS() << "isFolderLockedWithoutExceptionAux(" << cat->getName() << ", " << attach_or_detach << ", [" << dumpList2String(list_of_restrictions, ",") << "])" << LL_ENDL;
	}

	FolderLock current_lock = FolderLock_unlocked;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;

	LLInventoryCategory* it = NULL;
	LLInventoryCategory* cat_option = NULL;
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
	
	const LLUUID& cat_id = cat->getUUID();
	it = gInventory.getCategory (cat_id);
	
	do {
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "it=" << it->getName() << LL_ENDL;
		}

		for (unsigned int i = 0; i < list_of_restrictions.size(); ++i)
		{
			command = list_of_restrictions[i];
			if (sRestrainedLoveDebug) {
				LL_INFOS() << "command2=" << command << LL_ENDL;
			}

			// param will always be equal to "n" in this case since we added it to command, but we don't care about this here
			if (parseCommand (command+"=n", behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
			{
				cat_option = getCategoryUnderRlvShare(option);
				if (cat_option == it) {
					if (it == cat) {
						if (behav == attach_or_detach+"this_except" || behav == attach_or_detach+"allthis_except") {
							current_lock = FolderLock_locked_with_except;
						}
						else if (behav == attach_or_detach+"this"|| behav == attach_or_detach+"allthis") {
							return FolderLock_locked_without_except;
						}
					}
					else {
						if (behav == attach_or_detach+"allthis_except") {
							current_lock = FolderLock_locked_with_except;
						}
						else if (behav == attach_or_detach+"allthis") {
							if (current_lock == FolderLock_locked_with_except) return FolderLock_locked_with_except;
							else return FolderLock_locked_without_except;
						}
					}
				}
			}
		}

		const LLUUID& parent_id = it->getParentUUID();
		it = gInventory.getCategory (parent_id);
	} while (it && it->getUUID() != root_id);
	return FolderLock_unlocked; // this should never happen since list_of_commands is supposed to contain at least one "{attach|detach}[all]this" restriction
}

BOOL RRInterface::add (LLUUID object_uuid, std::string action, std::string option)
{
	if (sRestrainedLoveDebug) {
		LL_INFOS() << object_uuid.asString() << "       " << action << "      " << option << LL_ENDL;
	}
	
	std::string canon_action = action;
	if (option!="") action+=":"+option;
    
	if (isAllowed (object_uuid, action)) {
		// Notify if needed
		notify (object_uuid, action, "=n");

		// If this action is blacklisted, do nothing
		if (canon_action != "notify" && isBlacklisted (canon_action, false)) {
			return TRUE;
		}
		
		// Actions to do BEFORE inserting the new behav
		if (action=="fly") {
 			gAgent.setFlying (FALSE);
   		}
		else if (action=="edit") {
			LLPipeline::setRenderBeacons(FALSE);
			LLPipeline::setRenderScriptedBeacons(FALSE);
			LLPipeline::setRenderScriptedTouchBeacons(FALSE);
			LLPipeline::setRenderPhysicalBeacons(FALSE);
			LLPipeline::setRenderSoundBeacons(FALSE);
			LLPipeline::setRenderParticleBeacons(FALSE);
			LLPipeline::setRenderHighlights(FALSE);
			LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
		}
		else if (action=="setenv") {
			if (sRRNoSetEnv) {
				return TRUE;
			}
//			LLFloaterEnvSettings::instance()->close();
//			LLFloaterWater::instance()->close();
//			LLFloaterPostProcess::instance()->close();
//			LLFloaterDayCycle::instance()->close();
//			LLFloaterWindLight::instance()->close();
			gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
			gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
			LLFloaterReg::hideInstance("env_settings");
			LLFloaterReg::hideInstance("env_water");
			LLFloaterReg::hideInstance("env_post_process");
			LLFloaterReg::hideInstance("env_day_cycle");
			LLFloaterReg::hideInstance("env_windlight");
		}
		else if (action=="setdebug") {
			if (!sRRNoSetEnv) {
				gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
				gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
			}
		}

		// Insert the new behav
		mSpecialObjectBehaviours.insert(std::pair<std::string, std::string>(object_uuid.asString(), action));
		refreshCachedVariable(action);

		// Actions to do AFTER inserting the new behav
		if (action=="showhovertextall" || action=="showloc" || action=="shownames"
			|| action=="showhovertexthud" || action=="showhovertextworld" ) {
			updateAllHudTexts();
		}
		else if (canon_action == "showhovertext") {
			updateOneHudText(LLUUID(option));
		}
		else if (canon_action.find("cam") == 0 || canon_action.find("setcam") == 0) {
			updateCameraLimits ();
			// Force an update of the zoom if necessary
			if (canon_action == "camzoommax" || canon_action == "camzoommin" || canon_action == "setcam_fovmin" || canon_action == "setcam_fovmax") {
				LLViewerCamera::getInstance()->setDefaultFOV(gSavedSettings.getF32("CameraAngle"));
				gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
			}
		}
		else if (canon_action == "fartouch"
			|| canon_action == "touchfar"
			|| canon_action == "sittp"
			|| canon_action == "tplocal"
			) {
			updateLimits();
		}

		// Check if we can see wireframe or not, deactivate if our vision is restricted (we have locked HUDs or mCamDistDrawMax is not infinite)
		if (gAgent.mRRInterface.hasLockedHuds() || gAgent.mRRInterface.mCamDistDrawMax < EXTREMUM)
		{
			if (gUseWireframe)
			{
				gUseWireframe = FALSE;
				// Copied from LLAdvancedToggleWireframe::handle_event() in llviewermenu.cpp
				gWindowResized = TRUE;
				LLPipeline::updateRenderDeferred();
				gPipeline.resetVertexBuffers();

				if (!gUseWireframe && !gInitialDeferredModeForWireframe && LLPipeline::sRenderDeferred != ((bool)gInitialDeferredModeForWireframe && gPipeline.isInit()))
				{
					LLPipeline::refreshCachedSettings();
					gPipeline.releaseGLBuffers();
					gPipeline.createGLBuffers();
					LLViewerShaderMgr::instance()->setShaders();
				}
			}
		}

		// Update the stored last standing location, to allow grabbers to transport a victim inside a cage while sitting, and restrict them
		// before standing up. If we didn't do this, the avatar would snap back to a safe location when being unsitted by the grabber,
		// which would be rather silly.
		if (action == "standtp") {
			gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal ());
			gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
		}

		return TRUE;
	}
	return FALSE;
}

BOOL RRInterface::remove (LLUUID object_uuid, std::string action, std::string option)
{
	if (sRestrainedLoveDebug) {
		LL_INFOS() << object_uuid.asString() << "       " << action << "      " << option << LL_ENDL;
	}

	std::string canon_action = action;
	if (option!="") action+=":"+option;
	
	// Notify if needed
	notify (object_uuid, action, "=y");
	
	// Actions to do BEFORE removing the behav

	// Remove the behav
	RRMAP::iterator it = mSpecialObjectBehaviours.find (object_uuid.asString());
	while (it != mSpecialObjectBehaviours.end() &&
			it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	{
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "  checking " << it->second << LL_ENDL;
		}
		if (it->second == action) {
			mSpecialObjectBehaviours.erase(it);
			if (sRestrainedLoveDebug) {
				LL_INFOS() << "  => removed. " << LL_ENDL;
			}
			refreshCachedVariable(action);

			// Actions to do AFTER removing the behav
			if (action=="showhovertextall" || action=="showloc" || action=="shownames"
				|| action=="showhovertexthud" || action=="showhovertextworld" ) {
				updateAllHudTexts();
			}
			if (canon_action == "showhovertext") {
				updateOneHudText(LLUUID(option));
			}
			else if (canon_action.find("cam") == 0 || canon_action.find("setcam") == 0) {
				updateCameraLimits ();
			}
			else if (canon_action == "fartouch"
				|| canon_action == "touchfar"
				|| canon_action == "sittp"
				|| canon_action == "tplocal"
				) {
				updateLimits();
			}
			else if (action == "standtp" && gAgentAvatarp && !gAgentAvatarp->isSitting()) { // If we are not sitting, then we can remove the @standtp restriction normally
				gAgent.mRRInterface.mLastStandingLocation.clear();
				gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
			}

			return TRUE;
		}
		it++;
	}

	return FALSE;
}

BOOL RRInterface::clear (LLUUID object_uuid, std::string command)
{
	if (sRestrainedLoveDebug) {
		LL_INFOS() << object_uuid.asString() << "   /   " << command << LL_ENDL;
	}

	// Notify if needed
	notify (object_uuid, "clear" + (command!=""? ":"+command : ""), "");
	
	RRMAP::iterator it;
	it = mSpecialObjectBehaviours.begin ();
	while (it != mSpecialObjectBehaviours.end()) {
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "  checking " << it->second << LL_ENDL;
		}
		if (it->first==object_uuid.asString() && (command=="" || it->second.find (command)!=-1)) {
			notify (object_uuid, it->second, "=y");
			if (sRestrainedLoveDebug) {
				LL_INFOS() << it->second << " => removed. " << LL_ENDL;
			}
			std::string tmp = it->second;
			mSpecialObjectBehaviours.erase(it);
			refreshCachedVariable(tmp);
			it = mSpecialObjectBehaviours.begin ();
		}
		else {
			it++;
		}
	}
	updateAllHudTexts();
	updateCameraLimits();
	updateLimits();
	if (gAgentAvatarp && !gAgentAvatarp->isSitting()) { // If we are not sitting, then we can remove the @standtp restriction normally
		gAgent.mRRInterface.mLastStandingLocation.clear();
		gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
	}

	return TRUE;
}

void RRInterface::replace (LLUUID what, LLUUID by)
{
	RRMAP::iterator it;
	LLUUID uuid;
	it = mSpecialObjectBehaviours.begin ();
	while (it != mSpecialObjectBehaviours.end()) {
		uuid.set (it->first);
		if (uuid == what) {
			// found the UUID to replace => add a copy of the command with the new UUID
			mSpecialObjectBehaviours.insert(std::pair<std::string, std::string>(by.asString(), it->second));
		}
		it++;
	}
	// and then clear the old UUID
	clear (what, "");
}


BOOL RRInterface::garbageCollector (BOOL all) {
	RRMAP::iterator it;
	BOOL res=FALSE;
	LLUUID uuid;
	LLViewerObject *objp=NULL;
	it = mSpecialObjectBehaviours.begin ();
	while (it != mSpecialObjectBehaviours.end()) {
		uuid.set (it->first);
		if (all || !uuid.isNull ()) {
//			if (sRestrainedLoveDebug) {
//				LL_INFOS() << "testing " << it->first << LL_ENDL;
//			}
			objp = gObjectList.findObject(uuid);
			if (!objp) {
				if (sRestrainedLoveDebug) {
					LL_INFOS() << it->first << " not found => cleaning... " << LL_ENDL;
				}
				clear (uuid);
				res=TRUE;
				it=mSpecialObjectBehaviours.begin ();
			} else {
				it++;
			}
		} else {
			if (sRestrainedLoveDebug) {
				LL_INFOS() << "ignoring " << it->second << LL_ENDL;
			}
			it++;
		}
    }
	mGarbageCollectorCalledOnce = TRUE;
    return res;
}

std::deque<std::string> RRInterface::parse (std::string str, std::string sep)
{
	int ind;
	int length = sep.length();
	std::string token;
	std::deque<std::string> res;
	
	do {
		ind=str.find(sep);
		if (ind!=-1) {
			token = str.substr (0, ind);
			if (token != "") {
				res.push_back (token);
			}
			str=str.substr (ind+length);
		}
		else {
			if (str != "") {
				res.push_back (str);
			}
		}
	} while (ind!=-1);
	
	return res;
}


void RRInterface::notify (LLUUID object_uuid, std::string action, std::string suffix)
{
	// scan the list of restrictions, when finding "notify" say the restriction on the specified channel
	RRMAP::iterator it;
	int length = 7; // size of "notify:"
	int size;
	std::deque<std::string> tokens;
	LLUUID uuid;
	std::string rule;
	it = mSpecialObjectBehaviours.begin ();
	
	while (it != mSpecialObjectBehaviours.end()) {
		uuid.set (it->first);
		rule = it->second; // we are looking for rules like "notify:2222;tp", if action contains "tp" then notify the scripts on channel 2222
		if (rule.find("notify:") == 0) {
			// found a possible notification to send
			rule = rule.substr(length); // keep right part only (here "2222;tp")
			tokens = parse (rule, ";");
			size = tokens.size();
			if (size == 1 || size > 1 && action.find(tokens[1]) != -1) {
				answerOnChat(tokens[0], "/" + action + suffix); // suffix can be "=n", "=y" or whatever else we want, "/" is needed to avoid some clever griefing
			}
		}
		it++;
	}
}


BOOL RRInterface::parseCommand (std::string command, std::string& behaviour, std::string& option, std::string& param)
{
	int ind = command.find("=");
	behaviour=command;
	option="";
	param="";
	if (ind!=-1) {
		behaviour=command.substr(0, ind);
		param=command.substr(ind+1);
		ind=behaviour.find(":");
		if (ind!=-1) {
			option=behaviour.substr(ind+1);
			behaviour=behaviour.substr(0, ind); // keep in this order (option first, then behav) or crash
		}
		return TRUE;
	}
	return FALSE;
}

BOOL RRInterface::handleCommand (LLUUID uuid, std::string command)
{
	// 1. check the command is actually a single one or a list of commands separated by ","
	if (command.find (",")!=-1) {
		BOOL res=TRUE;
		std::deque<std::string> list_of_commands=parse (command, ",");
		for (unsigned int i=0; i<list_of_commands.size (); ++i) {
			if (!handleCommand (uuid, list_of_commands.at(i))) res=FALSE;
		}
		return res;
	}
	
	// 2. this is a single command, possibly inside a 1-level recursive call (unimportant)
	// if the viewer is not fully initialized and the user does not have control of their avatar,
	// don't execute the command but retain it for later, when it is fully initialized
	// If there is another object still waiting to be automatically reattached, retain all RLV commands
	// as well to avoid an infinite loop if the one it will kick off is also locked.
	// This is valid as the object that would possibly be kicked off by the one to reattach, will have
	// its restrictions wiped out by the garbage collector
	if (LLStartUp::getStartupState() < STATE_CLEANUP
		|| (!mAssetsToReattach.empty() && !mReattachTimeout)) {
		Command cmd;
		cmd.uuid=uuid;
		cmd.command=command;
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "Retaining command : " << command << LL_ENDL;
		}
		mRetainedCommands.push_back (cmd);
		return TRUE;
	}
	
	// 3. parse the command, which is of one of these forms :
	// behav=param
	// behav:option=param
	std::string behav;
	std::string option;
	std::string param;
	LLStringUtil::toLower(command);
	if (parseCommand (command, behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
	{
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "[" << uuid.asString() << "]  [" << behav << "]  [" << option << "] [" << param << "]" << LL_ENDL;
		}
		if (behav=="version") return answerOnChat (param, getVersion ());
		else if (behav=="versionnew") return answerOnChat (param, getVersion2 ());
		else if (behav=="versionnum") return answerOnChat (param, RR_VERSION_NUM);
		else if (behav=="versionnumbl") return answerOnChat (param, getVersionNum());
		else if (behav=="getblacklist") return answerOnChat (param, dumpList2String (getBlacklist(option), ","));
		else if (behav=="getoutfit") return answerOnChat (param, getOutfit (option));
		else if (behav=="getattach") return answerOnChat (param, getAttachments (option));
		else if (behav=="getstatus") return answerOnChat (param, getStatus (uuid, option));
		else if (behav=="getstatusall") {
			uuid.setNull();
			return answerOnChat (param, getStatus (uuid, option));
		}
		else if (behav=="getinv") return answerOnChat (param, getInventoryList (option));
		else if (behav=="getinvworn") return answerOnChat (param, getInventoryList (option, TRUE));
		else if (behav=="getsitid") return answerOnChat (param, getSitTargetId ().asString());
		else if (behav=="getpath") return answerOnChat (param, getFullPath (getItem(uuid), option, false)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
		else if (behav=="getpathnew") return answerOnChat (param, getFullPath (getItem(uuid), option)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
		else if (behav == "findfolder") return answerOnChat(param, getFullPath(findCategoryUnderRlvShare(option)));
		else if (behav == "findfolders") {
			std::deque<std::string> options = parse(option, ";");
			std::string folder_to_find = options.at(0);
			std::string separator = (options.size() > 1) ? options.at(1) : ",";
			std::deque<LLInventoryCategory*> cats = findCategoriesUnderRlvShare(folder_to_find);
			std::string response = "";
			for (int i = 0; i < cats.size(); i++) {
				if (i > 0) response += separator;
				response += getFullPath(cats.at(i));
			}
			return answerOnChat(param, response);
		}
		else if (behav.find ("getenv_") == 0) return answerOnChat (param, getEnvironment (behav));
		else if (behav.find ("getdebug_") == 0) return answerOnChat (param, getDebugSetting (behav));
		else if (behav=="getgroup") {
			std::string group_name = gAgent.getGroupName();
			if (gAgent.getGroupID() == LLUUID::null) group_name = "none"; // "none" is not localized here because a script should not have to bother about viewer language
			return answerOnChat (param, group_name);
		}
		else if (behav=="getcam_avdistmin") {
			std::stringstream str;
			str << std::fixed << mCamDistMin;
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_avdistmax") {
			std::stringstream str;
			str << std::fixed << mCamDistMax;
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_zoommin") {
			std::stringstream str;
			str << std::fixed << mCamZoomMin;
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_zoommax") {
			std::stringstream str;
			str << std::fixed << mCamZoomMax;
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_fovmin") {
			std::stringstream str;
			str << std::fixed << DEFAULT_FIELD_OF_VIEW / mCamZoomMax;
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_fovmax") {
			std::stringstream str;
			str << std::fixed << DEFAULT_FIELD_OF_VIEW / mCamZoomMin;
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_zoom") {
			std::stringstream str;
			str << std::fixed << DEFAULT_FIELD_OF_VIEW / LLViewerCamera::getInstance()->getView();
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_fov") {
			std::stringstream str;
			str << std::fixed << LLViewerCamera::getInstance()->getView();
			return answerOnChat(param, str.str());
		}
		else if (behav == "getcam_textures") {
			return answerOnChat(param, mCamTexturesCustom->getID().asString());
		}

		else {
			if (param=="n" || param=="add") add (uuid, behav, option);
			else if (param=="y" || param=="rem") remove (uuid, behav, option);
			else if (behav=="clear") clear (uuid, param);
			else if (param=="force") force (uuid, behav, option);
			else return FALSE;
		}
	}
	else // clear
	{
		if (sRestrainedLoveDebug) {
			LL_INFOS() << uuid.asString() << "       " << behav << LL_ENDL;
		}
		if (behav=="clear") clear (uuid);
		else return FALSE;
	}
	return TRUE;
}

BOOL RRInterface::fireCommands ()
{
	BOOL ok=TRUE;
	if (mRetainedCommands.size ()) {
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "Firing commands : " << mRetainedCommands.size () << LL_ENDL;
		}
		Command cmd;
		while (!mRetainedCommands.empty ()) {
			cmd=mRetainedCommands[0];
			ok=ok & handleCommand (cmd.uuid, cmd.command);
			mRetainedCommands.pop_front ();
		}
	}
	return ok;
}

static void force_sit(LLUUID object_uuid)
{
	// Note : Although it would make sense that only the UUID should be needed, we actually need to send an
	// offset to the sim, therefore we need the object to be known to the viewer. In other words, issuing @sit=force
	// right after a teleport is not going to work because the object will not have had time to rez.
	LLViewerObject *object = gObjectList.findObject(object_uuid);
	if (object) {
		LLVOAvatar* avatar = gAgentAvatarp;
		if (avatar && gAgent.mRRInterface.mContainsUnsit && avatar->mIsSitting) {
			// Do not allow a script to force the avatar to sit somewhere if already forced to stay sitting here
			return;
		}

		if (gAgent.mRRInterface.mContainsInteract)
		{
			return;
		}

		if (gAgent.mRRInterface.contains("sit"))
		{
			return;
		}

		if (gAgentAvatarp && !gAgentAvatarp->mIsSitting)
		{
			// We are now standing, and we want to sit down => store our current location so that we can snap back here when we stand up, if under @standtp
			if (gAgent.mRRInterface.mContainsStandtp)
			{
				gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal ());
				gSavedPerAccountSettings.setVector3d("RestrainedLoveLastStandingLocation", gAgent.mRRInterface.mLastStandingLocation);
			}
		}
		gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
		gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
		gMessageSystem->addVector3Fast(_PREHASH_Offset,
			gAgentCamera.calcFocusOffset(object, gAgent.getPositionAgent(), (S32)0.0f, (S32)0.0f));
		object->getRegion()->sendReliableMessage();
	}
}


BOOL RRInterface::force (LLUUID object_uuid, std::string command, std::string option)
{
	if (sRestrainedLoveDebug) {
		LL_INFOS() << command << "     " << option << LL_ENDL;
	}

	// If this action is blacklisted, do nothing
	if (isBlacklisted (command, true)) {
		return TRUE;
	}
		
	if (command=="sit") { // sit:UUID
		BOOL allowed_to_sittp=TRUE;
		if (!isAllowed (object_uuid, "sittp")) {
			allowed_to_sittp=FALSE;
			remove (object_uuid, "sittp", "");
		}
		LLUUID uuid (option);
		force_sit (uuid);
		if (!allowed_to_sittp) add (object_uuid, "sittp", "");
	}
	else if (command=="unsit") { // unsit
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "trying to unsit" << LL_ENDL;
		}
		if (gAgentAvatarp &&
			gAgentAvatarp->mIsSitting) {
			if (sRestrainedLoveDebug) {
				LL_INFOS() << "found avatar object" << LL_ENDL;
			}
			if (gAgent.mRRInterface.mContainsUnsit) {
				if (sRestrainedLoveDebug) {
					LL_INFOS() << "prevented from unsitting" << LL_ENDL;
				}
				return TRUE;
			}
			if (sRestrainedLoveDebug) {
				LL_INFOS() << "unsitting agent" << LL_ENDL;
			}
//			LLOverlayBar::onClickStandUp(NULL);
			gAgent.standUp();
			send_agent_update(TRUE, TRUE);
		}
	}
	else if (command=="remoutfit") { // remoutfit:shoes
		if (option=="") {
//			gAgentWearables.removeWearable (WT_GLOVES, false, 0);
//			gAgentWearables.removeWearable (WT_JACKET, false, 0);
//			gAgentWearables.removeWearable (WT_PANTS, false, 0);
//			gAgentWearables.removeWearable (WT_SHIRT, false, 0);
//			gAgentWearables.removeWearable (WT_SHOES, false, 0);
//			gAgentWearables.removeWearable (WT_SKIRT, false, 0);
//			gAgentWearables.removeWearable (WT_SOCKS, false, 0);
//			gAgentWearables.removeWearable (WT_UNDERPANTS, false, 0);
//			gAgentWearables.removeWearable (WT_UNDERSHIRT, false, 0);

			for (int i = LLAgentWearables::MAX_CLOTHING_PER_TYPE - 1; i >= 0; --i) {
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_GLOVES, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_JACKET, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_PANTS, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SHIRT, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SHOES, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SKIRT, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_SOCKS, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_UNDERPANTS, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_UNDERSHIRT, i)));

#if ALPHA_AND_TATTOO
//			gAgentWearables.removeWearable (WT_ALPHA, false, 0);
//			gAgentWearables.removeWearable (WT_TATTOO, false, 0);
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_ALPHA, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_TATTOO, i)));
				removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (LLWearableType::WT_PHYSICS, i)));
#endif
			}
		}
		else {
			LLWearableType::EType type = getOutfitLayerAsType (option);
			if (type != LLWearableType::WT_INVALID) {
				 // clothes only, not skin, eyes, hair or shape
				if (LLWearableType::getAssetType(type) == LLAssetType::AT_CLOTHING) {
//					gAgentWearables.removeWearable (type, false, 0); // remove by layer
					for (int i = LLAgentWearables::MAX_CLOTHING_PER_TYPE - 1; i >= 0; --i) {
						removeItemFromAvatar (gInventory.getItem(gAgentWearables.getWearableItemID (type, i)));
					}
				}
			}
			else forceDetachByName (option, FALSE); // remove by category (in RLV share)
		}
	}
	else if (command=="detach" || command=="remattach") { // detach:chest=force OR detach:restraints/cuffs=force (@remattach is a synonym)
		if (LLUUID::validate (option)) { // if option is a UUID, detach this object only
			return forceDetachByUuid(option); // remove by uuid
		}
		else {
			LLViewerJointAttachment* attachpt = findAttachmentPointFromName(option, TRUE); // exact name
			if (attachpt != NULL || option == "") return forceDetach(option); // remove by attach pt
			else forceDetachByName(option, FALSE);
		}
	}
	else if (command=="detachme") { // detachme=force to detach this object specifically
		return forceDetachByUuid (object_uuid.asString()); // remove by uuid
	}
	else if (command=="detachthis") { // detachthis=force to detach the folder containing this object
		std::string pathes_str;
		if (LLUUID::validate(option)) { // if option is a UUID, we're not detaching the folder contain the calling object, but the referenced object
			pathes_str = getFullPath(getItem(LLUUID (option)));
		}
		else {
			pathes_str = getFullPath(getItem(object_uuid), option);
		}
		std::deque<std::string> pathes = parse (pathes_str, ",");
		BOOL res = TRUE;
		for (unsigned int i = 0; i < pathes.size(); ++i) {
			res &= forceDetachByName (pathes.at(i), FALSE);
		}
		return res;
	}
	else if (command=="detachall") { // detachall:cuffs=force to detach a folder and its subfolders
		BOOL res = FALSE;
		// We're now doing the same thing as "Remove From Current Outfit" in the inventory, except that we need to check for "nostrip"
		// during this action, hence the need for mUserUpdateAttachmentsCalledFromScript
		mUserUpdateAttachmentsCalledFromScript = TRUE;
		res = forceDetachByName (option, TRUE);
		mUserUpdateAttachmentsCalledFromScript = FALSE;
		return res;
	}
	else if (command=="detachallthis") { // detachallthis=force to detach the folder containing this object and also its subfolders
		std::string pathes_str;
		if (LLUUID::validate(option)) { // if option is a UUID, we're not detaching the folder contain the calling object, but the referenced object
			pathes_str = getFullPath(getItem(LLUUID(option)));
		}
		else {
			pathes_str = getFullPath(getItem(object_uuid), option);
		}
		std::deque<std::string> pathes = parse(pathes_str, ",");
		BOOL res = TRUE;
		for (unsigned int i = 0; i < pathes.size(); ++i) {
			res &= forceDetachByName (pathes.at(i), TRUE);
		}
		return res;
	}
	else if (command=="tpto") { // tpto:[region/]X/Y/Z[;lookat]=force (X, Y, Z are local or global coordinates, depending on the existence of the region)
		size_t ind = option.find(";"); // lookat present ? (FIXME : broken at the moment because gAgent.teleportViaLocationLookAt () does not take a "lookat" parameter (to add it we have to change a whole lot of calls all over the code)
		LLVector3 vecLookAt = LLVector3::zero;
		if (ind != std::string::npos && ind + 1 < option.length()) {
			F32 lookat = (F32)atof(option.substr(ind + 1).c_str());
			vecLookAt = LLVector3::x_axis;
			vecLookAt.rotVec(lookat, LLVector3::z_axis);
			vecLookAt.normalize();
			option = option.substr(0, ind); // keep the coordinates only
		}
		BOOL allowed_to_tploc = TRUE;
		BOOL allowed_to_unsit=TRUE;
		BOOL allowed_to_sittp=TRUE;
		BOOL res;
		if (!isAllowed (object_uuid, "tploc")) {
			allowed_to_tploc=FALSE;
			remove (object_uuid, "tploc", "");
		}
		if (!isAllowed (object_uuid, "unsit")) {
			allowed_to_unsit=FALSE;
			remove (object_uuid, "unsit", "");
		}
		if (!isAllowed (object_uuid, "sittp")) {
			allowed_to_sittp=FALSE;
			remove (object_uuid, "sittp", "");
		}
		res = forceTeleport(option, vecLookAt);
		if (!allowed_to_tploc) add (object_uuid, "tploc", "");
		if (!allowed_to_unsit) add (object_uuid, "unsit", "");
		if (!allowed_to_sittp) add (object_uuid, "sittp", "");
		return res;
	}
	//else if (command=="offertp") { // force to offer a tp to someone (regardless of showloc restriction, but do not show a confirmation)
	//	LLSD edit_args;
	//	edit_args["REGION"] = gAgent.getRegion()->getName();

	//	LLSD payload;
	//	if (LLUUID::validate(option)) {
	//		LLUUID uuid(option);
	//		payload["ids"].append(uuid);

	//	}
	//}
	else if (command=="attach" || command == "addoutfit") { // attach:cuffs=force
		return forceAttach (option, FALSE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
	}
	else if (command=="attachover" || command == "addoutfitover") { // attachover:cuffs=force
		return forceAttach (option, FALSE, AttachHow_over);
	}
	else if (command=="attachoverorreplace" || command == "addoutfitoverorreplace") { // attachoverorreplace:cuffs=force
		return forceAttach (option, FALSE, AttachHow_over_or_replace);
	}
	else if (command=="attachthis" || command == "addoutfitthis") { // attachthis=force to attach the folder containing this object
		BOOL res = TRUE;
		std::string pathes_str = getFullPath (getItem(object_uuid), option);
		if (pathes_str != "") {
			std::deque<std::string> pathes = parse (pathes_str, ",");
			for (unsigned int i = 0; i < pathes.size(); ++i) {
				res &= forceAttach (pathes.at(i), FALSE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
			}
		}
		return res;
	}
	else if (command=="attachthisover" || command == "addoutfitthisover") { // attachthisover=force to attach the folder containing this object
		BOOL res = TRUE;
		std::string pathes_str = getFullPath (getItem(object_uuid), option);
		if (pathes_str != "") {
			std::deque<std::string> pathes = parse (pathes_str, ",");
			for (unsigned int i = 0; i < pathes.size(); ++i) {
				res &= forceAttach (pathes.at(i), FALSE, AttachHow_over);
			}
		}
		return res;
	}
	else if (command=="attachthisoverorreplace" || command == "addoutfitthisoverorreplace") { // attachthisoverorreplace=force to attach the folder containing this object
		BOOL res = TRUE;
		std::string pathes_str = getFullPath (getItem(object_uuid), option);
		if (pathes_str != "") {
			std::deque<std::string> pathes = parse (pathes_str, ",");
			for (unsigned int i = 0; i < pathes.size(); ++i) {
				res &= forceAttach (pathes.at(i), FALSE, AttachHow_over_or_replace);
			}
		}
		return res;
	}
	else if (command=="attachall" || command == "addoutfitall") { // attachall:cuffs=force to attach a folder and its subfolders
		return forceAttach (option, TRUE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
	}
	else if (command=="attachallover" || command == "addoutfitallover") { // attachallover:cuffs=force to attach a folder and its subfolders
		return forceAttach (option, TRUE, AttachHow_over);
	}
	else if (command=="attachalloverorreplace" || command == "addoutfitalloverorreplace") { // attachalloverorreplace:cuffs=force to attach a folder and its subfolders
		return forceAttach (option, TRUE, AttachHow_over_or_replace);
	}
	else if (command=="attachallthis" || command == "addoutfitallthis") { // attachallthis=force to attach the folder containing this object and its subfolders
		BOOL res = TRUE;
		std::string pathes_str = getFullPath (getItem(object_uuid), option);
		if (pathes_str != "") {
			std::deque<std::string> pathes = parse (pathes_str, ",");
			for (unsigned int i = 0; i < pathes.size(); ++i) {
				res &= forceAttach (pathes.at(i), TRUE, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
			}
		}
		return res;
	}
	else if (command=="attachallthisover" || command == "addoutfitallthisover") { // attachallthisover=force to attach the folder containing this object and its subfolders
		BOOL res = TRUE;
		std::string pathes_str = getFullPath (getItem(object_uuid), option);
		if (pathes_str != "") {
			std::deque<std::string> pathes = parse (pathes_str, ",");
			for (unsigned int i = 0; i < pathes.size(); ++i) {
				res &= forceAttach (pathes.at(i), TRUE, AttachHow_over);
			}
		}
		return res;
	}
	else if (command=="attachallthisoverorreplace" || command == "addoutfitallthisoverorreplace") { // attachallthisoverorreplace=force to attach the folder containing this object and its subfolders
		BOOL res = TRUE;
		std::string pathes_str = getFullPath (getItem(object_uuid), option);
		if (pathes_str != "") {
			std::deque<std::string> pathes = parse (pathes_str, ",");
			for (unsigned int i = 0; i < pathes.size(); ++i) {
				res &= forceAttach (pathes.at(i), TRUE, AttachHow_over_or_replace);
			}
		}
		return res;
	}
	else if (command.find ("setenv_") == 0) {
		BOOL res = TRUE;
		BOOL allowed = TRUE;
		if (!sRRNoSetEnv) {
			if (!isAllowed (object_uuid, "setenv")) {
				allowed=FALSE;
				remove (object_uuid, "setenv", "");
			}
			if (!mContainsSetenv) res = forceEnvironment (command, option);
			if (!allowed) add (object_uuid, "setenv", "");
		}
		return res;
	}
	else if (command.find ("setdebug_") == 0) {
		BOOL res = TRUE;
		BOOL allowed = TRUE;
		if (!isAllowed (object_uuid, "setdebug")) {
			allowed=FALSE;
			remove (object_uuid, "setdebug", "");
		}
		if (!contains("setdebug")) res = forceDebugSetting (command, option);
		if (!allowed) add (object_uuid, "setdebug", "");
		return res;
	}
	else if (command=="setrot") { // setrot:angle_radians=force
		BOOL res = TRUE;
		LLVOAvatar* avatar = gAgentAvatarp;
		if (!avatar) return FALSE;
		F32 val = atof (option.c_str());
		gAgentCamera.startCameraAnimation();
		LLVector3 rot (0.0, 1.0, 0.0);
		rot = rot.rotVec(-val, LLVector3::z_axis);
		rot.normalize();
		gAgent.resetAxes(rot);
		return res;
	}
    else if (command == "adjustheight") { // adjustheight:adjustment_centimeters=force or adjustheight:ref_pelvis_to_foot;scalar[;delta]=force
//        if (!gSavedPerAccountSettings.controlExists("RestrainedLoveOffsetAvatarZ")) {
        if (!gSavedPerAccountSettings.controlExists("AvatarHoverOffsetZ")) {
            return FALSE;
        }
        LLVOAvatar* avatar = gAgentAvatarp;
        if (avatar) {
            F32 val = (F32)atoi(option.c_str()) / 100.0f;
            size_t i = option.find(";");
            if (i != std::string::npos && i + 1 < option.length()) {
                F32 scalar = (F32)atof(option.substr(i + 1).c_str());
                if (scalar != 0.0f) {
                    if (sRestrainedLoveDebug) {
                        LL_INFOS() << "Pelvis to foot = " << avatar->getPelvisToFoot() << "m" << LL_ENDL;
                    }
                    val = (atof(option.c_str()) - avatar->getPelvisToFoot()) * scalar;
                    option = option.substr(i + 1);
                    i = option.find(";");
                    if (i != std::string::npos && i + 1 < option.length()) {
                        val += (F32)atof(option.substr(i + 1).c_str());
                    }
                }
            }
            if (val > 2.0f) {
                val = 2.0f;
            }
			else if (val < -2.0f) {
                val = -2.0f;
            }
//            gSavedPerAccountSettings.setF32("RestrainedLoveOffsetAvatarZ", val);
            gSavedPerAccountSettings.setF32("AvatarHoverOffsetZ", val);
        }
	}
	else if (command == "setgroup") {
		std::string target_group_name = option;
		LLStringUtil::toLower(target_group_name);
		if (target_group_name != "none") { // "none" is not localized here because a script should not have to bother about viewer language
			S32 nb = gAgent.mGroups.size();
			for (S32 i=0; i<nb; ++i) {
				LLGroupData group = gAgent.mGroups.at(i);
				std::string this_group_name = group.mName;
				LLStringUtil::toLower(this_group_name);
				if (this_group_name == target_group_name) {
					LLGroupActions::activate(group.mID);
					break;
				}
			}
		}
		else {
			LLGroupActions::activate(LLUUID::null);
		}
		// not found => do nothing
	}
	else if (command == "setcam_fov") {
		F32 new_fov_rad = atof(option.c_str());
		LLViewerCamera::getInstance()->setDefaultFOV(new_fov_rad);
		gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
	}
	return TRUE;
}

void RRInterface::removeItemFromAvatar(LLViewerInventoryItem* item)
{
	if (!item) return;
	std::string name = item->getName();
	LLStringUtil::toLower(name);
	if (!canUnwear (item)) return;
	LLAppearanceMgr::instance().removeItemFromAvatar(item->getUUID());
	//LLWearableBridge::removeItemFromAvatar (item);
}

BOOL RRInterface::answerOnChat (std::string channel, std::string msg)
{
	S32 chan = (S32)atoi(channel.c_str());
	if (chan == 0) {
		// protection against abusive "@getstatus=0" commands, or against a non-numerical channel
		return FALSE;
	}
//	if (msg.length() > (size_t)(chan > 0 ? 1023 : 255)) {
//		LL_WARNS() << "Too large an answer: maximum is " << (chan > 0 ? "1023 characters" : "255 characters for a negative channel") << ". Aborted command." << LL_ENDL;
//		return FALSE;
//	}
	//if (chan > 0) {
	//	std::ostringstream temp;
	//	temp << "/" << chan << " " << msg;
	//	LLFloaterIMNearbyChat::sendChatFromViewer(temp.str(), CHAT_TYPE_SHOUT, FALSE);
	//} else {
		gMessageSystem->newMessage("ScriptDialogReply");
		gMessageSystem->nextBlock("AgentData");
		gMessageSystem->addUUID("AgentID", gAgent.getID());
		gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
		gMessageSystem->nextBlock("Data");
		gMessageSystem->addUUID("ObjectID", gAgent.getID());
		gMessageSystem->addS32("ChatChannel", chan);
		gMessageSystem->addS32("ButtonIndex", 1);
		gMessageSystem->addString("ButtonLabel", msg);
		gAgent.sendReliableMessage();
	//}
	if (sRestrainedLoveDebug) {
		LL_INFOS() << "/" << chan << " " << msg << LL_ENDL;
	}
	return TRUE;
}

std::string RRInterface::crunchEmote (std::string msg, unsigned int truncateTo) {
	// Don't treat text before "/me" if there is any, we want to crunch only the emote part of the message (everything after "/me").
	int	ind = msg.find("/me");
	std::string prefix = "";
	if (ind > 0) {
		prefix = msg.substr(0, ind);
		msg = msg.substr(ind);
	}

	std::string crunched = msg;

	if (msg.find ("/me ") == 0
	|| msg.find ("/me'") == 0
	|| (msg.find (":") == 0 && msg.length() >= 3 && isalpha(msg[1]))
	|| msg.find (":'") == 0
	) {
		// Only allow emotes without "spoken" text.
		// Forbid text containing any symbol which could be used as quotes.
		if (msg.find ("\"") != -1 || msg.find ("''") != -1
		    || msg.find ("(")  != -1 || msg.find (")")  != -1
		    || msg.find (" -") != -1 || msg.find ("- ") != -1
		    || msg.find ("*")  != -1 || msg.find ("=")  != -1
		    || msg.find ("^")  != -1 || msg.find ("_")  != -1
		    || msg.find ("~")  != -1)
		{
			crunched = "...";
		}
		else if (truncateTo > 0 && !contains ("emote")) {
			// Only allow short emotes.
			int i = msg.find (".");
			if (i != -1) {
				crunched = msg.substr (0, ++i);
			}
			if (crunched.length () > truncateTo) {
				crunched = crunched.substr (0, truncateTo);
			}
		}
	}
	else if (msg.find ("/") == 0) {
		// only allow short gesture names (to avoid cheats).
		if (msg.length () > 7) { // allows things like "/ao off", "/hug X"
			crunched = "...";
		}
	}
	else if (msg.find ("((") != 0 || msg.find ("))") != msg.length () - 2 || !sCanOoc) {
		// Only allow OOC chat, starting with "((" and ending with "))".
		crunched = "...";
	}
	return crunched;
	//return prefix + crunched;
}

std::string RRInterface::getOutfitLayerAsString (LLWearableType::EType layer)
{
	switch (layer) {
		case LLWearableType::WT_SKIN: return WS_SKIN;
		case LLWearableType::WT_GLOVES: return WS_GLOVES;
		case LLWearableType::WT_JACKET: return WS_JACKET;
		case LLWearableType::WT_PANTS: return WS_PANTS;
		case LLWearableType::WT_SHIRT: return WS_SHIRT;
		case LLWearableType::WT_SHOES: return WS_SHOES;
		case LLWearableType::WT_SKIRT: return WS_SKIRT;
		case LLWearableType::WT_SOCKS: return WS_SOCKS;
		case LLWearableType::WT_UNDERPANTS: return WS_UNDERPANTS;
		case LLWearableType::WT_UNDERSHIRT: return WS_UNDERSHIRT;
#if ALPHA_AND_TATTOO
		case LLWearableType::WT_ALPHA: return WS_ALPHA;
		case LLWearableType::WT_TATTOO: return WS_TATTOO;
		case LLWearableType::WT_PHYSICS: return WS_PHYSICS;
#endif
		case LLWearableType::WT_EYES: return WS_EYES;
		case LLWearableType::WT_HAIR: return WS_HAIR;
		case LLWearableType::WT_SHAPE: return WS_SHAPE;
		default: return "";
	}
}

LLWearableType::EType RRInterface::getOutfitLayerAsType (std::string layer)
{
	if (layer==WS_SKIN) return LLWearableType::WT_SKIN;
	if (layer==WS_GLOVES) return LLWearableType::WT_GLOVES;
	if (layer==WS_JACKET) return LLWearableType::WT_JACKET;
	if (layer==WS_PANTS) return LLWearableType::WT_PANTS;
	if (layer==WS_SHIRT) return LLWearableType::WT_SHIRT;
	if (layer==WS_SHOES) return LLWearableType::WT_SHOES;
	if (layer==WS_SKIRT) return LLWearableType::WT_SKIRT;
	if (layer==WS_SOCKS) return LLWearableType::WT_SOCKS;
	if (layer==WS_UNDERPANTS) return LLWearableType::WT_UNDERPANTS;
	if (layer==WS_UNDERSHIRT) return LLWearableType::WT_UNDERSHIRT;
#if ALPHA_AND_TATTOO
	if (layer==WS_ALPHA) return LLWearableType::WT_ALPHA;
	if (layer==WS_TATTOO) return LLWearableType::WT_TATTOO;
	if (layer==WS_PHYSICS) return LLWearableType::WT_PHYSICS;
#endif
	if (layer==WS_EYES) return LLWearableType::WT_EYES;
	if (layer==WS_HAIR) return LLWearableType::WT_HAIR;
	if (layer==WS_SHAPE) return LLWearableType::WT_SHAPE;
	return LLWearableType::WT_INVALID;
}

std::string RRInterface::getOutfit (std::string layer)
{
	if (layer==WS_SKIN) return (gAgentWearables.getWearable (LLWearableType::WT_SKIN, 0) != NULL? "1" : "0");
	if (layer==WS_GLOVES) return (gAgentWearables.getWearable (LLWearableType::WT_GLOVES, 0) != NULL? "1" : "0");
	if (layer==WS_JACKET) return (gAgentWearables.getWearable (LLWearableType::WT_JACKET, 0) != NULL? "1" : "0");
	if (layer==WS_PANTS) return (gAgentWearables.getWearable (LLWearableType::WT_PANTS, 0) != NULL? "1" : "0");
	if (layer==WS_SHIRT)return (gAgentWearables.getWearable (LLWearableType::WT_SHIRT, 0) != NULL? "1" : "0");
	if (layer==WS_SHOES) return (gAgentWearables.getWearable (LLWearableType::WT_SHOES, 0) != NULL? "1" : "0");
	if (layer==WS_SKIRT) return (gAgentWearables.getWearable (LLWearableType::WT_SKIRT, 0) != NULL? "1" : "0");
	if (layer==WS_SOCKS) return (gAgentWearables.getWearable (LLWearableType::WT_SOCKS, 0) != NULL? "1" : "0");
	if (layer==WS_UNDERPANTS) return (gAgentWearables.getWearable (LLWearableType::WT_UNDERPANTS, 0) != NULL? "1" : "0");
	if (layer==WS_UNDERSHIRT) return (gAgentWearables.getWearable (LLWearableType::WT_UNDERSHIRT, 0) != NULL? "1" : "0");
#if ALPHA_AND_TATTOO
//	if (layer==WS_ALPHA) return (gAgent.getWearable (LLWearableType::WT_ALPHA, 0) != NULL? "1" : "0");
//	if (layer==WS_TATTOO) return (gAgent.getWearable (LLWearableType::WT_TATTOO, 0) != NULL? "1" : "0");
	if (layer==WS_ALPHA) return (gAgentWearables.getWearable (LLWearableType::WT_ALPHA, 0) != NULL? "1" : "0");
	if (layer==WS_TATTOO) return (gAgentWearables.getWearable (LLWearableType::WT_TATTOO, 0) != NULL? "1" : "0");
	if (layer==WS_PHYSICS) return (gAgentWearables.getWearable (LLWearableType::WT_PHYSICS, 0) != NULL? "1" : "0");
#endif
	if (layer==WS_EYES) return (gAgentWearables.getWearable (LLWearableType::WT_EYES, 0) != NULL? "1" : "0");
	if (layer==WS_HAIR) return (gAgentWearables.getWearable (LLWearableType::WT_HAIR, 0) != NULL? "1" : "0");
	if (layer==WS_SHAPE) return (gAgentWearables.getWearable (LLWearableType::WT_SHAPE, 0) != NULL? "1" : "0");
	return getOutfit (WS_GLOVES)+getOutfit (WS_JACKET)+getOutfit (WS_PANTS)
			+getOutfit (WS_SHIRT)+getOutfit (WS_SHOES)+getOutfit (WS_SKIRT)
			+getOutfit (WS_SOCKS)+getOutfit (WS_UNDERPANTS)+getOutfit (WS_UNDERSHIRT)
			+getOutfit (WS_SKIN)+getOutfit (WS_EYES)+getOutfit (WS_HAIR)+getOutfit (WS_SHAPE)
#if ALPHA_AND_TATTOO
			+getOutfit (WS_ALPHA)+getOutfit (WS_TATTOO)+getOutfit (WS_PHYSICS)
#endif
			;
}

std::string RRInterface::getAttachments (std::string attachpt)
{
	std::string res="";
	std::string name;
	LLVOAvatar* avatar = gAgentAvatarp;
	if (!avatar) {
		LL_WARNS() << "NULL avatar pointer. Aborting." << LL_ENDL;
		return res;
	}
	if (attachpt=="") res+="0"; // to match the LSL macros
	for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
		iter != avatar->mAttachmentPoints.end(); iter++)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter;
		LLViewerJointAttachment* attachment = curiter->second;
		name=attachment->getName ();
		LLStringUtil::toLower(name);
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "trying <" << name << ">" << LL_ENDL;
		}
		if (attachpt=="" || attachpt==name) {
			if (attachment->getNumObjects() > 0) res+="1"; //attachment->getName ();
			else res+="0";
		}
	}
	return res;
}

std::string RRInterface::getStatus (LLUUID object_uuid, std::string rule)
{
	std::string res="";
	std::string name;
	std::string separator = "/";
	// If rule contains a specification of the separator, extract it
	int ind = rule.find (";");
	if (ind != -1) {
		separator = rule.substr (ind+1);
		rule = rule.substr (0, ind);
	}
	if (separator == "") {
		separator = "/"; // Prevent a hack to force the avatar to say something
	}
	
	RRMAP::iterator it;
	if (object_uuid.isNull()) {
		it = mSpecialObjectBehaviours.begin();
	}
	else {
		it = mSpecialObjectBehaviours.find (object_uuid.asString());
	}
	bool is_first=true;
	while (it != mSpecialObjectBehaviours.end() &&
			(object_uuid.isNull() || it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	)
	{
		if (rule=="" || it->second.find (rule)!=-1) {
			//if (!is_first) 
			res+=separator;
			res+=it->second;
			is_first=false;
		}
		it++;
	}
	return res;
}

BOOL RRInterface::forceDetach (std::string attachpt)
{
	std::string name;
	BOOL res=FALSE;
	LLVOAvatar* avatar = gAgentAvatarp;
	if (!avatar) return res;
	for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
		 iter != avatar->mAttachmentPoints.end(); iter++)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter;
		LLViewerJointAttachment* attachment = curiter->second;
		name=attachment->getName ();
		LLStringUtil::toLower(name);
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "trying <" << name << ">" << LL_ENDL;
		}
		if (attachpt=="" || attachpt==name) {
			if (sRestrainedLoveDebug) {
				LL_INFOS() << "found => detaching" << LL_ENDL;
			}
			detachAllObjectsFromAttachment (attachment);
			res=TRUE;
		}
	}
	return res;
}


BOOL RRInterface::forceDetachByUuid (std::string object_uuid)
{
	BOOL res=FALSE;
	LLVOAvatar* avatar = gAgentAvatarp;
	if (!avatar) return res;
	LLViewerObject* object = gObjectList.findObject(LLUUID (object_uuid));
	if (object) {
		object = object->getRootEdit();
		for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
			 iter != avatar->mAttachmentPoints.end(); iter++)
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter;
			LLViewerJointAttachment* attachment = curiter->second;
			if (attachment && attachment->isObjectAttached (object)) {
				detachObject (object);
				res=TRUE;
			}
		}
	}
	return res;
}

BOOL RRInterface::hasLockedHuds ()
{
	LLVOAvatar* avatar = gAgentAvatarp;
	if (!avatar) return FALSE;
	for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
		 iter != avatar->mAttachmentPoints.end(); iter++)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter;
		LLViewerJointAttachment* attachment = curiter->second;
		LLViewerObject* obj;
		if (attachment) {
			for (unsigned int i = 0; i < attachment->mAttachedObjects.size(); ++i) {
				obj = attachment->mAttachedObjects.at(i);
				if (obj && obj->isHUDAttachment() && !canDetach(obj)) return TRUE;
			}
		}
	}
	return FALSE;
}


std::deque<LLInventoryItem*> RRInterface::getListOfLockedItems (LLInventoryCategory* root)
{
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	std::deque<LLInventoryItem*> res;
	std::deque<LLInventoryItem*> tmp;
	res.clear();
	
	if (root && avatar) {
		
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		S32 count;
		S32 count_tmp;
		S32 i;
		S32 j;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;
		//		LLViewerObject* attached_object = NULL;
		std::string attach_point_name = "";
		
		// Try to find locked items in the current category
		count = items->size();
		for (i = 0; i < count; ++i) {
			item = items->at(i);
			// If this is an object, add it if it is worn and locked, or worn and its attach point is locked
			if (item && item->getType() == LLAssetType::AT_OBJECT) {
				LLViewerObject* attached_object = avatar->getWornAttachment (item->getLinkedUUID());
				if (attached_object) {
					avatar->getAttachedPointName (item->getLinkedUUID(), attach_point_name);
					if (!gAgent.mRRInterface.canDetach(attached_object)) {
						if (sRestrainedLoveDebug) {
							LL_INFOS() << "found a locked object : " << item->getName() << " on " << attach_point_name << LL_ENDL;
						}
						res.push_back (item);
					}
				}
			}
			// If this is a piece of clothing, add it if the avatar can't unwear clothes, or if this layer itself can't be unworn
			else if (item && item->getType() == LLAssetType::AT_CLOTHING) {
				if (gAgent.mRRInterface.contains ("remoutfit")
					|| gAgent.mRRInterface.containsSubstr ("remoutfit:")
					) {
					if (sRestrainedLoveDebug) {
						LL_INFOS() << "found a locked clothing : " << item->getName() << LL_ENDL;
					}
					res.push_back (item);
				}
			}
		}
		
		// We have all the locked objects contained directly in this folder, now add all the ones contained in children folders recursively
		count = cats->size();
		for (i = 0; i < count; ++i) {
			cat = cats->at(i);
			tmp = getListOfLockedItems (cat);
			count_tmp = tmp.size();
			for (j = 0; j < count_tmp; ++j) {
				item = tmp[j];
				if (item) res.push_back (item);
			}
		}
		
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "number of locked objects under " << root->getName() << " =  " << res.size() << LL_ENDL;
		}
	}
	
	return res;
}

std::deque<std::string> RRInterface::getListOfRestrictions (LLUUID object_uuid, std::string rule /*= ""*/)
{
	std::deque<std::string> res;
	std::string name;
	RRMAP::iterator it;
	if (object_uuid.isNull()) {
		it = mSpecialObjectBehaviours.begin();
	}
	else {
		it = mSpecialObjectBehaviours.find (object_uuid.asString());
	}
	while (it != mSpecialObjectBehaviours.end() &&
			(object_uuid.isNull() || it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	)
	{
		if (rule=="" || it->second.find (rule)!=-1) {
			res.push_back(it->second);
		}
		it++;
	}
	return res;
}


std::string RRInterface::getInventoryList (std::string path, BOOL withWornInfo /* = FALSE */)
{
	std::string res = "";
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	LLInventoryCategory* root = NULL;
	if (path == "") root = getRlvShare();
	else root = getCategoryUnderRlvShare (path);
	
	if (root) {
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		if(cats) {
			S32 count = cats->size();
			bool found_one = false;
			if (withWornInfo) {
				std::string worn_items = getWornItems (root);
				res += "|";
				found_one = true;
				if (worn_items == "n") {
					res += "10";
				}
				else if (worn_items == "N") {
					res += "30";
				}
				else {
					res += worn_items;
				}
			}
			for(S32 i = 0; i < count; ++i) {
				LLInventoryCategory* cat = cats->at(i);
				std::string name = cat->getName();
				if (name != "" && name[0] !=  '.') { // hidden folders => invisible to the list
					if (found_one) res += ",";
					res += name.c_str();
					if (withWornInfo) {
						std::string worn_items = getWornItems (cat);
						res += "|";
						found_one = true;
						if (worn_items == "n") {
							res += "10";
						}
						else if (worn_items == "N") {
							res += "30";
						}
						else {
							res += worn_items;
						}
					}
					found_one = true;
				}
			}
		}
	}

	return res;
}

std::string RRInterface::getWornItems (LLInventoryCategory* cat)
{
	// Returns a string of 2 digits according to the proportion of worn items in this folder and its children :
	// First digit is this folder, second digit is children folders
	// 0 : No item contained in the folder
	// 1 : Some items contained but none is worn
	// 2 : Some items contained and some of them are worn
	// 3 : Some items contained and all of them are worn
	std::string res_as_string = "0";
	int res			= 0;
	int subRes		= 0;
	int prevSubRes	= 0;
	int nbItems		= 0;
	int nbWorn		= 0;
	BOOL isNoMod	= FALSE;
	BOOL isRoot		= (getRlvShare() == cat);
	
	// if cat exists, scan all the items inside it
	if (cat) {
	
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
		if (!isRoot && items) { // do not scan the shared root
		
			// scan them one by one
			S32 count = items->size();
			for(S32 i = 0; i < count; ++i) {
			
				LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->at(i);

				if (item) {
					LLVOAvatarSelf* avatar = gAgentAvatarp;
					if (item->getType() == LLAssetType::AT_OBJECT
					 || item->getType() == LLAssetType::AT_CLOTHING
					 || item->getType() == LLAssetType::AT_BODYPART
					) {
						nbItems++;
					}
					if( avatar && avatar->isWearingAttachment( item->getLinkedUUID() ) 
						|| gAgentWearables.isWearingItem (item->getLinkedUUID())) nbWorn++;

					// special case : this item is no-mod, hence we need to check its parent folder
					// is correctly named, and that the item is alone in its folder.
					// If so, then the calling method will have to deal with a special character instead
					// of a number
					if (count == 1
					 && item->getType() == LLAssetType::AT_OBJECT
					 && !item->getPermissions().allowModifyBy(gAgent.getID())) {
						if (findAttachmentPointFromName (cat->getName()) != NULL) {
							isNoMod = TRUE;
						}
					}
				}
			}
		}
		
		// scan every subfolder of the folder we are scanning, recursively
		// note : in the case of no-mod items we shouldn't have sub-folders, so no need to check
		if (cats && !isNoMod) {
		
			S32 count = cats->size();
			for(S32 i = 0; i < count; ++i) {

				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->at(i);

				if (cat_child) {
					std::string tmp = getWornItems (cat_child);
					// translate the result for no-mod items into something the upper levels can understand
					if (tmp == "N") {
						if (!isRoot) {
							nbWorn++;
							nbItems++;
							subRes = 3;
						}
					}
					else if (tmp== "n") {
						if (!isRoot) {
							nbItems++;
							subRes = 1;
						}
					}
					else if (cat_child->getName() != "" && cat_child->getName()[0] != '.') { // we don't want to include invisible folders, except the ones containing a no-mod item
						// This is an actual sub-folder with several items and sub-folders inside,
						// so retain its score to integrate it into the current one
						// As it is a sub-folder, to integrate it we need to reduce its score first (consider "0" as "ignore")
						// "00" = 0, "01" = 1, "10" = 1, "30" = 3, "03" = 3, "33" = 3, all the rest gives 2 (some worn, some not worn)
						if      (tmp == "00")                               subRes = 0;
						else if (tmp == "11" || tmp == "01" || tmp == "10") subRes = 1;
						else if (tmp == "33" || tmp == "03" || tmp == "30") subRes = 3;
						else subRes = 2;

						// Then we must combine with the previous sibling sub-folders
						// Same rule as above, set to 2 in all cases except when prevSubRes == subRes or when either == 0 (nothing present, ignore)
						if      (prevSubRes == 0 && subRes == 0) subRes = 0;
						else if (prevSubRes == 0 && subRes == 1) subRes = 1;
						else if (prevSubRes == 1 && subRes == 0) subRes = 1;
						else if (prevSubRes == 1 && subRes == 1) subRes = 1;
						else if (prevSubRes == 0 && subRes == 3) subRes = 3;
						else if (prevSubRes == 3 && subRes == 0) subRes = 3;
						else if (prevSubRes == 3 && subRes == 3) subRes = 3;
						else subRes = 2;
						prevSubRes = subRes;
					}
				}
			}
		}
	}

	if (isNoMod) {
		// the folder contains one no-mod object and is named from an attachment point
		// => return a special character that will be handled by the calling method
		if (nbWorn > 0) return "N";
		else return "n";
	}
	else {
		if (isRoot || nbItems == 0) res = 0; // forcibly hide all items contained directly under #RLV
		else if (nbWorn >= nbItems) res = 3;
		else if (nbWorn > 0) res = 2;
		else res = 1;
	}
	std::stringstream str;
	str << res;
	str << subRes;
	res_as_string = str.str();
	return res_as_string;
}

LLInventoryCategory* RRInterface::getRlvShare ()
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf (
					gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY), cats, items
	);

	if(cats) {
		S32 count = cats->size();
		for(S32 i = 0; i < count; ++i) {
			LLInventoryCategory* cat = cats->at(i);
			std::string name = cat->getName();
			if (name == RR_SHARED_FOLDER) {
//				if (sRestrainedLoveDebug) {
//					LL_INFOS() << "found " << name << LL_ENDL;
//				}
				return cat;
			}
		}
	}
	return NULL;
}

BOOL RRInterface::isUnderRlvShare (LLInventoryItem* item)
{
	const LLUUID& cat_id = item->getParentUUID();
	return isUnderFolder(getRlvShare(), gInventory.getCategory(cat_id));
/*
	if (item == NULL) return FALSE;
	LLInventoryCategory* res = NULL;
	LLInventoryCategory* rlv = getRlvShare();
	if (rlv == NULL) return FALSE;
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

	const LLUUID& cat_id = item->getParentUUID();
	res = gInventory.getCategory (cat_id);
	
	while (res && res->getUUID() != root_id) {
		if (res == rlv) return TRUE;
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory (parent_id);
	}
	return FALSE;
*/
}

BOOL RRInterface::isUnderRlvShare (LLInventoryCategory* cat)
{
	return isUnderFolder (getRlvShare(), cat);
/*
	if (cat == NULL) return FALSE;
	LLInventoryCategory* res = NULL;
	LLInventoryCategory* rlv = getRlvShare();
	if (rlv == NULL) return FALSE;
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
	
	const LLUUID& cat_id = cat->getParentUUID();
	res = gInventory.getCategory (cat_id);
	
	while (res && res->getUUID() != root_id) {
		if (res == rlv) return TRUE;
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory (parent_id);
	}
	return FALSE;
*/
}

BOOL RRInterface::isUnderFolder (LLInventoryCategory* cat_parent, LLInventoryCategory* cat_child)
{
	if (cat_parent == NULL || cat_child == NULL) {
		return FALSE;
	}
	if (cat_child == cat_parent) {
		return TRUE;
	}
	LLInventoryCategory* res = NULL;
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
	
	const LLUUID& cat_id = cat_child->getParentUUID();
	res = gInventory.getCategory (cat_id);
	
	while (res && res->getUUID() != root_id) {
		if (res == cat_parent) {
			return TRUE;
		}
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory (parent_id);
	}
	return FALSE;
}

/*
void RRInterface::renameAttachment (LLInventoryItem* item, LLViewerJointAttachment* attachment)
{
  // DEPRECATED : done directly in the viewer code
	// if item is worn and shared, check its name
	// if it doesn't contain the name of attachment, append it
	// (but truncate the name first if it's too long)
	if (!item || !attachment) return;
	LLVOAvatar* avatar = gAgentAvatarp;
	
	if( avatar && avatar->isWearingAttachment( item->getUUID() ) ) {
		if (isUnderRlvShare (item)) {
			LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
			if (attachpt == NULL) {
				
			}
		}
	}
}
*/
LLInventoryCategory* RRInterface::getCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	if (catName == "") return root;
	LLStringUtil::toLower (catName);
	std::deque<std::string> tokens = parse (catName, "/");

	// Preliminary action : remove everything after pipes ("|"), including pipes themselves
	// This way we can feed the result of a @getinvworn command directly into this method
	// without having to clean what is after the pipes
	int nb = tokens.size();
	for (int i=0; i<nb; ++i) {
		std::string tok = tokens[i];
		int ind = tok.find ("|");
		if (ind != -1) {
			tok = tok.substr (0, ind);
			tokens[i] = tok;
		}
	}
	
	if (root) {

		bool exact_match = false;
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		
		if(cats) {
			S32 count = cats->size();
			LLInventoryCategory* cat = NULL;
			
			// we need to scan first and retain the best match
			int max_size_index = -1;
			int max_size = 0;
			
			for(S32 i = 0; i < count; ++i) {
				cat = cats->at(i);
				std::string name = cat->getName();
				if (name != "" && name[0] !=  '.') { // ignore invisible folders
					LLStringUtil::toLower (name);
					
					int size = match (tokens, name, exact_match);
					if (size > max_size || (exact_match && size == max_size)) {
						max_size = size;
						max_size_index = i;
					}
				}
			}

			// only now we can grab the best match and either continue deeper or return it
			if (max_size > 0) {
				cat = cats->at(max_size_index);
				if (max_size == tokens.size()) return cat;
				else return getCategoryUnderRlvShare (
									dumpList2String (
										getSubList (tokens, max_size)
									, "/")
								, cat);
			}
		}
	}

	if (sRestrainedLoveDebug) {
		LL_INFOS() << "category not found" << LL_ENDL;
	}
	return NULL;
}

LLInventoryCategory* RRInterface::findCategoryUnderRlvShare (std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	LLStringUtil::toLower (catName);
	std::deque<std::string> tokens = parse (catName, "&&");
	
	if (root) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		
		if(cats)
		{
			S32 count = cats->size();
			LLInventoryCategory* cat = NULL;
			
			for(S32 i = 0; i < count; ++i)
			{
				cat = cats->at(i);
				
				std::string name = cat->getName();
				LLStringUtil::toLower(name);
				// We can't find invisible folders ('.') and dropped folders ('~')
				if (name != "" && name[0] != '.' && name[0] != '~') {
					// search deeper first
					LLInventoryCategory* found = findCategoryUnderRlvShare(catName, cat);
					if (found != NULL) return found;
				}
			}
		}
		// return this category if it matches
		std::string name = root->getName();
		LLStringUtil::toLower (name);
		// We can't find invisible folders ('.') and dropped folders ('~')
		if (name != "" && name[0] != '.' && name[0] != '~' && findMultiple (tokens, name)) return root;
	}
	// didn't find anything
	return NULL;
}

std::deque<LLInventoryCategory*> RRInterface::findCategoriesUnderRlvShare(std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	LLStringUtil::toLower(catName);
	std::deque<std::string> tokens = parse(catName, "&&");
	std::deque<LLInventoryCategory*> res;

	if (root) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);

		if (cats)
		{
			S32 count = cats->size();
			LLInventoryCategory* cat = NULL;

			for (S32 i = 0; i < count; ++i)
			{
				cat = cats->at(i);

				std::string name = cat->getName();
				LLStringUtil::toLower(name);
				// We can't find invisible folders ('.') and dropped folders ('~')
				if (name != "" && name[0] != '.' && name[0] != '~') {
					// search deeper first
					std::deque<LLInventoryCategory*> found = findCategoriesUnderRlvShare(catName, cat);
					for (int i = 0; i < found.size(); i++) {
						res.push_back(found.at(i));
					}
				}

			}
		}

		// add this category if it matches
		std::string name = root->getName();
		LLStringUtil::toLower(name);
		// We can't find invisible folders ('.') and dropped folders ('~')
		if (name != "" && name[0] != '.' && name[0] != '~' && findMultiple(tokens, name)) res.push_back(root);
	}

	return res;
}

std::string RRInterface::findAttachmentNameFromPoint(LLViewerJointAttachment* attachpt)
{
	// return the lowercased name of the attachment, or empty if null
	if (attachpt == NULL) return "";
	std::string res = attachpt->getName();
	LLStringUtil::toLower(res);
	return res;
}

// This struct is meant to be used in RRInterface::findAttachmentPointFromName below
typedef struct
{
	int length;
	int index;
	LLViewerJointAttachment* attachment;
} Candidate;

LLViewerJointAttachment* RRInterface::findAttachmentPointFromName (std::string objectName, BOOL exactName)
{
	// for each possible attachment point, check whether its name appears in the name of
	// the item.
	// We are going to scan the whole list of attachments, but we won't decide which one to take right away.
	// Instead, for each matching point, we will store in lists the following results :
	// - length of its name
	// - right-most index where it is found in the name
	// - a pointer to that attachment point
	// When we have that list, choose the highest index, and in case of ex-aequo choose the longest length
	LLVOAvatar* avatar = gAgentAvatarp;
	if (!avatar) {
		LL_WARNS() << "NULL avatar pointer. Aborting." << LL_ENDL;
		return NULL;
	}
	LLStringUtil::toLower(objectName);
	// HACK : we replace "(avatar center)" by "(root)", to make those two equivalent
	objectName = stringReplace (objectName, "(avatar center)", "(root)");
	std::string attachName;
	int ind = -1;
	bool found_one = false;
	std::vector<Candidate> candidates;
	
	for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
		 iter != avatar->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment) {
			attachName = attachment->getName();
			LLStringUtil::toLower(attachName);
//			if (sRestrainedLoveDebug) {
//				LL_INFOS() << "trying attachment " << attachName << LL_ENDL;
//			}
			if (exactName && objectName == attachName) return attachment;
			else if (!exactName && (ind = objectName.rfind ("("+attachName+")")) != -1)
			{
				Candidate new_candidate;
				new_candidate.index = ind+1;
				new_candidate.length = attachName.length();
				new_candidate.attachment = attachment;
				candidates.push_back (new_candidate);
				found_one = true;
				if (sRestrainedLoveDebug) {
					LL_INFOS() << "new candidate '" << attachName << "' : index=" << new_candidate.index << "   length=" << new_candidate.length << LL_ENDL;
				}
			}
		}
	}
	if (!found_one) {
		if (sRestrainedLoveDebug) {
			LL_INFOS() << "no attachment found" << LL_ENDL;
		}
		return NULL;
	}
	// Now that we have at least one candidate, we have to decide which one to return
	LLViewerJointAttachment* res = NULL;
	Candidate candidate;
	unsigned int i;
	int ind_res = -1;
	int max_index = -1;
	int max_length = -1;
	// Find the highest index
	for (i=0; i<candidates.size(); ++i) {
		candidate = candidates[i];
		if (candidate.index > max_index) max_index = candidate.index;
	}
	// Find the longest match among the ones found at that index
	for (i=0; i<candidates.size(); ++i) {
		candidate = candidates[i];
		if (candidate.index == max_index) {
			if (candidate.length > max_length) {
				max_length = candidate.length;
				ind_res = i;
			}
		}
	}
	// Return this attachment point
	if (ind_res > -1) {
		candidate = candidates[ind_res];
		res = candidate.attachment;
		if (sRestrainedLoveDebug && res) {
			LL_INFOS() << "returning '" << res->getName() << "'" << LL_ENDL;
		}
	}
	return res;
}

LLViewerJointAttachment* RRInterface::findAttachmentPointFromParentName (LLInventoryItem* item)
{
	if (item) {
		// => look in parent folder (this could be a no-mod item), use its name to find the target
		// attach point
		LLViewerInventoryCategory* cat;
		const LLUUID& parent_id = item->getParentUUID();
		cat = gInventory.getCategory (parent_id);
		return findAttachmentPointFromName (cat->getName());
	}
	return NULL;
}

S32 RRInterface::findAttachmentPointNumber (LLViewerJointAttachment* attachment)
{
	LLVOAvatar* avatar = gAgentAvatarp;
	if (avatar) {
		for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin();
			 iter != avatar->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment) {
				return iter->first;
			}
		}
	}
	return -1;
}
/*
// This function is added here to keep retrocompatibility with 1.x code, since it has now been turned into an event,
// which cannot be called (see LLAttachmentDetachFromPoint::handleEvent() for an example)
bool RRInterface::handle_detach_from_avatar(LLViewerJointAttachment* attachment)
{
	if (attachment->getNumObjects() > 0)
	{
		if (gRRenabled && !gAgent.mRRInterface.canDetach(attachment->getObject()))
		{
			return true;
		}
		gMessageSystem->newMessage("ObjectDetach");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		
		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator iter = attachment->mAttachedObjects.begin();
			 iter != attachment->mAttachedObjects.end();
			 iter++)
		{
			LLViewerObject *attached_object = (*iter);
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, attached_object->getLocalID());
		}
		gMessageSystem->sendReliable( gAgent.getRegionHost() );
	}
	return true;
}
*/
void RRInterface::detachObject(LLViewerObject* object)
{
	// Handle the detach message to the sim here, after a check
	if (!object) return;
	if (gRRenabled && !gAgent.mRRInterface.canDetach(object)) return;

	LLInventoryItem* item = getItem (object->getID());
	if (item) {
		LLAppearanceMgr::getInstance()->removeItemFromAvatar (item->getUUID());
	}
	//gMessageSystem->newMessage("ObjectDetach");
	//gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	//gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	//gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	//gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	//gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
	//gMessageSystem->sendReliable( gAgent.getRegionHost() );
}

void RRInterface::detachAllObjectsFromAttachment(LLViewerJointAttachment* attachment)
{
	if (!attachment) return;
	LLViewerObject* object;

	// We need to remove all the objects from attachment->mAttachedObjects, one by one.
	// To do this, and in order to avoid any race condition, we are going to copy the list and 
	// iterate on the copy instead of the original which changes everytime something is
	// attached and detached, asynchronously.
	LLViewerJointAttachment::attachedobjs_vec_t attachedObjectsCopy = attachment ->mAttachedObjects;

	for (unsigned int i = 0; i < attachedObjectsCopy.size(); ++i) {
		object = attachedObjectsCopy.at(i);
		detachObject (object);
	}
}

bool RRInterface::canDetachAllObjectsFromAttachment(LLViewerJointAttachment* attachment)
{
	if (!attachment) return false;
	LLViewerObject* object;

	for (unsigned int i = 0; i <  attachment ->mAttachedObjects.size(); ++i) {
		object =  attachment ->mAttachedObjects.at(i);
		if (!gAgent.mRRInterface.canDetach(object)) return false;
	}

	return true;
}

void RRInterface::fetchInventory (LLInventoryCategory* root)
{
	// do this only once on login

	if (mInventoryFetched) return;
	
	bool last_step = false;
	
	if (root == NULL) {
		root = getRlvShare();
		last_step = true;
	}
	
	if (root) {
		LLViewerInventoryCategory* viewer_root = (LLViewerInventoryCategory*) root;
		viewer_root->fetch ();

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		// retrieve all the shared folders
		gInventory.getDirectDescendentsOf (viewer_root->getUUID(), cats, items);
		if (cats) {
			S32 count = cats->size();
			for(S32 i = 0; i < count; ++i) {
				LLInventoryCategory* cat = (LLInventoryCategory*)cats->at(i);
				fetchInventory (cat);
			}
		}

	}
	
	if (last_step) mInventoryFetched = TRUE;
}

BOOL RRInterface::forceAttach (std::string category, BOOL recursive, AttachHow how)
{
	// recursive is TRUE in the case of an attachall command
	// find the category under RLV shared folder
	if (category == "") return TRUE; // just a safety
	LLInventoryCategory* cat = getCategoryUnderRlvShare (category);
	BOOL isRoot = (getRlvShare() == cat);
	BOOL replacing = (how == AttachHow_replace || how == AttachHow_over_or_replace); // we're replacing for now, but the name of the category could decide otherwise
	
	// if exists, wear all the items inside it
	if (cat) {
	
		// If the name of the category begins with a special string (specified by the user, "+" by default), then we force to stack instead of replacing
		if (how == AttachHow_over_or_replace) {
			if (cat->getName().find (gSavedSettings.getString ("RestrainedLoveStackWhenFolderBeginsWith")) == 0) {
				replacing = FALSE;
			}
		}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
		if (items) {
		
			// wear them one by one
			S32 count = items->size();
			for(S32 i = 0; i < count; ++i) {
				if (!isRoot) {
					LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->at(i);
					if (sRestrainedLoveDebug) {
						LL_INFOS() << "trying to attach " << item->getName() << LL_ENDL;
					}
					
					// this is an object to attach somewhere
					if (item && item->getType() == LLAssetType::AT_OBJECT) {
						LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
						
						if (attachpt) {
							if (sRestrainedLoveDebug) {
								LL_INFOS() << "attaching item to " << attachpt->getName() << LL_ENDL;
							}
							if (replacing) {
								// we're replacing => mimick rez_attachment without displaying an Xml alert to confirm
								S32 number = findAttachmentPointNumber (attachpt);
								if (canDetach(attachpt->getName()) && canAttach(item))
								{
									LLSD payload;
									payload["item_id"] = item->getLinkedUUID();
									payload["attachment_point"] = number;
									LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
								}
							}
							else {
								// we're stacking => call rez_attachment directly
								rez_attachment (item, attachpt, false);
							}
						}
						else {
							// attachment point is not in the name => stack
							rez_attachment (item, attachpt, false);
						}
					}
					// this is a piece of clothing
					else if (item->getType() == LLAssetType::AT_CLOTHING
							 || item->getType() == LLAssetType::AT_BODYPART) {
//						LLAppearanceMgr::instance().addCOFItemLink(item);
						LLAppearanceMgr::instance().wearItemOnAvatar(item->getLinkedUUID(), true, replacing);
					}
					// this is a gesture
					else if (item->getType() == LLAssetType::AT_GESTURE) {
						if (!LLGestureMgr::instance().isGestureActive(item->getLinkedUUID())) {
							LLGestureMgr::instance().activateGesture(item->getLinkedUUID());
						}
					}
				}
			}
		}
		
		// scan every subfolder of the folder we are attaching, in order to attach no-mod items
		if (cats) {
		
			// for each subfolder, attach the first item it contains according to its name
			S32 count = cats->size();
			for(S32 i = 0; i < count; ++i) {
				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->at(i);
				LLViewerJointAttachment* attachpt = findAttachmentPointFromName (cat_child->getName());
				
				if (!isRoot) {
					// this subfolder is properly named => attach the first item it contains
					LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
					LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
					gInventory.getDirectDescendentsOf (cat_child->getUUID(), 
														cats_grandchildren, items_grandchildren);

					if (items_grandchildren && items_grandchildren->size() == 1) {
						LLViewerInventoryItem* item_grandchild = 
								(LLViewerInventoryItem*)items_grandchildren->at(0);

						if (attachpt) {
							if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
								&& !item_grandchild->getPermissions().allowModifyBy(gAgent.getID())
								&& findAttachmentPointFromParentName (item_grandchild) != NULL) { // it is no-mod and its parent is named correctly
								// we use the attach point from the name of the folder, not the no-mod item
								if (replacing) {
									// we're replacing => mimick rez_attachment without displaying an Xml alert to confirm
									S32 number = findAttachmentPointNumber (attachpt);
									if (canDetach(attachpt->getName()) && canAttach(item_grandchild))
									{
										LLSD payload;
										payload["item_id"] = item_grandchild->getUUID();
										payload["attachment_point"] = number;
										LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
									}
								}
								else {
									// we're stacking => call rez_attachment directly
									rez_attachment (item_grandchild, attachpt, false);
								}
							}
						}
						else {
							// attachment point is not in the name => stack
//							rez_attachment (item_grandchild, attachpt, false);
						}
					}
				}

				if (recursive && cat_child->getName().find (".") != 0) { // attachall and not invisible)
					forceAttach (getFullPath (cat_child), recursive, how);
				}
			}
		}
	}
	return TRUE;
}

BOOL RRInterface::forceDetachByName (std::string category, BOOL recursive)
{
//LLWearableBridge::removeItemFromAvatar(item);		
	// find the category under RLV shared folder
	if (category == "") return TRUE; // just a safety
	LLInventoryCategory* cat = getCategoryUnderRlvShare (category);
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (!avatar) return FALSE;
	BOOL isRoot = (getRlvShare() == cat);
	
	// if exists, detach/unwear all the items inside it
	if (cat) {
	
		if (mHandleNoStrip) {
			std::string name = cat->getName();
			LLStringUtil::toLower(name);
			if (name.find ("nostrip") != -1) return false;
		}

		if (recursive) {
			LLUUID cat_id = cat->getLinkedUUID();

			// The following code is almost a carbon copy of LLAppearanceMgr::takeOffOutfit()
			// (which is called when pressing "Remove from Current Outfit" in the inventory).
			// We don't want it recursive all the time though, it depends on the parameter above.
			LLInventoryModel::cat_array_t cats;
			LLInventoryModel::item_array_t items;
			LLFindWearablesEx collector(/*is_worn=*/ true, /*include_body_parts=*/ false);

			gInventory.collectDescendentsRecIf(cat_id, cats, items, recursive, FALSE, collector);

			LLInventoryModel::item_array_t::const_iterator it = items.begin();
			const LLInventoryModel::item_array_t::const_iterator it_end = items.end();
			uuid_vec_t uuids_to_remove;
			for( ; it_end != it; ++it)
			{
				LLViewerInventoryItem* item = *it;
				uuids_to_remove.push_back(item->getUUID());
			}
			LLAppearanceMgr::instance().removeItemsFromAvatar(uuids_to_remove);

			// deactivate all gestures in the outfit folder
			LLInventoryModel::item_array_t gest_items;
			LLAppearanceMgr::instance().getDescendentsOfAssetType(cat_id, gest_items, LLAssetType::AT_GESTURE);
			for(S32 i = 0; i  < gest_items.size(); ++i)
			{
				LLViewerInventoryItem *gest_item = gest_items[i];
				if ( LLGestureMgr::instance().isGestureActive( gest_item->getLinkedUUID()) )
				{
					LLGestureMgr::instance().deactivateGesture( gest_item->getLinkedUUID() );
				}
			}

		}
		else {
			LLInventoryModel::cat_array_t* cats;
			LLInventoryModel::item_array_t* items;
		
			// retrieve all the objects contained in this folder
			gInventory.getDirectDescendentsOf (cat->getUUID(), cats, items);
			if (items) {
		
				// unwear them one by one
				S32 count = items->size();
				for(S32 i = 0; i < count; ++i) {
					if (!isRoot) {
						LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->at(i);
						if (sRestrainedLoveDebug) {
							LL_INFOS() << "trying to detach " << item->getName() << LL_ENDL;
						}
					
						// this is an attached object
						if (item->getType() == LLAssetType::AT_OBJECT) {
							// find the attachpoint from which to detach
							for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
								 iter != avatar->mAttachmentPoints.end(); )
							{
								LLVOAvatar::attachment_map_t::iterator curiter = iter++;
								LLViewerJointAttachment* attachment = curiter->second;
								LLViewerObject* object = avatar->getWornAttachment(item->getLinkedUUID());
								if (attachment->isObjectAttached(object)) {
									detachObject (object);
									break;
								}
							}
						
						}
						// this is a piece of clothing
						else if (item->getType() == LLAssetType::AT_CLOTHING) {
	//						const LLWearable* layer = gAgentWearables.getWearableFromItemID (item->getLinkedUUID());
	//						if (layer != NULL) gAgentWearables.removeWearable (layer->getType(), false, 0);
							if (canDetach (item)) removeItemFromAvatar(item);		
						}
						// this is a gesture
						else if (item->getType() == LLAssetType::AT_GESTURE) {
							if (LLGestureMgr::instance().isGestureActive(item->getLinkedUUID())) {
								LLGestureMgr::instance().deactivateGesture(item->getLinkedUUID());
							}
						}
					}
				}
			}

			if (cats) {
				// for each subfolder, detach the first item it contains (only for single no-mod items contained in appropriately named folders)
				S32 count = cats->size();
				for(S32 i = 0; i < count; ++i) {
					LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->at(i);

					if (mHandleNoStrip) {
						std::string name = cat_child->getName();
						LLStringUtil::toLower(name);
						if (name.find ("nostrip") != -1) continue;
					}

					LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
					LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
					gInventory.getDirectDescendentsOf (cat_child->getUUID(), 
														cats_grandchildren, items_grandchildren);

					if (!isRoot && items_grandchildren && items_grandchildren->size() == 1) { // only one item
						LLViewerInventoryItem* item_grandchild = 
								(LLViewerInventoryItem*)items_grandchildren->at(0);

						if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
							&& !item_grandchild->getPermissions().allowModifyBy(gAgent.getID())
							&& findAttachmentPointFromParentName (item_grandchild) != NULL) { // and it is no-mod and its parent is named correctly
							// detach this object
							// find the attachpoint from which to detach
							for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
								 iter != avatar->mAttachmentPoints.end(); )
							{
								LLVOAvatar::attachment_map_t::iterator curiter = iter++;
								LLViewerJointAttachment* attachment = curiter->second;
								LLViewerObject* object = avatar->getWornAttachment(item_grandchild->getUUID());
								if (attachment->isObjectAttached(object)) {
									detachObject (object);
									break;
								}
							}
						}
					}

					if (recursive && cat_child->getName().find (".") != 0) { // detachall and not invisible)
						forceDetachByName (getFullPath (cat_child), recursive);
					}
				}
			}
		}
	}
	return TRUE;
}

BOOL RRInterface::forceTeleport(std::string location, const LLVector3& vecLookAt)
{
	// location must be X/Y/Z where X, Y and Z are ABSOLUTE coordinates => use a script in-world to translate from local to global
	// OR it can be Region/X/Y/Z where X, Y and Z are LOCAL coordinates => no need to use a script in-world
	std::string loc (location);
	std::string region_name = "";
	S64 x = 128;
	S64 y = 128;
	S64 z = 0;
	std::deque<std::string> tokens=parse (location, "/");
	if (tokens.size()==3) {
		x=atoi (tokens.at(0).c_str());
		y=atoi (tokens.at(1).c_str());
		z=atoi (tokens.at(2).c_str());
	}
	else if (tokens.size() == 4) {
		region_name = tokens.at(0);
		x = atoi(tokens.at(1).c_str());
		y = atoi(tokens.at(2).c_str());
		z = atoi(tokens.at(3).c_str());
	}
	else {
		return FALSE;
	}

	if (sRestrainedLoveDebug) {
		LL_INFOS() << tokens.at(0) << "," << tokens.at(1) << "," << tokens.at(2) << "     " << x << "," << y << "," << z << LL_ENDL;
	}

	mAllowCancelTp = FALSE; // will be checked once receiving the tp order from the sim, then set to TRUE again

	if (region_name == "")
	{
		LLVector3d pos_global;
		pos_global.mdV[VX] = (F64)x;
		pos_global.mdV[VY] = (F64)y;
		pos_global.mdV[VZ] = (F64)z;

		gAgent.teleportViaLocation(pos_global);
	}
	else {
		LLVector3 pos_local;
		pos_local.mV[VX] = (F32)x;
		pos_local.mV[VY] = (F32)y;
		pos_local.mV[VZ] = (F32)z;

		LLWorldMapMessage::url_callback_t cb = boost::bind(&RRInterface::forceTeleportCallback, _1, pos_local, vecLookAt);
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name, cb, std::string(""), true);
	}

	return TRUE;
}

// From KB
void RRInterface::forceTeleportCallback(U64 hRegion, const LLVector3& posRegion, const LLVector3& vecLookAt)
{
	if (hRegion)
	{
		const LLVector3d posGlobal = from_region_handle(hRegion) + (LLVector3d)posRegion;
		if (vecLookAt.isExactlyZero()) {
			gAgent.teleportViaLocation(posGlobal);
		}
		else {
			gAgent.teleportViaLocationLookAt(posGlobal);
		}
	}
}
// from KB

std::string RRInterface::stringReplace(std::string s, std::string what, std::string by, BOOL caseSensitive /* = FALSE */)
{
//	LL_INFOS() << "trying to replace <" << what << "> in <" << s << "> by <" << by << ">" << LL_ENDL;
	if (what == "" || what == " ") return s; // avoid an infinite loop
	int ind;
	int old_ind = 0;
	int len_what = what.length();
	int len_by = by.length();
	if (len_by == 0) len_by = 1; // avoid an infinite loop
	
	while ((ind = s.find ("%20")) != -1) // unescape
	{
		s = s.replace (ind, 3, " ");
	}
	
	std::string lower = s;
	if (!caseSensitive) {
		LLStringUtil::toLower (lower);
		LLStringUtil::toLower (what);
	}
	
	while ((ind = lower.find (what, old_ind)) != -1)
	{
//		LL_INFOS() << "ind=" << ind << "    old_ind=" << old_ind << LL_ENDL;
		s = s.replace (ind, len_what, by);
		old_ind = ind + len_by;
		lower = s;
		if (!caseSensitive) LLStringUtil::toLower (lower);
	}
	return s;
	
}

// same as stringReplace, but checks for neighbors of the occurrences of "what", and replace only if these neighbors are NOT alphanum characters
std::string RRInterface::stringReplaceWholeWord(std::string s, std::string what, std::string by, BOOL caseSensitive /* = FALSE */)
{
	//	LL_INFOS() << "trying to replace <" << what << "> in <" << s << "> by <" << by << ">" << LL_ENDL;
	if (what == "" || what == " ") return s; // avoid an infinite loop
	int ind;
	int old_ind = 0;
	int len_what = what.length();
	int len_by = by.length();
	int len_s = s.length();
	if (len_by == 0) len_by = 1; // avoid an infinite loop

	while ((ind = s.find("%20")) != -1) // unescape
	{
		s = s.replace(ind, 3, " ");
	}

	std::string lower = s;
	if (!caseSensitive) {
		LLStringUtil::toLower(lower);
		LLStringUtil::toLower(what);
	}

	while ((ind = lower.find(what, old_ind)) != -1)
	{
		//		LL_INFOS() << "ind=" << ind << "    old_ind=" << old_ind << LL_ENDL;
		char prec = ' ';
		char succ = ' ';
		if (ind > 0) prec = s.at(ind - 1);
		if (ind < len_s - len_what - 1) succ = s.at(ind + len_what);
		if (!isalnum (prec) && prec != '\'' && !isalnum (succ) && succ != '\'')
		{
			s = s.replace(ind, len_what, by);
			len_s = s.length();
			lower = s;
			if (!caseSensitive) LLStringUtil::toLower(lower);
		}
		old_ind = ind + len_by;
	}
	return s;

}

std::string RRInterface::getDummyName(std::string name, EChatAudible audible /* = CHAT_AUDIBLE_FULLY */)
{
	int len = name.length();
	if (len == 0)
	{
		return "";
	}
	int at = 3;
	if (len <= 3) at = len-1; // just to avoid crashing in some cases
	// We use mLaunchTimestamp in order to modify the scrambling when the session restarts (it stays consistent during the session though)
	// But in crashy situations, let's not make it change at EVERY session, more like once a day or so
	// A day is 86400 seconds, the closest power of two is 65536, that's a 16-bit shift
	unsigned char hash = name.at(at) + len + (mLaunchTimestamp >> 16); // very lame hash function I know... but it should be linear enough (the old length method was way too gaussian with a peak at 11 to 16 characters)
	unsigned char mod = hash % 28;
	std::string res = "";
	switch (mod) {
		case 0:		res = "A resident";			break;
		case 1:		res = "This resident";		break;
		case 2:		res = "That resident";		break;
		case 3:		res = "An individual";		break;
		case 4:		res = "This individual";	break;
		case 5:		res = "That individual";	break;
		case 6:		res = "A person";			break;
		case 7:		res = "This person";		break;
		case 8:		res = "That person";		break;
		case 9:		res = "A stranger";			break;
		case 10:	res = "This stranger";		break;
		case 11:	res = "That stranger";		break;
		case 12:	res = "A human being";		break;
		case 13:	res = "This human being";	break;
		case 14:	res = "That human being";	break;
		case 15:	res = "An agent";			break;
		case 16:	res = "This agent";			break;
		case 17:	res = "That agent";			break;
		case 18:	res = "A soul";				break;
		case 19:	res = "This soul";			break;
		case 20:	res = "That soul";			break;
		case 21:	res = "Somebody";			break;
		case 22:	res = "Anonymous one";		break;
		case 23:	res = "Someone";			break;
		case 24:	res = "Mysterious one";		break;
		case 25:	res = "An unknown being";	break;
		case 26:	res = "Unidentified one";	break;
		default:	res = "An unknown person";	break;
	}
	if (audible == CHAT_AUDIBLE_BARELY) res += " afar";
	return res;
}

std::string RRInterface::getCensoredMessage (std::string str)
{
	// HACK: if the message is under the form secondlife:///app/agent/UUID/about, clear it
	// (we could just as well clear just the first part, or return a bogus message, 
	// the purpose here is to avoid showing the profile of an avatar and displaying their name on the chat)
	if (str.find("secondlife:///app/agent/") == 0)
	{
		return "";
	}

	// First we need to build a list of all the exceptions to @shownames and @shownametags
	// Each avatar object which UUID is contained in this list should not be censored
	std::string command;
	std::string behav;
	std::string option;
	std::string param;
	std::deque<LLUUID> exceptions;
	RRMAP::iterator it = mSpecialObjectBehaviours.begin();
	while (it != mSpecialObjectBehaviours.end()) {
		command = it->second;
		LLStringUtil::toLower(command);
		if (command.find("shownames:") == 0 || command.find("shownametags:") == 0) {
			if (parseCommand(command, behav, option, param)) {
				LLUUID uuid;
				uuid.set(option, FALSE);
				exceptions.push_back(uuid);
			}
		}
		it++;
	}

	// Hide every occurrence of the name of anybody around (found in cache, so not completely accurate nor completely immediate)
	S32 i;
	for (i=0; i<gObjectList.getNumObjects(); ++i) {
		LLViewerObject* object = gObjectList.getObject(i);
		if (object) {
			LLAvatarName av_name;
			std::string clean_user_name;
			std::string user_name;
			std::string display_name;
			std::string dummy_name;
			
			if (object->isAvatar()) {
				if (std::find(exceptions.begin(), exceptions.end(), object->getID()) == exceptions.end()) // ignore exceptions
				{
					if (LLAvatarNameCache::get(object->getID(), &av_name))
					{
						user_name = av_name.getUserName();
						clean_user_name = LLCacheName::cleanFullName(user_name);
						display_name = av_name.mDisplayName; // not "getDisplayName()" because we need this whether we use display names or user names

						dummy_name = getDummyName(clean_user_name);
						str = stringReplaceWholeWord(str, clean_user_name, dummy_name);

						dummy_name = getDummyName(user_name);
						str = stringReplaceWholeWord(str, user_name, dummy_name);

						dummy_name = getDummyName(display_name);
						str = stringReplaceWholeWord(str, display_name, dummy_name);
					}
				}
			}
		}
	}
	return str;
}

void updateAndSave (WLColorControl* color)
{
	if (color == NULL) return;
	color->i = color->r;
	if (color->g > color->i) {
		color->i = color->g;
	}
	if (color->b > color->i) {
		color->i = color->b;
	}
	color->update (LLWLParamManager::getInstance()->mCurParams);
}

void updateAndSave (WLFloatControl* floatControl)
{
	if (floatControl == NULL) return;
	floatControl->update (LLWLParamManager::getInstance()->mCurParams);
}

BOOL RRInterface::forceEnvironment (std::string command, std::string option)
{
	// command is "setenv_<something>"
	double val = atof (option.c_str());

	int length = 7; // size of "setenv_"
	command = command.substr (length);
	LLWLParamManager* params = LLWLParamManager::getInstance();

	params->mAnimator.mIsRunning = false;
	//params->mAnimator.mUseLindenTime = false;
	params->mAnimator.setTimeType(LLWLAnimator::TIME_CUSTOM);

	if (command == "daytime") {
		if (val > 1.0) val = 1.0;
		if (val >= 0.0) {
			params->mAnimator.setDayTime(val);
			params->mAnimator.update(params->mCurParams);
		}
		else {
			LLWLParamManager::getInstance()->mAnimator.mIsRunning = true;
			//LLWLParamManager::getInstance()->mAnimator.mUseLindenTime = true;
			LLWLParamManager::getInstance()->mAnimator.setTimeType(LLWLAnimator::TIME_LINDEN);
			LLEnvManagerNew::instance().useRegionSettings();
		}
	}
	else if (command == "bluehorizonr") {
		params->mBlueHorizon.r = val*2;
		updateAndSave (&(params->mBlueHorizon));
	}
	else if (command == "bluehorizong") {
		params->mBlueHorizon.g = val*2;
		updateAndSave (&(params->mBlueHorizon));
	}
	else if (command == "bluehorizonb") {
		params->mBlueHorizon.b = val*2;
		updateAndSave (&(params->mBlueHorizon));
	}
	else if (command == "bluehorizoni") {
		F32 old_intensity = llmax(params->mBlueHorizon.r, params->mBlueHorizon.g, params->mBlueHorizon.b);
		if (val == 0 || old_intensity == 0) {
			params->mBlueHorizon.r = params->mBlueHorizon.g = params->mBlueHorizon.b = val * 2;
		}
		else {
			params->mBlueHorizon.r *= val * 2 / old_intensity;
			params->mBlueHorizon.g *= val * 2 / old_intensity;
			params->mBlueHorizon.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mBlueHorizon));
	}

	else if (command == "bluedensityr") {
		params->mBlueDensity.r = val*2;
		updateAndSave (&(params->mBlueDensity));
	}
	else if (command == "bluedensityg") {
		params->mBlueDensity.g = val*2;
		updateAndSave (&(params->mBlueDensity));
	}
	else if (command == "bluedensityb") {
		params->mBlueDensity.b = val*2;
		updateAndSave (&(params->mBlueDensity));
	}
	else if (command == "bluedensityi") {
		F32 old_intensity = llmax(params->mBlueDensity.r, params->mBlueDensity.g, params->mBlueDensity.b);
		if (val == 0 || old_intensity == 0) {
			params->mBlueDensity.r = params->mBlueDensity.g = params->mBlueDensity.b = val * 2;
		}
		else {
			params->mBlueDensity.r *= val * 2 / old_intensity;
			params->mBlueDensity.g *= val * 2 / old_intensity;
			params->mBlueDensity.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mBlueDensity));
	}

	else if (command == "hazehorizon") {
		params->mHazeHorizon.x = val*2;
		updateAndSave (&(params->mHazeHorizon));
	}
	else if (command == "hazedensity") {
		params->mHazeDensity.x = val*2;
		updateAndSave (&(params->mHazeDensity));
	}

	else if (command == "densitymultiplier") {
		params->mDensityMult.x = val / 1000;
		params->mDensityMult.mult = 1.0;
		updateAndSave (&(params->mDensityMult));
//		LLWaterParamManager* water_params = LLWaterParamManager::instance();
//		water_params->mFogDensity.mExp = 5.0;
//		water_params->mFogDensity.update (water_params->mCurParams);
	}
	else if (command == "distancemultiplier") {
		params->mDistanceMult.x = val;
		params->mDistanceMult.mult = 1.0;
		updateAndSave (&(params->mDistanceMult));
//		LLWaterParamManager* water_params = LLWaterParamManager::instance();
//		water_params->mUnderWaterFogMod.mX = 1.0;
//		water_params->mUnderWaterFogMod.update (water_params->mCurParams);
	}
	else if (command == "maxaltitude") {
		params->mMaxAlt.x = val;
		updateAndSave (&(params->mMaxAlt));
	}

	else if (command == "sunmooncolorr") {
		params->mSunlight.r = val*3;
		updateAndSave (&(params->mSunlight));
	}
	else if (command == "sunmooncolorg") {
		params->mSunlight.g = val*3;
		updateAndSave (&(params->mSunlight));
	}
	else if (command == "sunmooncolorb") {
		params->mSunlight.b = val*3;
		updateAndSave (&(params->mSunlight));
	}
	else if (command == "sunmooncolori") {
		F32 old_intensity = llmax(params->mSunlight.r, params->mSunlight.g, params->mSunlight.b);
		if (val == 0 || old_intensity == 0) {
			params->mSunlight.r = params->mSunlight.g = params->mSunlight.b = val * 2;
		}
		else {
			params->mSunlight.r *= val * 2 / old_intensity;
			params->mSunlight.g *= val * 2 / old_intensity;
			params->mSunlight.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mSunlight));
	}

	else if (command == "ambientr") {
		params->mAmbient.r = val*3;
		updateAndSave (&(params->mAmbient));
	}
	else if (command == "ambientg") {
		params->mAmbient.g = val*3;
		updateAndSave (&(params->mAmbient));
	}
	else if (command == "ambientb") {
		params->mAmbient.b = val*3;
		updateAndSave (&(params->mAmbient));
	}
	else if (command == "ambienti") {
		F32 old_intensity = llmax(params->mAmbient.r, params->mAmbient.g, params->mAmbient.b);
		if (val == 0 || old_intensity == 0) {
			params->mAmbient.r = params->mAmbient.g = params->mAmbient.b = val * 2;
		}
		else {
			params->mAmbient.r *= val * 2 / old_intensity;
			params->mAmbient.g *= val * 2 / old_intensity;
			params->mAmbient.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mAmbient));
	}
	else if (command == "sunglowfocus") {
		params->mGlow.b = -val*5;
		updateAndSave (&(params->mGlow));
	}
	else if (command == "sunglowsize") {
		params->mGlow.r = (2-val)*20;
		updateAndSave (&(params->mGlow));
	}
	else if (command == "scenegamma") {
		params->mWLGamma.x = val;
		updateAndSave (&(params->mWLGamma));
	}
	else if (command == "sunmoonposition") {
		params->mCurParams.setSunAngle (F_TWO_PI * val);
	}
	else if (command == "eastangle") {
		params->mCurParams.setEastAngle (F_TWO_PI * val);
	}
	else if (command == "starbrightness") {
		params->mCurParams.setStarBrightness (val);
	}

	else if (command == "cloudcolorr") {
		params->mCloudColor.r = val;
		updateAndSave (&(params->mCloudColor));
	}
	else if (command == "cloudcolorg") {
		params->mCloudColor.g = val;
		updateAndSave (&(params->mCloudColor));
	}
	else if (command == "cloudcolorb") {
		params->mCloudColor.b = val;
		updateAndSave (&(params->mCloudColor));
	}
	else if (command == "cloudcolori") {
		F32 old_intensity = llmax(params->mCloudColor.r, params->mCloudColor.g, params->mCloudColor.b);
		if (val == 0 || old_intensity == 0) {
			params->mCloudColor.r = params->mCloudColor.g = params->mCloudColor.b = val * 2;
		}
		else {
			params->mCloudColor.r *= val * 2 / old_intensity;
			params->mCloudColor.g *= val * 2 / old_intensity;
			params->mCloudColor.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mCloudColor));
	}

	else if (command == "cloudx") {
		params->mCloudMain.r = val;
		updateAndSave (&(params->mCloudMain));
	}
	else if (command == "cloudy") {
		params->mCloudMain.g = val;
		updateAndSave (&(params->mCloudMain));
	}
	else if (command == "cloudd") {
		params->mCloudMain.b = val;
		updateAndSave (&(params->mCloudMain));
	}

	else if (command == "clouddetailx") {
		params->mCloudDetail.r = val;
		updateAndSave (&(params->mCloudDetail));
	}
	else if (command == "clouddetaily") {
		params->mCloudDetail.g = val;
		updateAndSave (&(params->mCloudDetail));
	}
	else if (command == "clouddetaild") {
		params->mCloudDetail.b = val;
		updateAndSave (&(params->mCloudDetail));
	}

	else if (command == "cloudcoverage") {
		params->mCloudCoverage.x = val;
		updateAndSave (&(params->mCloudCoverage));
	}
	else if (command == "cloudscale") {
		params->mCloudScale.x = val;
		updateAndSave (&(params->mCloudScale));
	}

	else if (command == "cloudscrollx") {
		params->mCurParams.setCloudScrollX (val+10);
	}
	else if (command == "cloudscrolly") {
		params->mCurParams.setCloudScrollY (val+10);
	}
	// sunglowfocus 0-0.5, sunglowsize 0-2, scenegamma 0-10, starbrightness 0-2
	// cloudcolor rgb 0-1, cloudxydensity xyd 0-1, cloudcoverage 0-1, cloudscale 0-1, clouddetail xyd 0-1
	// cloudscrollx 0-1, cloudscrolly 0-1, drawclassicclouds 0/1

	else if (command == "preset") {
//		params->loadPreset (option);
		LLEnvManagerNew::getInstance()->useSkyPreset(option);
	}

	// send the current parameters to shaders
	LLWLParamManager::getInstance()->propagateParameters();

	return TRUE;
}

std::string RRInterface::getEnvironment (std::string command)
{
	F64 res = 0;
	int length = 7; // size of "getenv_"
	command = command.substr (length);
	LLWLParamManager* params = LLWLParamManager::getInstance();

	if (command == "daytime") {
		if (params->mAnimator.mIsRunning && params->mAnimator.getTimeType() == LLWLAnimator::TIME_LINDEN) res = -1;
		else res = params->mAnimator.getDayTime();
	}

	else if (command == "bluehorizonr") res = params->mBlueHorizon.r/2;
	else if (command == "bluehorizong") res = params->mBlueHorizon.g/2;
	else if (command == "bluehorizonb") res = params->mBlueHorizon.b/2;
	else if (command == "bluehorizoni") res = max (max (params->mBlueHorizon.r, params->mBlueHorizon.g), params->mBlueHorizon.b) / 2;

	else if (command == "bluedensityr") res = params->mBlueDensity.r/2;
	else if (command == "bluedensityg") res = params->mBlueDensity.g/2;
	else if (command == "bluedensityb") res = params->mBlueDensity.b/2;
	else if (command == "bluedensityi") res = max (max (params->mBlueDensity.r, params->mBlueDensity.g), params->mBlueDensity.b) / 2;

	else if (command == "hazehorizon")  res = params->mHazeHorizon.x;
	else if (command == "hazedensity")  res = params->mHazeDensity.x;

	else if (command == "densitymultiplier")  res = params->mDensityMult.x*1000;
	else if (command == "distancemultiplier") res = params->mDistanceMult.x;
	else if (command == "maxaltitude")        res = params->mMaxAlt.x;

	else if (command == "sunmooncolorr") res = params->mSunlight.r/3;
	else if (command == "sunmooncolorg") res = params->mSunlight.g/3;
	else if (command == "sunmooncolorb") res = params->mSunlight.b/3;
	else if (command == "sunmooncolori") res = max (max (params->mSunlight.r, params->mSunlight.g), params->mSunlight.b) / 3;

	else if (command == "ambientr") res = params->mAmbient.r/3;
	else if (command == "ambientg") res = params->mAmbient.g/3;
	else if (command == "ambientb") res = params->mAmbient.b/3;
	else if (command == "ambienti") res = max (max (params->mAmbient.r, params->mAmbient.g), params->mAmbient.b) / 3;

	else if (command == "sunglowfocus")	res = -params->mGlow.b/5;
	else if (command == "sunglowsize")		res = 2-params->mGlow.r/20;
	else if (command == "scenegamma")		res = params->mWLGamma.x;

	else if (command == "sunmoonposition")		res = params->mCurParams.getSunAngle()/F_TWO_PI;
	else if (command == "eastangle")			res = params->mCurParams.getEastAngle()/F_TWO_PI;
	else if (command == "starbrightness")		res = params->mCurParams.getStarBrightness();

	else if (command == "cloudcolorr") res = params->mCloudColor.r;
	else if (command == "cloudcolorg") res = params->mCloudColor.g;
	else if (command == "cloudcolorb") res = params->mCloudColor.b;
	else if (command == "cloudcolori") res = max (max (params->mCloudColor.r, params->mCloudColor.g), params->mCloudColor.b);

	else if (command == "cloudx")  res = params->mCloudMain.r;
	else if (command == "cloudy")  res = params->mCloudMain.g;
	else if (command == "cloudd")  res = params->mCloudMain.b;

	else if (command == "clouddetailx")  res = params->mCloudDetail.r;
	else if (command == "clouddetaily")  res = params->mCloudDetail.g;
	else if (command == "clouddetaild")  res = params->mCloudDetail.b;

	else if (command == "cloudcoverage")	res = params->mCloudCoverage.x;
	else if (command == "cloudscale")		res = params->mCloudScale.x;

	else if (command == "cloudscrollx") res = params->mCurParams.getCloudScrollX() - 10;
	else if (command == "cloudscrolly") res = params->mCurParams.getCloudScrollY() - 10;

	else if (command == "preset") return getLastLoadedPreset();

	std::stringstream str;
	str << res;
	return str.str();
}

BOOL RRInterface::forceDebugSetting (std::string command, std::string option)
{
	//	MK: As some debug settings are critical to the user's experience and others
	//	are just useless/not used, we are following a whitelist approach : only allow
	//	certain debug settings to be changed and not all.
	
	// command is "setdebug_<something>"
	
	int length = 9; // size of "setdebug_"
	command = command.substr (length);
	LLStringUtil::toLower(command);

	// Find the index of the command in the list of allowed commands, ignoring the case
	int ind = -1;
	std::string real_command = "";
	std::string tmp;
	int nb = mAllowedSetDebug.size();
	for (int i = 0; i < nb; i++)
	{
		tmp = mAllowedSetDebug.at(i);
		LLStringUtil::toLower(tmp);
		if (tmp == command)
		{
			real_command = mAllowedSetDebug.at(i);
			ind = i;
			break;
		}
	}

	if (ind != -1) {
		if (gSavedSettings.controlExists(real_command))
		{
			switch (gSavedSettings.getControl(real_command)->type())
			{
			case TYPE_U32:
				gSavedSettings.setU32(real_command, atoi(option.c_str()));
				break;

			case TYPE_S32:
				gSavedSettings.setS32(real_command, atoi(option.c_str()));
				break;

			case TYPE_F32:
				gSavedSettings.setF32(real_command, atoi(option.c_str()));
				break;

			case TYPE_BOOLEAN:
				gSavedSettings.setBOOL(real_command, atoi(option.c_str()));
				break;

			case TYPE_STRING:
				gSavedSettings.setString(real_command, option.c_str());
				break;

			case TYPE_VEC3:
				//gSavedSettings.setVector3(real_command, atoi(option.c_str()));
				break;

			case TYPE_VEC3D:
				//gSavedSettings.setVector3d(real_command, atoi(option.c_str()));
				break;

			case TYPE_RECT:
				//gSavedSettings.setRect(real_command, atoi(option.c_str()));
				break;

			case TYPE_COL4:
				//gSavedSettings.setColor4(real_command, atoi(option.c_str()));
				break;

			case TYPE_COL3:
				//gSavedSettings.setColor4(real_command, atoi(option.c_str()));
				break;

			default:
				break;
			}
		}
		return TRUE;
	}
	
	return FALSE;
}

std::string RRInterface::getDebugSetting (std::string command)
{
	std::stringstream res;
	int length = 9; // size of "setdebug_"
	command = command.substr(length);
	LLStringUtil::toLower(command);

	// Find the index of the command in the list of allowed commands, ignoring the case
	int ind = -1;
	std::string real_command = "";
	std::string tmp;
	int nb = mAllowedGetDebug.size();
	for (int i = 0; i < nb; i++)
	{
		tmp = mAllowedGetDebug.at(i);
		LLStringUtil::toLower(tmp);
		if (tmp == command)
		{
			real_command = mAllowedGetDebug.at(i);
			ind = i;
			break;
		}
	}

	if (ind != -1) {
		if (gSavedSettings.controlExists(real_command))
		{
			switch (gSavedSettings.getControl(real_command)->type())
			{
			case TYPE_U32:
				res << gSavedSettings.getU32(real_command);
				break;

			case TYPE_S32:
				res << gSavedSettings.getS32(real_command);
				break;

			case TYPE_F32:
				res << gSavedSettings.getF32(real_command);
				break;

			case TYPE_BOOLEAN:
				res << gSavedSettings.getBOOL(real_command);
				break;

			case TYPE_STRING:
				res << gSavedSettings.getString(real_command);
				break;

			case TYPE_VEC3:
				res << gSavedSettings.getVector3(real_command);
				break;

			case TYPE_VEC3D:
				res << gSavedSettings.getVector3d(real_command);
				break;

			case TYPE_RECT:
				res << gSavedSettings.getRect(real_command);
				break;

			case TYPE_COL4:
				res << gSavedSettings.getColor4(real_command);
				break;

			case TYPE_COL3:
				res << gSavedSettings.getColor3(real_command);
				break;

			default:
				break;
			}
		}
	}
	
	return res.str();
}

std::string RRInterface::getFullPath (LLInventoryCategory* cat)
{
	if (cat == NULL) return "";
	LLInventoryCategory* rlv = gAgent.mRRInterface.getRlvShare();
	if (rlv == NULL) return "";
	LLInventoryCategory* res = cat;
	std::deque<std::string> tokens;
	
	while (res && res != rlv) {
		tokens.push_front (res->getName());
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory (parent_id);
	}
	return dumpList2String (tokens, "/");
}

std::string RRInterface::getFullPath (LLInventoryItem* item, std::string option, bool full_list /*= true*/)
{
	if (sRestrainedLoveDebug) {
		LL_INFOS() << "getFullPath(" << (item? item->getName(): "NULL") << ", " << option << ", " << full_list << ")" << LL_ENDL;
	}
	// Returns the path from the shared root to this object, or to the object worn at the attach point or clothing layer pointed by option if any
	if (option != "") {
		item = NULL; // an option is specified => we don't want to check the item that issued the command, but something else that is currently worn (object or clothing)
		LLWearableType::EType wearable_type;

		if (LLUUID::validate(option)) { // if option is a UUID, get the path of the viewer object which has this UUID
			std::deque<std::string> res;
			LLUUID id;
			id.set(option, FALSE);
			item = getItem(id); // we want the viewer object from the UUID, no the inventory object
			if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
			else {
				// We have found the inventory item => add its path to the list
				// it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
				res.push_back(getFullPath(item, ""));
				if (sRestrainedLoveDebug) {
					LL_INFOS() << "res=" << dumpList2String(res, ", ") << LL_ENDL;
				}
			}
			return dumpList2String(res, ",");
		}
		else if ((wearable_type = gAgent.mRRInterface.getOutfitLayerAsType(option)) != LLWearableType::WT_INVALID) { // this is a clothing layer => replace item with the piece clothing
			std::deque<std::string> res;
			for (unsigned int i = 0; i < LLAgentWearables::MAX_CLOTHING_PER_TYPE; ++i) {
				LLUUID id = gAgentWearables.getWearableItemID (wearable_type, i);
				if (id.notNull()) {
					item = gInventory.getItem(id);
					if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
					else {
						// We have found the inventory item => add its path to the list
						// it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
						res.push_back (getFullPath (item, ""));
						if (sRestrainedLoveDebug) {
							LL_INFOS() << "res=" << dumpList2String(res, ", ") << LL_ENDL;
						}
						if (!full_list) break; // old behaviour : we only return the first folder, not a full list
					}
				}
			}
			return dumpList2String (res, ",");
		}
		else { // this is not a clothing layer => it has to be an attachment point
			// Since 2.1, we need to browse through the list of attached objects, and we'll return their respective folders in a list speratated by commas
			LLViewerJointAttachment* attach_point = gAgent.mRRInterface.findAttachmentPointFromName (option, TRUE);
			if (attach_point) {
				std::deque<std::string> res;
				for (unsigned int i = 0; i < attach_point->mAttachedObjects.size(); ++i) {
					LLViewerObject* attached_object = attach_point->mAttachedObjects.at(i);
					if (attached_object) {
						item = getItemAux (attached_object, gAgent.mRRInterface.getRlvShare());
						if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
						else {
							// We have found the inventory item => add its path to the list
							// it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
							res.push_back (getFullPath (item, ""));
							if (sRestrainedLoveDebug) {
								LL_INFOS() << "res=" << dumpList2String(res, ", ") << LL_ENDL;
							}
							if (!full_list) break; // old behaviour : we only return the first folder, not a full list
						}
					}
				}
				return dumpList2String (res, ",");
			}
		}
	}
	
	if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
	if (item == NULL) return "";
	LLUUID parent_id = item->getParentUUID();
	LLInventoryCategory* parent_cat = gInventory.getCategory (parent_id);
	
	if (item->getType() == LLAssetType::AT_OBJECT && !item->getPermissions().allowModifyBy(gAgent.getID())) {
		if (gAgent.mRRInterface.findAttachmentPointFromName(parent_cat->getName()) != NULL) {
			// this item is no-mod and its parent folder contains the name of an attach point
			// => probably we want the full path only to the containing folder of that folder
			parent_id = parent_cat->getParentUUID();
			parent_cat = gInventory.getCategory (parent_id);
			return getFullPath (parent_cat);
		}
	}
	
	return getFullPath (parent_cat);
}


LLInventoryItem* RRInterface::getItemAux (LLViewerObject* attached_object, LLInventoryCategory* root)
{
	// auxiliary function for getItem()
	if (!attached_object) return NULL;
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (root && avatar) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (root->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;
		
		// Try to find the item in the current category
		count = items->size();
		for(i = 0; i < count; ++i) {
			item = items->at(i);
			if (item 
				&& (item->getType() == LLAssetType::AT_OBJECT || item->getType() == LLAssetType::AT_CLOTHING)
				&& avatar->getWornAttachment (item->getLinkedUUID()) == attached_object
				) {
				// found the item in the current category
				return item;
			}
		}
		
		// We didn't find it here => browse the children categories
		count = cats->size();
		for(i = 0; i < count; ++i) {
			cat = cats->at(i);
			item = getItemAux (attached_object, cat);
			if (item != NULL) return item;
		}
	}
	// We didn't find it (this should not happen)
	return NULL;
}

LLInventoryItem* RRInterface::getItem (LLUUID wornObjectUuidInWorld)
{
	// return the inventory item corresponding to the viewer object which UUID is "wornObjectUuidInWorld", if any
	LLViewerObject* object = gObjectList.findObject (wornObjectUuidInWorld);
	if (object != NULL) {
		object = object->getRootEdit();
		if (object->isAttachment()) {
			return gInventory.getItem(object->getAttachmentItemID());
			//return getItemAux (object, gInventory.getCategory(gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY))); //gAgent.mRRInterface.getRlvShare());
		}
	}
	// This object is not worn => it has nothing to do with any inventory item
	return NULL;
}

void RRInterface::attachObjectByUUID (LLUUID assetUUID, int attachPtNumber)
{
	// caution : this method does NOT check that the target attach point is already used by a locked item
	LLSD payload;
	payload["item_id"] = assetUUID;
	payload["attachment_point"] = attachPtNumber;
	LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
}

bool RRInterface::canDetachAllSelectedObjects ()
{
	for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLViewerObject* object = (*curiter)->getObject();
		if (object && !canDetach(object))
		{
			return false;
		}
	}
	return true;
}

bool RRInterface::isSittingOnAnySelectedObject()
{
	if (gAgentAvatarp && !gAgentAvatarp->mIsSitting) {
		return false;
	}
	
	for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLViewerObject* object = (*curiter)->getObject();
		if (object && object->isSeat())
		{
			return true;
		}
	}
	return false;
}

bool RRInterface::canAttachCategory(LLInventoryCategory* folder, bool with_exceptions /*= true*/)
{
	// return false if :
	// - at least one object issued a @attachthis:folder restriction
	// - at least one item in this folder is to be worn on a @attachthis:attachpt restriction
	// - at least one piece of clothing in this folder is to be worn on a @attachthis:layer restriction
	// - any parent folder returns false with @attachallthis
	if (!folder) return true;
	if (IsInventoryFolderNew(folder)) return true;
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (!avatar) return true;
	LLInventoryCategory* rlvShare = getRlvShare();
	bool shared = isUnderRlvShare(folder);
	if (!rlvShare || !shared) {
		if (contains ("unsharedwear")) return false;
		else return true;
	}
	else {
		if (contains("sharedwear")) return false;
	}
	return canAttachCategoryAux(folder, false, false, with_exceptions);
}

bool RRInterface::canAttachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions /*= true*/)
{
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (!avatar) return true;
	FolderLock folder_lock = FolderLock_unlocked;
	if (folder) {
		// check @attachthis:folder in all restrictions
		RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
		//LLInventoryCategory* restricted_cat;
		std::string path_to_check;
		std::string restriction = "attachthis";
		if (in_parent) restriction = "attachallthis";
		folder_lock = isFolderLockedWithoutException(folder, "attach");
		if (folder_lock == FolderLock_locked_without_except) {
			return false;
		}

		if (!with_exceptions && folder_lock == FolderLock_locked_with_except) {
			return false;
		}

		//while (it != mSpecialObjectBehaviours.end()) {
		//	if (it->second.find (restriction+":") == 0) {
		//		path_to_check = it->second.substr (restriction.length()+1); // remove ":" as well
		//		restricted_cat = getCategoryUnderRlvShare(path_to_check);
		//		if (restricted_cat == folder) return false;
		//	}
		//	it++;
		//}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (folder->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;
		
		// Try to find the item in the current category
		count = items->size();
		for(i = 0; i < count; ++i) {
			item = items->at(i);
			if (item) {
				if (item->getType() == LLAssetType::AT_OBJECT) {
					LLViewerJointAttachment* attachpt = NULL;
					if (in_no_mod) {
						if (item->getPermissions().allowModifyBy(gAgent.getID()) || count > 1) return true;
						LLInventoryCategory* parent = gInventory.getCategory (folder->getParentUUID());
						attachpt = findAttachmentPointFromName(parent->getName());
					}
					else {
						attachpt = findAttachmentPointFromName(item->getName());
					}
					if (attachpt && contains (restriction+":"+attachpt->getName())) return false;
				}
				else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
					const LLWearable* wearable = gAgentWearables.getWearableFromItemID (item->getLinkedUUID());
					if (wearable) {
						if (contains(restriction+":"+getOutfitLayerAsString(wearable->getType()))) return false;
					}
				}
			}
		}

		// now check all no-mod items => look at the sub-categories and return false if any of them returns false on a call to canAttachCategoryAux()
		count = cats->size();
		for(i = 0; i < count; ++i) {
			cat = cats->at(i);
			if (cat) {
				std::string name = cat->getName();
				if (name != "" && name[0] ==  '.' && findAttachmentPointFromName(name) != NULL) {
					if (!canAttachCategoryAux(cat, false, true, with_exceptions)) return false;
				}
			}
		}
	}
	if (folder == getRlvShare()) return true;
	if (!in_no_mod && folder_lock == FolderLock_unlocked) {
		return canAttachCategoryAux(gInventory.getCategory (folder->getParentUUID()), true, false, with_exceptions); // check for @attachallthis in the parent
	}
	return true;
}

bool RRInterface::canDetachCategory(LLInventoryCategory* folder, bool with_exceptions)
{
	// return false if :
	// - at least one object contained in this folder issued a @detachthis restriction
	// - at least one object issued a @detachthis:folder restriction
	// - at least one worn attachment in this folder is worn on a @detachthis:attachpt restriction
	// - at least one worn piece of clothing in this folder is worn on a @detachthis:layer restriction
	// - any parent folder returns false with @detachallthis
	if (!folder) return true;
	std::string name = folder->getName();
	if (IsInventoryFolderNew(folder) && name.length() > 0 && name.at(0) != '~') return true; // if folder is new and it is not a temporary RLV folder
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (!avatar) return true;
	LLInventoryCategory* rlvShare = getRlvShare();
	bool shared = isUnderRlvShare(folder);
	if (!rlvShare || !shared) {
		if (contains("unsharedunwear")) {
			return false;
		}
		else {
			return true;
		}
	}
	else {
		if (contains("sharedunwear")) return false;
	}
	return canDetachCategoryAux(folder, false, false, with_exceptions);
}

bool RRInterface::canDetachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions)
{
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if (!avatar) return true;
	FolderLock folder_lock = FolderLock_unlocked;
	if (folder) {
		if (mHandleNoStrip) {
			std::string name = folder->getName();
			LLStringUtil::toLower(name);
			if (name.find ("nostrip") != -1 && (!mUserUpdateAttachmentsUpdatesAll || mUserUpdateAttachmentsCalledFromScript)) return false;
		}
		// check @detachthis:folder in all restrictions
		RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
		//LLInventoryCategory* restricted_cat;
		std::string path_to_check;
		std::string restriction = "detachthis";
		if (in_parent) restriction = "detachallthis";
		folder_lock = isFolderLockedWithoutException(folder, "detach");
		if (folder_lock == FolderLock_locked_without_except) {
			return false;
		}

		if (!with_exceptions && folder_lock == FolderLock_locked_with_except) {
			return false;
		}

		//while (it != mSpecialObjectBehaviours.end()) {
		//	if (it->second.find (restriction+":") == 0) {
		//		path_to_check = it->second.substr (restriction.length()+1); // remove ":" as well
		//		restricted_cat = getCategoryUnderRlvShare(path_to_check);
		//		if (restricted_cat == folder) return false;
		//	}
		//	it++;
		//}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf (folder->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;
		
		// Try to find the item in the current category
		count = items->size();
		for(i = 0; i < count; ++i) {
			item = items->at(i);
			if (item) {
				if (item->getType() == LLAssetType::AT_OBJECT) {
					if (in_no_mod) {
						if (item->getPermissions().allowModifyBy(gAgent.getID()) || count > 1) return true;
					}
					LLViewerObject* attached_object = avatar->getWornAttachment (item->getLinkedUUID());
					if (attached_object) {
						std::string attach_point_name;
						if (avatar->getAttachedPointName(item->getLinkedUUID(), attach_point_name))
						{
							if (!isAllowed(attached_object->getRootEdit()->getID(), restriction)) return false;
							if (!in_parent && !isAllowed(attached_object->getRootEdit()->getID(), "detachallthis")) return false; // special case for objects contained into this folder and that issued a @detachallthis command without any parameter without issuing a @detachthis command along with it
							if (contains (restriction+":"+attach_point_name)) return false;
						}
					}
				}
				else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
					const LLWearable* wearable = gAgentWearables.getWearableFromItemID (item->getLinkedUUID());
					if (wearable) {
						if (contains(restriction+":"+getOutfitLayerAsString(wearable->getType()))) return false;
					}
				}
			}
		}

		// now check all no-mod items => look at the sub-categories and return false if any of them returns false on a call to canDetachCategoryAux()
		count = cats->size();
		for(i = 0; i < count; ++i) {
			cat = cats->at(i);
			if (cat) {
				std::string name = cat->getName();
				if (name != "" && name[0] ==  '.' && findAttachmentPointFromName(name) != NULL) {
					if (!canDetachCategoryAux(cat, false, true, with_exceptions)) return false;
				}
			}
		}
	}
	if (folder == getRlvShare()) return true;
	if (!in_no_mod && folder_lock == FolderLock_unlocked) {
		return canDetachCategoryAux(gInventory.getCategory (folder->getParentUUID()), true, false, with_exceptions); // check for @detachallthis in the parent
	}
	return true;
}

bool RRInterface::canUnwear(LLInventoryItem* item)
{
	if (item) {
		LLInventoryCategory* parent = gInventory.getCategory (item->getParentUUID());
		if (item->getType() ==  LLAssetType::AT_OBJECT) {
			if (!canDetach (item)) return false;
		}
		else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
			const LLViewerInventoryItem *vitem = dynamic_cast<const LLViewerInventoryItem*>(item);
			if (vitem) {
				if (!canUnwear (vitem->getWearableType())) return false;
			}
			if (!canDetachCategory (parent, true)) return false;
		}
	}
	return true;
}

bool RRInterface::canUnwear(LLWearableType::EType type)
{
	if (contains ("remoutfit")) {
		return false;
	}
	else if (contains ("remoutfit:"+getOutfitLayerAsString (type))) {
		return false;
	}
	return true;
}

bool RRInterface::canWear(LLInventoryItem* item)
{
	// CA: Apply the same defensive measures here as canWear(LLWearableType)
	if (!gAgentAvatarp) {
		return true;
	}

	// If we are still a cloud, allow to wear whatever the restrictions (we are probably logging on)
	if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
	{
		return true;
	}
	if (item) {
		// If the item was just received, let the user wear it
		if (isInventoryItemNew(item)) {
			return true;
		}
		LLInventoryCategory* parent = gInventory.getCategory (item->getParentUUID());
		if (item->getType() ==  LLAssetType::AT_OBJECT) {
			LLViewerJointAttachment* attachpt = findAttachmentPointFromName(item->getName());
			if (attachpt) {
				if (!canAttach (NULL, attachpt->getName())) return false;
			}
			if (!canAttachCategory (parent)) return false;
		}
		else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
			const LLViewerInventoryItem *vitem = dynamic_cast<const LLViewerInventoryItem*>(item);
			if (vitem) {
				//LLWearableType::EType type = vitem->getWearableType();
				//if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
				//{
				//	if (type == LLWearableType::WT_SHAPE
				//		|| type == LLWearableType::WT_HAIR
				//		|| type == LLWearableType::WT_EYES
				//		|| type == LLWearableType::WT_SKIN
				//	) {
				//		return true;
				//	}
				//}
				if (!canWear (vitem->getWearableType())) return false;
			}
			if (!canAttachCategory (parent)) return false;
		}
	}
	return true;
}

bool RRInterface::canWear(LLWearableType::EType type, bool from_server /*= false*/)
{
	// CA: Defensive code
	if (!gAgentAvatarp) {
		return true;
	}

	// If we are still a cloud, allow to wear whatever the restrictions (we are probably logging on)
	if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
	{
		return true;
	}
	//// If we are still a cloud, we can always wear bodyparts because we are still logging on
	//if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
	//{
	//	if (type == LLWearableType::WT_SHAPE
	//		|| type == LLWearableType::WT_HAIR
	//		|| type == LLWearableType::WT_EYES
	//		|| type == LLWearableType::WT_SKIN
	//	) {
	//		return true;
	//	}
	//}
	// If from_server is true, return true because we are probably trying to wear our bodyparts while logging on
	else if (from_server) {
		return true;
	}
	else if (contains ("addoutfit")) {
		return false;
	}
	else if (contains ("addoutfit:"+getOutfitLayerAsString (type))) {
		return false;
	}
	return true;
}

bool RRInterface::canDetach(LLInventoryItem* item)
{
//	if (!scriptsEnabled() && !getScriptsEnabledOnce()) return false;
	if (item == NULL) {
		return true;
	}
	LLInventoryItem* linked_item = gInventory.getLinkedItem(item->getUUID());
	if (!linked_item) {
		return true;
	}
	LLVOAvatarSelf* avatarp = gAgentAvatarp;
	if (!avatarp) return true;
	
	if (mHandleNoStrip) {
		std::string name = item->getName();
		LLStringUtil::toLower(name);
		if (name.find ("nostrip") != -1) return false;
	}

	if (item->getType() == LLAssetType::AT_OBJECT) {
		// we'll check canDetachCategory() inside this function
		return canDetach (avatarp->getWornAttachment(linked_item->getUUID()));
	}
	else if (item->getType() == LLAssetType::AT_CLOTHING) {
		LLInventoryCategory* cat_parent = gInventory.getCategory (linked_item->getParentUUID());
		if (cat_parent && !canDetachCategory(cat_parent, true)) return false;
		const LLWearable* wearable = gAgentWearables.getWearableFromItemID(linked_item->getUUID ());
		if (wearable) return canUnwear (wearable->getType());
		return true;
	}
	return true;
}

bool RRInterface::canDetach(LLViewerObject* attached_object)
{
//	if (!scriptsEnabled() && !getScriptsEnabledOnce()) return false;
	if (attached_object == NULL) return true;
	LLViewerObject* root = attached_object->getRootEdit();
	if (root == NULL) return true;

	// Check all the current restrictions, if "detach" is issued from a child prim of the root prim of
	// attached_object, then the whole object is undetachable
	RRMAP::iterator it = mSpecialObjectBehaviours.begin ();
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second == "detach") {
			LLViewerObject* this_prim = gObjectList.findObject(LLUUID(it->first));
			if (this_prim && (this_prim->getRootEdit() == root)) {
				return false;
			}
		}
		it++;
	}

//	if (!isAllowed (attached_object->getRootEdit()->getID(), "detach", FALSE)) return false;
	if (!isAllowed (attached_object->getID(), "detach", FALSE)) return false;
	if (!isAllowed (attached_object->getID(), "detachthis", FALSE)) return false;
	if (!isAllowed (attached_object->getID(), "detachallthis", FALSE)) return false;

	LLInventoryItem* item = getItem (attached_object->getRootEdit()->getID());

	if (item) {
		LLInventoryCategory* cat_parent = gInventory.getCategory (item->getParentUUID());
		std::string cat_parent_name = "";
		if (cat_parent) {
			cat_parent_name = cat_parent->getName();
		}

		// If the item has just been received, let the user detach it (we know it has not issued a @detach restriction already)
		if (isInventoryItemNew(item) && cat_parent_name.length() > 0 && cat_parent_name.at(0) != '~') return true; // we must check for temp RLV folders here too, not only in canDetachCategory(), because we can't know which one we will check first

		if (cat_parent && !canDetachCategory(cat_parent, true)) return false;

		if (mHandleNoStrip) {
			std::string name = item->getName();
			LLStringUtil::toLower(name);
			if (name.find ("nostrip") != -1) return false;
		}

		LLVOAvatarSelf* avatarp = gAgentAvatarp;
		if (avatarp) {
			std::string attachpt;
			if (avatarp->getAttachedPointName(item->getLinkedUUID(), attachpt))
			{
				if (contains("detach:"+attachpt)) return false;
				if (contains("remattach")) return false;
				if (contains("remattach:"+attachpt)) return false;
			}
	//				if (!canDetach(attachpt)) return false;
		}
	}
	return true;
}

bool RRInterface::canDetach(std::string attachpt)
{
//	if (!scriptsEnabled() && !getScriptsEnabledOnce()) return false;
	LLStringUtil::toLower(attachpt);
	if (contains("detach:"+attachpt)) return false;
	if (contains("remattach")) return false;
	if (contains("remattach:"+attachpt)) return false;
	LLViewerJointAttachment* attachment = findAttachmentPointFromName (attachpt, TRUE);
	if (!canDetachAllObjectsFromAttachment (attachment)) return false;
	return true;
}

bool RRInterface::canAttach(LLViewerObject* object_to_attach, std::string attachpt, bool from_server /* = false */)
{
	// If from_server == true, the check is done while receiving an order from the server => always allow
	if (from_server) return true;

	// Attention : this function does not check if we are replacing and there is a locked object already present on the attachment point
	LLStringUtil::toLower(attachpt);
	if (contains("addattach")) return false;
	if (contains("addattach:"+attachpt)) return false;
	if (object_to_attach) {
		LLInventoryItem* item = getItem(object_to_attach->getRootEdit()->getID());
		if (item) {
			// If the item has just been received, let the user attach it
			if (isInventoryItemNew(item)) return true;

			LLInventoryCategory* cat_parent = gInventory.getCategory (item->getParentUUID());
			if (cat_parent && !canAttachCategory(cat_parent)) return false;
		}
	}
	
	return true;
}

bool RRInterface::canAttach(LLInventoryItem* item, bool from_server /* = false */)
{
	// If from_server == true, the check is done while receiving an order from the server => always allow
	if (from_server) return true;

	// If the item has just been received, let the user attach it
	if (isInventoryItemNew(item)) return true;

	if (contains("addattach")) return false;
	if (!item) return true;
	LLViewerJointAttachment* attachpt = findAttachmentPointFromName (item->getName());
	if (attachpt && contains("addattach:"+attachpt->getName())) return false;
	LLInventoryCategory* cat_parent = gInventory.getCategory (item->getParentUUID());
	if (cat_parent && !canAttachCategory(cat_parent)) return false;
	return true;
}

bool RRInterface::canEdit(LLViewerObject* object)
{
	if (!object || !object->getRootEdit()) return false;
	if (containsWithoutException ("edit", object->getRootEdit()->getID().asString())) return false;
	if (contains ("editobj:"+object->getRootEdit()->getID().asString())) return false;
	if (!object->isHUDAttachment() && mContainsInteract) return false;
	return true;
}

bool RRInterface::canTouch(LLViewerObject* object, LLVector3 pick_intersection /* = LLVector3::zero */)
{
	if (!object) return true;

	LLViewerObject* root = object->getRootEdit();
	if (!root) return true;

	if (!isAllowed (object->getRootEdit()->getID(), "touchme")) return true; // to check the presence of "touchme" on this object, which means that we can touch it

	if (!root->isHUDAttachment() && contains ("touchall")) return false;

//	if (!root->isHUDAttachment() && contains ("touchallnonhud")) return false;

	if (root->isHUDAttachment() && containsWithoutException("touchhud", object->getRootEdit()->getID().asString())) return false;

	if (contains ("touchthis:"+object->getRootEdit()->getID().asString())) return false;

	if (!canTouchFar (object, pick_intersection)) return false;

	if (root->isAttachment()) {
		if (!root->isHUDAttachment()) {
			if (contains ("touchattach")) return false;

			LLInventoryItem* inv_item = getItem (root->getID());
			if (inv_item) { // this attachment is in my inv => it belongs to me
				if (contains ("touchattachself")) {
					return false;
				}
			}
			else { // this attachment is not in my inv => it does not belong to me
				if (contains ("touchattachother")) {
					return false;
				}
			}
		}
	}
	else {
		if (containsWithoutException ("touchworld", object->getRootEdit()->getID().asString())) return false;
	}
	return true;
}

bool RRInterface::canTouchFar(LLViewerObject* object, LLVector3 pick_intersection /* = LLVector3::zero */)
{
	if (!object) return true;

	if (mContainsInteract && !object->isHUDAttachment())
	{
		return false;
	}

	LLVector3 pos = object->getPositionRegion ();
	if (pick_intersection != LLVector3::zero) pos = pick_intersection;
	pos -= gAgent.getPositionAgent ();
	F32 dist = pos.magVec();
	if (!object->isHUDAttachment()) {
		if (dist > mFartouchMax) return false;
		if (dist > mCamDistDrawMax) return false; // don't allow touching or selecting something that is obstructed
	}

	return true;
}

bool RRInterface::isInventoryItemNew(LLInventoryItem* item)
{
	// Return true if the item or its parent category has been received during this session
	if (!item) return false; // just in case
	std::string name = item->getName();
	std::string parent_name = "";
	LLUUID parent_uuid = item->getParentUUID();
	LLInventoryCategory* parent = gInventory.getCategory(parent_uuid);
	if (parent) {
		parent_name = parent->getName();
	}
	size_t size = gAgent.mRRInterface.mReceivedInventoryObjects.size();
	for (size_t i = 0; i < size; i++) {
		if (gAgent.mRRInterface.mReceivedInventoryObjects[i] == name
			|| gAgent.mRRInterface.mReceivedInventoryObjects[i] == parent_name) {
			return true;
		}
	}

	return false;
}

bool RRInterface::IsInventoryFolderNew(LLInventoryCategory* folder)
{
	// Return true if the folder has been received during this session
	if (!folder) return false; // just in case
	std::string name = folder->getName();
	size_t size = gAgent.mRRInterface.mReceivedInventoryObjects.size();
	for (size_t i = 0; i < size; i++) {
		if (gAgent.mRRInterface.mReceivedInventoryObjects[i] == name) {
			return true;
		}
	}

	return false;
}




bool RRInterface::scriptsEnabled()
{
	LLViewerParcelMgr* vpm = LLViewerParcelMgr::getInstance();
	LLViewerRegion* agent_region = gAgent.getRegion();
	LLParcel* agent_parcel = vpm->getAgentParcel();
	if (!agent_region || !agent_parcel)
		return false;
	return vpm->allowAgentScripts(agent_region, agent_parcel);
}

void RRInterface::printOnChat (std::string message)
{
	LLChat chat(message);
	chat.mSourceType = CHAT_SOURCE_UNKNOWN;
	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if(nearby_chat)
	{
		nearby_chat->addMessage(chat);
	}
}

void RRInterface::listRlvRestrictions(std::string substr /*= ""*/)
{
	printOnChat(getRlvRestrictions(substr));
}

std::string RRInterface::getRlvRestrictions(std::string substr /*= ""*/)
{
	RRMAP::iterator it = mSpecialObjectBehaviours.begin();
	std::string object_name = "";
	std::string old_object_name = "";
	std::string res = "";
	res += ("\n### RLV RESTRICTIONS #####################");
	while (it != mSpecialObjectBehaviours.end())
	{
		old_object_name = object_name;
		LLInventoryItem* item = getItem(LLUUID(it->first));
		if (item) object_name = item->getName();
		else object_name = "???";

		if (substr == "" || object_name.find(substr) != -1) {
			// print the name of the object
			if (object_name != old_object_name) {
				res += ("\n### " + object_name + " #####################");
			}
			res += ("\n-------- " + it->second);
		}
		it++;
	}
	res += ("\n##########################################");
	return res;
}

BOOL RRInterface::checkCameraLimits(BOOL and_correct /* = FALSE*/)
{
	// Check that we are within the imposed limits
	// Force the camera back into the limits when not
	// Return TRUE when the camera is ok
	if (!gAgentCamera.isInitialized()) {
		return TRUE;
	}

	if (mCamDistMax <= 0.f && !gAgentCamera.cameraMouselook())
	{
		if (and_correct) {
			gAgentCamera.changeCameraToMouselook ();
		}
		return FALSE;
	}
	else if (mCamDistMin > 0.f && gAgentCamera.cameraMouselook())
	{
		if (and_correct) {
			gAgentCamera.changeCameraToDefault ();
		}
		return FALSE;
	}
	return TRUE;
}

BOOL RRInterface::updateCameraLimits ()
{
	// Update the min and max 
	mShowavsDistMax = getMin ("camavdist", EXTREMUM);

	if (mShowavsDistMax < EXTREMUM) {
		LLVOAvatar::sUseImpostors = TRUE;
	}
	else {
		if (LLStartUp::getStartupState() >= STATE_STARTED) {
			LLVOAvatar::sUseImpostors = gSavedSettings.getBOOL ("RenderUseImpostors");
		}

	}

	F32 old_mCamDistDrawMax = mCamDistDrawMax;
	F32 old_mCamDistDrawMin = mCamDistDrawMin;
	F32 old_mCamDistDrawAlphaMax = mCamDistDrawAlphaMax;
	F32 old_mCamDistDrawAlphaMin = mCamDistDrawAlphaMin;

    mCamZoomMax = getMin("camzoommax", EXTREMUM);
    if (mCamZoomMax == 0.f) mCamZoomMax = EXTREMUM;
    mCamZoomMin = getMax("camzoommin", -EXTREMUM);
    if (mCamZoomMin == 0.f) mCamZoomMin = -EXTREMUM;

    // setcam_fovmin and setcam_fovmax set the FOV, i.e. 60°/multiplier;
    // in other words, they are equivalent to camzoommin and camzoommax.
    F32 fovmin = getMax("setcam_fovmin", 0.001f);
    if (fovmin != 0.f && fovmin != 0.001f)
    {
        F32 zoommax_from_fovmin = DEFAULT_FIELD_OF_VIEW / fovmin;
        if (zoommax_from_fovmin < mCamZoomMax)
        {
            mCamZoomMax = zoommax_from_fovmin;
        }
    }
    F32 fovmax = getMin("setcam_fovmax", EXTREMUM);
    if (fovmax != 0.f && fovmax != EXTREMUM)
    {
        F32 zoommin_from_fovmax = DEFAULT_FIELD_OF_VIEW / fovmax;
        if (zoommin_from_fovmax > mCamZoomMin)
        {
            mCamZoomMin = zoommin_from_fovmax;
        }
    }

    mCamDistMax = getMin("camdistmax,setcam_avdistmax", EXTREMUM);
    mCamDistMin = getMax("camdistmin,setcam_avdistmin", -EXTREMUM);

	mCamDistDrawMax = getMin ("camdrawmax", EXTREMUM);
	mCamDistDrawMin = getMin ("camdrawmin", EXTREMUM);

	mCamDistDrawAlphaMin = getMax ("camdrawalphamin", 0.f);
	mCamDistDrawAlphaMax = getMax ("camdrawalphamax", 1.f);

	mCamDistDrawColor = getMixedColors ("camdrawcolor", LLColor3::black);

	if (mCamDistDrawMin <= 0.4f) { // So we're sure to render the spheres even when restricted to mouselook
		mCamDistDrawMin = 0.4f;
	}

	if (mCamDistDrawMax < mCamDistDrawMin && mCamDistDrawMin < EXTREMUM) { // sort the two limits in order
		mCamDistDrawMax = mCamDistDrawMin;
	}

	if (mCamDistMax >= mCamDistDrawMin) { // make sure we can't move the camera outside the minimum render limit
		mCamDistMax = mCamDistDrawMin * 0.75f;
	}
	if (mCamDistMax >= mCamDistDrawMax) { // make sure we can't move the camera outside the maximum render limit
		mCamDistMax = mCamDistDrawMax * 0.75f;
	}

	if (mCamDistDrawAlphaMax < mCamDistDrawAlphaMin) { // make sure the "fog" goes in the right direction
		mCamDistDrawAlphaMax = mCamDistDrawAlphaMin;
	}

	if (mCamZoomMin > mCamZoomMax) {
		mCamZoomMin = mCamZoomMax;
	}

	if (mCamDistMin > mCamDistMax) {
		mCamDistMin = mCamDistMax;
	}

// DKO
// read "gRRenabled" below has never been tested or it would not be there. flycam is broken and will not work even when RLV is enabled
// only time this should happen when it is set by RLV user sets the limit. Obviously that is broken. 
// note the command "handle_toggle_flycam" is used for another purpose. here it is always part of RRInterface.o object file
//DKO
//	if (gRRenabled) // RLV_108 : don't toggle flycam if RLV is disabled since we call this method at startup
//	{
//		LLViewerJoystick::getInstance()->getOverrideCamera();
//		if (mCamDistMax < EXTREMUM * 0.75f)
//		{
//			handle_toggle_flycam();
//		}
//	}

	// silly hack, but we need to force all textures in world to be updated (code copied from camtextures above)
	// Do so only when the camera limits have changed, though, because this method is called every time a new RLV restriction
	// is received or removed, so it would slow down the viewer otherwise.
	if (old_mCamDistDrawMax != mCamDistDrawMax 
		|| old_mCamDistDrawMin != mCamDistDrawMin 
		|| old_mCamDistDrawAlphaMax != mCamDistDrawAlphaMax 
		|| old_mCamDistDrawAlphaMin != mCamDistDrawAlphaMin) {

		// Force all the rendering types back to TRUE (and we won't be able to switch them off while the vision is restricted)
		if (mCamDistDrawMin < EXTREMUM || mCamDistDrawMax < EXTREMUM) {
			LLPipeline::setRenderBeacons(FALSE);
			LLPipeline::setRenderScriptedBeacons(FALSE);
			LLPipeline::setRenderScriptedTouchBeacons(FALSE);
			LLPipeline::setRenderPhysicalBeacons(FALSE);
			LLPipeline::setRenderSoundBeacons(FALSE);
			LLPipeline::setRenderParticleBeacons(FALSE);
			LLPipeline::setRenderHighlights(FALSE);
			LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
			gPipeline.setAllRenderTypes();

			// Also make sure the basic shaders are enabled. On some video cards, turning them off completely hides the vision spheres.
			if (gSavedSettings.getBOOL("VertexShaderEnable") == FALSE) {
				if (gGLManager.mGLVersion >= 3.f || !gGLManager.mIsIntel) {
					gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
				}
			}

		}

		S32 i;
		for (i=0; i<gObjectList.getNumObjects(); ++i) {
			LLViewerObject* object = gObjectList.getObject(i);
			if (object) {
				object->setSelected(FALSE);
			}
		}
	}

	// And check the camera is still within the limits
	return checkCameraLimits (TRUE);
}

#define UPPER_ALPHA_LIMIT 0.999999f
// This function returns the effective alpha to set to each step when going from
// 0.0 to "desired_alpha", so that everything seen through the last layer will
// be obscured as if it were behind only one layer of alpha "desired_alpha", 
// regardless of the number of layers "nb_layers"
// If we have N layers and want a transparency T (T = 1-A), we want to find X so that
// X**N = T (because combined transparencies multiply), in other words, X = T**(1/N)
// The problem with this formula is that with a target transparency of 0 (alpha = 1), we would
// not get any gradient at all so we need to limit the alpha to a maximum that is lower than 1.
F32 calculateDesiredAlphaPerStep (F32 desired_alpha, int nb_layers)
{
	double trans_at_this_step;
	if (desired_alpha > UPPER_ALPHA_LIMIT) {
		desired_alpha = UPPER_ALPHA_LIMIT;
	}
	double desired_trans = (double)(1.f - desired_alpha);
	trans_at_this_step = pow (desired_trans, 1.0/(double)nb_layers);

	return (F32)(1.0 - trans_at_this_step);
}

// This method draws several big black spheres around the avatar, with various alphas
// Alpha goes from mCamDistDrawAlphaMin to mCamDistDrawAlphaMax
// Things to remember :
// - There are two render limits in RLV : min and max (min is a sphere with a variable alpha
//   and max is an opaque sphere).
// - Render limit min <= render limit max.
// - If a render limit is <= 1.0, make it 1.0 because we'll be forced into mouselook anyway,
//   so it would be better to render the sphere
// - If a render limit is unspecified (i.e. equal to EXTREMUM), don't render it.
// - If both render limits are specified and different, render both and several in-between at 
//   regular intervals, with a linear interpolation for alpha between mCamDistDrawAlphaMin and
//   mCamDistDrawAlphaMax for each sphere.
// - There are not too many spheres to render, because stacking alphas makes the video card
//   complain.
void RRInterface::drawRenderLimit ()
{
	//if (sRenderLimitRenderedThisFrame) { // already rendered the vision spheres during this rendering frame ? => bail
	//	return;
	//}

	if (mCamDistDrawMin >= EXTREMUM && mCamDistDrawMax >= EXTREMUM) { // not vision restricted ? => bail
		return;
	}

	gGL.setColorMask(true, true);
	if (LLGLSLShader::sNoFixedFunction) {
		gUIProgram.bind();
	}

	//gGL.flush();
	//glClearColor(0,0,0,1);
	//glClearColor(0,0,0,0);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLEnable gls_blend(GL_BLEND);
	LLGLEnable gls_cull(GL_CULL_FACE);
	LLGLEnable alpha_test (GL_ALPHA_TEST);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	//gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
	//gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	gPipeline.disableLights();
	gGL.blendFunc(LLRender::BF_SOURCE_ALPHA,
					LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
					LLRender::BF_ZERO,
					LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

	//gGL.flush();
	//glClearColor(0,0,0,1);
	//gGL.setColorMask(true, true);
	//glClearColor(0,0,0,0);

	gGL.setColorMask(true, false);
	LLVector3 center = isAgentAvatarValid() 
		? getCamDistDrawFromJoint()->getWorldPosition()
		: gAgent.getPositionAgent();

	//LLVector3 center = gAgentAvatarp->mHeadp->getWorldPosition();

	//LLVector3 center = gAgentAvatarp && gAgentAvatarp->mHeadp 
	//	? gAgentAvatarp->mHeadp->getWorldPosition()
	//	: gAgent.getPositionAgent();

	// Render the inner sphere first
	if (mCamDistDrawMin < mCamDistDrawMax) {
		drawSphere (center, mCamDistDrawMin, mCamDistDrawColor, mCamDistDrawAlphaMin);
	}

	do { // do this block only once, using break statements like glorified GOTOs
		// If the inner sphere is opaque, no need to go any further
		if (mCamDistDrawAlphaMin >= 1.f) {
			break;
		}

		// Now render every sphere from the inner one (excluded) to the outer one (included) with
		// an alpha that depends on the number of spheres, so that the apparent alpha of all the
		// combined spheres is equal to mCamDistDrawAlphaMax
		F32 alpha_step = calculateDesiredAlphaPerStep (mCamDistDrawAlphaMax, mCamDistNbGradients);
		for (int i = 1; i <= mCamDistNbGradients; i++) {
			if (mCamDistDrawAlphaMax > UPPER_ALPHA_LIMIT && i == mCamDistNbGradients) {
				// Outer sphere and we aim an alpha of 1 => make it opaque
				alpha_step = 1.f;
			}
			// If this is the outer sphere and the desired alpha is 1, make sure to draw it opaque
			// (because of rounding errors and of the upper limit in calculateDesiredAlphaPerStep(), 
			// drawing white or grey spheres would not give a complete opacity otherwise).
			drawSphere (center
				, lerp (mCamDistDrawMin, mCamDistDrawMax, (F32)i / (F32)mCamDistNbGradients)
				, mCamDistDrawColor
				, alpha_step
			);
		}

	} while (false);

	gGL.flush();

	if (LLGLSLShader::sNoFixedFunction) {
		gUIProgram.unbind();
	}

	sRenderLimitRenderedThisFrame = TRUE;
}

void RRInterface::drawSphere (LLVector3 center, F32 scale, LLColor3 color, F32 alpha)
{
	//gGL.pushMatrix();
	{
		gGL.pushMatrix();
		//gGL.loadIdentity();
		{
			gGL.translatef(center[0], center[1], center[2]);
			gGL.scalef(scale, scale, scale);

			LLColor4 color_alpha(color, alpha);
			gGL.color4fv(color_alpha.mV);
				
			// Render inside only (the camera is not supposed to go outside anyway)
			glCullFace(GL_FRONT);
			gSphere.render();
			glCullFace(GL_BACK);
		}
		gGL.popMatrix();
	}
	//gGL.popMatrix();
}

LLJoint* RRInterface::getCamDistDrawFromJoint ()
{
	if (!gAgentAvatarp) {
		return NULL;
	}
	if (!mCamDistDrawFromJoint || CAMERA_MODE_MOUSELOOK == gAgentCamera.getCameraMode()) {
		return gAgentAvatarp->mHeadp;
	}
	return mCamDistDrawFromJoint;
}

BOOL RRInterface::isBlacklisted (std::string action, bool force)
{
	std::string blacklist;
	blacklist = ","+sBlacklist+",";
	if (force) {
		return (blacklist.find (","+action+"%f,") != -1);
	}
	else {
		return (blacklist.find (","+action+",") != -1);
	}
}

std::deque<std::string> RRInterface::getBlacklist (std::string filter /* = ""*/)
{
	std::deque<std::string> list, res;
	list = parse (sBlacklist, ",");
	res.clear();

	unsigned int size = list.size();
	for (unsigned int i = 0; i < size; i++) {
		if (filter == "" || list[i].find (filter) != -1) {
			res.push_back (list[i]);
		}
	}

	return res;
}

void RRInterface::updateLimits() {
	// Update all the min and max that are not related to camera and vision restrictions
	// For example, tplocal, fartouch, sittp etc

	// fartouch/touchfar
	// First we need to find the lowest betwen 
	float fartouch_max = getMin("fartouch", EXTREMUM);
	float touchfar_max = getMin("touchfar", EXTREMUM);
	if (fartouch_max > touchfar_max){
		fartouch_max = touchfar_max;
	}
	mFartouchMax = fartouch_max;

	// sittp
	mSittpMax = getMin("sittp", EXTREMUM);

	// tplocal
	mTplocalMax = getMin("tplocal", EXTREMUM);
}

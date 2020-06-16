/**
 * @file fsfloatersearchlegacy.cpp
 * @brief Legacy Search Floater
 *
 * $LicenseInfo:firstyear=2012&license=fsviewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2012, Cinder Roxley <cinder@cinderblocks.biz>
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
 * The Phoenix Viewer Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.phoenixviewer.com
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsfloatersearchlegacy.h"
#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llcallingcard.h"
#include "llclassifiedflags.h"
#include "llclassifiedstatsresponder.h"
#include "llfloatersidepanelcontainer.h"
#include "llqueryflags.h"
#include "llgroupmgr.h"
#include "llparcel.h"
#include "llclassifiedinfo.h"
#include "llremoteparcelrequest.h"
#include "llavatarpropertiesprocessor.h"
#include "llproductinforequest.h"
#include "llsearcheditor.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewerfloaterreg.h"
#include "llviewergenericmessage.h"
#include "llviewerregion.h"
#include "llnotificationsutil.h"
#include "lldispatcher.h"
#include "lltrans.h"
#include "message.h"

#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llloadingindicator.h"
#include "lltimer.h"

#include "llavataractions.h"
#include "llgroupactions.h"
#include "llfloaterworldmap.h"
#include "fspanelclassified.h"

#include <boost/algorithm/string.hpp>

#define MIN_SEARCH_STRING_SIZE 3

////////////////////////////////////////
//          Observer Classes          //
////////////////////////////////////////

class FSLegacySearchRemoteParcelInfoObserver : public LLRemoteParcelInfoObserver
{
public:
	FSLegacySearchRemoteParcelInfoObserver(FSFloaterSearchLegacy* floater) : LLRemoteParcelInfoObserver(),
		mFloater(floater)
	{}
	
	~FSLegacySearchRemoteParcelInfoObserver()
	{
		// remove any in-flight observers
		std::set<LLUUID>::iterator it;
		for (it = mParcelIDs.begin(); it != mParcelIDs.end(); ++it)
			{
				const LLUUID &id = *it;
				LLRemoteParcelInfoProcessor::getInstance()->removeObserver(id, this);
			}
		mParcelIDs.clear();
	}
	
	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data)
	{
		if (mFloater)
		{
			mFloater->sendParcelDetails(LLVector3d(parcel_data.global_x,
												   parcel_data.global_y,
												   parcel_data.global_z),
										parcel_data.name,
										parcel_data.desc,
										parcel_data.snapshot_id);
		}
		mParcelIDs.erase(parcel_data.parcel_id);
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(parcel_data.parcel_id, this);
	}
	
	/*virtual*/ void setParcelID(const LLUUID& parcel_id)
	{
		if (!parcel_id.isNull())
		{
			mParcelIDs.insert(parcel_id);
			LLRemoteParcelInfoProcessor::getInstance()->addObserver(parcel_id, this);
			LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(parcel_id);
		}
	}
	
	/*virtual*/ void setErrorStatus(S32 status, const std::string& reason)
	{
		LL_ERRS() << "Can't complete remote parcel request. Http Status: " << status << ". Reason : " << reason << LL_ENDL;
	}
private:
	std::set<LLUUID>	mParcelIDs;
	FSFloaterSearchLegacy*		mFloater;
};

class FSLegacySearchGroupInfoObserver : public LLGroupMgrObserver
{
public:
	FSLegacySearchGroupInfoObserver(const LLUUID& group_id, FSFloaterSearchLegacy* floater) :
	LLGroupMgrObserver(group_id),
	mFloater(floater)
	{
		LLGroupMgr* groupmgr = LLGroupMgr::getInstance();
		groupmgr->addObserver(this);
		groupmgr->sendGroupPropertiesRequest(group_id);
	}
	
	~FSLegacySearchGroupInfoObserver()
	{
		LLGroupMgr::getInstance()->removeObserver(this);
	}
	
	void changed(LLGroupChange gc)
	{
		if (gc == GC_PROPERTIES)
		{
			mFloater->processGroupData();
			LLGroupMgr::getInstance()->removeObserver(this);
		}
	}
private:
	FSFloaterSearchLegacy* mFloater;
};

/////////////////////////////////////////
// Silly Classified Clickthrough Class //
/////////////////////////////////////////

class FSDispatchClassifiedClickThrough : public LLDispatchHandler
{
public:
	virtual bool operator()(
							const LLDispatcher* dispatcher,
							const std::string& key,
							const LLUUID& invoice,
							const sparam_t& strings)
	{
		if (strings.size() != 4) return false;
		LLUUID classified_id(strings[0]);
		S32 teleport_clicks = atoi(strings[1].c_str());
		S32 map_clicks = atoi(strings[2].c_str());
		S32 profile_clicks = atoi(strings[3].c_str());
		
		FSPanelClassifiedInfo::setClickThrough(classified_id, teleport_clicks, map_clicks, profile_clicks, FALSE);
		return TRUE;
	}
};
static FSDispatchClassifiedClickThrough sClassifiedClickThrough;

// Just to debug errors. Can be thrown away later.
class FSClassifiedClickMessageResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(FSClassifiedClickMessageResponder);
	
public:
	// If we get back an error (not found, etc...), handle it here
	virtual void errorWithContent(U32 status,
								  const std::string& reason,
								  const LLSD& content)
	{
		LL_WARNS() << "Sending click message failed (" << status << "): [" << reason << "]" << LL_ENDL;
		LL_WARNS() << "Content: [" << content << "]" << LL_ENDL;
	}
};

////////////////////////////////////////
//         The floater itself         //
////////////////////////////////////////

FSFloaterSearchLegacy::FSFloaterSearchLegacy(const LLSD& key)
:	LLFloater(key),
	ESearchMode(SM_PEOPLE),
	mStartSearch(0),
	mResultsPerPage(100),
	mResultsReceived(0),
	mResultsContent()
{
	mRemoteParcelObserver = new FSLegacySearchRemoteParcelInfoObserver(this);
	mCommitCallbackRegistrar.add("SearchLegacy.Search", boost::bind(&FSFloaterSearchLegacy::find, this));
}

FSFloaterSearchLegacy::~FSFloaterSearchLegacy()
{
	mQueryID.setNull();
	delete mRemoteParcelObserver;
	gGenericDispatcher.addHandler("classifiedclickthrough", NULL);
}

bool FSFloaterSearchLegacy::postBuild()
{
	LLSearchEditor *search_terms = getChild<LLSearchEditor>("search_terms");
	if (search_terms) {
		search_terms->setFocusChangedCallback(boost::bind(&FSFloaterSearchLegacy::onSearchFocusChanged, this, _1));
		search_terms->setKeystrokeCallback(boost::bind(&FSFloaterSearchLegacy::onSearchTextEntry, this));
	}

	LLButton *search_btn = getChild<LLButton>("search_btn");
	if (search_btn) {
		search_btn->setEnabled(FALSE);
	}

	LLLineEditor* price_edit = findChild<LLLineEditor>("price_edit");
	if (price_edit)
	{
		price_edit->setCommitOnFocusLost(FALSE);
		price_edit->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::find, this));
	}
	LLLineEditor* area_edit = findChild<LLLineEditor>("area_edit");
	if (area_edit)
	{
		area_edit->setCommitOnFocusLost(FALSE);
		area_edit->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::find, this));
	}
	childSetAction("Next", boost::bind(&FSFloaterSearchLegacy::onBtnNext, this));
	childSetAction("Back", boost::bind(&FSFloaterSearchLegacy::onBtnBack, this));
	childSetAction("people_profile_btn", boost::bind(&FSFloaterSearchLegacy::onBtnPeopleProfile, this));
	childSetAction("people_message_btn", boost::bind(&FSFloaterSearchLegacy::onBtnPeopleIM, this));
	childSetAction("people_friend_btn", boost::bind(&FSFloaterSearchLegacy::onBtnPeopleFriend, this));
	childSetAction("group_profile_btn", boost::bind(&FSFloaterSearchLegacy::onBtnGroupProfile, this));
	childSetAction("group_chat_btn", boost::bind(&FSFloaterSearchLegacy::onBtnGroupChat, this));
	childSetAction("group_join_btn", boost::bind(&FSFloaterSearchLegacy::onBtnGroupJoin, this));
	childSetAction("parcel_profile_btn", boost::bind(&FSFloaterSearchLegacy::onBtnParcelProfile, this));
	childSetAction("parcel_teleport_btn", boost::bind(&FSFloaterSearchLegacy::onBtnParcelTeleport, this));
	childSetAction("parcel_map_btn", boost::bind(&FSFloaterSearchLegacy::onBtnParcelMap, this));
	childSetAction("search_results_land", boost::bind(&FSFloaterSearchLegacy::find, this));
	getChildView("Next")->setEnabled(FALSE);
	getChildView("Back")->setEnabled(FALSE);
	
	mSearchRadio = getChild<LLRadioGroup>("search_category_radio");
	mSearchRadio->setCommitCallback(onModeSelect, this);
	
	LLComboBox* places_combobox = findChild<LLComboBox>("places_category");
	places_combobox->add(getString("all_categories"), LLSD("any"));
	places_combobox->addSeparator();
	places_combobox->setCommitCallback(onModeSelect, this);

	for (int category = LLParcel::C_LINDEN; category < LLParcel::C_COUNT; category++)
	{
		LLParcel::ECategory eCategory = (LLParcel::ECategory)category;
		places_combobox->add(LLParcel::getCategoryUIString(eCategory), LLParcel::getCategoryString(eCategory));
	}
	
	LLComboBox* classifieds_combobox = getChild<LLComboBox>("classifieds_category");
	LLClassifiedInfo::cat_map::iterator iter;
	classifieds_combobox->add(getString("all_categories"), LLSD("any"));
	classifieds_combobox->addSeparator();
	classifieds_combobox->setCommitCallback(onModeSelect, this);

	for (iter = LLClassifiedInfo::sCategories.begin();
		 iter != LLClassifiedInfo::sCategories.end();
		 iter++)
	{
		classifieds_combobox->add(LLTrans::getString(iter->second));
	}
	
	LLScrollListCtrl* search_results_people = getChild<LLScrollListCtrl>("search_results_people");
	search_results_people->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::onSelectItem, this));
	search_results_people->setEnabled(FALSE);
	search_results_people->setCommentText(getString("no_results"));
	
	LLScrollListCtrl* search_results_groups = getChild<LLScrollListCtrl>("search_results_groups");
	search_results_groups->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::onSelectItem, this));
	search_results_groups->setEnabled(FALSE);
	search_results_groups->setCommentText(getString("no_results"));
	
	LLScrollListCtrl* search_results_places = getChild<LLScrollListCtrl>("search_results_places");
	search_results_places->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::onSelectItem, this));
	search_results_places->setEnabled(FALSE);
	search_results_places->setCommentText(getString("no_results"));
	
	LLScrollListCtrl* search_results_land = getChild<LLScrollListCtrl>("search_results_land");
	search_results_land->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::onSelectItem, this));
	search_results_land->setEnabled(FALSE);
	search_results_land->setCommentText(getString("no_results"));
	
	LLScrollListCtrl* search_results_events = getChild<LLScrollListCtrl>("search_results_events");
	search_results_events->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::onSelectItem, this));
	search_results_events->setEnabled(FALSE);
	search_results_events->setCommentText(getString("no_results"));
	
	LLScrollListCtrl* search_results_classifieds = getChild<LLScrollListCtrl>("search_results_classifieds");
	search_results_classifieds->setCommitCallback(boost::bind(&FSFloaterSearchLegacy::onSelectItem, this));
	search_results_classifieds->setEnabled(FALSE);
	search_results_classifieds->setCommentText(getString("no_results"));
	
	childSetVisible("people_profile_btn", false);
	childSetVisible("people_message_btn", false);
	childSetVisible("people_friend_btn", false);
	childSetVisible("group_profile_btn", false);
	childSetVisible("group_chat_btn", false);
	childSetVisible("group_join_btn", false);
	childSetVisible("parcel_profile_btn", false);
	childSetVisible("parcel_teleport_btn", false);
	childSetVisible("parcel_map_btn", false);
	
	mDetailTitle =	getChild<LLTextEditor>("detail_title");
	mDetailDesc =	getChild<LLTextEditor>("detail_desc");
	mSnapshotCtrl = getChild<LLTextureCtrl>("snapshot");

	setSelectionDetails(LLStringUtil::null, LLStringUtil::null, LLUUID::null);
	mParcelID.setNull();
	mParcelGlobal.setZero();
	refreshSearchVerbs();
	
	return true;
}

// static
void FSFloaterSearchLegacy::onModeSelect(LLUICtrl *ctrl, void *userdata)
{
	FSFloaterSearchLegacy *self = (FSFloaterSearchLegacy *)userdata;
	U32 search_mode = self->mSearchRadio->getValue().asInteger();
	
	if(search_mode == 0)
		self->ESearchMode = SM_PEOPLE;
	else if (search_mode == 1)
		self->ESearchMode = SM_GROUPS;
	else if (search_mode == 2)
		self->ESearchMode = SM_PLACES;
	else if (search_mode == 3)
		self->ESearchMode = SM_LAND;
	else if (search_mode == 4)
		self->ESearchMode = SM_EVENTS;
	else if (search_mode == 5)
		self->ESearchMode = SM_CLASSIFIEDS;
	else {
		self->ESearchMode = SM_PEOPLE;
		LL_WARNS() << "Invalid search category.  Defaulting to people" << LL_ENDL;
	}
	self->refreshSearchVerbs();
	self->resetSearch();
}

void FSFloaterSearchLegacy::refreshSearchVerbs()
{
	const bool is_classifieds = isClassifieds();
	const bool is_events = isEvents();
	const bool is_groups = isGroups();
	const bool is_land = isLand();
	const bool is_people = isPeople();
	const bool is_places = isPlaces();

	childSetVisible("rating_icon_general", !is_people);
	childSetVisible("rating_icon_moderate", !is_people);
	childSetVisible("rating_icon_adult", !is_people);
	childSetVisible("pg_group", is_groups);
	childSetVisible("mature_group", is_groups);
	childSetVisible("adult_group", is_groups);
	childSetVisible("pg_sims", is_places);
	childSetVisible("mature_sims", is_places);
	childSetVisible("adult_sims", is_places);
	childSetVisible("pg_land", is_land);
	childSetVisible("mature_land", is_land);
	childSetVisible("adult_land", is_land);
	childSetVisible("pg_events", is_events);
	childSetVisible("mature_events", is_events);
	childSetVisible("adult_events", is_events);
	childSetVisible("pg_classifieds", is_classifieds);
	childSetVisible("mature_classifieds", is_classifieds);
	childSetVisible("adult_classifieds", is_classifieds);
	childSetVisible("dummy_category", is_people || is_groups);
	childSetVisible("places_category", is_places);
	childSetVisible("land_category", is_land);
	childSetVisible("events_category", is_events);
	childSetVisible("classifieds_category", is_classifieds);
	childSetVisible("search_results_people", is_people);
	childSetVisible("search_results_groups", is_groups);
	childSetVisible("search_results_places", is_places);
	childSetVisible("search_results_land", is_land);
	childSetVisible("search_results_events", is_events);
	childSetVisible("search_results_classifieds", is_classifieds);
	childSetVisible("edit_price", is_land);
	childSetVisible("edit_area", is_land);
	childSetVisible("price_check", is_land);
	childSetVisible("area_check", is_land);
	childSetVisible("ascending_check", is_land);

	childSetEnabled("search_terms", !is_land);
}

void FSFloaterSearchLegacy::refreshActionButtons()
{
	const BOOL is_classifieds = isClassifieds();
	const BOOL is_groups = isGroups();
	const BOOL is_land = isLand();
	const BOOL is_people = isPeople();
	const BOOL is_places = isPlaces();

	childSetVisible("people_profile_btn", is_people);
	childSetVisible("people_message_btn", is_people);
	childSetVisible("people_friend_btn", is_people);
	childSetVisible("group_profile_btn", is_groups);
	childSetVisible("group_chat_btn", is_groups);
	childSetVisible("group_join_btn", is_groups);
	childSetVisible("parcel_profile_btn", is_places || is_land || is_classifieds);
	childSetVisible("parcel_teleport_btn", is_places || is_land || is_classifieds);
	childSetVisible("parcel_map_btn", is_places || is_land || is_classifieds);

	if (is_people) {
		const bool is_self = (mSelectedID == gAgentID);
		const bool is_friend = is_self || LLAvatarActions::isFriend(mSelectedID);
		childSetEnabled("people_message_btn", !is_self);
		childSetEnabled("people_friend_btn", !is_friend);
	}
}

void FSFloaterSearchLegacy::onBtnNext()
{
	mStartSearch += mResultsPerPage;
	getChildView("Back")->setEnabled(TRUE);
	
	find();
}

void FSFloaterSearchLegacy::onBtnBack()
{
	mStartSearch -= mResultsPerPage;
	getChildView("Back")->setEnabled(mStartSearch > 0);
	
	find();
}

void FSFloaterSearchLegacy::onBtnPeopleProfile()
{
	LLAvatarActions::showProfile(getSelectedID());
}

void FSFloaterSearchLegacy::onBtnPeopleIM()
{
	LLAvatarActions::startIM(getSelectedID());
}

void FSFloaterSearchLegacy::onBtnPeopleFriend()
{
	LLAvatarActions::requestFriendshipDialog(getSelectedID());
}

void FSFloaterSearchLegacy::onBtnGroupProfile()
{
	LLGroupActions::show(getSelectedID());
}

void FSFloaterSearchLegacy::onBtnGroupChat()
{
	LLGroupActions::startIM(getSelectedID());
}

void FSFloaterSearchLegacy::onBtnGroupJoin()
{
	LLGroupActions::join(getSelectedID());
}

void FSFloaterSearchLegacy::onBtnParcelProfile()
{
	LLSD params;

	params["id"] = mParcelID;
	params["type"] = "remote_place";

	LLFloaterSidePanelContainer::showPanel("places", params);
}

void FSFloaterSearchLegacy::onBtnParcelTeleport()
{
	if (!mParcelGlobal.isExactlyZero())
	{
		gAgent.teleportViaLocation(mParcelGlobal);
		LLFloaterWorldMap::getInstance()->trackLocation(mParcelGlobal);
	}
}

void FSFloaterSearchLegacy::onBtnParcelMap()
{
	LLFloaterWorldMap::getInstance()->trackLocation(mParcelGlobal);
	LLFloaterReg::showInstance("world_map", "center");
}

S32 FSFloaterSearchLegacy::showNextButton(S32 rows)
{
	bool show_next_button = (mResultsReceived > mResultsPerPage);
	getChildView("Next")->setEnabled(show_next_button);
	if (show_next_button)
	{
		rows -= (mResultsReceived - mResultsPerPage);
	}
	return rows;
}

void FSFloaterSearchLegacy::showResultsCount()
{
	LLUICtrl *result_count_text = getChild<LLUICtrl>("result_count_text");
	
	if (result_count_text) {
		S32 last_row = mStartSearch + (mResultsReceived > mResultsPerPage ? mResultsPerPage : mResultsReceived);

		if (last_row) {
			result_count_text->setTextArg("[FIRST]", llformat("%d", mStartSearch + 1));
			result_count_text->setTextArg("[LAST]", llformat("%d", last_row));
			result_count_text->setVisible(TRUE);
		}
		else {
			result_count_text->setVisible(FALSE);
		}
	}
}

void FSFloaterSearchLegacy::setupSearch()
{
	if (mQueryID.notNull()) {
		mQueryID.setNull();
	}

	mResultsReceived = 0;
	showResultsCount();
}

void FSFloaterSearchLegacy::resetSearch()
{
	mStartSearch = 0;
	mResultsPerPage = (ESearchMode == SM_EVENTS) ? 200 : 100;

	getChildView("Back")->setEnabled(FALSE);
	getChildView("Next")->setEnabled(FALSE);

	updateSearchEnabled();

	if (getChild<LLUICtrl>("search_terms")->getValue().asString().length() >= MIN_SEARCH_STRING_SIZE || isLand()) {
		find();
	}
}

void FSFloaterSearchLegacy::onSelectItem()
{
	LLScrollListCtrl* list = NULL;
	switch(ESearchMode)
	{
		case SM_PEOPLE:
			list = getChild<LLScrollListCtrl>("search_results_people");
			break;
		case SM_GROUPS:
			list = getChild<LLScrollListCtrl>("search_results_groups");
			break;
		case SM_PLACES:
			list = getChild<LLScrollListCtrl>("search_results_places");
			break;
		case SM_LAND:
			list = getChild<LLScrollListCtrl>("search_results_land");
			break;
		case SM_EVENTS:
			list = getChild<LLScrollListCtrl>("search_results_events");
			break;
		case SM_CLASSIFIEDS:
			list = getChild<LLScrollListCtrl>("search_results_classifieds");
			break;
	}
	if (list == NULL)
	{
		return;
	}

	mSelectedID = list->getSelectedValue();
	if (mSelectedID.isNull()) {
		return;
	}

	refreshActionButtons();
	mSnapshotCtrl->setVisible(TRUE);
	mParcelID.setNull();

	LLAvatarPropertiesProcessor* mAvatarPropertiesProcessor = LLAvatarPropertiesProcessor::getInstance();
	switch(ESearchMode)
	{
		case SM_PEOPLE:
			mAvatarPropertiesProcessor->addObserver(getSelectedID(), this);
			mAvatarPropertiesProcessor->sendAvatarPropertiesRequest(getSelectedID());
			setLoadingProgress(TRUE);
			break;
		case SM_GROUPS:
			mGroupPropertiesRequest = new FSLegacySearchGroupInfoObserver(getSelectedID(), this);
			setLoadingProgress(TRUE);
			break;
		case SM_PLACES:
		case SM_LAND:
			mParcelID = getSelectedID();
			mRemoteParcelObserver->setParcelID(getSelectedID());
			setLoadingProgress(TRUE);
			break;
		case SM_EVENTS:
			// Implement me!
			break;
		case SM_CLASSIFIEDS:
			mAvatarPropertiesProcessor->addObserver(getSelectedID(), this);
			mAvatarPropertiesProcessor->sendClassifiedInfoRequest(getSelectedID());
			gGenericDispatcher.addHandler("classifiedclickthrough", &sClassifiedClickThrough);
			setLoadingProgress(TRUE);
			break;
	}
}

void FSFloaterSearchLegacy::setSelectionDetails(const std::string& title, const std::string& desc, const LLUUID& id)
{
	mDetailTitle->setValue(title);
	mDetailDesc->setValue(desc);
	mSnapshotCtrl->setValue(id);
}

void FSFloaterSearchLegacy::processGroupData()
{
	LLGroupMgrGroupData* data = LLGroupMgr::getInstance()->getGroupData(getSelectedID());
	
	if (data)
	{
		setSelectionDetails(LLTrans::getString("LoadingData"), LLSD(data->mCharter), LLSD(data->mInsigniaID));
		delete mGroupPropertiesRequest;
		gCacheName->getGroup(getSelectedID(),
							 boost::bind(&FSFloaterSearchLegacy::groupNameUpdatedCallback,
										 this, _1, _2, _3));

		LLGroupData tmp_data;
		const bool is_member = gAgent.getGroupData(mSelectedID, tmp_data) || gAgent.isGodlike();
		const bool is_open = !is_member && data->mOpenEnrollment;

		childSetEnabled("group_profile_btn", TRUE);
		childSetEnabled("group_chat_btn", is_member);
		childSetEnabled("group_join_btn", is_open);

		setLoadingProgress(FALSE);
	}
}

//virtual
void FSFloaterSearchLegacy::processProperties(void* data, EAvatarProcessorType type)
{
	if (APT_PROPERTIES == type)
	{
		const LLAvatarData *avatar_data = static_cast<const LLAvatarData*>(data);
		if (avatar_data)
		{
			setSelectionDetails(LLTrans::getString("LoadingData"), avatar_data->about_text, avatar_data->image_id);
			LLAvatarNameCache::get(avatar_data->avatar_id,
								   boost::bind(&FSFloaterSearchLegacy::avatarNameUpdatedCallback,
											   this, _1, _2));
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(avatar_data->avatar_id, this);
		}
	}
	if (APT_CLASSIFIED_INFO == type)
	{
		LLAvatarClassifiedInfo* c_info = static_cast<LLAvatarClassifiedInfo*>(data);
		if(c_info && getSelectedID() == c_info->classified_id)
		{
			setSelectionDetails(c_info->name, c_info->description, c_info->snapshot_id);
			mParcelID = c_info->parcel_id;
			mParcelGlobal = c_info->pos_global;
			setLoadingProgress(FALSE);
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(c_info->classified_id, this);
			std::string url = gAgent.getRegionCapability("SearchStatRequest");
			if (!url.empty())
			{
				LL_INFOS() << "Classified stat request via capability" << LL_ENDL;
				LLSD body;
				body["classified_id"] = getSelectedID();
				LLHTTPClient::post(url, body, new LLClassifiedStatsResponder(getSelectedID()));
			}
		}
	}
}

void FSFloaterSearchLegacy::onOpen(const LLSD &params)
{
	const std::string category = params["category"].asString();

	if (category.empty()) {
		return;
	}

	else if (category == "groups") {
		mSearchRadio->setValue(1);
	}

	onModeSelect(mSearchRadio, this);
}

void FSFloaterSearchLegacy::groupNameUpdatedCallback(const LLUUID& id, const std::string& name, bool is_group)
{
	if (id == getSelectedID())
	{
		mDetailTitle->setValue( LLSD(name) );
		setLoadingProgress(FALSE);
	}
	// Otherwise possibly a request for an older inspector, ignore it.
}

void FSFloaterSearchLegacy::avatarNameUpdatedCallback(const LLUUID& id, const LLAvatarName& av_name)
{
	if (id == getSelectedID())
	{
		mDetailTitle->setValue(av_name.getCompleteName());
		setLoadingProgress(FALSE);
	}
	// Otherwise possibly a request for an older inspector, ignore it.
}


void FSFloaterSearchLegacy::sendParcelDetails(const LLVector3d &global_pos,
											  const std::string& name,
											  const std::string& desc,
											  const LLUUID& snapshot_id)
{
	mParcelGlobal = global_pos;
	setSelectionDetails(name, desc, snapshot_id);
	setLoadingProgress(FALSE);
}

void FSFloaterSearchLegacy::setLoadingProgress(const BOOL started)
{
	LLLoadingIndicator *indicator = getChild<LLLoadingIndicator>("loading");
	
	indicator->setVisible(started);
	
	if (started) {
		indicator->start();
	}
	else {
		indicator->stop();
	}
}
					
void FSFloaterSearchLegacy::onSearchTextEntry()
{
	updateSearchEnabled();
}

void FSFloaterSearchLegacy::onSearchFocusChanged(LLFocusableElement *focus)
{
	updateSearchEnabled();
}

void FSFloaterSearchLegacy::updateSearchEnabled()
{
	if (getChild<LLUICtrl>("search_terms")->getValue().asString().length() >= MIN_SEARCH_STRING_SIZE || isLand()) {
		setDefaultBtn("search_btn");
		childSetEnabled("search_btn", TRUE);
	}
	else {
		setDefaultBtn(NULL);
		childSetEnabled("search_btn", FALSE);
	}
}

void FSFloaterSearchLegacy::find()
{
	std::string search_terms = getChild<LLUICtrl>("search_terms")->getValue().asString();
	/// FIXME: This stops the user from sending single-word two queries that
	/// are too small, but they can still search for multiple item queries
	/// too small to search. eg. "to be pi"
	boost::trim(search_terms);

	if (search_terms.empty()) {
		LLButton *search_btn = getChild<LLButton>("search_btn");

		if (search_btn) {
			search_btn->setEnabled(FALSE);
		}
		return;
	}

	if (search_terms.size() < MIN_SEARCH_STRING_SIZE && (!isLand())) {
		getChild<LLScrollListCtrl>("search_results_people")->setCommentText(getString("search_short"));
		return;
	}
	
	setupSearch();
	
	if (mQueryID.notNull())
		mQueryID.setNull();
	mQueryID.generate();
	
	switch(ESearchMode)
	{
		case SM_PEOPLE:
		{
			U32 scope = DFQ_PEOPLE;
			sendSearchQuery(gMessageSystem,
							mQueryID,
							search_terms,
							scope,
							mStartSearch);
			getChild<LLScrollListCtrl>("search_results_people")->deleteAllItems();
			getChild<LLScrollListCtrl>("search_results_people")->setCommentText(getString("searching"));
			setLoadingProgress(TRUE);
			break;
		}
		case SM_GROUPS:
		{
			static LLUICachedControl<bool> inc_pg("ShowPGGroups", true);
			static LLUICachedControl<bool> inc_mature("ShowMatureGroups", false);
			static LLUICachedControl<bool> inc_adult("ShowAdultGroups", false);

			if (!(inc_pg || inc_mature || inc_adult)) {
				gSavedSettings.setBOOL("ShowPGGroups", TRUE);
			}

			U32 scope = DFQ_GROUPS;
			if (inc_pg)
                scope |= DFQ_INC_PG;
			if (inc_mature)
                scope |= DFQ_INC_MATURE;
			if (inc_adult)
                scope |= DFQ_INC_ADULT;
			sendSearchQuery(gMessageSystem,
							 mQueryID,
							 search_terms,
							 scope,
							 mStartSearch);
			getChild<LLScrollListCtrl>("search_results_groups")->deleteAllItems();
			getChild<LLScrollListCtrl>("search_results_groups")->setCommentText(getString("searching"));
			setLoadingProgress(TRUE);
			break;
			
		}
		case SM_PLACES:
		{
			static LLUICachedControl<bool> inc_pg("ShowPGSims", true);
			static LLUICachedControl<bool> inc_mature("ShowMatureSims", false);
			static LLUICachedControl<bool> inc_adult("ShowAdultSims", false);

			if (!(inc_pg || inc_mature || inc_adult)) {
				gSavedSettings.setBOOL("ShowPGSims", TRUE);
			}

			U32 scope = 0;
			if (gAgent.wantsPGOnly())
				scope |= DFQ_PG_SIMS_ONLY;
			bool adult_enabled = gAgent.canAccessAdult();
			bool mature_enabled = gAgent.canAccessMature();
			if (inc_pg)
                scope |= DFQ_INC_PG;
			if (inc_mature && mature_enabled)
                scope |= DFQ_INC_MATURE;
			if (inc_adult && adult_enabled)
                scope |= DFQ_INC_ADULT;
			S8 category = LLParcel::getCategoryFromString(findChild<LLComboBox>("places_category")->getSelectedValue().asString());
			scope |= DFQ_DWELL_SORT;
			sendPlacesSearchQuery(gMessageSystem,
								  mQueryID,
								  search_terms,
								  scope,
								  category,
								  mStartSearch);
			getChild<LLScrollListCtrl>("search_results_places")->deleteAllItems();
			getChild<LLScrollListCtrl>("search_results_places")->setCommentText(getString("searching"));
			setLoadingProgress(TRUE);
			break;
		}
		case SM_LAND:
		{
			static LLUICachedControl<bool> inc_pg("ShowPGLand", true);
			static LLUICachedControl<bool> inc_mature("ShowMatureLand", false);
			static LLUICachedControl<bool> inc_adult("ShowAdultLand", false);
			static LLUICachedControl<bool> limit_price("FindLandPrice", true);
			static LLUICachedControl<bool> limit_area("FindLandArea", true);

			if (!(inc_pg || inc_mature || inc_adult)) {
				gSavedSettings.setBOOL("ShowPGLand", TRUE);
			}
			
			U32 category = ST_ALL;
			const std::string& selection = findChild<LLComboBox>("land_category")->getSelectedValue().asString();
			if (!selection.empty())
			{
				if (selection == "Auction")
					category = ST_AUCTION;
				else if (selection == "Mainland")
					category = ST_MAINLAND;
				else if (selection == "Estate")
					category = ST_ESTATE;
			}
			
			U32 scope = 0;
			if (gAgent.wantsPGOnly())
				scope |= DFQ_PG_SIMS_ONLY;
			bool adult_enabled = gAgent.canAccessAdult();
			bool mature_enabled = gAgent.canAccessMature();
			
			if (inc_pg)
				scope |= DFQ_INC_PG;
			if (inc_mature && mature_enabled)
				scope |= DFQ_INC_MATURE;
			if (inc_adult && adult_enabled)
				scope |= DFQ_INC_ADULT;
			const std::string& sort = findChild<LLComboBox>("land_sort_combo")->getSelectedValue().asString();
			if (!sort.empty())
			{
				if (sort == "Name")
					scope |= DFQ_NAME_SORT;
				else if (sort == "Price")
					scope |= DFQ_PRICE_SORT;
				else if (sort == "PPM")
					scope |= DFQ_PER_METER_SORT;
				else if (sort == "Area")
					scope |= DFQ_AREA_SORT;
			}
			else
			{
				scope |= DFQ_PRICE_SORT;
			}
			if (childGetValue("ascending_check").asBoolean())
				scope |= DFQ_SORT_ASC;
			if (limit_price)
				scope |= DFQ_LIMIT_BY_PRICE;
			if (limit_area)
				scope |= DFQ_LIMIT_BY_AREA;
			S32 price = childGetValue("edit_price").asInteger();
			S32 area = childGetValue("edit_area").asInteger();
			
			sendLandSearchQuery(gMessageSystem,
								mQueryID,
								scope,
								category,
								price,
								area,
								mStartSearch);
			LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("search_results_land");
			search_results->deleteAllItems();
			search_results->setCommentText(getString("searching"));
			setLoadingProgress(TRUE);
			break;
		}
		case SM_EVENTS:
		{
			LL_WARNS() << "Event searching is not implimented yet. Stay tuned!" << LL_ENDL;
			break;
		}
		case SM_CLASSIFIEDS:
		{
			static LLUICachedControl<bool> inc_pg("ShowPGClassifieds", true);
			static LLUICachedControl<bool> inc_mature("ShowMatureClassifieds", false);
			static LLUICachedControl<bool> inc_adult("ShowAdultClassifieds", false);

			if (!(inc_pg || inc_mature || inc_adult)) {
				gSavedSettings.setBOOL("ShowPGClassifieds", TRUE);
			}

			U32 category = childGetValue("classifieds_category").asInteger();
			BOOL auto_renew = FALSE;
			U32 flags = pack_classified_flags_request(auto_renew, inc_pg, inc_mature, inc_adult);
			sendClassifiedsSearchQuery(gMessageSystem,
									  mQueryID,
									  search_terms,
									  flags,
									  category,
									  mStartSearch);
			getChild<LLScrollListCtrl>("search_results_classifieds")->deleteAllItems();
			getChild<LLScrollListCtrl>("search_results_classifieds")->setCommentText(getString("searching"));
			setLoadingProgress(TRUE);
			break;
		}
	}
	mNumResultsReturned = 0;
}

// static
void FSFloaterSearchLegacy::sendSearchQuery(LLMessageSystem* msg,
											const LLUUID& query_id,
											const std::string& text,
											U32 flags,
											S32 query_start)
{
	msg->newMessage("DirFindQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", query_id);
	msg->addString("QueryText", text);
	msg->addU32("QueryFlags", flags);
	msg->addS32("QueryStart", query_start);
	gAgent.sendReliableMessage();
	
	LL_INFOS() << "Firing off search request: " << query_id << LL_ENDL;
}

void FSFloaterSearchLegacy::sendPlacesSearchQuery(LLMessageSystem* msg,
												  const LLUUID& query_id,
												  const std::string& text,
												  U32 flags,
												  S8 category,
												  S32 query_start)
{	
	msg->newMessage("DirPlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", query_id);
	msg->addString("QueryText", text);
	msg->addU32("QueryFlags", flags);
	msg->addS8("Category", category);
	msg->addString("SimName", "");
	msg->addS32("QueryStart", query_start);
	gAgent.sendReliableMessage();
	
	LL_INFOS() << "Firing off places search request: " << query_id << LL_ENDL;
}

void FSFloaterSearchLegacy::sendLandSearchQuery(LLMessageSystem* msg,
												const LLUUID& query_id,
												U32 flags,
												U32 category,
												S32 price,
												S32 area,
												S32 query_start)
{
	msg->newMessage("DirLandQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", query_id);
	msg->addU32("QueryFlags", flags);
	msg->addU32("SearchType", category);
	msg->addS32("Price", price);
	msg->addS32("Area", area);
	msg->addS32("QueryStart", query_start);
	gAgent.sendReliableMessage();
	
	LL_INFOS() << "Firing off places search request: " << query_id << category << LL_ENDL;
}

void FSFloaterSearchLegacy::sendClassifiedsSearchQuery(LLMessageSystem* msg,
													   const LLUUID& query_id,
													   const std::string& text,
													   U32 flags,
													   U32 category,
													   S32 query_start)
{
	msg->newMessageFast(_PREHASH_DirClassifiedQuery);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_QueryData);
	msg->addUUIDFast(_PREHASH_QueryID, query_id );
	msg->addStringFast(_PREHASH_QueryText, text);
	msg->addU32Fast(_PREHASH_QueryFlags, flags);
	msg->addU32Fast(_PREHASH_Category, category);
	msg->addS32Fast(_PREHASH_QueryStart, query_start);
	gAgent.sendReliableMessage();
	
	LL_INFOS() << "Firing off classified ad search request: " << query_id << LL_ENDL;
}

////////////////////////////////////////
//       Process search replies       //
////////////////////////////////////////

// static
void FSFloaterSearchLegacy::processSearchPeopleReply(LLMessageSystem* msg, void**)
{
	LLUUID query_id;
	std::string   first_name;
	std::string   last_name;
	LLUUID agent_id;
	LLUUID  avatar_id;
	//U8 online;
	
	msg->getUUIDFast(_PREHASH_QueryData,	_PREHASH_QueryID,	query_id);
	msg->getUUIDFast(_PREHASH_AgentData,	_PREHASH_AgentID,	agent_id);
	
	// This result is not for us.
	if (agent_id != gAgent.getID()) return;
	LL_DEBUGS() << "received search results - QueryID: " << query_id << " AgentID: " << agent_id << LL_ENDL;
	
	FSFloaterSearchLegacy* self = LLFloaterReg::findTypedInstance<FSFloaterSearchLegacy>("search_legacy");
	// floater is closed or these are not results from our last request
	if (NULL == self || query_id != self->mQueryID)
	{
		return;
	}
	
	LLScrollListCtrl* search_results = self->getChild<LLScrollListCtrl>("search_results_people");
	
	
	if (self->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_short"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_banned"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_disabled"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
	}
	
	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);

	if (!num_new_rows) {
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();

		search_results->setEnabled(FALSE);
		search_results->setCommentText(self->getString("not_found", map));
		self->mResultsReceived = 0;
		self->showResultsCount();
		self->setLoadingProgress(FALSE);

		return;
	}

	self->mResultsReceived += num_new_rows;
	num_new_rows = self->showNextButton(num_new_rows);
	self->showResultsCount();
	
	for (S32 i = 0; i < num_new_rows; i++) {
		msg->getStringFast(	_PREHASH_QueryReplies,	_PREHASH_FirstName,	first_name, i);
		msg->getStringFast(	_PREHASH_QueryReplies,	_PREHASH_LastName,	last_name, i);
		msg->getUUIDFast(	_PREHASH_QueryReplies,	_PREHASH_AgentID,	agent_id, i);
		//msg->getU8Fast(	_PREHASH_QueryReplies,	_PREHASH_Online,	online, i);
		
		if (agent_id.isNull()) {
			LL_DEBUGS() << "No results returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
		}
		else {
			LL_DEBUGS() << "Got: " << first_name << " " << last_name << " AgentID: " << agent_id << LL_ENDL;
			search_results->setEnabled(TRUE);
			found_one = TRUE;
			
			std::string avatar_name;
			avatar_name = LLCacheName::buildFullName(first_name, last_name);
			
			LLSD content;
			LLSD element;
			
			element["id"] = agent_id;
			
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "Generic_Person";
			
			element["columns"][1]["column"]	= "username";
			element["columns"][1]["value"]	= avatar_name;
			
			content["username"] = avatar_name;
			
			search_results->addElement(element, ADD_BOTTOM);
			self->mResultsContent[agent_id.asString()] = content;
		}
	}
	
	if (found_one) {
		search_results->setFocus(TRUE);
	}
	self->setLoadingProgress(FALSE);
}

// static
void FSFloaterSearchLegacy::processSearchGroupsReply(LLMessageSystem* msg, void**)
{
	LLUUID query_id;
	LLUUID group_id;
	LLUUID agent_id;
	std::string group_name;
	S32 members;
	F32 search_order;
	
	msg->getUUIDFast(	_PREHASH_QueryData,	_PREHASH_QueryID,	query_id);
	msg->getUUIDFast(	_PREHASH_AgentData,	_PREHASH_AgentID,	agent_id);
	
	// Not for us
	if (agent_id != gAgent.getID()) return;
	LL_DEBUGS() << "received directory request - QueryID: " << query_id << " AgentID: " << agent_id << LL_ENDL;
	
	FSFloaterSearchLegacy* self = LLFloaterReg::findTypedInstance<FSFloaterSearchLegacy>("search_legacy");
	
	// floater is closed or these are not results from our last request
	if (NULL == self || query_id != self->mQueryID)
	{
		return;
	}
	
	LLScrollListCtrl* search_results = self->getChild<LLScrollListCtrl>("search_results_groups");
	
	// Clear "Searching" label on first results
	if (self->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_short"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_banned"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_disabled"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
	}
	
	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);

	if (!num_new_rows) {
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();

		search_results->setEnabled(FALSE);
		search_results->setCommentText(self->getString("not_found", map));
		self->mResultsReceived = 0;
		self->showResultsCount();
		self->setLoadingProgress(FALSE);

		return;
	}

	self->mResultsReceived += num_new_rows;
	num_new_rows = self->showNextButton(num_new_rows);
	self->showResultsCount();
	
	for (S32 i = 0; i < num_new_rows; i++) {
		msg->getUUIDFast(	_PREHASH_QueryReplies,	_PREHASH_GroupID,		group_id,	i);
		msg->getStringFast(	_PREHASH_QueryReplies,	_PREHASH_GroupName,		group_name,	i);
		msg->getS32Fast(	_PREHASH_QueryReplies,	_PREHASH_Members,		members,	i);
		msg->getF32Fast(	_PREHASH_QueryReplies,	_PREHASH_SearchOrder,	search_order,i);

		if (group_id.isNull()) {
			LL_DEBUGS() << "No results returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
		}
		else {
			LL_DEBUGS() << "Got: " << group_name << " GroupID: " << group_id << LL_ENDL;
			search_results->setEnabled(TRUE);
			found_one = TRUE;
			
			LLSD content;
			LLSD element;
			
			element["id"] = group_id;
			
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "Generic_Group";
			
			element["columns"][1]["column"]	= "group_name";
			element["columns"][1]["value"]	= group_name;
			
			element["columns"][2]["column"]	= "members";
			element["columns"][2]["value"]	= members;
			
			content["group_name"] = group_name;
			
			search_results->addElement(element, ADD_BOTTOM);
			self->mResultsContent[group_id.asString()] = content;
		}
	}
	
	if (found_one) {
		search_results->setFocus(TRUE);
	}
	self->setLoadingProgress(FALSE);
}

// static
void FSFloaterSearchLegacy::processSearchPlacesReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	parcel_id;
	std::string	name;
	BOOL	is_for_sale;
	BOOL	is_auction;
	F32		dwell;
	
	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("QueryData", "QueryID", query_id);
	
	// Not for us
	if (agent_id != gAgent.getID()) return;
	LL_DEBUGS() << "received directory request - QueryID: " << query_id << " AgentID: " << agent_id << LL_ENDL;
	
	FSFloaterSearchLegacy* self = LLFloaterReg::findTypedInstance<FSFloaterSearchLegacy>("search_legacy");
	// floater is closed or these are not results from our last request
	if (NULL == self || query_id != self->mQueryID)
	{
		return;
	}
	
	LLScrollListCtrl* search_results = self->getChild<LLScrollListCtrl>("search_results_places");
	
	// Clear "Searching" label on first results
	if (self->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_short"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_banned"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_disabled"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
	}
	
	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);

	if (!num_new_rows) {
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();

		search_results->setEnabled(FALSE);
		search_results->setCommentText(self->getString("not_found", map));
		self->mResultsReceived = 0;
		self->showResultsCount();
		self->setLoadingProgress(FALSE);

		return;
	}

	self->mResultsReceived += num_new_rows;
	num_new_rows = self->showNextButton(num_new_rows);
	self->showResultsCount();
	
	for (S32 i = 0; i < num_new_rows; i++) {
		msg->getUUID(	"QueryReplies",	"ParcelID",	parcel_id,	i);
		msg->getString(	"QueryReplies",	"Name",		name,		i);
		msg->getBOOL(	"QueryReplies",	"ForSale",	is_for_sale,i);
		msg->getBOOL(	"QueryReplies",	"Auction",	is_auction,	i);
		msg->getF32(	"QueryReplies",	"Dwell",	dwell,		i);
		if (parcel_id.isNull()) {
			LL_DEBUGS() << "Null result returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
		}
		else {
			LL_DEBUGS() << "Got: " << name << " ParcelID: " << parcel_id << LL_ENDL;
			search_results->setEnabled(TRUE);
			found_one = TRUE;
			
			std::string place_name;
			
			LLSD content;
			LLSD element;
			
			element["id"] = parcel_id;
			
			if (is_auction) {
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_Auction";
			}
			else if (is_for_sale) {
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_For_Sale";
			}
			else {
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Inv_Landmark";
			}
			
			element["columns"][1]["column"]	= "place_name";
			element["columns"][1]["value"]	= name;

			content["place_name"] = name;
			
			std::string buffer = llformat("%.0f", (F64)dwell);
			element["columns"][2]["column"]	= "dwell";
			element["columns"][2]["value"]	= buffer;
			
			search_results->addElement(element, ADD_BOTTOM);
			self->mResultsContent[parcel_id.asString()] = content;
		}
	}
	
	if (found_one) {
		search_results->setFocus(TRUE);
	}
	self->setLoadingProgress(FALSE);
}

// static
void FSFloaterSearchLegacy::processSearchLandReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	parcel_id;
	std::string	name;
	std::string land_sku;
	std::string land_type;
	BOOL	is_auction;
	BOOL	is_for_sale;
	S32		price;
	S32		area;
	
	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("QueryData", "QueryID", query_id);
	
	// Not for us
	if (agent_id != gAgent.getID()) return;
	LL_INFOS() << "received directory request - QueryID: " << query_id << " AgentID: " << agent_id << LL_ENDL;
	
	FSFloaterSearchLegacy* self = LLFloaterReg::findTypedInstance<FSFloaterSearchLegacy>("search_legacy");
	// floater is closed or these are not results from our last request
	if (NULL == self || query_id != self->mQueryID)
	{
		return;
	}
	
	LLScrollListCtrl* search_results = self->getChild<LLScrollListCtrl>("search_results_land");
	// clear "Searching" label on first results
	if (self->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}
	
	static LLUICachedControl<bool> use_price("FindLandPrice", true);
	static LLUICachedControl<bool> use_area("FindLandArea", true);

	S32 limit_price = self->childGetValue("price_edit").asInteger();
	S32 limit_area = self->childGetValue("area_edit").asInteger();
	
	S32 for_sale = 0;
	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);

	if (!num_new_rows) {
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();

		search_results->setEnabled(FALSE);
		search_results->setCommentText(self->getString("not_found", map));
		self->mResultsReceived = 0;
		self->showResultsCount();
		self->setLoadingProgress(FALSE);

		return;
	}

	self->mResultsReceived += num_new_rows;
	num_new_rows = self->showNextButton(num_new_rows);
	self->showResultsCount();

	for (S32 i = 0; i < num_new_rows; i++) {
		msg->getUUID(	"QueryReplies", "ParcelID",		parcel_id,	i);
		msg->getString(	"QueryReplies", "Name",			name,		i);
		msg->getBOOL(	"QueryReplies", "Auction",		is_auction,	i);
		msg->getBOOL(	"QueryReplies", "ForSale",		is_for_sale,i);
		msg->getS32(	"QueryReplies", "SalePrice",	price,		i);
		msg->getS32(	"QueryReplies", "ActualArea",	area,		i);
		
		if ( msg->getSizeFast(_PREHASH_QueryReplies, i, _PREHASH_ProductSKU) > 0 ) {
			msg->getStringFast(	_PREHASH_QueryReplies, _PREHASH_ProductSKU, land_sku, i);
			land_type = LLProductInfoRequestManager::instance().getDescriptionForSku(land_sku);
		}
		else {
			land_sku.clear();
			land_type = LLTrans::getString("land_type_unknown");
		}

		if (parcel_id.isNull())
			continue;

		if (use_price && (price > limit_price))
			continue;

		if (use_area && (area < limit_area))
			continue;
		
		LLSD content;
		LLSD element;
		
		element["id"] = parcel_id;
		if (is_auction) {
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "Icon_Auction";
		}
		else if (is_for_sale) {
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "Icon_For_Sale";
		}
		else {
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "Icon_Place";
		}
		
		element["columns"][1]["column"]	= "land_name";
		element["columns"][1]["value"]	= name;
		
		content["place_name"] = name;
		
		std::string buffer = "Auction";
		if (!is_auction) {
			buffer = llformat("%d", price);
			for_sale++;
		}
		element["columns"][2]["column"]	= "price";
		element["columns"][2]["value"]	= price;
		
		element["columns"][3]["column"]	= "area";
		element["columns"][3]["value"]	= area;
		if (!is_auction) {
			F32 ppm;
			if (area > 0)
				ppm = (F32)price / (F32)area;
			else
				ppm = 0.f;
			std::string buffer = llformat("%.1f", ppm);
			element["columns"][4]["column"]	= "ppm";
			element["columns"][4]["value"]	= buffer;
		}
		else {
			element["columns"][4]["column"]	= "ppm";
			element["columns"][4]["column"]	= "1.0";
		}
		
		element["columns"][5]["column"]	= "land_type";
		element["columns"][5]["value"]	= land_type;
		
		search_results->addElement(element, ADD_BOTTOM);
		self->mResultsContent[parcel_id.asString()] = content;
	}
	// We test against non-auction properties because they don't count towards the page limit.
	self->showNextButton(for_sale);
	
	if (found_one) {
		search_results->setFocus(TRUE);
	}
	self->setLoadingProgress(FALSE);
}

// static
void FSFloaterSearchLegacy::processSearchEventsReply(LLMessageSystem* msg, void**)
{
	LL_WARNS() << "Received an event search reply, but we don't handle these yet!" << LL_ENDL;
}

// static
void FSFloaterSearchLegacy::processSearchClassifiedsReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	classified_id;
	std::string name;
	U32		creation_date;
	U32		expiration_date;
	S32		price_for_listing;
	
	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("QueryData", "QueryID", query_id);
	
	// Not for us
	if (agent_id != gAgent.getID()) return;
	LL_DEBUGS() << "received directory request - QueryID: " << query_id << " AgentID: " << agent_id << LL_ENDL;
	
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_CLASSIFIEDS_BANNEDWORD)
		{
			LLNotificationsUtil::add("SearchWordBanned");
		}
	}
	
	FSFloaterSearchLegacy* self = LLFloaterReg::findTypedInstance<FSFloaterSearchLegacy>("search_legacy");
	// floater is closed or these are not results from our last request
	if (NULL == self || query_id != self->mQueryID)
	{
		return;
	}
	
	LLScrollListCtrl* search_results = self->getChild<LLScrollListCtrl>("search_results_classifieds");
	
	// Clear "Searching" label on first results
	if (self->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_short"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_banned"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("search_disabled"));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
			return;
		}
	}
	
	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);

	if (!num_new_rows) {
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();

		search_results->setEnabled(FALSE);
		search_results->setCommentText(self->getString("not_found", map));
		self->mResultsReceived = 0;
		self->showResultsCount();
		self->setLoadingProgress(FALSE);

		return;
	}

	self->mResultsReceived += num_new_rows;
	num_new_rows = self->showNextButton(num_new_rows);
	self->showResultsCount();
	
	for (S32 i = 0; i < num_new_rows; i++) {
		msg->getUUID(	"QueryReplies", "ClassifiedID",		classified_id,	i);
		msg->getString(	"QueryReplies", "Name",				name,			i);
		msg->getU32(	"QueryReplies", "CreationDate",		creation_date,	i);
		msg->getU32(	"QueryReplies", "ExpirationDate",	expiration_date,i);
		msg->getS32(	"QueryReplies", "PriceForListing",	price_for_listing,i);
		if (classified_id.isNull()) {
			LL_DEBUGS() << "Null result returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("search_terms")->getValue().asString();
			search_results->setEnabled(FALSE);
			search_results->setCommentText(self->getString("not_found", map));
			self->mResultsReceived = 0;
			self->showResultsCount();
			self->setLoadingProgress(FALSE);
		}
		else {
			LL_DEBUGS() << "Got: " << name << " ClassifiedID: " << classified_id << LL_ENDL;
			search_results->setEnabled(TRUE);
			found_one = TRUE;
			
			std::string classified_name;
			
			LLSD content;
			LLSD element;
			
			element["id"] = classified_id;
			
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "icon_top_pick.tga";
			
			element["columns"][1]["column"]	= "classified_name";
			element["columns"][1]["value"]	= name;
			
			element["columns"][2]["column"]	= "price";
			element["columns"][2]["value"]	= price_for_listing;
			
			search_results->addElement(element, ADD_BOTTOM);
			self->mResultsContent[classified_id.asString()] = content;
		}
	}

	if (found_one) {
		search_results->setFocus(TRUE);
	}
	self->setLoadingProgress(FALSE);
}

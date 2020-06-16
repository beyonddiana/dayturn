/** 
 * @file fsareasearch.cpp
 * @brief Search the local area for objects.
 *
 * Copyright (c) 2013 Jessica Wabbit
 * Portions copyright (c) 2013 Linden Research, Inc.
 * Portions copyright (c) 2013 Various Firestorm viewer contributors
 * Portions copyright (c) 2009 Modular Systems Ltd
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * ----------------------------------------------------------------------------
 *
 * This source code is licenced under the terms of the LGPL version 2.1,
 * detailed above.  This file contains portions of source code provided
 * under a BSD-style licence, and the following text is included to comply
 * with the terms of that original licence:
 *
 * Copyright (c) 2009
 *
 * Modular Systems Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met IN ADDITION TO THE TERMS OF THE LGPL v2.1:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modified, debugged, optimized and improved by Henri Beauchamp Feb 2010.
 * Refactored for Viewer2 by Kadah Coba, April 2011
 * Rewritten for viewer 3 by Jessica Wabbit, November 2013
 */

#include "llviewerprecompiledheaders.h"

#include "fsareasearch.h"

#include "llagent.h"
#include "llblocklist.h"
#include "llcallbacklist.h"
#include "llcheckboxctrl.h"
#include "llfiltereditor.h"
#include "llfloaterreg.h"
#include "llfloaterreporter.h"
#include "llmenubutton.h"
#include "llmutelist.h"
#include "llpanelblockedlist.h"
#include "llprogressbar.h"
#include "lltextbox.h"
#include "lltoggleablemenu.h"
#include "lltoolgrab.h"
#include "lltracker.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"

#include <boost/algorithm/string/find.hpp>	//for boost::ifind_first

#define MAX_OBJECTS_PER_PACKET	255	// max number of objects that can be (de)selected in a single request
#define REFRESH_INTERVAL	1.0f	// seconds between refreshes when active
#define MIN_REFRESH_INTERVAL	0.25f	// minimum seconds between list refreshes
#define REQUEST_TIMEOUT		30.0f	// time to resend object properties request

class FSAreaSearch::FSParcelChangeObserver : public LLParcelObserver
{
public:
	FSParcelChangeObserver(FSAreaSearch *area_search_floater) : mAreaSearchFloater(area_search_floater) {}

private:
	/*virtual*/ void changed()
	{
		if (mAreaSearchFloater) {
			mAreaSearchFloater->checkRegion();
		}
	}

	FSAreaSearch *mAreaSearchFloater;
};

FSAreaSearch::FSAreaSearch(const LLSD& key) :  
	LLFloater(key),
	mActive(false),
	mFilterName(NULL),
	mFilterDescription(NULL),
	mFilterOwner(NULL),
	mFilterGroup(NULL),
	mCheckboxPhysical(NULL),
	mCheckboxTemporary(NULL),
	mCheckboxAttachment(NULL),
	mCheckboxOther(NULL),
	mResultsList(NULL),
	mStatusBarText(NULL),
	mStatusBarProgress(NULL),
	mRequestQueuePause(false),
	mRequestRequired(false),
	mOptionsMenu(NULL),
	mAutoTrackSelections(false)
{
	mLastRegion = gAgent.getRegion();
	mOptionsMenu = new OptionsMenu(this);

	gIdleCallbacks.addFunction(callbackIdle, this);

	mParcelChangedObserver = new FSParcelChangeObserver(this);
	LLViewerParcelMgr::getInstance()->addObserver(mParcelChangedObserver);

	mLastUpdateTimer.reset();
}

FSAreaSearch::~FSAreaSearch()
{
	delete mOptionsMenu;

	if (!gIdleCallbacks.deleteFunction(callbackIdle, this)) {
		LL_WARNS("FSAreaSearch") << "FSAreaSearch::~FSAreaSearch() failed to delete idle callback" << LL_ENDL;
	}

	if (mParcelChangedObserver)
	{
		LLViewerParcelMgr::getInstance()->removeObserver(mParcelChangedObserver);
		delete mParcelChangedObserver;
	}
}

//static
void FSAreaSearch::callbackIdle(void *user_data)
{
	FSAreaSearch *self = (FSAreaSearch *)user_data;

	self->findObjects();
	self->processRequestQueue();
}

bool FSAreaSearch::postBuild()
{
	mFilterName = getChild<LLFilterEditor>("name_filter");
	mFilterDescription = getChild<LLFilterEditor>("description_filter");
	mFilterOwner = getChild<LLFilterEditor>("owner_filter");
	mFilterGroup = getChild<LLFilterEditor>("group_filter");

	if (!mFilterName || !mFilterDescription || !mFilterOwner || !mFilterGroup) {
		LL_WARNS("FSAreaSearch") << "One or more of the required filter controls are missing" << LL_ENDL;
		return false;
	}

	mFilterName->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));
	mFilterDescription->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));
	mFilterOwner->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));
	mFilterGroup->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));

	mCheckboxPhysical = getChild<LLCheckBoxCtrl>("physical_checkbox");
	mCheckboxTemporary = getChild<LLCheckBoxCtrl>("temporary_checkbox");
	mCheckboxAttachment = getChild<LLCheckBoxCtrl>("attachments_checkbox");
	mCheckboxOther = getChild<LLCheckBoxCtrl>("others_checkbox");

	if (!mCheckboxPhysical || !mCheckboxTemporary || !mCheckboxAttachment || !mCheckboxOther) {
		LL_WARNS("FSAreaSearch") << "One or more of the required checkbox controls are missing" << LL_ENDL;
		return false;
	}

	mCheckboxPhysical->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));
	mCheckboxTemporary->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));
	mCheckboxAttachment->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));
	mCheckboxOther->setCommitCallback(boost::bind(&FSAreaSearch::refreshList, this, false));

	childSetAction("refresh_btn", boost::bind(&FSAreaSearch::refreshList, this, true));

	mStatusBarText = getChild<LLTextBox>("status_text");
	mStatusBarProgress = getChild<LLProgressBar>("status_progress");

	if (!mStatusBarText || !mStatusBarProgress) {
		LL_WARNS("FSAreaSearch") << "One or more of the required status controls are missing" << LL_ENDL;
		return false;
	}

	mStatusBarText->setVisible(FALSE);
	mStatusBarProgress->setVisible(FALSE);

	if (!mOptionsMenu) {
		LL_WARNS("FSAreaSearch") << "The options menu is missing" << LL_ENDL;
		return false;
	}

	LLMenuButton *menu_btn = getChild<LLMenuButton>("options_menu_btn");

	if (!menu_btn) {
		LL_WARNS("FSAreaSearch") << "Couldn't find the options_menu_btn control" << LL_ENDL;
		return false;
	}
	menu_btn->setMenu(mOptionsMenu->getMenu());

	mResultsList = getChild<LLScrollListCtrl>("result_list");

	if (!mResultsList) {
		LL_WARNS("FSAreaSearch") << "Couldn't find the result_list control" << LL_ENDL;
		return false;
	}

	mResultsList->setCommitCallback(boost::bind(&FSAreaSearch::onSelectRow, this));
	mResultsList->setDoubleClickCallback(boost::bind(&FSAreaSearch::onDoubleClick, this));
	mResultsList->setRightMouseDownCallback(boost::bind(&FSAreaSearch::onRightClick, this, _2, _3));
	mResultsList->sortByColumn("name_column", TRUE);

	refreshList(true);
	return true;
}

void FSAreaSearch::checkRegion()
{
	if (!mActive) {
		return;
	}

	//
	//	check if we changed region
	//	if we did then clear the object details cache
	//
	LLViewerRegion *region = gAgent.getRegion();

	if (!region) {
		LL_WARNS("FSAreaSearch") << "region is NULL!" << LL_ENDL;
		return;
	}

	if (region == mLastRegion) {
		//
		//	we haven't changed region
		//	(nothing more to be done here)
		//
		return;
	}

	std::vector<LLViewerRegion*> uniqueRegions;
	region->getNeighboringRegions(uniqueRegions);

	if (std::find(uniqueRegions.begin(), uniqueRegions.end(), mLastRegion) != uniqueRegions.end()) {
		//
		//	we crossed into a neighboring region
		//	so there's no need to clear everything
		//
		mLastRegion = region;
	}
	else {
		//
		//	we teleported into a new region
		//
		mLastRegion = region;
		mRequested = 0;
		mObjectDetails.clear();
		mRegionRequests.clear();
		mLastProptiesRecievedTimer.start();
		mResultsList->deleteAllItems();
		mStatusBarText->setVisible(FALSE);
		mRefresh = true;
	}
}

void FSAreaSearch::refreshList(const bool cache_clear)
{
	mActive = true;
	checkRegion();

	if (cache_clear) {
		mRequested = 0;
		mObjectDetails.clear();
		mRegionRequests.clear();
		mLastProptiesRecievedTimer.start();
		mNamesRequested.clear();
	}
	else {
		std::map<LLUUID, ObjectDetails>::iterator iter = mObjectDetails.begin();
		std::map<LLUUID, ObjectDetails>::iterator iter_end = mObjectDetails.end();

		for (; iter != iter_end; iter++) {
			iter->second.listed = false;
		}
	}

	mResultsList->deleteAllItems();
	mRefresh = true;

	updateStatusBar();
	findObjects();
}

void FSAreaSearch::findObjects()
{
	//
	//	only loop through the gObjectList every so often
	//	(there's a performance hit if it's done too often)
	//	
	if (!(mActive && (
		(mRefresh && mLastUpdateTimer.getElapsedTimeF32() > MIN_REFRESH_INTERVAL) ||
		mLastUpdateTimer.getElapsedTimeF32() > REFRESH_INTERVAL
	))) {
		return;
	}
	
	LLViewerRegion *region = gAgent.getRegion();

	if (!region) {
		//
		//	we got disconnected or are in the middle of a teleport
		//
		return;
	}
	
	mLastUpdateTimer.stop();
	mRequestQueuePause = true;

	checkRegion();

	mRefresh = false;
	mSearchableObjects = 0;
	S32 object_count = gObjectList.getNumObjects();

	for (S32 i = 0; i < object_count; i++) {
		LLViewerObject *object = gObjectList.getObject(i);

		if (!object || !isSearchableObject(object, region)) {
			continue;
		}

		LLUUID object_id = object->getID();

		if (object_id.isNull()) {
			LL_WARNS("FSAreaSearch") << "Found a selectable object with a NULL UUID!" << LL_ENDL;
			continue;
		}

		mSearchableObjects++;
		
		if (mObjectDetails.count(object_id) == 0) {
			ObjectDetails &details = mObjectDetails[object_id];

			details.id = object_id;
			details.local_id = object->getLocalID();
			details.region_handle = object->getRegion()->getHandle();

			mRequestRequired = true;
			mRequested++;
		}
		else {
			ObjectDetails &details = mObjectDetails[object_id];

			if (details.request == ObjectDetails::FINISHED) {
				matchObject(details, object);
			}

			if (details.request == ObjectDetails::FAILED) {
				//
				//	object came back into view
				//
				details.request = ObjectDetails::NEED;
				details.local_id = object->getLocalID();
				details.region_handle = object->getRegion()->getHandle();

				mRequestRequired = true;
				mRequested++;
			}
		}
	}

	updateScrollList();

	S32 request_count = 0;

	std::map<LLUUID, ObjectDetails>::iterator iter = mObjectDetails.begin();
	std::map<LLUUID, ObjectDetails>::iterator iter_end = mObjectDetails.end();

	//
	//	requests for non-existent objects will never arrive
	//	check and update the queue
	//
	for (; iter != iter_end; iter++) {
		if (
			iter->second.request == ObjectDetails::NEED ||
			iter->second.request == ObjectDetails::SENT
		) {
			LLUUID id = iter->second.id;
			LLViewerObject *object = gObjectList.findObject(id);

			if (!object) {
				iter->second.request = ObjectDetails::FAILED;
				mRequested--;
			}
			else {
				request_count++;
			}
		}
	}
	
	if (mRequested != request_count) {
		mRequested = request_count;
	}

	updateStatusBar();
	mLastUpdateTimer.start();
	mRequestQueuePause = false;
}

void FSAreaSearch::updateStatusBar()
{
	if (mSearchableObjects > 0) {
		if (mRequested > 0) {
			LLStringUtil::format_map_t args;
			args["PENDING"] = llformat("%d", mRequested);

			mStatusBarText->setTextArg("[PENDING]", getString("pending_string", args));
		}
		else {
			mStatusBarText->setTextArg("[PENDING]", LLStringExplicit(""));
		}

		mStatusBarText->setTextArg("[LISTED]", llformat("%d", mResultsList->getItemCount()));
		mStatusBarText->setVisible(TRUE);

		if (mRequested > 0) {
			mStatusBarProgress->setValue(((mSearchableObjects - mRequested) * 100) / mSearchableObjects);
			mStatusBarProgress->setVisible(TRUE);
		}
		else {
			mStatusBarProgress->setVisible(FALSE);
		}
	}
	else {
		mStatusBarText->setVisible(FALSE);
		mStatusBarProgress->setVisible(FALSE);
	}
}

void FSAreaSearch::onSelectRow()
{
	if (mAutoTrackSelections) {
		onDoubleClick();
	}
}

void FSAreaSearch::onDoubleClick()
{
 	LLScrollListItem *item = getFirstSelectedResult();

	if (item) {
		LLViewerObject *object = gObjectList.findObject(item->getUUID());

		if (object) {
			LLTracker::trackLocation(
				object->getPositionGlobal(),
				mObjectDetails[item->getUUID()].name,
				"",
				LLTracker::LOCATION_ITEM
			);
		}
	}
}

void FSAreaSearch::onRightClick(S32 x, S32 y)
{
	LLScrollListItem *item = mResultsList->hitItem(x, y);

	if (!item) {
		LL_WARNS("FSAreaSearch") << "item is NULL!" << LL_ENDL;
		return;
	}

	LLUUID id = item->getUUID();

	if (id.notNull()) {
		uuid_vec_t uuids;
		uuids.push_back(id);

		mResultsList->selectByID(id);

		LLViewerObject *object = gObjectList.findObject(id);

		if (object) {
			if (mAutoTrackSelections) {
				LLTracker::trackLocation(
					object->getPositionGlobal(),
					mObjectDetails[id].name,
					"",
					LLTracker::LOCATION_ITEM
				);
			}

			LLSelectMgr::getInstance()->deselectAll();
			mSelectionHandle = LLSelectMgr::getInstance()->selectObjectAndFamily(object);
		}

		ContextMenu::instance().show(mResultsList, uuids, x, y);
	}
}

void FSAreaSearch::processRequestQueue()
{
	if (!mActive || mRequestQueuePause) {
	      return;
	}

	if (mLastProptiesRecievedTimer.getElapsedTimeF32() > REQUEST_TIMEOUT) {
		S32 request_count = 0;
		S32 failed_count = 0;

		std::map<LLUUID, ObjectDetails>::iterator iter = mObjectDetails.begin();
		std::map<LLUUID, ObjectDetails>::iterator iter_end = mObjectDetails.end();

		for (; iter != iter_end; iter++) {
			if (iter->second.request == ObjectDetails::SENT) {
				iter->second.request = ObjectDetails::NEED;

				mRequestRequired = true;
				request_count++;
			}
			
			if (iter->second.request == ObjectDetails::FAILED) {
				failed_count++;
			}
		}

		mRegionRequests.clear();
		mLastProptiesRecievedTimer.start();
	}

	if (!mRequestRequired) {
	      return;
	}

	mRequestRequired = false;
	
	LLWorld::region_list_t::const_iterator region_iter = LLWorld::getInstance()->getRegionList().begin();
	LLWorld::region_list_t::const_iterator region_iter_end = LLWorld::getInstance()->getRegionList().end();

	for (; region_iter != region_iter_end; region_iter++) {
		LLViewerRegion *region = *region_iter;

		if (!region) {
			continue;
		}

		U64 region_handle = region->getHandle();

		if (mRegionRequests[region_handle] > MAX_OBJECTS_PER_PACKET + 128) {
			mRequestRequired = true;
			return;
		}
		
		std::vector<U32> request_list;
		std::map<LLUUID, ObjectDetails>::iterator object_iter = mObjectDetails.begin();
		std::map<LLUUID, ObjectDetails>::iterator object_iter_end = mObjectDetails.end();

		for (; object_iter != object_iter_end; object_iter++) {
			if (
				object_iter->second.request != ObjectDetails::NEED ||
				object_iter->second.region_handle != region_handle
			) {
				continue;
			}

			request_list.push_back(object_iter->second.local_id);
			object_iter->second.request = ObjectDetails::SENT;

			mRegionRequests[region_handle]++;

			if (mRegionRequests[region_handle] >= (MAX_OBJECTS_PER_PACKET * 3) - 3) {
				mRequestRequired = true;
				break;
			}
		}

		if (!request_list.empty()) {
			requestObjectProperties(request_list, true, region);
			requestObjectProperties(request_list, false, region);
		}
	}
}

void FSAreaSearch::requestObjectProperties(const std::vector<U32> &request_list, const bool select, LLViewerRegion *region)
{
	if (!region) {
		LL_WARNS("FSAreaSearch") << "region is NULL!" << LL_ENDL;
		return;
	}

	LLHost host = region->getHost();
	bool start_new_message = true;
	S32 select_count = 0;

	std::vector<U32>::const_iterator iter = request_list.begin();
	std::vector<U32>::const_iterator iter_end = request_list.end();
	
	for (; iter != iter_end; iter++) {
		if (start_new_message) {
			if (select) {
				gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
			}
			else {
				gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
			}

			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

			select_count++;
			start_new_message = false;
		}
		
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, *iter);
		select_count++;
		
		if (gMessageSystem->isSendFull(NULL) || select_count >= MAX_OBJECTS_PER_PACKET) {
			gMessageSystem->sendReliable(host);

			select_count = 0;
			start_new_message = true;
		}
	}

	if (!start_new_message) {
		gMessageSystem->sendReliable(host);
	}
}

void FSAreaSearch::processObjectProperties(LLMessageSystem *msg)
{
	if (!mActive) {
		return;
	}

	LLViewerRegion *region = gAgent.getRegion();

	if (!region) {
		LL_WARNS("FSAreaSearch") << "region is NULL!" << LL_ENDL;
		return;
	}

	bool counter_text_update = false;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_ObjectData);

	for (S32 i = 0; i < count; i++) {
		LLUUID object_id;

		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, object_id, i);

		if (object_id.isNull()) {
			LL_WARNS("FSAreaSearch") << "Got Object Properties with NULL id" << LL_ENDL;
			continue;
		}

		LLViewerObject *object = gObjectList.findObject(object_id);
		if (!object) {
			continue;
		}

		ObjectDetails &details = mObjectDetails[object_id];

		if (details.request != ObjectDetails::FINISHED) {
			details.request = ObjectDetails::FINISHED;
			mLastProptiesRecievedTimer.start();

			if (details.id.isNull()) {
				//
				//	recieved object properties we didn't request
				//
				details.id = object_id;
			}
			else {
				if (mRequested > 0) {
					mRequested--;
				}

				mRegionRequests[details.region_handle]--;
				counter_text_update = true;
			}

			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, details.owner_id, i);
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID, details.group_id, i);

			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, details.name, i);
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, details.description, i);
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_TouchName, details.touch_name, i);
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_SitName, details.sit_name, i);
			
			if (isSearchableObject(object, region)) {
				matchObject(details, object);
			}
		}
	}

	if (counter_text_update) {
		updateStatusBar();
	}
}

void FSAreaSearch::matchObject(ObjectDetails &details, const LLViewerObject *object)
{
	//
	//	check if the object is already shown in the scroll list
	//
	if (details.listed) {
		return;
	}

	//
	//	check the object against the object type checkboxes
	//	on the floater to see if we want this object type
	//
	if (!wantObjectType(object)) {
		return;
	}
	
	//
	//	check the object against the user-entered
	//	filter strings (if any)
	//
	std::string owner_name;
	std::string group_name;
	std::string object_name = details.name;
	std::string object_description = details.description;

	details.name_requested = false;
	getNameFromUUID(details.owner_id, owner_name, false, details.name_requested);
	getNameFromUUID(details.group_id, group_name, true, details.name_requested);

	std::string filter_name = mFilterName->getText();
	std::string filter_description = mFilterDescription->getText();
	std::string filter_owner = mFilterOwner->getText();
	std::string filter_group = mFilterGroup->getText();

	if (
		(!filter_name.empty() && boost::ifind_first(object_name, filter_name).empty()) ||
		(!filter_description.empty() && boost::ifind_first(object_description, filter_description).empty()) ||
		(!filter_owner.empty() && boost::ifind_first(owner_name, filter_owner).empty()) ||
		(!filter_group.empty() && boost::ifind_first(group_name, filter_group).empty())
	) {
		return;
	}

	//
	//	add this object to the results list
	//
	LLScrollListItem::Params row_params;
	LLScrollListCell::Params cell_params;

	row_params.value = details.id.asString();
	
	cell_params.column = "name_column";
	cell_params.value = details.name;
	row_params.columns.add(cell_params);

	cell_params.column = "description_column";
	cell_params.value = details.description;
	row_params.columns.add(cell_params);

	cell_params.column = "owner_column";
	cell_params.value = owner_name;
	row_params.columns.add(cell_params);

	cell_params.column = "group_column";
	cell_params.value = group_name;
	row_params.columns.add(cell_params);

	mResultsList->addRow(row_params);

	details.listed = true;
}

void FSAreaSearch::getNameFromUUID(LLUUID &id, std::string &name, const BOOL is_group, bool &name_requested)
{
	BOOL have_name;

	if (is_group) {
		have_name = gCacheName->getGroupName(id, name);
	}
	else {
		have_name = gCacheName->getFullName(id, name);
	}
	
	if (!have_name && !name_requested) {
		if (std::find(mNamesRequested.begin(), mNamesRequested.end(), id) == mNamesRequested.end()) {
			mNamesRequested.push_back(id);

			gCacheName->get(id, is_group, boost::bind(&FSAreaSearch::callbackLoadFullName, this, _1, _2));
		}

		name_requested = true;
	}
}

void FSAreaSearch::callbackLoadFullName(const LLUUID &id, const std::string &full_name)
{
	LLViewerRegion *region = gAgent.getRegion();

	if (!region) {
		LL_WARNS("FSAreaSearch") << "region is NULL!" << LL_ENDL;
		return;
	}

	std::map<LLUUID, ObjectDetails>::iterator iter = mObjectDetails.begin();
	std::map<LLUUID, ObjectDetails>::iterator iter_end = mObjectDetails.end();
	
	for (; iter != iter_end; iter++) {
		if (iter->second.name_requested && !iter->second.listed) {
			LLViewerObject *object = gObjectList.findObject(iter->second.id);

			if (object && isSearchableObject(object, region)) {
				matchObject(iter->second, object);
			}
		}
	}
  
	updateName(id, full_name);
}

bool FSAreaSearch::isSearchableObject(LLViewerObject *object, LLViewerRegion *region)
{
	//
	//	a quick sanity check before we go any further
	//
	if (!object || !region) {
		LL_WARNS("FSAreaSearch") << "Sanity check failed!" << LL_ENDL;
		return false;
	}

	//
	//	we need to be in the same region as the object
	//	and the object cannot be the land itself
	//
	if (
		!object->getRegion() ||
		object->getRegion() != region ||
		object->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH
	) {
		return false;
	}

	//
	//	the object needs to be selectable and cannot be an avatar
	//
	if (!object->mbCanSelect || object->isAvatar()) {
		return false;
	}
	
	//
	//	exclude child prims, whether rezzed or attached to an avatar
	//
	if (!(object->isRoot() || (object->isAttachment() && object->isRootEdit()))) {
		return false;
	}

	//
	//	check the object against the option checkboxes
	//
	return wantObjectType(object);
}

bool FSAreaSearch::wantObjectType(const LLViewerObject *object)
{
	if (!object) {
		LL_WARNS("FSAreaSearch") << "object is NULL!" << LL_ENDL;
		return false;
	}

	bool is_attachment = object->isAttachment();
	bool is_physical = object->flagUsePhysics();
	bool is_temporary = object->flagTemporaryOnRez();
	bool is_other = !(is_attachment || is_physical || is_temporary);

	//
	//	only show objects if checkboxes are ticked for their type
	//
	if (
		(is_attachment && !mCheckboxAttachment->getValue().asBoolean()) ||
		(is_physical && !mCheckboxPhysical->getValue().asBoolean()) ||
		(is_temporary && !mCheckboxTemporary->getValue().asBoolean()) ||
		(is_other && !mCheckboxOther->getValue().asBoolean())
	) {
		return false;
	}

	return true;
}

void FSAreaSearch::updateScrollList()
{
	LLViewerRegion *region = gAgent.getRegion();

	if (!region) {
		LL_WARNS("FSAreaSearch") << "region is NULL!" << LL_ENDL;
		return;
	}

	bool do_update = false;

	//
	//	iterate over the rows in the list
	//	deleting objects that have gone away
	//
	std::vector<LLScrollListItem*> items = mResultsList->getAllData();
	std::vector<LLScrollListItem*>::iterator iter = items.begin();
	std::vector<LLScrollListItem*>::iterator iter_end = items.end();

	for (; iter != iter_end; iter++) {
		LLScrollListItem *item = *iter;

		if (!item) {
			continue;
		}

		LLUUID row_id = item->getUUID();
		LLViewerObject *object = gObjectList.findObject(row_id);
		
		if (!object || !isSearchableObject(object, region)) {
			//
			//	this item's object has been deleted so remove it
			//
			//	removing the row won't throw off our iteration
			//	becayse we have a local copy of the array
			//	
			//	we just need to make sure we don't access this
			//	item after the delete
			//
			mResultsList->deleteSingleItem(mResultsList->getItemIndex(row_id));
			mObjectDetails[row_id].listed = false;

			do_update = true;
		}
	}

	if (do_update) {
		mResultsList->updateLayout();
	}
}

void FSAreaSearch::updateName(const LLUUID id, const std::string name)
{
	LLScrollListColumn *owner_column = mResultsList->getColumn("owner_column");
	LLScrollListColumn *group_column = mResultsList->getColumn("group_column");

	if (!owner_column || !group_column) {
		LL_WARNS("FSAreaSearch") << "Missing column(s) detected" << LL_ENDL;
		return;
	}
  
	//
	//	iterate over the rows in the list
	//	updating the ones with a matching UUID
	//
	std::vector<LLScrollListItem*> items = mResultsList->getAllData();
	std::vector<LLScrollListItem*>::iterator iter = items.begin();
	std::vector<LLScrollListItem*>::iterator iter_end = items.end();

	for (; iter != iter_end; iter++) {
		LLScrollListItem *item = *iter;

		if (!item) {
			continue;
		}

		LLUUID row_id = item->getUUID();
		ObjectDetails &details = mObjectDetails[row_id];

		if (id == details.owner_id) {
			LLScrollListText *owner_text = (LLScrollListText*)item->getColumn(owner_column->mIndex);
			owner_text->setText(name);
		}
		else if (id == details.group_id) {
			LLScrollListText *group_text = (LLScrollListText*)item->getColumn(group_column->mIndex);
			group_text->setText(name);
		}
	}
}

FSAreaSearch::ContextMenu::ContextMenu() :
	mResultsList(NULL)
{
}

void FSAreaSearch::ContextMenu::show(LLView *view, const uuid_vec_t &uuids, S32 x, S32 y)
{
	llassert_always(view);

	mResultsList = dynamic_cast<LLScrollListCtrl*>(view);
	LLListContextMenu::show(view, uuids, x, y);
	mResultsList = NULL;	// to avoid dereferencing an invalid pointer
}

LLContextMenu *FSAreaSearch::ContextMenu::createMenu()
{
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	enable_registrar.add(
		"AreaSearch.Enable",
		boost::bind(&FSAreaSearch::ContextMenu::enableSelection, mResultsList, _2)
	);

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar commit_registrar;

	commit_registrar.add(
		"AreaSearch.Action",
		boost::bind(&FSAreaSearch::ContextMenu::handleSelection, mResultsList, _2)
	);

	LLContextMenu *menu = createFromFile("menu_area_search.xml");

	if (!menu) {
		LL_WARNS("FSAreaSearch") << "Failed to create the context menu" << LL_ENDL;
	}

	return menu;
}

// static
bool FSAreaSearch::ContextMenu::enableSelection(LLScrollListCtrl *ctrl, const LLSD &userdata)
{
 	LLScrollListItem *item = ctrl->getFirstSelected();

	if (!item) {
		LL_WARNS("FSAreaSearch") << "Nothing is selected" << LL_ENDL;
		return false;
	}

	LLViewerObject *object = gObjectList.findObject(item->getUUID());

	if (!object) {
		LL_WARNS("FSAreaSearch") << "Failed to find the object for UUID " << item->getUUID().asString() << LL_ENDL;
		return false;
	}

	std::string parameter = userdata.asString();

	if (parameter == "touch") {
		if (!object->flagHandleTouch()) {
			//
			//	not a touchable object
			//
			return false;
		}

		FSAreaSearch *floater = dynamic_cast<FSAreaSearch*>(gFloaterView->getParentFloater(ctrl));

		if (!floater) {
			LL_WARNS("FSAreaSearch") << "floater is NULL!" << LL_ENDL;
			return false;
		}

		//
		//	see if there's a defined touch name
		//	(update the menu if there is)
		//
		std::string touch_name = floater->getObjectTouchName(object->getID());
	
		if (!touch_name.empty()) {
			gMenuHolder->childSetText("touch", touch_name);
		}

		return true;
	}
	else if (parameter == "track") {
		//
		//	we can place a tracking beacon on anything that's
		//	not an avatar attachment
		//
		//	this option is disabled if the auto-track facility
		//	is switched on
		//
		FSAreaSearch *floater = dynamic_cast<FSAreaSearch*>(gFloaterView->getParentFloater(ctrl));

		if (!floater) {
			LL_WARNS("FSAreaSearch") << "floater is NULL!" << LL_ENDL;
			return false;
		}

		return !(floater->getAutoTrackStatus() || object->isAttachment());
	}
	else if (parameter == "return") {
		return enable_object_return();
	}
	else if (parameter == "take") {
		//
		//	WABBIT TODO: this doesn't work
		//
		return visible_take_object();
	}
	else if (parameter == "takecopy") {
		return enable_object_take_copy();
	}
	else if (parameter == "buy") {
		//
		//	WABBIT TODO: this doesn't work
		//
		return enable_buy_object();
	}
	else if (parameter == "pay") {
		return enable_pay_object();
	}
	else if (parameter == "delete") {
		return enable_object_delete();
	}

	return true;
}

// static
void FSAreaSearch::ContextMenu::handleSelection(LLScrollListCtrl *ctrl, const LLSD &userdata)
{
 	LLScrollListItem *item = ctrl->getFirstSelected();

	if (!item) {
		LL_WARNS("FSAreaSearch") << "Nothing is selected" << LL_ENDL;
		return;
	}

	LLViewerObject *object = gObjectList.findObject(item->getValue().asUUID());

	if (!object) {
		LL_WARNS("FSAreaSearch") << "Failed to find the object for UUID " << item->getUUID().asString() << LL_ENDL;
		return;
	}

	LLObjectSelectionHandle handle = LLSelectMgr::getInstance()->selectObjectAndFamily(object);

	std::string parameter = userdata.asString();

	if (parameter == "touch") {
		handle_object_touch();
	}
	else if (parameter == "teleport") {
		gAgent.teleportViaLocation(object->getPositionGlobal());
	}
	else if (parameter == "zoomin") {
		handle_zoom_to_object(item->getUUID());
	}
	else if (parameter == "track") {
		FSAreaSearch *floater = dynamic_cast<FSAreaSearch*>(gFloaterView->getParentFloater(ctrl));

		if (!floater) {
			LL_WARNS("FSAreaSearch") << "floater is NULL!" << LL_ENDL;
			return;
		}

		LLTracker::trackLocation(
			object->getPositionGlobal(),
			floater->getObjectName(item->getUUID()),
			"",
			LLTracker::LOCATION_ITEM
		);
	}
	else if (parameter == "inspect") {
		handle_object_inspect();
	}
	else if (parameter == "report") {
		LLFloaterReporter::showFromObject(item->getValue().asUUID());
	}
	else if (parameter == "block") {
		FSAreaSearch *floater = dynamic_cast<FSAreaSearch*>(gFloaterView->getParentFloater(ctrl));

		if (!floater) {
			LL_WARNS("FSAreaSearch") << "floater is NULL!" << LL_ENDL;
			return;
		}

		LLMute mute(item->getValue().asUUID(), floater->getObjectName(item->getValue().asUUID()), LLMute::OBJECT);

		if (LLMuteList::getInstance()->isMuted(mute.mID)) {
			LLMuteList::getInstance()->remove(mute);
		}
		else {
			LLMuteList::getInstance()->add(mute);
			LLPanelBlockedList::showPanelAndSelect(mute.mID);
		}
	}
	else if (parameter == "return") {
		handle_object_return();
	}
	else if (parameter == "take") {
		handle_take();
	}
	else if (parameter == "takecopy") {
		handle_take_copy();
	}
	else if (parameter == "buy") {
		handle_buy();
	}
	else if (parameter == "pay") {
		handle_give_money_dialog();
	}
	if (parameter == "edit") {
		handle_object_edit();
	}
	else if (parameter == "delete") {
		handle_object_delete();
	}
	else {
		LL_WARNS("FSAreaSearch") << "Unrecognised context menu handler parameter (" << parameter << ")" << LL_ENDL;
	}
}

FSAreaSearch::OptionsMenu::OptionsMenu(FSAreaSearch *floater) :
	mFloater(floater),
	mMenu(NULL)
{
	llassert_always(floater);

	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	enable_registrar.add(
		"AreaSearchOptions.Enable",
		boost::bind(&FSAreaSearch::OptionsMenu::enableSelection, this, _2)
	);
	enable_registrar.add(
		"AreaSearchOptions.Check",
		boost::bind(&FSAreaSearch::OptionsMenu::checkSelection, this, _2)
	);

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar commit_registrar;

	commit_registrar.add(
		"AreaSearchOptions.Action",
		boost::bind(&FSAreaSearch::OptionsMenu::handleSelection, this, _2)
	);

	mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
		"menu_area_search_options.xml",
		gMenuHolder,
		LLViewerMenuHolderGL::child_registry_t::instance()
	);

	llassert(mMenu);
}

bool FSAreaSearch::OptionsMenu::enableSelection(LLSD::String parameter)
{
	if (parameter == "stop_tracking") {
		return LLTracker::isTracking(NULL);
	}

	return true;
}

bool FSAreaSearch::OptionsMenu::checkSelection(LLSD::String parameter)
{
	if (parameter == "track") {
		return mFloater->getAutoTrackStatus();
	}

	LL_WARNS("FSAreaSearch") << "Unrecognised options menu check parameter (" << parameter << ")" << LL_ENDL;
	return false;
}

void FSAreaSearch::OptionsMenu::handleSelection(LLSD::String parameter)
{
	if (parameter == "track") {
		bool track = !mFloater->getAutoTrackStatus();

		mFloater->setAutoTrackStatus(track);

		if (track) {
			//
			//	auto-track is now switched on so try to set a
			//	tracking beacon on the currently-selected object
			//
			LLScrollListItem *item = mFloater->getFirstSelectedResult();

			if (item) {
				LLViewerObject *object = gObjectList.findObject(item->getUUID());

				if (object) {
					LLTracker::trackLocation(
						object->getPositionGlobal(),
						mFloater->getObjectName(item->getUUID()),
						"",
						LLTracker::LOCATION_ITEM
					);
				}
			}
		}
		else {
			//
			//	auto-track is now switched off so stop
			//	tracking the current object
			//
			LLTracker::stopTracking((void *)(ptrdiff_t)LLTracker::isTracking(NULL));
		}
	}
	else if (parameter == "stop_tracking") {
		//
		//	if we are asked to stop tracking then switch
		//	off auto-tracking mode as well
		//
		LLTracker::stopTracking((void *)(ptrdiff_t)LLTracker::isTracking(NULL));

		mFloater->setAutoTrackStatus(false);
	}
	else {
		LL_WARNS("FSAreaSearch") << "Unrecognised options menu handler parameter (" << parameter << ")" << LL_ENDL;
	}
}

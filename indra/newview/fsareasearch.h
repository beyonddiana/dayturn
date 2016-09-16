/** 
 * @file fsareasearch.h
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
 * Modular Systems. All rights reserved.
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
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS “AS IS”
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

#include "llfloater.h"
#include "llframetimer.h"
#include "lllistcontextmenu.h"
#include "llscrolllistctrl.h"
#include "llselectmgr.h"
#include "llsingleton.h"
#include "llstring.h"
#include "lluuid.h"

class LLCheckBoxCtrl;
class LLFilterEditor;
class LLProgressBar;
class LLTextBox;
class LLToggleableMenu;
class LLViewerRegion;

class FSAreaSearch : public LLSingleton<FSAreaSearch>, public LLFloater
{
public:
	struct ObjectDetails
	{
		typedef enum e_object_properties_request
		{
			NEED,
			SENT,
			FINISHED,
			FAILED
		} EObjectPropertiesRequest;

		EObjectPropertiesRequest request;

		LLUUID id;
		LLUUID owner_id;
		LLUUID group_id;
		std::string name;
		std::string description;

		std::string touch_name;
		std::string sit_name;

		bool requested;
		bool listed;
		bool name_requested;
		U32 local_id;
		U64 region_handle;

		ObjectDetails() :
			id(NULL),
			request(NEED),
			listed(false),
			name_requested(false)
		{
		}
	};

	FSAreaSearch(const LLSD &);
	virtual ~FSAreaSearch();

	/*virtual*/ BOOL postBuild();

	void findObjects();
	void refreshList(const bool cache_clear);
	void matchObject(ObjectDetails &details, const LLViewerObject *object);
	void updateScrollList();
	void updateStatusBar();
	void updateName(const LLUUID id, const std::string name);
	void requestObjectProperties(const std::vector<U32> &request_list, const bool select, LLViewerRegion *region);
	void processObjectProperties(LLMessageSystem *msg);
	void processRequestQueue();
	void getNameFromUUID(LLUUID &id, std::string &name, const BOOL is_group, bool &name_requested);
	std::string getObjectName(const LLUUID &id) { return mObjectDetails[id].name; }
	std::string getObjectTouchName(const LLUUID &id) { return mObjectDetails[id].touch_name; }

	bool isSearchActive() const { return mActive; }
	bool isSearchableObject(LLViewerObject *object, LLViewerRegion *region);

	bool wantObjectType(const LLViewerObject *object);

	void callbackLoadFullName(const LLUUID &id, const std::string &full_name);

	static void callbackIdle(void *user_data);

	bool getAutoTrackStatus() const { return mAutoTrackSelections; }
	void setAutoTrackStatus(const bool val) { mAutoTrackSelections  = val; }

	LLScrollListItem *getFirstSelectedResult() const { return mResultsList->getFirstSelected(); }

	class ContextMenu : public LLListContextMenu, public LLSingleton<ContextMenu>
	{
        LLSINGLETON(ContextMenu);

	public:
		/*virtual*/ void show(LLView *view, const uuid_vec_t &uuids, S32 x, S32 y);
		/*virtual*/ LLContextMenu *createMenu();

		static bool enableSelection(LLScrollListCtrl *ctrl, const LLSD &userdata);
		static void handleSelection(LLScrollListCtrl *ctrl, const LLSD &userdata);

	protected:
		LLScrollListCtrl *mResultsList;
	};

	class OptionsMenu
	{
	public:
		OptionsMenu(FSAreaSearch *floater);
		LLToggleableMenu *getMenu() { return mMenu; }

		bool enableSelection(LLSD::String param);
		bool checkSelection(LLSD::String param);
		void handleSelection(LLSD::String param);

	private:
		LLToggleableMenu *mMenu;
		FSAreaSearch *mFloater;
	};

private:
	bool mActive;
	bool mRefresh;
	bool mFloaterCreated;
	bool mRequestQueuePause;
	bool mRequestRequired;
	S32 mRequested;
	S32 mSearchableObjects;
	std::map<U64,S32> mRegionRequests;

	LLFilterEditor *mFilterName;
	LLFilterEditor *mFilterDescription;
	LLFilterEditor *mFilterOwner;
	LLFilterEditor *mFilterGroup;
	LLCheckBoxCtrl *mCheckboxPhysical;
	LLCheckBoxCtrl *mCheckboxTemporary;
	LLCheckBoxCtrl *mCheckboxAttachment;
	LLCheckBoxCtrl *mCheckboxOther;
	LLScrollListCtrl *mResultsList;
	LLTextBox *mStatusBarText;
	LLProgressBar *mStatusBarProgress;

	LLObjectSelectionHandle mSelectionHandle;
	OptionsMenu *mOptionsMenu;
	bool mAutoTrackSelections;

	LLFrameTimer mLastUpdateTimer;
	LLFrameTimer mLastProptiesRecievedTimer;

	std::vector<LLUUID> mNamesRequested;
	std::map<LLUUID, ObjectDetails> mObjectDetails;

	LLViewerRegion *mLastRegion;
	
	class FSParcelChangeObserver;
	friend class FSParcelChangeObserver;
	FSParcelChangeObserver*	mParcelChangedObserver;

	void results();
	void checkRegion();
	void requestIfNeeded(class LLViewerObject *objectp);

	void onSelectRow();
	void onDoubleClick();
	void onRightClick(S32 x, S32 y);
};

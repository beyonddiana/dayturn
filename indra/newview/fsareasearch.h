/**
 * @file fsareasearch.h
 * @brief Floater to search and list objects in view or is known to the viewer.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (c) 2012 Techwolf Lupindo
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
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#ifndef FS_AREASEARCH_H
#define FS_AREASEARCH_H

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

	/*virtual*/ bool postBuild();

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

#endif // FS_AREASEARCH_H

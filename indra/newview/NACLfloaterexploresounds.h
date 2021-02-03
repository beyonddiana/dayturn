// <edit>

#ifndef LL_LLFLOATEREXPLORESOUNDS_H
#define LL_LLFLOATEREXPLORESOUNDS_H

#include "llfloater.h"
#include "lleventtimer.h"
#include "llaudioengine.h"
#include "llavatarnamecache.h"

class LLCheckBoxCtrl;
class LLScrollListCtrl;

class NACLFloaterExploreSounds
: public LLFloater, public LLEventTimer
{
public:
	NACLFloaterExploreSounds(const LLSD& key);
	bool postBuild();

	bool tick();

	LLSoundHistoryItem getItem(const LLUUID& itemID);
	void requestNameCallback(LLMessageSystem* msg);		// KKA-796 object name query callback

private:
	virtual ~NACLFloaterExploreSounds();
	void handlePlayLocally();
	void handleLookAt();
	void handleStop();
	void handleSelection();
	void blockSound(); //KKA-796

	LLScrollListCtrl*	mHistoryScroller;
	LLCheckBoxCtrl*		mCollisionSounds;
	LLCheckBoxCtrl*		mRepeatedAssets;
	LLCheckBoxCtrl*		mAvatarSounds;
	LLCheckBoxCtrl*		mObjectSounds;
	LLCheckBoxCtrl*		mPaused;
	
	std::list<LLSoundHistoryItem> mLastHistory;

protected:
	std::vector<LLUUID> mRequestedIDs;			// KKA-796 list of object IDs we requested named for
};

#endif

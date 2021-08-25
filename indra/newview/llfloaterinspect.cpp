/** 
 * @file llfloaterinspect.cpp
 * @brief Floater for object inspection tool
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llfloaterinspect.h"

#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llgroupactions.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llselectmgr.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "lluictrlfactory.h"

// PoundLife - Improved Object Inspect
#include "llresmgr.h"
#include "lltexturectrl.h"
#include "llviewerobjectlist.h" //gObjectList
#include "llviewertexturelist.h"
// PoundLife END

//LLFloaterInspect* LLFloaterInspect::sInstance = NULL;

LLFloaterInspect::LLFloaterInspect(const LLSD& key)
  : LLFloater(key),
	mDirty(false),
	mOwnerNameCacheConnection(),
	mCreatorNameCacheConnection()
{
	mCommitCallbackRegistrar.add("Inspect.OwnerProfile",	boost::bind(&LLFloaterInspect::onClickOwnerProfile, this));
	mCommitCallbackRegistrar.add("Inspect.CreatorProfile",	boost::bind(&LLFloaterInspect::onClickCreatorProfile, this));
	mCommitCallbackRegistrar.add("Inspect.SelectObject",	boost::bind(&LLFloaterInspect::onSelectObject, this));
}

bool LLFloaterInspect::postBuild()
{
	mObjectList = getChild<LLScrollListCtrl>("object_list");
//	childSetAction("button owner",onClickOwnerProfile, this);
//	childSetAction("button creator",onClickCreatorProfile, this);
//	childSetCommitCallback("object_list", onSelectObject, NULL);
	
	refresh();
	
	return true;
}

LLFloaterInspect::~LLFloaterInspect(void)
{
	if (mOwnerNameCacheConnection.connected())
	{
		mOwnerNameCacheConnection.disconnect();
	}
	if (mCreatorNameCacheConnection.connected())
	{
		mCreatorNameCacheConnection.disconnect();
	}
	if(!LLFloaterReg::instanceVisible("build"))
	{
		if(LLToolMgr::getInstance()->getBaseTool() == LLToolCompInspect::getInstance())
		{
			LLToolMgr::getInstance()->clearTransientTool();
		}
		// Switch back to basic toolset
		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);	
	}
	else
	{
		LLFloaterReg::showInstance("build", LLSD(), TRUE);
	}
}

void LLFloaterInspect::onOpen(const LLSD& key)
{
	BOOL forcesel = LLSelectMgr::getInstance()->setForceSelection(TRUE);
	LLToolMgr::getInstance()->setTransientTool(LLToolCompInspect::getInstance());
	LLSelectMgr::getInstance()->setForceSelection(forcesel);	// restore previouis value
	mObjectSelection = LLSelectMgr::getInstance()->getSelection();
	refresh();
}
void LLFloaterInspect::onClickCreatorProfile()
{
	if(mObjectList->getAllSelected().size() == 0)
	{
		return;
	}
	LLScrollListItem* first_selected =mObjectList->getFirstSelected();

	if (first_selected)
	{
		struct f : public LLSelectedNodeFunctor
		{
			LLUUID obj_id;
			f(const LLUUID& id) : obj_id(id) {}
			virtual bool apply(LLSelectNode* node)
			{
				return (obj_id == node->getObject()->getID());
			}
		} func(first_selected->getUUID());
		LLSelectNode* node = mObjectSelection->getFirstNode(&func);
		if(node)
		{
			LLAvatarActions::showProfile(node->mPermissions->getCreator());
		}
	}
}

void LLFloaterInspect::onClickOwnerProfile()
{
	if(mObjectList->getAllSelected().size() == 0) return;
	LLScrollListItem* first_selected =mObjectList->getFirstSelected();

	if (first_selected)
	{
		LLUUID selected_id = first_selected->getUUID();
		struct f : public LLSelectedNodeFunctor
		{
			LLUUID obj_id;
			f(const LLUUID& id) : obj_id(id) {}
			virtual bool apply(LLSelectNode* node)
			{
				return (obj_id == node->getObject()->getID());
			}
		} func(selected_id);
		LLSelectNode* node = mObjectSelection->getFirstNode(&func);
		if(node)
		{
			if(node->mPermissions->isGroupOwned())
			{
				const LLUUID& idGroup = node->mPermissions->getGroup();
				LLGroupActions::show(idGroup);
			}
			else
			{
				const LLUUID& owner_id = node->mPermissions->getOwner();
				LLAvatarActions::showProfile(owner_id);
			}

		}
	}
}

void LLFloaterInspect::onSelectObject()
{
	if(LLFloaterInspect::getSelectedUUID() != LLUUID::null)
	{
		getChildView("button owner")->setEnabled(true);
		getChildView("button creator")->setEnabled(true);
	}
}

LLUUID LLFloaterInspect::getSelectedUUID()
{
	if(mObjectList->getAllSelected().size() > 0)
	{
		LLScrollListItem* first_selected =mObjectList->getFirstSelected();
		if (first_selected)
		{
			return first_selected->getUUID();
		}
		
	}
	return LLUUID::null;
}

void LLFloaterInspect::refresh()
{
	LLUUID creator_id;
	std::string creator_name;
	S32 pos = mObjectList->getScrollPos();
	// PoundLife - Improved Object Inspect
	LLLocale locale("");
	LLResMgr& res_mgr = LLResMgr::instance();
	LLSelectMgr& sel_mgr = LLSelectMgr::instance();
	S32 fcount = 0;
	S32 tcount = 0;
	S32 vcount = 0;
	S32 objcount = 0;
	S32 primcount = 0;
	mTextureList.clear();
	mTextureMemory = 0;
	mTextureVRAMMemory = 0;
	// PoundLife - End
	getChildView("button owner")->setEnabled(false);
	getChildView("button creator")->setEnabled(false);
	LLUUID selected_uuid;
	S32 selected_index = mObjectList->getFirstSelectedIndex();
	if(selected_index > -1)
	{
		LLScrollListItem* first_selected =
			mObjectList->getFirstSelected();
		if (first_selected)
		{
			selected_uuid = first_selected->getUUID();
		}
	}
	mObjectList->operateOnAll(LLScrollListCtrl::OP_DELETE);
	//List all transient objects, then all linked objects

	for (LLObjectSelection::valid_iterator iter = mObjectSelection->valid_begin();
		 iter != mObjectSelection->valid_end(); iter++)
	{
		LLSelectNode* obj = *iter;
		LLSD row;
		std::string owner_name, creator_name;

		if (obj->mCreationDate == 0)
		{	// Don't have valid information from the server, so skip this one
			continue;
		}

		time_t timestamp = (time_t) (obj->mCreationDate/1000000);
		std::string timeStr = getString("timeStamp");
		LLSD substitution;
		substitution["datetime"] = (S32) timestamp;
		LLStringUtil::format (timeStr, substitution);

		const LLUUID& idOwner = obj->mPermissions->getOwner();
		const LLUUID& idCreator = obj->mPermissions->getCreator();
		LLAvatarName av_name;

		if(obj->mPermissions->isGroupOwned())
		{
			std::string group_name;
			const LLUUID& idGroup = obj->mPermissions->getGroup();
			if(gCacheName->getGroupName(idGroup, group_name))
			{
				owner_name = "[" + group_name + "] (group)";
			}
			else
			{
				owner_name = LLTrans::getString("RetrievingData");
				if (mOwnerNameCacheConnection.connected())
				{
					mOwnerNameCacheConnection.disconnect();
				}
				mOwnerNameCacheConnection = gCacheName->getGroup(idGroup, boost::bind(&LLFloaterInspect::onGetOwnerNameCallback, this));
			}
		}
		else
		{
			// Only work with the names if we actually get a result
			// from the name cache. If not, defer setting the
			// actual name and set a placeholder.
			if (LLAvatarNameCache::get(idOwner, &av_name))
			{
				owner_name = av_name.getCompleteName();
			}
			else
			{
				owner_name = LLTrans::getString("RetrievingData");
				if (mOwnerNameCacheConnection.connected())
				{
					mOwnerNameCacheConnection.disconnect();
				}
				mOwnerNameCacheConnection = LLAvatarNameCache::get(idOwner, boost::bind(&LLFloaterInspect::onGetOwnerNameCallback, this));
			}
		}

		if (LLAvatarNameCache::get(idCreator, &av_name))
		{
			creator_name = av_name.getCompleteName();
		}
		else
		{
			creator_name = LLTrans::getString("RetrievingData");
			if (mCreatorNameCacheConnection.connected())
			{
				mCreatorNameCacheConnection.disconnect();
			}
			mCreatorNameCacheConnection = LLAvatarNameCache::get(idCreator, boost::bind(&LLFloaterInspect::onGetCreatorNameCallback, this));
		}
		
		row["id"] = obj->getObject()->getID();
		row["columns"][0]["column"] = "object_name";
		row["columns"][0]["type"] = "text";
		// make sure we're either at the top of the link chain
		// or top of the editable chain, for attachments
		if(!(obj->getObject()->isRoot() || obj->getObject()->isRootEdit()))
		{
			row["columns"][0]["value"] = std::string("   ") + obj->mName;
		}
		else
		{
			row["columns"][0]["value"] = obj->mName;
		}
		row["columns"][1]["column"] = "owner_name";
		row["columns"][1]["type"] = "text";
		row["columns"][1]["value"] = owner_name;
		row["columns"][2]["column"] = "creator_name";
		row["columns"][2]["type"] = "text";
		row["columns"][2]["value"] = creator_name;
		row["columns"][3]["column"] = "creation_date";
		row["columns"][3]["type"] = "text";
		row["columns"][3]["value"] = timeStr;

		// PoundLife - Improved Object Inspect
		
	    std::string fcount_string;
		res_mgr.getIntegerString(fcount_string, obj->getObject()->getNumFaces());
		row["columns"][4]["column"] = "facecount";
		row["columns"][4]["type"] = "text";
		row["columns"][4]["value"] = fcount_string;

	    std::string vcount_string;
		res_mgr.getIntegerString(vcount_string, obj->getObject()->getNumVertices());
		row["columns"][5]["column"] = "vertexcount";
		row["columns"][5]["type"] = "text";
		row["columns"][5]["value"] = vcount_string;

	    std::string tcount_string;
		res_mgr.getIntegerString(tcount_string, obj->getObject()->getNumIndices() / 3);
		row["columns"][6]["column"] = "trianglecount";
		row["columns"][6]["type"] = "text";
		row["columns"][6]["value"] = tcount_string;

		// Poundlife - Get VRAM
		U32 texture_memory = 0;
		U32 vram_memory = 0;
		getObjectTextureMemory(obj->getObject(), texture_memory, vram_memory);
		primcount = sel_mgr.getSelection()->getObjectCount();
		objcount = sel_mgr.getSelection()->getRootObjectCount();
		fcount += obj->getObject()->getNumFaces();
		tcount += obj->getObject()->getNumIndices() / 3;
		vcount += obj->getObject()->getNumVertices();
		// PoundLife - END
		mObjectList->addElement(row, ADD_TOP);
	}
	if(selected_index > -1 && mObjectList->getItemIndex(selected_uuid) == selected_index)
	{
		mObjectList->selectNthItem(selected_index);
	}
	else
	{
		mObjectList->selectNthItem(0);
	}
	onSelectObject();
	mObjectList->setScrollPos(pos);

	// PoundLife - Total linkset stats.
	LLStringUtil::format_map_t args;
	std::string objcount_string;
	res_mgr.getIntegerString(objcount_string, objcount);
	args["NUM_OBJECTS"] = objcount_string;
	std::string primcount_string;
	res_mgr.getIntegerString(primcount_string, primcount);
	args["NUM_PRIMS"] = primcount_string;
	std::string ftotcount_string;
	res_mgr.getIntegerString(ftotcount_string, fcount);
	args["NUM_FACES"] = ftotcount_string;
	std::string vtotcount_string;
	res_mgr.getIntegerString(vtotcount_string, vcount);
	args["NUM_VERTICES"] = vtotcount_string;
	std::string ttotcount_string;
	res_mgr.getIntegerString(ttotcount_string, tcount);
	args["NUM_TRIANGLES"] = ttotcount_string;
	std::string texcount_string;
	res_mgr.getIntegerString(texcount_string, mTextureList.size());
	args["NUM_TEXTURES"] = texcount_string;
	std::string texmem_string;
	res_mgr.getIntegerString(texmem_string, mTextureMemory / 1024);
	args["TEXTURE_MEMORY"] = texmem_string;
	std::string vram_string;
	res_mgr.getIntegerString(vram_string, mTextureVRAMMemory / 1024);
	args["VRAM_USAGE"] = vram_string;
	getChild<LLTextBase>("linksetstats_text")->setText(getString("stats_list", args));
	// PoundLife - End
}

// PoundLife - Improved Object Inspect
void LLFloaterInspect::getObjectTextureMemory(LLViewerObject* object, U32& object_texture_memory, U32& object_vram_memory)
{
	uuid_vec_t object_texture_list;

	if (!object)
	{
		return;
	}

	LLUUID uuid;
	U8 te_count = object->getNumTEs();

	for (U8 j = 0; j < te_count; j++)
	{
		LLViewerTexture* img = object->getTEImage(j);
		if (img)
		{
			calculateTextureMemory(img, object_texture_list, object_texture_memory, object_vram_memory);
		}

		// materials per face
		if (object->getTE(j)->getMaterialParams().notNull())
		{
			uuid = object->getTE(j)->getMaterialParams()->getNormalID();
			if (uuid.notNull())
			{
				LLViewerTexture* img = gTextureList.getImage(uuid);
				if (img)
				{
					calculateTextureMemory(img, object_texture_list, object_texture_memory, object_vram_memory);
				}
			}

			uuid = object->getTE(j)->getMaterialParams()->getSpecularID();
			if (uuid.notNull())
			{
				LLViewerTexture* img = gTextureList.getImage(uuid);
				if (img)
				{
					calculateTextureMemory(img, object_texture_list, object_texture_memory, object_vram_memory);
				}
			}
		}
	}

	// sculpt map
	if (object->isSculpted() && !object->isMesh())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)(object->getParameterEntry(LLNetworkData::PARAMS_SCULPT));
		uuid = sculpt_params->getSculptTexture();
		LLViewerTexture* img = gTextureList.getImage(uuid);
		if (img)
		{
			calculateTextureMemory(img, object_texture_list, object_texture_memory, object_vram_memory);
		}
	}
}

void LLFloaterInspect::calculateTextureMemory(LLViewerTexture* texture, uuid_vec_t& object_texture_list, U32& object_texture_memory, U32& object_vram_memory)
{
	const LLUUID uuid = texture->getID();
	U32 vram_memory = (texture->getFullHeight() * texture->getFullWidth() * 32 / 8);
	U32 texture_memory = (texture->getFullHeight() * texture->getFullWidth() * texture->getComponents());

	if (std::find(mTextureList.begin(), mTextureList.end(), uuid) == mTextureList.end())
	{
		mTextureList.push_back(uuid);
		mTextureMemory += texture_memory;
		mTextureVRAMMemory += vram_memory;
	}

	if (std::find(object_texture_list.begin(), object_texture_list.end(), uuid) == object_texture_list.end())
	{
		object_texture_list.push_back(uuid);
		object_texture_memory += texture_memory;
		object_vram_memory += vram_memory;
	}
}
// PoundLife - End



void LLFloaterInspect::onFocusReceived()
{
	LLToolMgr::getInstance()->setTransientTool(LLToolCompInspect::getInstance());
	LLFloater::onFocusReceived();
}

void LLFloaterInspect::dirty()
{
	setDirty();
}

void LLFloaterInspect::onGetOwnerNameCallback()
{
	mOwnerNameCacheConnection.disconnect();
	setDirty();
}

void LLFloaterInspect::onGetCreatorNameCallback()
{
	mCreatorNameCacheConnection.disconnect();
	setDirty();
}

void LLFloaterInspect::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = false;
	}

	LLFloater::draw();
}

/** 
 * @file llviewerjointattachment.cpp
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerjointattachment.h"

#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llgl.h"
#include "llhudtext.h"
#include "llrender.h"
#include "llvoavatarself.h"
#include "llvolume.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llinventorymodel.h"
#include "llviewerobjectlist.h"
#include "llface.h"
#include "llvoavatar.h"

#include "llglheaders.h"

//MK
//#include "llinventoryview.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llvoavatarself.h"
//mk

extern LLPipeline gPipeline;


//-----------------------------------------------------------------------------
// LLViewerJointAttachment()
//-----------------------------------------------------------------------------
LLViewerJointAttachment::LLViewerJointAttachment() :
	mVisibleInFirst(FALSE),
	mGroup(0),
	mIsHUDAttachment(FALSE),
	mPieSlice(-1)
{
	mValid = FALSE;
	mUpdateXform = FALSE;
	mAttachedObjects.clear();
}

//-----------------------------------------------------------------------------
// ~LLViewerJointAttachment()
//-----------------------------------------------------------------------------
LLViewerJointAttachment::~LLViewerJointAttachment()
{
}

//-----------------------------------------------------------------------------
// isTransparent()
//-----------------------------------------------------------------------------
BOOL LLViewerJointAttachment::isTransparent()
{
	return FALSE;
}

//-----------------------------------------------------------------------------
// drawShape()
//-----------------------------------------------------------------------------
U32 LLViewerJointAttachment::drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
{
	if (LLVOAvatar::sShowAttachmentPoints)
	{
		LLGLDisable cull_face(GL_CULL_FACE);
		
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		gGL.begin(LLRender::QUADS);
		{
			gGL.vertex3f(-0.1f, 0.1f, 0.f);
			gGL.vertex3f(-0.1f, -0.1f, 0.f);
			gGL.vertex3f(0.1f, -0.1f, 0.f);
			gGL.vertex3f(0.1f, 0.1f, 0.f);
		}gGL.end();
	}
	return 0;
}

void LLViewerJointAttachment::setupDrawable(LLViewerObject *object)
{
	if (!object->mDrawable)
		return;
	if (object->mDrawable->isActive())
	{
		object->mDrawable->makeStatic(FALSE);
	}

	object->mDrawable->mXform.setParent(getXform()); // LLViewerJointAttachment::lazyAttach
	object->mDrawable->makeActive();
	LLVector3 current_pos = object->getRenderPosition();
	LLQuaternion current_rot = object->getRenderRotation();
	LLQuaternion attachment_pt_inv_rot = ~(getWorldRotation());

	current_pos -= getWorldPosition();
	current_pos.rotVec(attachment_pt_inv_rot);

	current_rot = current_rot * attachment_pt_inv_rot;

	object->mDrawable->mXform.setPosition(current_pos);
	object->mDrawable->mXform.setRotation(current_rot);
	gPipeline.markMoved(object->mDrawable);
	gPipeline.markTextured(object->mDrawable); // face may need to change draw pool to/from POOL_HUD
	object->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
	
	if(mIsHUDAttachment)
	{
		for (S32 face_num = 0; face_num < object->mDrawable->getNumFaces(); face_num++)
		{
			LLFace *face = object->mDrawable->getFace(face_num);
			if (face)
			{
				face->setState(LLFace::HUD_RENDER);
			}
		}
	}

	LLViewerObject::const_child_list_t& child_list = object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
	{
		LLViewerObject* childp = *iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->setState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable); // face may need to change draw pool to/from POOL_HUD
			gPipeline.markMoved(childp->mDrawable);

			if(mIsHUDAttachment)
			{
				for (S32 face_num = 0; face_num < childp->mDrawable->getNumFaces(); face_num++)
				{
					LLFace * face = childp->mDrawable->getFace(face_num);
					if (face)
					{
						face->setState(LLFace::HUD_RENDER);
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// addObject()
//-----------------------------------------------------------------------------
BOOL LLViewerJointAttachment::addObject(LLViewerObject* object)
{
	object->extractAttachmentItemID();
//MK
	BOOL was_empty = true;
//mk

	// Same object reattached
	if (isObjectAttached(object))
	{
//MK
		was_empty = false;
//mk
		removeObject(object);
		// Pass through anyway to let setupDrawable()
		// re-connect object to the joint correctly
	}
//MK
	LLUUID item_id = object->getAttachmentItemID();;
//mk

	// Two instances of the same inventory item attached --
	// Request detach, and kill the object in the meantime.
	if (getAttachedObject(object->getAttachmentItemID()))
	{
		LL_INFOS() << "(same object re-attached)" << LL_ENDL;
		object->markDead();

		// If this happens to be attached to self, then detach.
		LLVOAvatarSelf::detachAttachmentIntoInventory(object->getAttachmentItemID());
		return FALSE;
	}

	mAttachedObjects.push_back(object);
	setupDrawable(object);
	
	if (mIsHUDAttachment)
	{
		if (object->mText.notNull())
		{
			object->mText->setOnHUDAttachment(TRUE);
		}
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); ++iter)
		{
			LLViewerObject* childp = *iter;
			if (childp && childp->mText.notNull())
			{
				childp->mText->setOnHUDAttachment(TRUE);
			}
		}
	}
	calcLOD();
	mUpdateXform = TRUE;

//MK
	if (gRRenabled)
	{
		LLInventoryItem* inv_item = gAgent.mRRInterface.getItem (object->getID());

		// If this attachment point is locked and empty then force detach, unless the attached object was supposed to be reattached automatically
		if (was_empty)
		{
			std::string name = getName();
			LLStringUtil::toLower(name);
			if (gAgent.mRRInterface.mGarbageCollectorCalledOnce && !gAgent.mRRInterface.canAttach(object, name, false)) // little trick to ignore illegal attaches while we're still loading (dummy restrictions are still active and the garbage collector hasn't been called yet)
			{
				bool just_reattaching = false;
				std::deque<AssetAndTarget>::iterator it = gAgent.mRRInterface.mAssetsToReattach.begin();
				for (; it != gAgent.mRRInterface.mAssetsToReattach.end(); ++it)
				{
					if (it->uuid == item_id)
					{
						just_reattaching = true;
						break;
					}
				}
				if (!just_reattaching)
				{
					// Find and remove this item from the COF.
//					LLInventoryItem* inv_item = gAgent.mRRInterface.getItem (object->getID());
					if (inv_item)
					{
						LLAppearanceMgr::instance().removeCOFItemLinks(inv_item->getUUID());
 						object->setAttachmentItemID (LLUUID::null);
						mAttachedObjects.pop_back ();
						gInventory.notifyObservers();
					}

					LL_INFOS() << "Attached to a locked point : " << item_id << LL_ENDL;
					gMessageSystem->newMessage("ObjectDetach");
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
					gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());	
					gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
					gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
					gMessageSystem->sendReliable( gAgent.getRegionHost() );

					gAgent.mRRInterface.mJustDetached.uuid = item_id;
					gAgent.mRRInterface.mJustDetached.attachpt = getName();

					// Now notify that this object has been attached and will be detached right away
					gAgent.mRRInterface.notify (LLUUID::null, "attached illegally " + getName(), "");
				}
				else
				{
					// Notify that this object has just been reattached
					gAgent.mRRInterface.notify (LLUUID::null, "reattached legally " + getName(), "");
				}
			}
			else
			{
				// Notify that this object has been attached
				if (inv_item)
				{
					gAgent.mRRInterface.notify (LLUUID::null, "attached legally " + getName(), "");
				}
			}
		}

		// If the UUID of the attached item is contained into the list of things waiting to reattach,
		// signal it and remove it from the list.
		std::deque<AssetAndTarget>::iterator it = gAgent.mRRInterface.mAssetsToReattach.begin();
		for (; it != gAgent.mRRInterface.mAssetsToReattach.end(); ++it)
		{
			if (it->uuid == item_id)
			{
				LL_INFOS() << "Reattached asset " << item_id << " automatically" << LL_ENDL;
				gAgent.mRRInterface.mReattaching = FALSE;
				gAgent.mRRInterface.mReattachTimeout = FALSE;
				gAgent.mRRInterface.mAssetsToReattach.erase(it);
//				gAgent.mRRInterface.mJustReattached.uuid = item_id;
//				gAgent.mRRInterface.mJustReattached.attachpt = getName();
				// Replace the previously stored asset id with the new viewer id in the list of restrictions
				gAgent.mRRInterface.replace(item_id, object->getRootEdit()->getID());
				break;
			}
		}
	}
//mk

	return TRUE;
}

//-----------------------------------------------------------------------------
// removeObject()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::removeObject(LLViewerObject *object)
{
//MK
	if (gRRenabled)
	{
		// We first need to check whether the object is locked, as some techniques (like llAttachToAvatar)
		// can kick even a locked attachment off.
		// If so, retain its UUID for later
		// Note : we need to delay the reattach a little, or we risk losing the item in the inventory.
		LLVOAvatarSelf *avatarp = gAgentAvatarp;
		LLUUID inv_item_id = LLUUID::null;
		LLInventoryItem* inv_item = gAgent.mRRInterface.getItem(object->getRootEdit()->getID());
		if (inv_item) inv_item_id = inv_item->getUUID();
		std::string target_attachpt = "";
		if (avatarp) 
		{
			avatarp->getAttachedPointName(inv_item_id, target_attachpt);
		}
		gAgent.mRRInterface.mHandleNoStrip = FALSE;
		if (!gAgent.mRRInterface.canDetach(object)
			&& gAgent.mRRInterface.mJustDetached.attachpt != target_attachpt	// we didn't just detach something from this attach pt automatically
			&& gAgent.mRRInterface.mJustReattached.attachpt != target_attachpt)	// we didn't just reattach something to this attach pt automatically
		{
			LL_INFOS() << "Detached a locked object : " << inv_item_id << LL_ENDL;

			// Now notify that this object has been detached and will be reattached right away
			gAgent.mRRInterface.notify (LLUUID::null, "detached illegally " + getName(), "");

			std::deque<AssetAndTarget>::iterator it = gAgent.mRRInterface.mAssetsToReattach.begin();
			bool found = false;
			bool found_for_this_point = false;
			for (; it != gAgent.mRRInterface.mAssetsToReattach.end(); ++it)
			{
				if (it->uuid == inv_item_id) found = true;
				if (it->attachpt == target_attachpt) found_for_this_point = true;
			}

			if (!found && !found_for_this_point)
			{
				AssetAndTarget at;
				at.uuid = inv_item_id;
				at.attachpt = target_attachpt;
				gAgent.mRRInterface.mReattachTimer.reset();
				gAgent.mRRInterface.mAssetsToReattach.push_back(at);
				// Little hack : store this item's asset id into the list of restrictions so they are automatically reapplied when it is reattached
				gAgent.mRRInterface.replace(object->getRootEdit()->getID(), inv_item_id);
			}
		}
		else {
			if (inv_item)
			{
				// Notify that this object has been detached
				gAgent.mRRInterface.notify (LLUUID::null, "detached legally " + getName(), "");
			}
		}
		gAgent.mRRInterface.mHandleNoStrip = TRUE;
		gAgent.mRRInterface.mJustDetached.uuid.setNull();
		gAgent.mRRInterface.mJustDetached.attachpt = "";
		gAgent.mRRInterface.mJustReattached.uuid.setNull();
		gAgent.mRRInterface.mJustReattached.attachpt = "";
	}
//mk

	attachedobjs_vec_t::iterator iter;
	for (iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject *attached_object = (*iter);
		if (attached_object == object)
		{
			break;
		}
	}
	if (iter == mAttachedObjects.end())
	{
		LL_WARNS() << "Could not find object to detach" << LL_ENDL;
		return;
	}

	// force object visibile
	setAttachmentVisibility(TRUE);

	mAttachedObjects.erase(iter);
	if (object->mDrawable.notNull())
	{
		//if object is active, make it static
		if(object->mDrawable->isActive())
		{
			object->mDrawable->makeStatic(FALSE);
		}

		LLVector3 cur_position = object->getRenderPosition();
		LLQuaternion cur_rotation = object->getRenderRotation();

		object->mDrawable->mXform.setPosition(cur_position);
		object->mDrawable->mXform.setRotation(cur_rotation);
		gPipeline.markMoved(object->mDrawable, TRUE);
		gPipeline.markTextured(object->mDrawable); // face may need to change draw pool to/from POOL_HUD
		object->mDrawable->clearState(LLDrawable::USE_BACKLIGHT);

		if (mIsHUDAttachment)
		{
			for (S32 face_num = 0; face_num < object->mDrawable->getNumFaces(); face_num++)
			{
				LLFace * face = object->mDrawable->getFace(face_num);
				if (face)
				{
					face->clearState(LLFace::HUD_RENDER);
				}
			}
		}
	}

	LLViewerObject::const_child_list_t& child_list = object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); ++iter)
	{
		LLViewerObject* childp = *iter;
		if (childp && childp->mDrawable.notNull())
		{
			childp->mDrawable->clearState(LLDrawable::USE_BACKLIGHT);
			gPipeline.markTextured(childp->mDrawable); // face may need to change draw pool to/from POOL_HUD
			if (mIsHUDAttachment)
			{
				for (S32 face_num = 0; face_num < childp->mDrawable->getNumFaces(); face_num++)
				{
					LLFace * face = childp->mDrawable->getFace(face_num);
					if (face)
					{
						face->clearState(LLFace::HUD_RENDER);
					}
				}
			}
		}
	} 

	if (mIsHUDAttachment)
	{
		if (object->mText.notNull())
		{
			object->mText->setOnHUDAttachment(FALSE);
		}
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); ++iter)
		{
			LLViewerObject* childp = *iter;
			if (childp->mText.notNull())
			{
				childp->mText->setOnHUDAttachment(FALSE);
			}
		}
	}
	if (mAttachedObjects.size() == 0)
	{
		mUpdateXform = FALSE;
	}
	object->setAttachmentItemID(LLUUID::null);
}

//-----------------------------------------------------------------------------
// setAttachmentVisibility()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::setAttachmentVisibility(BOOL visible)
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject *attached_obj = (*iter);
		if (!attached_obj || attached_obj->mDrawable.isNull() || 
			!(attached_obj->mDrawable->getSpatialBridge()))
			continue;
		
		if (visible)
		{
			// Hack to make attachments not visible by disabling their type mask!
			// This will break if you can ever attach non-volumes! - djs 02/14/03
			attached_obj->mDrawable->getSpatialBridge()->mDrawableType = 
				attached_obj->isHUDAttachment() ? LLPipeline::RENDER_TYPE_HUD : LLPipeline::RENDER_TYPE_VOLUME;
		}
		else
		{
			attached_obj->mDrawable->getSpatialBridge()->mDrawableType = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// setOriginalPosition()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::setOriginalPosition(LLVector3& position)
{
	mOriginalPos = position;
	// SL-315
	setPosition(position);
}

//-----------------------------------------------------------------------------
// getNumAnimatedObjects()
//-----------------------------------------------------------------------------
S32 LLViewerJointAttachment::getNumAnimatedObjects() const
{
    S32 count = 0;
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
        const LLViewerObject *attached_object = *iter;
        if (attached_object->isAnimatedObject())
        {
            count++;
        }
    }
    return count;
}

//-----------------------------------------------------------------------------
// clampObjectPosition()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::clampObjectPosition()
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		if (LLViewerObject *attached_object = (*iter))
		{
			// *NOTE: object can drift when hitting maximum radius
			LLVector3 attachmentPos = attached_object->getPosition();
			F32 dist = attachmentPos.normVec();
			dist = llmin(dist, MAX_ATTACHMENT_DIST);
			attachmentPos *= dist;
			attached_object->setPosition(attachmentPos);
		}
	}
}

//-----------------------------------------------------------------------------
// calcLOD()
//-----------------------------------------------------------------------------
void LLViewerJointAttachment::calcLOD()
{
	F32 maxarea = 0;
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		if (LLViewerObject *attached_object = (*iter))
		{
			maxarea = llmax(maxarea,attached_object->getMaxScale() * attached_object->getMidScale());
			LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
				 iter != child_list.end(); ++iter)
			{
				LLViewerObject* childp = *iter;
				F32 area = childp->getMaxScale() * childp->getMidScale();
				maxarea = llmax(maxarea, area);
			}
		}
	}
	maxarea = llclamp(maxarea, .01f*.01f, 1.f);
	F32 avatar_area = (4.f * 4.f); // pixels for an avatar sized attachment
	F32 min_pixel_area = avatar_area / maxarea;
	setLOD(min_pixel_area);
}

//-----------------------------------------------------------------------------
// updateLOD()
//-----------------------------------------------------------------------------
BOOL LLViewerJointAttachment::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL res = FALSE;
	if (!mValid)
	{
		setValid(TRUE, TRUE);
		res = TRUE;
	}
	return res;
}

BOOL LLViewerJointAttachment::isObjectAttached(const LLViewerObject *viewer_object) const
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		const LLViewerObject* attached_object = (*iter);
		if (attached_object == viewer_object)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//MK
// This is a non-const variant
BOOL LLViewerJointAttachment::isObjectAttached(LLViewerObject *viewer_object) const
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject* attached_object = (*iter);
		if (attached_object == viewer_object)
		{
			return TRUE;
		}
	}
	return FALSE;
}
//mk

const LLViewerObject *LLViewerJointAttachment::getAttachedObject(const LLUUID &object_id) const
{
	for (attachedobjs_vec_t::const_iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		const LLViewerObject* attached_object = (*iter);
		if (attached_object->getAttachmentItemID() == object_id)
		{
			return attached_object;
		}
	}
	return NULL;
}

LLViewerObject *LLViewerJointAttachment::getAttachedObject(const LLUUID &object_id)
{
	for (attachedobjs_vec_t::iterator iter = mAttachedObjects.begin();
		 iter != mAttachedObjects.end();
		 ++iter)
	{
		LLViewerObject* attached_object = (*iter);
		if (attached_object->getAttachmentItemID() == object_id)
		{
			return attached_object;
		}
	}
	return NULL;
}

//MK
LLViewerObject* LLViewerJointAttachment::getObject() const
{
	if (mAttachedObjects.size() > 0)
	{
		return mAttachedObjects.at(0);
	}
	return NULL;
}

const LLUUID& LLViewerJointAttachment::getItemID() const
{
	LLViewerObject* object = getObject();
	if (object)
	{
		return object->getAttachmentItemID();
	}
	return LLUUID::null;
}
//mk

/** 
 * @file llpanelpeoplemenus.h
 * @brief Menus used by the side tray "People" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// libs
#include "llmenugl.h"
#include "lluictrlfactory.h"

#include "llpanelpeoplemenus.h"

// newview
#include "llagent.h"
#include "llagentdata.h"			// for gAgentID
#include "llavataractions.h"
#include "llcallingcard.h"			// for LLAvatarTracker
#include "lllogchat.h"
#include "llviewermenu.h"			// for gMenuHolder
#include "llconversationmodel.h"
#include "llviewerobjectlist.h"
#include "llviewernetwork.h"
#include "llnotificationsutil.h"
#include "llslurl.h"
#include "llavatarpropertiesprocessor.h"

namespace LLPanelPeopleMenus
{

PeopleContextMenu gPeopleContextMenu;
NearbyPeopleContextMenu gNearbyPeopleContextMenu;
SuggestedFriendsContextMenu gSuggestedFriendsContextMenu;

//== PeopleContextMenu ===============================================================

LLContextMenu* PeopleContextMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	LLContextMenu* menu;

	if ( mUUIDs.size() == 1 )
	{
		// Set up for one person selected menu

		const LLUUID& id = mUUIDs.front();
		registrar.add("Avatar.Profile",			boost::bind(&LLAvatarActions::showProfile,				id));
		registrar.add("Avatar.AddFriend",		boost::bind(&LLAvatarActions::requestFriendshipDialog,	id));
		registrar.add("Avatar.RemoveFriend",	boost::bind(&LLAvatarActions::removeFriendDialog, 		id));
		registrar.add("Avatar.IM",				boost::bind(&LLAvatarActions::startIM,					id));
		registrar.add("Avatar.Call",			boost::bind(&LLAvatarActions::startCall,				id));
		registrar.add("Avatar.OfferTeleport",	boost::bind(&PeopleContextMenu::offerTeleport,			this));
		registrar.add("Avatar.ZoomIn",			boost::bind(&handle_zoom_to_object,						id));
		registrar.add("Avatar.ShowOnMap",		boost::bind(&LLAvatarActions::showOnMap,				id));
		registrar.add("Avatar.Share",			boost::bind(&LLAvatarActions::share,					id));
		registrar.add("Avatar.Pay",				boost::bind(&LLAvatarActions::pay,						id));
		registrar.add("Avatar.BlockUnblock",	boost::bind(&LLAvatarActions::toggleBlock,				id));
		registrar.add("Avatar.InviteToGroup",	boost::bind(&LLAvatarActions::inviteToGroup,			id));
		registrar.add("Avatar.TeleportRequest",	boost::bind(&PeopleContextMenu::requestTeleport,		this));
		registrar.add("Avatar.Calllog",			boost::bind(&LLAvatarActions::viewChatHistory,			id));
		registrar.add("Avatar.Freeze",			boost::bind(&handle_avatar_freeze,						id));
		registrar.add("Avatar.Eject",			boost::bind(&handle_avatar_eject,						id));
		registrar.add("Avatar.TeleportHome",	boost::bind(&LLAvatarActions::teleportHome,				id));
		registrar.add("Avatar.EstateBan",		boost::bind(&LLAvatarActions::estateBan,				id));
		registrar.add("Avatar.CopyName",		boost::bind(&LLAvatarActions::copyName,					id));
		registrar.add("Avatar.CopyUUID",		boost::bind(&LLAvatarActions::copyUUID,					id));
		registrar.add("Avatar.CopyProfileSLURL",	boost::bind(&LLAvatarActions::copyProfileSLURL,		id));
		registrar.add("Avatar.GrantOnlineStatus",	boost::bind(&PeopleContextMenu::handle_avatar_grant_online_status,	this, id));
		registrar.add("Avatar.GrantMapLocation",	boost::bind(&PeopleContextMenu::handle_avatar_grant_map_location,	this, id));
		registrar.add("Avatar.GrantModifyObjects",	boost::bind(&PeopleContextMenu::handle_avatar_grant_modify_objects,	this, id));

		enable_registrar.add("Avatar.EnableItem", boost::bind(&PeopleContextMenu::enableContextMenuItem, this, _2));
		enable_registrar.add("Avatar.CheckItem",  boost::bind(&PeopleContextMenu::checkContextMenuItem,	this, _2));

		// create the context menu from the XUI
		menu = createFromFile("menu_people_nearby.xml");
		buildContextMenu(*menu, 0x0);
	}
	else
	{
		// Set up for multi-selected People

		// registrar.add("Avatar.AddFriend",	boost::bind(&LLAvatarActions::requestFriendshipDialog,	mUUIDs)); // *TODO: unimplemented
		registrar.add("Avatar.IM",				boost::bind(&LLAvatarActions::startConference,			mUUIDs, LLUUID::null));
		registrar.add("Avatar.Call",			boost::bind(&LLAvatarActions::startAdhocCall,			mUUIDs, LLUUID::null));
		registrar.add("Avatar.OfferTeleport",	boost::bind(&PeopleContextMenu::offerTeleport,			this));
		registrar.add("Avatar.RemoveFriend",	boost::bind(&LLAvatarActions::removeFriendsDialog,		mUUIDs));
		// registrar.add("Avatar.Share",		boost::bind(&LLAvatarActions::startIM,					mUUIDs)); // *TODO: unimplemented
		// registrar.add("Avatar.Pay",			boost::bind(&LLAvatarActions::pay,						mUUIDs)); // *TODO: unimplemented
		
		enable_registrar.add("Avatar.EnableItem",	boost::bind(&PeopleContextMenu::enableContextMenuItem, this, _2));

		// create the context menu from the XUI
		menu = createFromFile("menu_people_nearby_multiselect.xml");
		buildContextMenu(*menu, ITEM_IN_MULTI_SELECTION);
	}

    return menu;
}

void PeopleContextMenu::buildContextMenu(class LLMenuGL& menu, U32 flags)
{
    menuentry_vec_t items;
    menuentry_vec_t disabled_items;
	
	if (flags & ITEM_IN_MULTI_SELECTION)
	{
		items.push_back(std::string("add_friends"));
		items.push_back(std::string("remove_friends"));
		items.push_back(std::string("im"));
		items.push_back(std::string("call"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("offer_teleport"));
	}
	else 
	{
		items.push_back(std::string("view_profile"));
		items.push_back(std::string("im"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("request_teleport"));
		items.push_back(std::string("voice_call"));
		items.push_back(std::string("chat_history"));
		items.push_back(std::string("copy_to_clipboard"));
		items.push_back(std::string("copy_name"));
		items.push_back(std::string("copy_uuid"));
		items.push_back(std::string("copy_profile_uri"));
		items.push_back(std::string("add_friend"));
		items.push_back(std::string("remove_friend"));
		items.push_back(std::string("invite_to_group"));
		items.push_back(std::string("map"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("separator_blockunblock"));
		items.push_back(std::string("block_unblock"));

		//
		//	enable the following if the selected avatar is a friend
		//
		if (LLAvatarActions::isFriend(mUUIDs.front())) {
			items.push_back(std::string("permissions"));
			items.push_back(std::string("permission_online_status"));
			items.push_back(std::string("permission_map_location"));
			items.push_back(std::string("permission_modify_objects"));
		}
	}

    hide_context_entries(menu, items, disabled_items);
}

bool PeopleContextMenu::enableContextMenuItem(const LLSD& userdata)
{
	if(gAgent.getID() == mUUIDs.front())
	{
		return false;
	}
	std::string item = userdata.asString();

	// Note: can_block and can_delete is used only for one person selected menu
	// so we don't need to go over all uuids.

	if (item == std::string("can_block"))
	{
		const LLUUID& id = mUUIDs.front();
		return LLAvatarActions::canBlock(id);
	}
	else if (item == std::string("can_add"))
	{
		// We can add friends if:
		// - there are selected people
		// - and there are no friends among selection yet.

		//EXT-7389 - disable for more than 1
		if(mUUIDs.size() > 1)
		{
			return false;
		}

		bool result = (mUUIDs.size() > 0);

		uuid_vec_t::const_iterator
			id = mUUIDs.begin(),
			uuids_end = mUUIDs.end();

		for (;id != uuids_end; ++id)
		{
			if ( LLAvatarActions::isFriend(*id) )
			{
				result = false;
				break;
			}
		}

		return result;
	}
	else if (item == std::string("can_delete"))
	{
		// We can remove friends if:
		// - there are selected people
		// - and there are only friends among selection.

		bool result = (mUUIDs.size() > 0);

		uuid_vec_t::const_iterator
			id = mUUIDs.begin(),
			uuids_end = mUUIDs.end();

		for (;id != uuids_end; ++id)
		{
			if ( !LLAvatarActions::isFriend(*id) )
			{
				result = false;
				break;
			}
		}

		return result;
	}
	else if (item == std::string("can_call"))
	{
		return LLAvatarActions::canCall();
	}
	else if (item == std::string("can_zoom_in"))
	{
		const LLUUID& id = mUUIDs.front();

		return gObjectList.findObject(id);
	}
	else if (item == std::string("can_show_on_map"))
	{
		const LLUUID& id = mUUIDs.front();

		return (LLAvatarTracker::instance().isBuddyOnline(id) && is_agent_mappable(id))
					|| gAgent.isGodlike();
	}
	else if(item == std::string("can_offer_teleport"))
	{
		return LLAvatarActions::canOfferTeleport(mUUIDs);
	}
	else if (item == std::string("can_callog"))
	{
		return LLLogChat::isTranscriptExist(mUUIDs.front());
	}
	else if (item == std::string("can_im") || item == std::string("can_invite") ||
	         item == std::string("can_share") || item == std::string("can_pay"))
	{
		return true;
	}
	return false;
}

bool PeopleContextMenu::checkContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLUUID& id = mUUIDs.front();

	if (item == std::string("is_blocked"))
	{
		return LLAvatarActions::isBlocked(id);
	}

	const LLRelationship *relationship = LLAvatarTracker::instance().getBuddyInfo(id);

	if (relationship) {
		S32 rights = relationship->getRightsGrantedTo();

		if (item == std::string("online_status") &&
		   (rights & LLRelationship::GRANT_ONLINE_STATUS)
		) {
			return true;
		}

		if (item == std::string("map_location") &&
		   (rights & LLRelationship::GRANT_MAP_LOCATION)
		) {
			return true;
		}

		if (item == std::string("modify_objects") &&
		   (rights & LLRelationship::GRANT_MODIFY_OBJECTS)
		) {
			return true;
		}
	}

	return false;
}

void PeopleContextMenu::requestTeleport()
{
	// boost::bind cannot recognize overloaded method LLAvatarActions::teleportRequest(),
	// so we have to use a wrapper.
	LLAvatarActions::teleportRequest(mUUIDs.front());
}

void PeopleContextMenu::offerTeleport()
{
	// boost::bind cannot recognize overloaded method LLAvatarActions::offerTeleport(),
	// so we have to use a wrapper.
	LLAvatarActions::offerTeleport(mUUIDs);
}

void PeopleContextMenu::handle_avatar_grant_online_status(const LLUUID& avatar_id)
{
	toggle_rights(avatar_id, LLRelationship::GRANT_ONLINE_STATUS);
}

void PeopleContextMenu::handle_avatar_grant_map_location(const LLUUID& avatar_id)
{
	toggle_rights(avatar_id, LLRelationship::GRANT_MAP_LOCATION);
}

void PeopleContextMenu::handle_avatar_grant_modify_objects(const LLUUID& avatar_id)
{
	toggle_rights(avatar_id, LLRelationship::GRANT_MODIFY_OBJECTS);
}

void PeopleContextMenu::toggle_rights(const LLUUID& avatar_id, S32 rights)
{
	const LLRelationship *relationship = LLAvatarTracker::instance().getBuddyInfo(avatar_id);

	if (relationship) {
		S32 current_rights = relationship->getRightsGrantedTo();

		if (rights & LLRelationship::GRANT_MODIFY_OBJECTS) {
			//
			//	modify objects permission is being changed so
			//	request confirmation
			//
			confirm_modify_rights(avatar_id, !(current_rights & LLRelationship::GRANT_MODIFY_OBJECTS), current_rights | rights);
			return;
		}

		rights = current_rights ^ rights;
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(avatar_id, rights);

		LLSD args;
		args["NAME"] = LLSLURL("agent", avatar_id, "displayname").getSLURLString();
		args["GRID_NAME"] = LLGridManager::getInstance()->getGridLabel();
		LLNotificationsUtil::add("NewPermissionsPending", args);
	}
}

void PeopleContextMenu::confirm_modify_rights(const LLUUID &avatar_id, const bool grant, const S32 rights)
{
	LLSD args;
	args["NAME"] = LLSLURL("agent", avatar_id, "displayname").getSLURLString();

	if (grant) {
		LLNotificationsUtil::add(
			"GrantModifyRights",
			args,
			LLSD(),
			boost::bind(&PeopleContextMenu::rights_confirmation_callback, this, _1, _2, avatar_id, rights)
		);
	}
	else {
		LLNotificationsUtil::add(
			"RevokeModifyRights",
			args,
			LLSD(),
			boost::bind(&PeopleContextMenu::rights_confirmation_callback, this, _1, _2, avatar_id, rights & ~LLRelationship::GRANT_MODIFY_OBJECTS)
		);
	}
}

void PeopleContextMenu::rights_confirmation_callback(const LLSD& notification, const LLSD& response, const LLUUID &avatar_id, const S32 rights)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (option == 0) {
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(
			avatar_id,
			rights
		);

		LLSD args;
		args["NAME"] = LLSLURL("agent", avatar_id, "displayname").getSLURLString();
		args["GRID_NAME"] = LLGridManager::getInstance()->getGridLabel();
		LLNotificationsUtil::add("NewPermissionsPending", args);
	}
}

//== NearbyPeopleContextMenu ===============================================================

void NearbyPeopleContextMenu::buildContextMenu(class LLMenuGL& menu, U32 flags)
{
    menuentry_vec_t items;
    menuentry_vec_t disabled_items;
	
	if (flags & ITEM_IN_MULTI_SELECTION)
	{
		items.push_back(std::string("add_friends"));
		items.push_back(std::string("remove_friends"));
		items.push_back(std::string("im"));
		items.push_back(std::string("call"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("offer_teleport"));
	}
	else 
	{
		items.push_back(std::string("view_profile"));
		items.push_back(std::string("im"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("request_teleport"));
		items.push_back(std::string("voice_call"));
		items.push_back(std::string("chat_history"));
		items.push_back(std::string("add_friend"));
		items.push_back(std::string("remove_friend"));
		items.push_back(std::string("copy_to_clipboard"));
		items.push_back(std::string("copy_name"));
		items.push_back(std::string("copy_uuid"));
		items.push_back(std::string("copy_profile_uri"));
		items.push_back(std::string("invite_to_group"));
		items.push_back(std::string("zoom_in"));
		items.push_back(std::string("map"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("separator_blockunblock"));
		items.push_back(std::string("block_unblock"));

		//
		//	enable the following if the selected avatar is a friend
		//
		if (LLAvatarActions::isFriend(mUUIDs.front())) {
			items.push_back(std::string("permissions"));
			items.push_back(std::string("permission_online_status"));
			items.push_back(std::string("permission_map_location"));
			items.push_back(std::string("permission_modify_objects"));
		}

		//
		//	enable the following if we can freeze/eject the
		//	selected avatar
		//
		if (enable_freeze_eject(mUUIDs.front())) {
			items.push_back(std::string("separator_freeze_eject"));
			items.push_back(std::string("freeze"));
			items.push_back(std::string("eject"));
		}

		//
		//	enable the following if we are an estate owner
		//	or manager
		//
		if (LLAvatarActions::isOnYourLand(mUUIDs.front())) {
			items.push_back(std::string("teleport_home"));
			items.push_back(std::string("estate_ban"));
		}
	}

    hide_context_entries(menu, items, disabled_items);
}

//== SuggestedFriendsContextMenu ===============================================================

LLContextMenu* SuggestedFriendsContextMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	LLContextMenu* menu;

	// Set up for one person selected menu
	const LLUUID& id = mUUIDs.front();
	registrar.add("Avatar.Profile",			boost::bind(&LLAvatarActions::showProfile,				id));
	registrar.add("Avatar.AddFriend",		boost::bind(&LLAvatarActions::requestFriendshipDialog,	id));

	// create the context menu from the XUI
	menu = createFromFile("menu_people_nearby.xml");
	buildContextMenu(*menu, 0x0);

	return menu;
}

void SuggestedFriendsContextMenu::buildContextMenu(class LLMenuGL& menu, U32 flags)
{ 
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	items.push_back(std::string("view_profile"));
	items.push_back(std::string("add_friend"));

	hide_context_entries(menu, items, disabled_items);
}

} // namespace LLPanelPeopleMenus

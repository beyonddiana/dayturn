/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llconversationlog.h"
#include "llsearcheditor.h"

#include "boost/move/unique_ptr.hpp" // <GN> Revert for 64-bit builds

namespace boost
{
	using ::boost::movelib::unique_ptr; // move unique_ptr into the boost namespace. Remove for 64-bit builds
}

class LLConversationLogObserver;
class LLPanelPreference;
class LLPanelLCD;
class LLPanelDebug;
class LLMessageSystem;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLSD;
class LLTextBox;

namespace ll
{
	namespace prefs
	{
		struct SearchData;
	}
}

typedef std::map<std::string, std::string> notifications_map;

typedef enum
	{
		GS_LOW_GRAPHICS,
		GS_MID_GRAPHICS,
		GS_HIGH_GRAPHICS,
		GS_ULTRA_GRAPHICS
		
	} EGraphicsSettings;


// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater, public LLAvatarPropertiesObserver, public LLConversationLogObserver
{
public: 
	LLFloaterPreference(const LLSD& key);
	~LLFloaterPreference();

	void apply();
	void cancel();
	/*virtual*/ void draw();
	/*virtual*/ bool postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);
	/*virtual*/ void changed();
	/*virtual*/ void changed(const LLUUID& session_id, U32 mask) {};

	// static data update, called from message handler
	static void updateUserInfo(const std::string& visibility, bool im_via_email);

	// refresh all the graphics preferences menus
	static void refreshEnabledGraphics();
	
	// translate user's do not disturb response message according to current locale if message is default, otherwise do nothing
	static void initDoNotDisturbResponse();
	// update Show Favorites checkbox
	static void updateShowFavoritesCheckbox(bool val);
	void processProperties( void* pData, EAvatarProcessorType type );
	void processProfileProperties(const LLAvatarData* pAvatarData );
	void storeAvatarProperties( const LLAvatarData* pAvatarData );
	void saveAvatarProperties( void );
	void selectPrivacyPanel();
	void selectChatPanel();
    void getControlNames(std::vector<std::string>& names);

protected:	
	void		onBtnOK();
	void		onBtnCancel();
	void		onBtnApply();

	void		onClickClearCache();			// Clear viewer texture cache, vfs, and VO cache on next startup
	void		onClickBrowserClearCache();		// Clear web history and caches as well as viewer caches above
	void		onLanguageChange();
	void		onNotificationsChange(const std::string& OptionName);
	void		onNameTagOpacityChange(const LLSD& newvalue);

	// set value of "DoNotDisturbResponseChanged" in account settings depending on whether do not disturb response

	void onStreamMetadataAnnounceChanged();
	void onMiniMapChatRingChanged();
	void onShowLookAtChanged();
	void onShowPointAtChanged();
	void onNameTagShowAgeChanged();
	void onNameTagShowAgeLimitChanged();

	// string differs from default after user changes.
	void onDoNotDisturbResponseChanged();
	// if the custom settings box is clicked
	void onChangeCustom();
	void updateMeterText(LLUICtrl* ctrl);
	void onOpenHardwareSettings();
	// callback for defaults
	void setHardwareDefaults();
	// callback for when client turns on shaders
	void onVertexShaderEnable();

	// callback for commit in the "Single click on land" and "Double click on land" comboboxes.
	void onClickActionChange();
	// updates click/double-click action settings depending on controls values
	void updateClickActionSettings();
	// updates click/double-click action controls depending on values from settings.xml
	void updateClickActionControls();
	
	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.	
	void saveSettings();
		

public:

	void setCacheLocation(const LLStringExplicit& location);

	void onClickSetCache();
    void changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name);
	void onClickResetCache();
	void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
	void onSelectSkin();
	void onClickSetKey();
	void setKey(KEY key);
	void setMouse(LLMouseHandler::EClickType click);
	void onClickSetMiddleMouse();
	void onClickSetSounds();
	void onClickEnablePopup();
	void onClickDisablePopup();	
	void resetAllIgnored();
	void setAllIgnored();
	void onClickLogPath();
    void changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name);
	bool moveTranscriptsAndLog();
	void enableHistory();
	void setPersonalInfo(const std::string& visibility, bool im_via_email);
	void refreshEnabledState();
	void disableUnavailableSettings();
	void onCommitWindowedMode();
	void refresh();	// Refresh enable/disable
	// if the quality radio buttons are changed
	void onChangeQuality(const LLSD& data);
	
	void updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box);
	void refreshUI();

	void onCommitMediaEnabled();
	void onCommitMusicEnabled();
	void applyResolution();
	void onChangeMaturity();
	void onClickBlockList();
	void onClickProxySettings();
	void onClickTranslationSettings();
	void onClickPermsDefault();
	void onClickAutoReplace();
	void onClickSpellChecker();
	void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
	void getUIColor(LLUICtrl* ctrl, const LLSD& param);
	void onLogChatHistorySaved();	
	void buildPopupLists();
	static void refreshSkin(void* data);
	void selectPanel(const LLSD& name);

private:

	void onDeleteTranscripts();
	void onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response);
	void updateDeleteTranscriptsButton();

	static std::string sSkin;
	notifications_map mNotificationOptions;
	bool mClickActionDirty; ///< Set to true when the click/double-click options get changed by user.
	bool mGotPersonalInfo;
	bool mOriginalIMViaEmail;
	bool mLanguageChanged;
	bool mAvatarDataInitialized;
	std::string mPriorInstantMessageLogPath;
	
	bool mOriginalHideOnlineStatus;
	std::string mDirectoryVisibility;
	
	LLAvatarData mAvatarProperties;
	

	LLSearchEditor *mFilterEdit;
	// std::unique_ptr< ll::prefs::SearchData > mSearchData; <GN> Revert for 64-bit builds
	boost::unique_ptr< ll::prefs::SearchData > mSearchData;

	void onUpdateFilterTerm( bool force = false );
	void collectSearchableItems();	
};

class LLPanelPreference : public LLPanel
{
public:
	LLPanelPreference();
	/*virtual*/ bool postBuild();
	
	virtual ~LLPanelPreference();

	virtual void apply();
	virtual void cancel();
	void setControlFalse(const LLSD& user_data);
	virtual void setHardwareDefaults(){};

	// Disables "Allow Media to auto play" check box only when both
	// "Streaming Music" and "Media" are unchecked. Otherwise enables it.
	void updateMediaAutoPlayCheckbox(LLUICtrl* ctrl);

	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.
	virtual void saveSettings();

	void onChangePreset(const LLSD& user_data);
	void DeletePreset(const LLSD& user_data);
	void SavePreset(const LLSD& user_data);

	class Updater;

protected:
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;

private:
	//for "Only friends and groups can call or IM me"
	static void showFriendsOnlyWarning(LLUICtrl*, const LLSD&);
    //for  "Allow Multiple Viewers"
    static void showMultipleViewersWarning(LLUICtrl*, const LLSD&);
	//for "Show my Favorite Landmarks at Login"
	static void handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value);

	static void toggleMuteWhenMinimized();
	typedef std::map<std::string, LLColor4> string_color_map_t;
	string_color_map_t mSavedColors;

	Updater* mBandWidthUpdater;
};

class LLPanelPreferenceGraphics : public LLPanelPreference
{
public:
	bool postBuild();
	void draw();
	void apply();
	void cancel();
	void saveSettings();
	void setHardwareDefaults();
	void setPresetNamesInComboBox();
	static const std::string getPresetsPath();

protected:
	bool hasDirtyChilds();
	void resetDirtyChilds();
	
private:
 
 	void onPresetsListChange();
};

// <FS:AW  opensim preferences>
class LLPanelPreferenceOpensim : public LLPanelPreference
{
public:
	LLPanelPreferenceOpensim();
// <FS:AW  grid management>
	/*virtual*/ bool postBuild();
	/*virtual*/ void apply();
	/*virtual*/ void cancel();

protected:

	void onClickAddGrid();
	void addedGrid(bool success);
	void onClickClearGrid();
	void onClickRefreshGrid();
	void onClickRemoveGrid();
	void onSelectGrid();
	bool removeGridCB(const LLSD& notification, const LLSD& response);
// <FS:AW  opensim search support>
	void onClickClearDebugSearchURL();
	void onClickPickDebugSearchURL();
// </FS:AW  opensim search support>

	void refreshGridList(bool success = true);
	LLScrollListCtrl* mGridListControl;
private:
	LLLineEditor* mEditorGridName;
	LLLineEditor* mEditorGridURI;
	LLLineEditor* mEditorLoginPage;
	LLLineEditor* mEditorHelperURI;
	LLLineEditor* mEditorWebsite;
	LLLineEditor* mEditorSupport;
	LLLineEditor* mEditorRegister;
	LLLineEditor* mEditorPassword;
	LLLineEditor* mEditorSearch;
	LLLineEditor* mEditorGridMessage;

	LOG_CLASS(LLPanelPreferenceOpensim);
// </FS:AW  grid management>
};
// </FS:AW  opensim preferences>

class LLFloaterPreferenceProxy : public LLFloater
{
public: 
	LLFloaterPreferenceProxy(const LLSD& key);
	~LLFloaterPreferenceProxy();

	/// show off our menu
	static void show();
	void cancel();
	
protected:
	bool postBuild();
	void onOpen(const LLSD& key);
	void onClose(bool app_quitting);
	void saveSettings();
	void onBtnOk();
	void onBtnCancel();
	void onClickCloseBtn(bool app_quitting = false);

	void onChangeSocksSettings();

private:
	
	bool mSocksSettingsDirty;
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;

};


#endif  // LL_LLPREFERENCEFLOATER_H

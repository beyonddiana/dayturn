/**
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * With modifications Copyright (C) 2012, arminweatherwax@lavabit.com
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

#include "llappviewer.h"
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llsecapi.h"
#include "lltrans.h"
#include "llweb.h"
#include "llbufferstream.h"
#include "lllogininstance.h"        /// to check if logged in yet
#include "llnotificationsutil.h"
#include "llhttpclient.h"
#if LL_WINDOWS
#include <Winsock2.h>
#else
#include <unistd.h>
#endif
/// url base for update queries

class GridInfoRequestResponder : public LLHTTPClient::Responder
{
public:
	GridInfoRequestResponder(LLGridManager* owner, GridEntry* grid_data, LLGridManager::AddState state)
	{
		mOwner = owner;
		mData = grid_data;
		mState = state;

		llwarns << "hello " << this << llendl;
		mOwner->incResponderCount();
	}

	~GridInfoRequestResponder()
	{
		llwarns << "goodbye " << this << llendl;
	}


	/// the grid info is no LLSD *sigh* ... override the default LLSD parsing behaviour 
	virtual void completedRaw(
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer)
	{
		mOwner->decResponderCount();
		LL_DEBUGS("GridManager") << mData->grid[GRID_VALUE] << " status: " << getStatus() << " reason: " << getReason() << llendl;
		if(LLGridManager::TRYLEGACY == mState && 200 == getStatus())
		{
			mOwner->addGrid(mData, LLGridManager::SYSTEM);
		}
		else if (200 == getStatus())/// OK
		{
			LL_DEBUGS("GridManager") << "Parsing gridinfo xml file from "
				<< mData->grid[GRID_VALUE] << LL_ENDL;
			LLBufferStream istr(channels, buffer.get());
			if(LLXMLNode::parseStream( istr, mData->info_root, NULL))
			{
				mOwner->gridInfoResponderCB(mData);
			}
			else
			{
				LLSD args;
				args["GRID"] = mData->grid[GRID_VALUE];
				///Could not add [GRID] to the grid list.
				args["REASON"] = "Server provided broken grid info xml. Please";
				///[REASON] contact support of [GRID].
				LLNotificationsUtil::add("CantAddGrid", args);
				llwarns << " Could not parse grid info xml from server."
					<< mData->grid[GRID_VALUE] << " skipping." << llendl;
				mOwner->addGrid(mData, LLGridManager::FAIL);
			}
		}
		else if (304 == getStatus() && !LLGridManager::TRYLEGACY == mState) /// not modified
		{
			mOwner->addGrid(mData, LLGridManager::FINISH);
		}
		else if (LLGridManager::LOCAL == mState) ///always add localhost, no matter
		{
			mOwner->addGrid(mData,	LLGridManager::FINISH);
			///since we know now that its not up we could also start it
		}
		else
		{
			error(getStatus(), getReason());
		}
	}


	virtual void error(U32 status, const std::string& reason)
	{
		if (504 == status)/// gateway timeout ... well ... retry once >_>
		{
			if (LLGridManager::FETCH == mState)
			{
				mOwner->addGrid(mData,	LLGridManager::RETRY);
			}

		}
		else if (LLGridManager::TRYLEGACY == mState) ///we did TRYLEGACY and faild
		{
			LLSD args;
			args["GRID"] = mData->grid[GRID_VALUE];
			///Could not add [GRID] to the grid list.
			std::string reason_dialog = "Server didn't provide grid info: ";
			reason_dialog.append(mData->last_http_error);
			reason_dialog.append("\nPlease check if the loginuri is correct and");
			args["REASON"] = reason_dialog;
			///[REASON] contact support of [GRID].
			LLNotificationsUtil::add("CantAddGrid", args);

			llwarns << "No legacy login page. Giving up for " << mData->grid[GRID_VALUE] << llendl;
			mOwner->addGrid(mData, LLGridManager::FAIL);
		}
		else
		{
			/// remember the error we got when trying to get grid info where we expect it
			std::ostringstream last_error;
			last_error << status << " " << reason;
			mData->last_http_error = last_error.str();

			mOwner->addGrid(mData, LLGridManager::TRYLEGACY);
		}

	}

private:
	LLGridManager* mOwner;
	GridEntry* mData;
	LLGridManager::AddState mState;
};


const std::string  GRID_UPDATE_SERVICE_URL = "update_query_url_base";
const std::string SL_UPDATE_QUERY_URL = "https://update.secondlife.com/update";
const std::string MAIN_GRID_LOGIN_URI = "https://login.agni.lindenlab.com/cgi-bin/login.cgi";
const char* DEFAULT_LOGIN_PAGE = "http://viewer-login.agni.lindenlab.com/";

const char* SYSTEM_GRID_SLURL_BASE = "secondlife://%s/secondlife/";
const char* MAIN_GRID_SLURL_BASE = "http://maps.secondlife.com/secondlife/";
const char* SYSTEM_GRID_APP_SLURL_BASE = "secondlife:///app";

const char* DEFAULT_HOP_BASE = "hop://%s/";
const char* DEFAULT_APP_SLURL_BASE = "x-grid-location-info://%s/app";


LLGridManager::LLGridManager()
:	mIsInSLMain(false),
	mIsInSLBeta(false),
	mIsInOpenSim(false),
	mReadyToLogin(false),
	mCommandLineDone(false),
	mResponderCount(0)
{
	mGrid.clear();
	mGridList = LLSD();
}
void LLGridManager::resetGrids()
{
	initGrids();
	if (!mGrid.empty())
	{
		setGridData(mConnectedGrid);
	}
}

void LLGridManager::initGrids()
{

	mGridList = LLSD();

	std::string grid_fallback_file  = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,  "grids.fallback.xml");

	std::string grid_user_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,  "grids.user.xml");

	std::string grid_remote_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,  "grids.remote.xml");


	mGridFile = grid_user_file;

	initSystemGrids();
	initGridList(grid_fallback_file, FINISH);
	initGridList(grid_remote_file, FINISH);
	initGridList(grid_user_file, FINISH);

	if(!mCommandLineDone)
	{
		initCmdLineGrids();
	}
}


void LLGridManager::initSystemGrids()
{
	std::string add_grid_item = LLTrans::getString("ServerComboAddGrid");

/// we get this now from the grid list

	addSystemGrid("Second Life Main Grid (Agni)",
							  MAINGRID,
							  MAIN_GRID_LOGIN_URI,
							  "https://secondlife.com/helpers/",
							  DEFAULT_LOGIN_PAGE,
							  SL_UPDATE_QUERY_URL,
							  "Agni");
	addSystemGrid("Second Life Beta Test Grid (Aditi)",
							  "util.aditi.lindenlab.com",
							  "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",
							  "http://aditi-secondlife.webdev.lindenlab.com/helpers/",
							  DEFAULT_LOGIN_PAGE,
							  SL_UPDATE_QUERY_URL,
							  "Aditi");

}


void LLGridManager::initGridList(std::string grid_file, AddState state)
{
	LLSD other_grids;
	llifstream llsd_xml;
	if (grid_file.empty())
	{
		return;
	}

	if(!gDirUtilp->fileExists(grid_file))
	{
		return;
	}

	llsd_xml.open( grid_file.c_str(), std::ios::in | std::ios::binary );

	/// parse through the gridfile
	if( llsd_xml.is_open()) 
	{
		LLSDSerialize::fromXMLDocument( other_grids, llsd_xml );
		if(other_grids.isMap())
		{
			for(LLSD::map_iterator grid_itr = other_grids.beginMap(); 
				grid_itr != other_grids.endMap();
				++grid_itr)
			{
				LLSD::String key_name = grid_itr->first;
				GridEntry* grid_entry = new GridEntry;
				grid_entry->set_current = false;
				grid_entry->grid = grid_itr->second;

				LL_DEBUGS("InitGridList","GridManager") << "reading: " << key_name << LL_ENDL;

				try
				{
					addGrid(grid_entry, state);
					LL_DEBUGS("InitGridList","GridManager")<< "Added grid: " << key_name << LL_ENDL;
				}
				catch (LLInvalidGridName ex)
				{
				}

			}
			llsd_xml.close();
		}
	}

}

void LLGridManager::initCmdLineGrids()
{
	mCommandLineDone = true;
/**
	// load a grid from the command line.
	// if the actual grid name is specified from the command line,
	// set it as the 'selected' grid.
*/
	std::string cmd_line_login_uri = gSavedSettings.getString("CmdLineLoginURI1");
	if ((!cmd_line_login_uri.empty()))
	{	
		mGrid = cmd_line_login_uri;
		gSavedSettings.setString("CmdLineLoginURI1",std::string());	///in case setGridChoice tries to addGrid 
	}
	std::string grid;


 	if (!cmd_line_login_uri.empty())
	{	
		grid = cmd_line_login_uri;

		/// clear in case setGridChoice tries to addGrid and addGrid recurses here;
		/// however this only happens refetching all grid infos.
		gSavedSettings.setString("CmdLineLoginURI1",std::string());
		LL_DEBUGS("GridManager") << "Setting grid from --loginuri " << grid << LL_ENDL;
		setGridChoice(grid);
		return;
	}

	std::string cmd_line_grid = gSavedSettings.getString("CmdLineGridChoice");
	if(!cmd_line_grid.empty())
	{

		/// try to find the grid assuming the command line parameter is
		/// the case-insensitive 'label' of the grid.  ie 'Agni'
		gSavedSettings.setString("CmdLineGridChoice",std::string());
		grid = getGridByGridNick(cmd_line_grid);

		if(grid.empty())
		{
			grid = getGridByLabel(cmd_line_grid);

		}
		if(grid.empty())
		{
			/**
			// if we couldn't find it, assume the
			// requested grid is the actual grid 'name' or index,
			// which would be the dns name of the grid (for non
			// linden hosted grids)
			// If the grid isn't there, that's ok, as it will be
			// automatically added later.
			*/
			grid = cmd_line_grid;
		}
	}

	else
	{
/**
		// if a grid was not passed in via the command line, grab it from the CurrentGrid setting.
		// if there's no current grid, that's ok as it'll be either set by the value passed
		// in via the login uri if that's specified, or will default to maingrid
*/
		grid = gSavedSettings.getString("CurrentGrid");
		LL_DEBUGS("initCmdLineGrids","GridManager") << "Setting grid from last selection " << grid << LL_ENDL;
	}

	if(grid.empty())
	{
		/// no grid was specified so default to maingrid
		LL_DEBUGS("initCmdLineGrids","GridManager") << "Setting grid to MAINGRID as no grid has been specified " << LL_ENDL;
		grid = MAINGRID;
	}
/**
	generate a 'grid list' entry for any command line parameter overrides
	or setting overides that we'll add to the grid list or override
	any grid list entries with.
*/
	GridEntry* grid_entry = new GridEntry;	
	if(mGridList.has(grid))
	{
 		grid_entry->grid = mGridList[grid];
		LL_DEBUGS("initCmdLineGrids","GridManager") << "Setting commandline grid " << grid << LL_ENDL;
		setGridChoice(grid);
	}
	else
	{
		LL_DEBUGS("initCmdLineGrids","GridManager") << "Trying to fetch commandline grid " << grid << LL_ENDL;
		grid_entry->set_current = true;
		grid_entry->grid = LLSD::emptyMap();	
		grid_entry->grid[GRID_VALUE] = grid;

		/// add the grid with the additional values, or update the
		/// existing grid if it exists with the given values
		try
		{
			addGrid(grid_entry, FETCH);
		}
		catch(LLInvalidGridName ex)
		{

		}
    }
}



LLGridManager::~LLGridManager()
{

}

void LLGridManager::getGridData(const std::string &grid, LLSD& grid_info)
{

	grid_info = mGridList[grid]; 

	

}
void LLGridManager::gridInfoResponderCB(GridEntry* grid_entry)
{
	for (LLXMLNode* node = grid_entry->info_root->getFirstChild(); node != NULL; node = node->getNextSibling())
	{
		std::string check;
		check = "login";
		if (node->hasName(check))
		{
			// allow redirect but not spoofing
			LLURI uri (node->getTextContents());
			std::string authority = uri.authority();
			if(! authority.empty())
			{
				grid_entry->grid[GRID_VALUE] = authority;
			}

			grid_entry->grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
			grid_entry->grid[GRID_LOGIN_URI_VALUE].append(node->getTextContents());
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[GRID_LOGIN_URI_VALUE] << LL_ENDL;
			continue;
		}
		check = "gridname";
		if (node->hasName(check))
		{
			grid_entry->grid[GRID_LABEL_VALUE] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[GRID_LABEL_VALUE] << LL_ENDL;
			continue;
		}
		check = "gridnick";
		if (node->hasName(check))
		{
			grid_entry->grid[check] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[check] << LL_ENDL;
			continue;
		}
		check = "gatekeeper";
		if (node->hasName(check))
		{
			LLURI gatekeeper(node->getTextContents());
			grid_entry->grid[check] = gatekeeper.authority();

			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[check] << LL_ENDL;
			continue;
		}
		check = "welcome";
		if (node->hasName(check))
		{
			grid_entry->grid[GRID_LOGIN_PAGE_VALUE] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[GRID_LOGIN_PAGE_VALUE] << LL_ENDL;
			continue;
		}
		check = GRID_REGISTER_NEW_ACCOUNT;
		if (node->hasName(check))
		{
			grid_entry->grid[check] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[check] << LL_ENDL;
			continue;
		}
		check = GRID_FORGOT_PASSWORD;
		if (node->hasName(check))
		{
			grid_entry->grid[check] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[check] << LL_ENDL;
			continue;
		}
		check = "help";
		if (node->hasName(check))
		{
			grid_entry->grid[check] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[check] << LL_ENDL;
			continue;
		}
		check = "about";
		if (node->hasName(check))
		{
			grid_entry->grid[check] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[check] << LL_ENDL;
			continue;
		}
		check = "helperuri";
		if (node->hasName(check))
		{
			grid_entry->grid[GRID_HELPER_URI_VALUE] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[GRID_HELPER_URI_VALUE] << LL_ENDL;
			///don't continue, also check the next
		}
		check = "economy";
		if (node->hasName(check))
		{	///sic! economy and helperuri is 2 names for the same
			grid_entry->grid[GRID_HELPER_URI_VALUE] = node->getTextContents();
			LL_DEBUGS("GridManager") << "[\""<<check<<"\"]: " << grid_entry->grid[GRID_HELPER_URI_VALUE] << LL_ENDL;
		}
	}

	std::string slurl_base = "hop://";
	slurl_base.append(grid_entry->grid[GRID_VALUE]);
	slurl_base.append("/");
	grid_entry->grid[GRID_SLURL_BASE]= slurl_base;
	LLDate now = LLDate::now();
	grid_entry->grid["LastModified"] = now;
	addGrid(grid_entry, FINISH);
}

void LLGridManager::addGrid(const std::string& loginuri)
{
	GridEntry* grid_entry = new GridEntry;
	grid_entry->set_current = true;
	grid_entry->grid = LLSD::emptyMap();	
	grid_entry->grid[GRID_VALUE] = loginuri;
	addGrid(grid_entry, FETCH);
}

void LLGridManager::addGrid(GridEntry* grid_entry,  AddState state)
{
	if(!grid_entry)
	{
		llwarns << "addGrid called with NULL grid_entry. Please send a bug report." << llendl;
		state = FAIL;
	}
	if(!grid_entry->grid.has(GRID_VALUE))
	{
		state = FAIL;
	}
	else if(grid_entry->grid[GRID_VALUE].asString().empty())
	{
		state = FAIL;
	}
	else if(!grid_entry->grid.isMap())
	{
		state = FAIL;
	}


	if ((FETCH == state) ||(FETCHTEMP == state) || (SYSTEM == state))
	{
		std::string grid = utf8str_tolower(grid_entry->grid[GRID_VALUE]);
		/// grid should be in the form of a dns address
		/// but also support localhost:9000 or localhost:9000/login
		bool is_special_entry = (grid.find("<") == 0 
					&& grid.rfind(">") == grid.length() -1);
		if (is_special_entry)
			{
			LLSD override;
			grid_entry->grid = override;
			grid_entry->grid[GRID_VALUE] = grid;
			grid_entry->grid["FLAG_TEMPORARY"] = "TRUE";
			state = FINISH;
		}
		else if (!grid.empty() && grid.find_first_not_of("abcdefghijklmnopqrstuvwxyz1234567890-_.:/@% ") != std::string::npos)
		{
		 /// grid should be in the form of a dns address
		 /// but also support localhost:9000 or localhost:9000/login
			LLSD args;
			args["GRID"] = grid;
			LLNotificationsUtil::add("InvalidGrid", args);
			state = FAIL;
		}

		/// trim last slash
		size_t pos = grid.find_last_of("/");
		if ( (grid.length()-1) == pos )
		{
			if (!mGridList.has(grid))/// deal with hand edited entries *sigh*
			{
			grid.erase(pos);
			grid_entry->grid[GRID_VALUE]  = grid;
			}
		}

 		/// trim region from hypergrid uris
		std::string  grid_trimmed = trimHypergrid(grid);
 		if (grid_trimmed != grid)
		{
			grid = grid_trimmed;
			grid_entry->grid[GRID_VALUE]  = grid;
			grid_entry->grid["HG"] = "TRUE";
		}

		if (FETCHTEMP == state)
		{
			grid_entry->grid["FLAG_TEMPORARY"] = "TRUE";
			state = FETCH;
		}

	}

	if ((FETCH == state) || (RETRY == state))
	{
		std::string grid = utf8str_tolower(grid_entry->grid[GRID_VALUE]);


		std::string match = "://";
		size_t find_scheme = grid.find(match);
		if ( std::string::npos != find_scheme)
		{
			/// We only support http so just remove anything the user might have chosen
			grid.erase(0,find_scheme+match.length());
			grid_entry->grid[GRID_VALUE]  = grid;
		}

		std::string uri = "http://"+grid;

		if (std::string::npos != uri.find("lindenlab.com"))
		{
			state = SYSTEM;
		}
		else
		{
			char host_name[255];
			host_name[254] ='\0';
			gethostname(host_name, 254);
			if (std::string::npos != uri.find(host_name)||
				std::string::npos != uri.find("127.0.0.1")
				|| std::string::npos != uri.find("localhost") )
			{
	
				LL_DEBUGS("GridManager") << "state = LOCAL" << LL_ENDL;
				state = LOCAL;
			}

			grid_entry->grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
			grid_entry->grid[GRID_LOGIN_URI_VALUE].append(uri);

			size_t pos = uri.find_last_of("/");
			if ( (uri.length()-1) != pos )
			{
				uri.append("/");
			}
			uri.append("get_grid_info");
	
			LL_DEBUGS("GridManager") << "get_grid_info uri: " << uri << LL_ENDL;
	
			time_t last_modified = 0;
			if(grid_entry->grid.has("LastModified"))
			{
				LLDate saved_value = grid_entry->grid["LastModified"];
				last_modified = (time_t)saved_value.secondsSinceEpoch();
			}
	
			LLHTTPClient::getIfModified(uri, new GridInfoRequestResponder(this, grid_entry, state), last_modified);
			return;
		}
	}

	if(TRYLEGACY == state)
	{
		std::string grid = utf8str_tolower(grid_entry->grid[GRID_VALUE]);
		std::string uri = "https://";
		uri.append(grid);
		size_t pos = uri.find_last_of("/");
		if ( (uri.length()-1) != pos )
		{
			uri.append("/");
		}
		uri.append("cgi-bin/login.cgi");

		llwarns << "No gridinfo found. Trying if legacy login page exists: " << uri << llendl;
		LLHTTPClient::get(uri, new GridInfoRequestResponder(this, grid_entry, state));
		return;
	}

	if(FAIL != state && REMOVE != state)
	{
		std::string grid = utf8str_tolower(grid_entry->grid[GRID_VALUE]);

		/// populate the other values if they don't exist
		if (!grid_entry->grid.has(GRID_LABEL_VALUE)) 
				{
			grid_entry->grid[GRID_LABEL_VALUE] = grid;
			llwarns << "No \"gridname\" found in grid info, setting to " << grid_entry->grid[GRID_LABEL_VALUE].asString() << llendl;
				}


		if (!grid_entry->grid.has(GRID_NICK_VALUE))
				{
			grid_entry->grid[GRID_NICK_VALUE] = grid;
			llwarns << "No \"gridnick\" found in grid info, setting to " << grid_entry->grid[GRID_NICK_VALUE].asString() << llendl;
				}
	}

	if (SYSTEM == state)
	{
		std::string grid = utf8str_tolower(grid_entry->grid[GRID_VALUE]);

		/// if the grid data doesn't include any of the URIs, then
		/// generate them from the grid

		if (!grid_entry->grid.has(GRID_LOGIN_URI_VALUE))
				{
			grid_entry->grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
			grid_entry->grid[GRID_LOGIN_URI_VALUE].append(std::string("https://") + grid + "/cgi-bin/login.cgi");
			llwarns << "Adding Legacy Login Service at:" << grid_entry->grid[GRID_LOGIN_URI_VALUE].asString() << llendl;
				}

		/// Populate to the default values
		if (!grid_entry->grid.has(GRID_LOGIN_PAGE_VALUE)) 
				{
			grid_entry->grid[GRID_LOGIN_PAGE_VALUE] = std::string("http://") + grid + "/app/login/";
			llwarns << "Adding Legacy Login Screen at:" << grid_entry->grid[GRID_LOGIN_PAGE_VALUE].asString() << llendl;
				}
		if (!grid_entry->grid.has(GRID_HELPER_URI_VALUE)) 
				{
			llwarns << "Adding Legacy Economy at:" << grid_entry->grid[GRID_HELPER_URI_VALUE].asString() << llendl;
			grid_entry->grid[GRID_HELPER_URI_VALUE] = std::string("https://") + grid + "/helpers/";
				}

	}

	if(FAIL != state)
	{
	lldebugs << "GRID_VALUE:" << grid_entry->grid[GRID_VALUE] << llendl;
    std::string grid = utf8str_tolower(grid_entry->grid[GRID_VALUE]);
		if(grid.empty())
 		{
			state = FAIL;
		}
		else
		{
			bool list_changed = false;

			LLURI login_uri(grid_entry->grid[GRID_LOGIN_URI_VALUE].get(0).asString());

			if (login_uri.authority() != grid && !grid_entry->grid.has("USER_DELETED"))
			{
				grid = login_uri.authority();
				grid_entry->grid[GRID_VALUE] = grid;
			}

			if (!grid_entry->grid.has(GRID_LOGIN_IDENTIFIER_TYPES) &&!grid_entry->grid.has("USER_DELETED"))
				{
					/// non system grids and grids that haven't already been configured with values
					/// get both types of credentials.
					grid_entry->grid[GRID_LOGIN_IDENTIFIER_TYPES] = LLSD::emptyArray();
					grid_entry->grid[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_AGENT);
					grid_entry->grid[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_ACCOUNT);
				}
	
		bool is_current = grid_entry->set_current;
		grid_entry->set_current = false;
	
	
			if (!mGridList.has(grid)) ///new grid
			{

				if (!grid_entry->grid.has("USER_DELETED")
					&& !grid_entry->grid.has("DEPRECATED"))
				{
				///finally add the grid
				mGridList[grid] = grid_entry->grid;
					list_changed = true;
				LL_DEBUGS("GridManager") << "Adding new entry: " << grid << LL_ENDL;
			}
			else
			{
					LL_DEBUGS("GridManager") << "Removing entry marked for deletion: " << grid << LL_ENDL;
				}
			}
			else
			{
				LLSD existing_grid = mGridList[grid];
				if (existing_grid.has("DEPRECATED"))
				{
					LL_DEBUGS("GridManager") << "Removing entry marked as deprecated in the fallback list: " << grid << LL_ENDL;
				}
				else if (grid_entry->grid.has("USER_DELETED"))
				{
					/// entries from the fallback list can't be deleted
					/// hide them instead
					mGridList[grid] = grid_entry->grid;
					list_changed = true;
					LL_DEBUGS("GridManager") << "Entry marked for deletion: " << grid << LL_ENDL;
				}
				else if (!existing_grid.has("LastModified"))
				{
					///lack of "LastModified" means existing_grid is from fallback list,
					/// assume its anyway older and override with the new entry
					mGridList[grid] = grid_entry->grid;
					list_changed = true;
					LL_DEBUGS("GridManager") << "Using custom entry: " << grid << LL_ENDL;
				}
				else if (grid_entry->grid.has("LastModified"))
				{
					LLDate testing_newer = grid_entry->grid["LastModified"];
					LLDate existing = existing_grid["LastModified"];

					LL_DEBUGS("GridManager") << "testing_newer " << testing_newer
								<< " existing " << existing << LL_ENDL;

					if(testing_newer.secondsSinceEpoch() > existing.secondsSinceEpoch())
					{
						///existing_grid is older, override.
						mGridList[grid] = grid_entry->grid;
						list_changed = true;
						LL_DEBUGS("GridManager") << "Updating entry: " << grid << LL_ENDL;
					}
				}
				else
				{
					LL_DEBUGS("GridManager") << "Same or newer entry already present: " << grid << LL_ENDL;
				}

			}
	
			if(is_current)
			{
				LL_DEBUGS("GridManager") << "Selected grid is " << grid << LL_ENDL;		
				setGridChoice(grid);
			}

			if(list_changed)
			{
				mGridListChangedSignal(true);
				mGridListChangedSignal.disconnect_all_slots();
			}
		}
	}

/** This is only of use if we want to fetch infos of entire gridlists at startup

	if(grid_entry && FINISH == state || FAIL == state)
	{

		if((FINISH == state && !mCommandLineDone && 0 == mResponderCount)
			||(FAIL == state && grid_entry->set_current) )
		{
			LL_DEBUGS("GridManager") << "init CmdLineGrids"  << LL_ENDL;

			initCmdLineGrids();
		}
	}
*/

	if (FAIL == state)
	{
		mGridListChangedSignal(false);
		mGridListChangedSignal.disconnect_all_slots();
	}


	if (grid_entry)
	{
		delete grid_entry;
		grid_entry = NULL;
			}
}

void LLGridManager::addSystemGrid(const std::string& label,
															  const std::string& name,
															  const std::string& login_uri,
															  const std::string& helper,
															  const std::string& login_page,
															  const std::string& update_url_base,
															  const std::string& login_id)
{
	LLSD grid = LLSD::emptyMap();
	grid[GRID_VALUE] = name;
	grid[GRID_LABEL_VALUE] = label;
	grid[GRID_HELPER_URI_VALUE] = helper;
	grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
	grid[GRID_LOGIN_URI_VALUE].append(login_uri);
	grid[GRID_LOGIN_PAGE_VALUE] = login_page;
	grid[GRID_UPDATE_SERVICE_URL] = update_url_base;
	grid[GRID_IS_SYSTEM_GRID_VALUE] = true;
	grid[GRID_LOGIN_IDENTIFIER_TYPES] = LLSD::emptyArray();
	grid[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_AGENT);

	grid[GRID_APP_SLURL_BASE] = SYSTEM_GRID_APP_SLURL_BASE;
	if (login_id.empty())
	{
			grid[GRID_ID_VALUE] = name;
	}
	else
	{
			grid[GRID_ID_VALUE] = login_id;
	}

	if (name == std::string(MAINGRID))
	{
			grid[GRID_SLURL_BASE] = MAIN_GRID_SLURL_BASE;
	}
	else
	{
			grid[GRID_SLURL_BASE] = llformat(SYSTEM_GRID_SLURL_BASE, grid[GRID_ID_VALUE].asString().c_str());
	}

        addGrid(grid);
}
		
void LLGridManager::reFetchGrid(const std::string& grid, bool set_current)
{
	GridEntry* grid_entry = new GridEntry;
	grid_entry->grid[GRID_VALUE] = grid;
	grid_entry->set_current = set_current;
	addGrid(grid_entry, FETCH);
}

void LLGridManager::removeGrid(const std::string& grid)
{
	LL_DEBUGS("GridManager") << "Remove Grid: " << grid << LL_ENDL;
	GridEntry* grid_entry = new GridEntry;
	grid_entry->grid[GRID_VALUE] = grid;
	grid_entry->grid["USER_DELETED"]="TRUE";
	grid_entry->set_current = false;
	addGrid(grid_entry, REMOVE);
}

boost::signals2::connection LLGridManager::addGridListChangedCallback(grid_list_changed_callback_t cb)
{
	return mGridListChangedSignal.connect(cb);
}

/// return a list of grid name -> grid label mappings for UI purposes
std::map<std::string, std::string> LLGridManager::getKnownGrids()
{
	std::map<std::string, std::string> result;
	std::string key, value;

	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++)
	{
		if(!(grid_iter->second.has("DEPRECATED")))///use in fallback list only
		{
			key = grid_iter->first;
			value = grid_iter->second[GRID_LABEL_VALUE].asString();
			result[key] = value;
		}
	}

	key = mGridList[mGrid].asString(); ///current grid might be TEMPORARY
	value = mGridList[mGrid][GRID_LABEL_VALUE].asString();

	result[key] = value;

	return result;
}

void LLGridManager::setGridChoice(const std::string& grid)
{
	if(grid.empty()) return;
	if (LLLoginInstance::getInstance()->authSuccess())
	{
		return;
	}
/**
	// Set the grid choice based on a string.
	// The string can be:
	// - a grid label from the gGridInfo table 
	// - a hostname
	// - an ip address

	// loop through.  We could do just a hash lookup but we also want to match
	// on label
*/
	mReadyToLogin = false;
	std::string grid_name = grid;
	if(mGridList.has(grid_name))
	{
		LL_DEBUGS("GridManager")<< "got grid from grid list: " << grid << LL_ENDL;
	}
	else
	{
		/// case insensitive
		grid_name = getGridByLabel(grid);
		if (!grid_name.empty())
		{
			LL_DEBUGS("GridManager")<< "got grid by label: " << grid << LL_ENDL;
		}
	}
	if(grid_name.empty())
	{
		/// case insensitive
		grid_name = getGridByGridNick(grid);
		if (!grid_name.empty())
		{
			LL_DEBUGS("GridManager")<< "got grid by gridnick: " << grid << LL_ENDL;
		}
	}
	if(grid_name.empty())
	{
		LL_DEBUGS("GridManager")<< "trying to fetch grid: " << grid << LL_ENDL;
		/// the grid was not in the list of grids.
		GridEntry* grid_entry = new GridEntry;
		grid_entry->grid = LLSD::emptyMap();
		grid_entry->grid[GRID_VALUE] = grid;
		grid_entry->set_current = true;
		addGrid(grid_entry, FETCH);
	}
	else
	{
		LL_DEBUGS("GridManager")<< "setting grid choice: " << grid << LL_ENDL;
		mGrid = grid; /// AW: don't set mGrid anywhere else
		updateIsInProductionGrid();
		std::string add_grid_item = LLTrans::getString("ServerComboAddGrid");
		if(grid == add_grid_item)
		{
			mReadyToLogin = false;
		}
		else
		{
			getGridData(mConnectedGrid);
			gSavedSettings.setString("CurrentGrid", grid); 
			LLTrans::setDefaultArg("CURRENT_GRID", getGridLabel());///<FS:AW make CURRENT_GRID a default substitution>
			std::string aboutthisgrid = getGridLabel();
			mReadyToLogin = true;
		}
	}
}
std::string LLGridManager::getGridByProbing( const std::string &probe_for, bool case_sensitive)
{
	std::string ret;
	ret = getGridByHostName(probe_for, case_sensitive);
	if (ret.empty())
	{
		getGridByGridNick(probe_for, case_sensitive);
	}
	if (ret.empty())
	{
		getGridByLabel(probe_for, case_sensitive);
	}

	return ret;
}

std::string LLGridManager::getGridByLabel( const std::string &grid_label, bool case_sensitive)
{
	return grid_label.empty() ? std::string() : getGridByAttribute(GRID_LABEL_VALUE, grid_label, case_sensitive);
}

std::string LLGridManager::getGridByGridNick( const std::string &grid_nick, bool case_sensitive)
{
	return grid_nick.empty() ? std::string() : getGridByAttribute(GRID_NICK_VALUE, grid_nick, case_sensitive);
}
std::string LLGridManager::getGridByHostName( const std::string &host_name, bool case_sensitive)
{
	return host_name.empty() ? std::string() : getGridByAttribute(GRID_VALUE, host_name, case_sensitive);
}

std::string LLGridManager::getGridByAttribute( const std::string &attribute, const std::string &attribute_value, bool case_sensitive)
{
	if(attribute.empty()||attribute_value.empty())
		return std::string();

	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
	grid_iter != mGridList.endMap();
		grid_iter++)
	{
		if (grid_iter->second.has(attribute))
		{
			if (0 == (case_sensitive?LLStringUtil::compareStrings(attribute_value, grid_iter->second[attribute].asString()):
				LLStringUtil::compareInsensitive(attribute_value, grid_iter->second[attribute].asString())))
				{
					return grid_iter->first;
				}
		}
	}
	return std::string();
}
/// this assumes that there is anyway only one uri saved
std::string LLGridManager::getLoginURI(const std::string& grid)
{
	std::string ret;
	if (!grid.empty()
		&& mGridList.has(grid)
		&& mGridList[grid].has(GRID_LOGIN_URI_VALUE)
		&& mGridList[grid][GRID_LOGIN_URI_VALUE].isArray()
	   )
	{
		ret =  mGridList[grid][GRID_LOGIN_URI_VALUE].beginArray()->asString();
	}

	return ret;
}

void LLGridManager::getLoginURIs(std::vector<std::string>& uris)
{
	uris.clear();
	for (LLSD::array_iterator llsd_uri = mGridList[mGrid][GRID_LOGIN_URI_VALUE].beginArray();
		 llsd_uri != mGridList[mGrid][GRID_LOGIN_URI_VALUE].endArray();
			 llsd_uri++)
	{
		std::string current = llsd_uri->asString();
		if(!current.empty())
		{
			uris.push_back(current);
		}
	}
}

std::string LLGridManager::getHelperURI() 
{
	std::string cmd_line_helper_uri = gSavedSettings.getString("CmdLineHelperURI");
	if(!cmd_line_helper_uri.empty())
	{
		return cmd_line_helper_uri;	
	}
	return mGridList[mGrid][GRID_HELPER_URI_VALUE];
}

std::string LLGridManager::getLoginPage()
{
	/// override the login page if it was passed in
	std::string cmd_line_login_page = gSavedSettings.getString("LoginPage");
	if(!cmd_line_login_page.empty())
	{
		return cmd_line_login_page;
	}

	return mGridList[mGrid][GRID_LOGIN_PAGE_VALUE];
}

std::string LLGridManager::getUpdateServiceURL()
{
	std::string update_url_base = gSavedSettings.getString("CmdLineUpdateService");
		LL_INFOS("UpdaterService","GridManager")
			<< "Update URL base overridden from command line: " << update_url_base
			<< LL_ENDL;
	std::string last_known_grid = gSavedSettings.getString("CurrentGrid");
/**
* The requirement to not have update checking on non-sl grids is implemented by looking at
* current grid saved setting which will also have the last grid value at system start.
* There is a not 100 % way to unsure that non - sl grids check for updates.
* This approach uses the last known grid and if it was an sl grid sets the update url.
* If not it retains a blank grid_nick and that allows a failure mode. After return to
* caller in failure mode there will be a one time reset of 20 seconds, this is the case
* where the last known grid was not an sl grid but the user has selected 
* an sl gird at current log in.
*/
	std::string grid_nick = getGridNick();
	if (grid_nick.empty())
	{
		if (last_known_grid == MAINGRID)
		{
			grid_nick = "agni";
		}
		else if (last_known_grid == "util.aditi.lindenlab.com")
		{
			grid_nick = "aditi";
		}
	}
		LL_INFOS("UpdaterService","GridManager")
			<< "The grid nick: " << grid_nick
			<< LL_ENDL;
	if (  update_url_base.empty() && (grid_nick == "agni" || grid_nick == "aditi")) 
	{
		update_url_base = "https://update.secondlife.com/update";
				LL_INFOS("UpdaterService","GridManager")
			<< "Update URL base is using SecondLife default: " << update_url_base
			<< LL_ENDL;
		return update_url_base;
	}
	if ( !update_url_base.empty()  )
	{
		LL_INFOS("UpdaterService","GridManager")
			<< "Update URL base overridden from command line: " << update_url_base
			<< LL_ENDL;
	}
	else if ( mGridList[mGrid].has(GRID_UPDATE_SERVICE_URL) )
	{
		update_url_base = mGridList[mGrid][GRID_UPDATE_SERVICE_URL].asString();
	}
	else 
	{
		LL_WARNS("UpdaterService","GridManager")
			<< "The grid property '" << GRID_UPDATE_SERVICE_URL
			<< "' is not defined for the grid '" << mGrid << "'"
			<< LL_ENDL;
		
	}

	return update_url_base;
}


void LLGridManager::updateIsInProductionGrid()
{
	mIsInSLMain = false;
	mIsInSLBeta = false;
	mIsInOpenSim = false;

	/// *NOTE:Mani This used to compare GRID_INFO_AGNI to gGridChoice,
	/// but it seems that loginURI trumps that.
	std::vector<std::string> uris;
	getLoginURIs(uris);
	if(uris.empty())
	{
		LL_INFOS("GridManager") << "uri is empty, no grid type set." << LL_ENDL;
		return;
	}

	LLStringUtil::toLower(uris[0]);
	LLURI login_uri = LLURI(uris[0]);

	/// LL looks if "agni" is contained in the string for SL main grid detection.
	/// cool for http://agni.nastyfraud.com/steal.php#allyourmoney
	if( login_uri.authority().find("login.agni.lindenlab.com") ==  0 )
	{
		LL_DEBUGS("GridManager")<< "uri: "<<  login_uri.authority() << " set IsInSLMain" << LL_ENDL;
		gSimulatorType = "SecondLife";
		llinfos << "Simulator Type : " << gSimulatorType <<llendl;
		mIsInSLMain = true;
		return;
	}
	else if( login_uri.authority().find("lindenlab.com") !=  std::string::npos )//here is no real money
	{
		LL_DEBUGS("GridManager")<< "uri: "<< login_uri.authority() << " set IsInSLBeta" << LL_ENDL;
		gSimulatorType = "SecondLife";
		llinfos << "Simulator Type : " << gSimulatorType <<llendl;
		mIsInSLBeta = true;
		return;
	}
/**
	// TPVP compliance: a SL login screen must connect to SL.
	// NOTE: This is more TPVP compliant than LLs own viewer, where
	// setting the command line login page can be used for spoofing.
*/
	LLURI login_page = LLURI(getLoginPage());
	if(login_page.authority().find("lindenlab.com") !=  std::string::npos)
	{
		setGridChoice("login.agni.lindenlab.com");
		return;
	}

	LL_DEBUGS("GridManager")<< "uri: "<< login_uri.authority() << " set IsInOpenSim" << LL_ENDL;
	gSimulatorType = "OpenSim";
	llinfos << "Simulator Type : " << gSimulatorType <<llendl;
	mIsInOpenSim = true;
}

bool LLGridManager::isInSLMain()
{
	return mIsInSLMain;
}

bool LLGridManager::isInSLBeta()
{
	return mIsInSLBeta;
}

bool LLGridManager::isInOpenSim()
{
	return mIsInOpenSim;
}

void LLGridManager::saveGridList()
{
	LL_DEBUGS("GridManager") << __FUNCTION__ << LL_ENDL;

	LLSD output_grid_list = LLSD::emptyMap();
	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++)
	{
		if(!(grid_iter->first.empty() ||
			grid_iter->second.isUndefined() ||
			grid_iter->second.has("FLAG_TEMPORARY") ||
			grid_iter->second.has("DEPRECATED")))
			{
				output_grid_list[grid_iter->first] = grid_iter->second;
			}
	}
	llofstream llsd_xml;
	llsd_xml.open( mGridFile.c_str(), std::ios::out | std::ios::binary);	
	LLSDSerialize::toPrettyXML(output_grid_list, llsd_xml);
	llsd_xml.close();
}

std::string LLGridManager::trimHypergrid(const std::string& trim)
{
	std::size_t pos;
	std::string grid = trim;

	pos = grid.find_last_of(":");
	if (pos != std::string::npos)
	{
		std::string  part = grid.substr(pos+1, grid.length()-1);
		/// in hope numbers only is a good guess for it's a port number
		if (std::string::npos != part.find_first_not_of("1234567890/"))
		{
			///and erase if not
			grid.erase(pos,grid.length()-1);
		}
	}
	return grid;
}

/// build a slurl for the given region within the selected grid
std::string LLGridManager::getSLURLBase(const std::string& grid)
{
	std::string grid_base;
	std::string ret;
	std::string grid_trimmed = trimHypergrid(grid);

	if(mGridList.has(grid_trimmed) && mGridList[grid_trimmed].has(GRID_SLURL_BASE))
	{
		ret = mGridList[grid_trimmed][GRID_SLURL_BASE].asString();
		LL_DEBUGS("GridManager") << "GRID_SLURL_BASE: " << ret << LL_ENDL;
		}
		else
		{
		LL_DEBUGS("GridManager") << "Trying to fetch info for:" << grid << LL_ENDL;
		GridEntry* grid_entry = new GridEntry;
		grid_entry->set_current = false;
		grid_entry->grid = LLSD::emptyMap();	
		grid_entry->grid[GRID_VALUE] = grid;

		/// add the grid with the additional values, or update the
		/// existing grid if it exists with the given values
		addGrid(grid_entry, FETCHTEMP);

		/// deal with hand edited entries
		std::string grid_norm = grid;
		size_t pos = grid_norm.find_last_of("/");
		if ( (grid_norm.length()-1) == pos )
		{
			grid_norm.erase(pos);
		}
		ret = llformat(DEFAULT_HOP_BASE, grid_norm.c_str());
		LL_DEBUGS("GridManager") << "DEFAULT_HOP_BASE: " << ret  << LL_ENDL;
		}
	return  ret;
	}

/// build a slurl for the given region within the selected grid
std::string LLGridManager::getAppSLURLBase(const std::string& grid)
{
	std::string grid_base;
	std::string ret;

	if(mGridList.has(grid) && mGridList[grid].has(GRID_APP_SLURL_BASE))
	{
		ret = mGridList[grid][GRID_APP_SLURL_BASE].asString();
		}
		else
		{
		std::string app_base;
		if(mGridList.has(grid) && mGridList[grid].has(GRID_SLURL_BASE))
		{
			std::string grid_slurl_base = mGridList[grid][GRID_SLURL_BASE].asString();
			if( 0 == grid_slurl_base.find("hop://"))
			{
				app_base = DEFAULT_HOP_BASE;
				app_base.append("app");
			}
			else
			{
				app_base = DEFAULT_APP_SLURL_BASE;
			}
		}
		else 
		{
			app_base = DEFAULT_APP_SLURL_BASE;
		}

		/// deal with hand edited entries
		std::string grid_norm = grid;
		size_t pos = grid_norm.find_last_of("/");
		if ( (grid_norm.length()-1) == pos )
		{
			grid_norm.erase(pos);
		}
		ret =  llformat(app_base.c_str(), grid_norm.c_str());
	}

	LL_DEBUGS("GridManager") << "App slurl base: " << ret << LL_ENDL;
	return  ret;
}

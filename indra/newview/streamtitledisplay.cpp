/** 
 * @file streamtitledisplay.cpp
 * @brief Stream title display
 *
 * Copyright (c) 2014 Jessica Wabbit
 * Portions copyright (c) 2014 Linden Research, Inc.
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
 * with the terms of that original licence, even though there are only
 * a couple of original lines left in this file:
 *
 * Copyright (c) 2010 Katharine Berry All rights reserved.
 * Copyright (c) 2013 Cinder Roxley <cinder.roxley@phoenixviewer.com>
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
 *   3. Neither the name Katharine Berry nor the names of any contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KATHARINE BERRY AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KATHARINE BERRY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "llviewerprecompiledheaders.h"

#include "streamtitledisplay.h"
#include "llagent.h"
#include "llaudioengine.h"
#include "llnotificationsutil.h"
#include "llnotificationmanager.h"
#include "llstreamingaudio.h"
#include "lltrans.h"
#include "message.h"

StreamTitleDisplay::StreamTitleDisplay() : LLEventTimer(2.f) { }

bool StreamTitleDisplay::tick()
{
	checkMetadata();
	return false;
}

void StreamTitleDisplay::checkMetadata()
{
	static LLCachedControl<U32> show_stream_metadata(gSavedSettings, "ShowStreamMetadata", 2);
	static LLCachedControl<bool> stream_metadata_announce(gSavedSettings, "StreamMetadataAnnounceToChat", false);

	if ((!show_stream_metadata && !stream_metadata_announce) || !gAudiop) {
		return;
	}

	LLStreamingAudioInterface *stream = gAudiop->getStreamingAudioImpl();

	if (stream && stream->hasNewMetadata()) {
		std::string title = stream->getCurrentTitle();
		std::string artist_title = stream->getCurrentArtist();

		if (!title.empty()) {
			if (!artist_title.empty()) {
				artist_title += " - ";
			}
			artist_title += title;
		}

		if (artist_title.empty()) {
			return;
		}

		std::string stream_name = stream->getCurrentStreamName();

		if (show_stream_metadata == 1) {
			//
			//	stream metadata to a toast
			//
			LLSD args;
			args["ARTIST_TITLE"] = artist_title;
			args["STREAM_NAME"] = stream_name.empty() ? LLTrans::getString("Audio Stream") : stream_name;

			LLNotificationsUtil::add("StreamMetadata", args);
		}
		else if (show_stream_metadata == 2) {
			//
			//	stream metadata to nearby chat
			//
			LLChat chat;
			chat.mText = artist_title;

			chat.mSourceType = CHAT_SOURCE_AUDIO_STREAM;
			chat.mFromID = AUDIO_STREAM_FROM;
			chat.mFromName = LLTrans::getString("Audio Stream");
			chat.mText = "<nolink>" + chat.mText + "</nolink>";

			if (!stream_name.empty()) {
				chat.mFromName += " - " + stream_name;
			}

			LLSD args;
			args["type"] = LLNotificationsUI::NT_NEARBYCHAT;
			LLNotificationsUI::LLNotificationManager::instance().onChat(chat, args);
		}

		if (stream_metadata_announce) {
			static LLCachedControl<S32> announce_channel(gSavedSettings, "StreamMetadataAnnounceChannel", 362394);

			if (announce_channel != 0) {
				LLMessageSystem *msg = gMessageSystem;

				msg->newMessageFast(_PREHASH_ChatFromViewer);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ChatData);
				msg->addStringFast(_PREHASH_Message, artist_title);
				msg->addU8Fast(_PREHASH_Type, CHAT_TYPE_WHISPER);
				msg->addS32(_PREHASH_Channel, announce_channel);

				gAgent.sendReliableMessage();
			}
		}
	}
}

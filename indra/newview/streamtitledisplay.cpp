/* Copyright (c) 2010 Katharine Berry All rights reserved.
 * Copyright (c) 2013 Cinder Roxley <cinder.roxley@phoenixviewer.com>
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
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
#include "llviewercontrol.h"
#include "lltrans.h"
#include "fscommon.h"
#include "message.h"

StreamTitleDisplay::StreamTitleDisplay() : LLEventTimer(2.f) { }

BOOL StreamTitleDisplay::tick()
{
	checkMetadata();
	return FALSE;
}

void StreamTitleDisplay::checkMetadata()
{
	static LLCachedControl<U32> show_stream_metadata(gSavedSettings, "ShowStreamMetadata", 2);

	if (show_stream_metadata == 0 || !gAudiop) {
		return;
	}

	LLStreamingAudioInterface *stream = gAudiop->getStreamingAudioImpl();

	if (!stream) {
		return;
	}

	if (stream->hasNewMetadata()) {
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

		if (show_stream_metadata == 1) {
			LLSD args;
			args["ARTIST_TITLE"] = artist_title;
			args["STREAM_NAME"] = stream->getCurrentStreamName();

			LLNotificationsUtil::add("StreamMetadata", args);
		}
		else if (show_stream_metadata == 2) {
			LLChat chat;
			chat.mText = artist_title;

			chat.mSourceType = CHAT_SOURCE_AUDIO_STREAM;
			chat.mFromID = AUDIO_STREAM_FROM;
			chat.mFromName = LLTrans::getString("Audio Stream");
			chat.mText = "<nolink>" + chat.mText + "</nolink>";

			static LLCachedControl<bool> show_stream_name(gSavedSettings, "ShowStreamName", true);

			if (show_stream_name) {
				std::string stream_name = stream->getCurrentStreamName();

				if (!stream_name.empty()) {
					chat.mFromName += " - " + stream_name;
				}
			}

			LLSD args;
			args["type"] = LLNotificationsUI::NT_NEARBYCHAT;
			LLNotificationsUI::LLNotificationManager::instance().onChat(chat, args);
		}
		else {
			return;
		}

		static LLCachedControl<bool> stream_metadata_announce(gSavedSettings, "StreamMetadataAnnounceToChat", false);

		if (stream_metadata_announce) {
			sendStreamTitleToChat(artist_title);
		}
	}
}

void StreamTitleDisplay::sendStreamTitleToChat(const std::string &title)
{
	static LLCachedControl<S32> streamMetadataAnnounceChannel(gSavedSettings, "StreamMetadataAnnounceChannel", 1);

	if (streamMetadataAnnounceChannel != 0) {
		LLMessageSystem *msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_ChatFromViewer);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ChatData);
		msg->addStringFast(_PREHASH_Message, title);
		msg->addU8Fast(_PREHASH_Type, CHAT_TYPE_WHISPER);
		msg->addS32("Channel", streamMetadataAnnounceChannel);

		gAgent.sendReliableMessage();
	}
}

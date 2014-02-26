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
#if LL_LINUX
void StreamTitleDisplay::checkMetadata()
{
bool ShowStreamConverted =  true; //temporary make work hack

	static LLCachedControl<U32> ShowStreamMetadata(gSavedSettings, "ShowStreamMetadata", 2);
	static LLCachedControl<bool> ShowStreamName(gSavedSettings, "ShowStreamName", true);
	static LLCachedControl<bool> StreamMetadataAnnounceToChat(gSavedSettings, "StreamMetadataAnnounceToChat", false);

if (ShowStreamMetadata >= 1)
{
  ShowStreamConverted =  true;
}
else
{
  ShowStreamConverted =  false;
}
	if(!gAudiop)
		return;
	if(gAudiop->getStreamingAudioImpl()->hasNewMetadata() && (ShowStreamConverted || StreamMetadataAnnounceToChat))
	{
		LLStreamingAudioInterface *stream = gAudiop->getStreamingAudioImpl();

		if (!stream) {
			return;
		}

		LLChat chat;
		std::string title = stream->getCurrentTitle();
		std::string artist = stream->getCurrentArtist();
		chat.mText = artist;

		if (!title.empty()) {
			if (!chat.mText.empty()) {
				chat.mText += " - ";
			}
			chat.mText += title;
		}

		if (!chat.mText.empty()) {
			if (StreamMetadataAnnounceToChat) {
				sendStreamTitleToChat(chat.mText);
			}

			if (ShowStreamConverted) {
				chat.mSourceType = CHAT_SOURCE_AUDIO_STREAM;
				chat.mFromID = AUDIO_STREAM_FROM;
				chat.mFromName = LLTrans::getString("Audio Stream");
				chat.mText = "<nolink>" + chat.mText + "</nolink>";

				if (ShowStreamName) {
					std::string stream_name = stream->getCurrentStreamName();

					if (!stream_name.empty()) {
						chat.mFromName += " - " + stream_name;
					}
				}

				LLSD args;
				args["type"] = LLNotificationsUI::NT_NEARBYCHAT;
				LLNotificationsUI::LLNotificationManager::instance().onChat(chat, args);
			}
		}
	}
}

#else //WINDOWS AND DARWIN AND OTHERS
void StreamTitleDisplay::checkMetadata()
{
	static LLCachedControl<U32> ShowStreamMetadata(gSavedSettings, "ShowStreamMetadata", 2);
	static LLCachedControl<bool> StreamMetadataAnnounceToChat(gSavedSettings, "StreamMetadataAnnounceToChat", false);

	if (!gAudiop)
		return;
		LL_DEBUGS("StreamTitles") << "Stream Meta data " << ShowStreamMetadata << LL_ENDL;
	if ((ShowStreamMetadata > 0 || StreamMetadataAnnounceToChat)
	    && gAudiop->getStreamingAudioImpl()->getNewMetadata(mMetadata))
	{
		std::string chat = "";
		
		if (mMetadata.has("ARTIST"))
		{
			chat = mMetadata["ARTIST"].asString();
		}
		if (mMetadata.has("TITLE"))
		{
			if (chat.length() > 0)
			{
				chat.append(" - ");
			}
			chat.append(mMetadata["TITLE"].asString());
		}
		if (chat.length() > 0)
		{
			if (StreamMetadataAnnounceToChat)
			{
				sendStreamTitleToChat(chat);
			}

			if (ShowStreamMetadata > 1)
			{
				chat = LLTrans::getString("StreamtitleNowPlaying") + " " + chat;
				reportToNearbyChat(chat);
			}
			else if (ShowStreamMetadata == 1
					 && (mMetadata.has("TITLE") || mMetadata.has("ARTIST")))
			{
				if (!mMetadata.has("TITLE"))
					mMetadata["TITLE"] = "";
				LLNotificationsUtil::add((mMetadata.has("ARTIST") ? "StreamMetadata" : "StreamMetadataNoArtist"), mMetadata);
			}
		}
	}
}
#endif

void StreamTitleDisplay::sendStreamTitleToChat(const std::string& title)
{
	static LLCachedControl<S32> streamMetadataAnnounceChannel(gSavedSettings, "StreamMetadataAnnounceChannel", 1);
	if (streamMetadataAnnounceChannel != 0)
	{
		LLMessageSystem* msg = gMessageSystem;
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

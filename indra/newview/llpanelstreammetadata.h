/**
 * @file llpanelstreammetadata.h
 * @brief Represents a class of stream metadata tip toast panels.
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Wabbit Viewer Source Code
 * Copyright (C) 2014, Jessica Wabbit
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
#include "llpaneltiptoast.h"

#ifndef LL_PANELSTREAMMETADATA_H
#define LL_PANELSTREAMMETADATA_H

class LLPanelStreamMetadata : public LLPanelTipToast
{
	// disallow instantiation of this class
private:
	// grant privileges to instantiate this class to LLToastPanel
	friend class LLToastPanel;

	LLPanelStreamMetadata(const LLNotificationPtr &notification);
	virtual ~LLPanelStreamMetadata() {}
};

#endif /* LL_PANELSTREAMMETADATA_H */

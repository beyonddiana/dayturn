/**
 * @file lljointoverridedata.h
 * @brief Declaration of LLJointOverrideData and LLAttachmentOverrideData
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer2020, Linden Research, Inc.
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

#ifndef LL_JOINTOVERRIDEDATA_H
#define LL_JOINTOVERRIDEDATA_H

//#include <map>
//#include <string>
//#include <vector>


struct LLJointOverrideData
{
    std::set<LLVector3> mPosOverrides;
    LLVector3 mActivePosOverride;
    std::set<LLVector3> mScaleOverrides;
    LLVector3 mActiveScaleOverride;
};

struct LLAttachmentOverrideData
{
    std::set<LLVector3> mPosOverrides;
    LLVector3 mActivePosOverride;
};

typedef std::map<std::string, LLJointOverrideData> joint_override_data_map_t;
typedef std::map<std::string, LLAttachmentOverrideData> attach_override_data_map_t;

#endif // LL_JOINTOVERRIDEDATA_H


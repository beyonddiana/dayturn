/**
* @file llsettingsbase.h
* @author optional
* @brief A base class for asset based settings groups.
*
* $LicenseInfo:2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2017, Linden Research, Inc.
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

#ifndef LL_SETTINGS_BASE_H
#define LL_SETTINGS_BASE_H

#include <string>
#include <map>
#include <vector>

#include "llsd.h"
#include "llsdutil.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"
#include "v4color.h"

class LLSettingsBase: private boost::noncopyable
{
    friend class LLEnvironment;

public:
    typedef boost::shared_ptr<LLSettingsBase> ptr_t;

    virtual ~LLSettingsBase() { };

    //---------------------------------------------------------------------
    virtual std::string getSettingType() const = 0;

    //---------------------------------------------------------------------
    // Settings status 
    inline bool hasSetting(const std::string &param) const { return mSettings.has(param); }
    inline bool isDirty() const { return mDirty; }
    inline void setDirtyFlag(bool dirty) { mDirty = dirty; }

    //---------------------------------------------------------------------
    // 
    inline void setValue(const std::string &name, const LLSD &value)
    {
        mSettings[name] = value;
        mDirty = true;
    }

    inline LLSD getValue(const std::string &name, const LLSD &deflt = LLSD())
    {
        if (!mSettings.has(name))
            return deflt;
        return mSettings[name];
    }

    inline void setValue(const std::string &name, const LLVector2 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLVector3 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLVector4 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLQuaternion &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLColor3 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLColor4 &value)
    {
        setValue(name, value.getValue());
    }

    // Note this method is marked const but may modify the settings object.
    // (note the internal const cast).  This is so that it may be called without
    // special consideration from getters.
    inline void update() const
    {
        if (!mDirty)
            return;
        (const_cast<LLSettingsBase *>(this))->updateSettings();
    }

    // TODO: This is temporary 
    virtual void exportSettings(std::string name) const;

protected:
    LLSettingsBase();
    LLSettingsBase(const LLSD setting);

    typedef std::set<std::string>   stringset_t;

    // combining settings objects. Customize for specific setting types
    virtual void lerpSettings(const LLSettingsBase &other, F32 mix);

    /// when lerping between settings, some may require special handling.  
    /// Get a list of these key to be skipped by the default settings lerp.
    /// (handling should be performed in the override of lerpSettings.
    virtual stringset_t getSkipInterpolateKeys() const { return stringset_t(); }  

    // A list of settings that represent quaternions and should be slerped 
    // rather than lerped.
    virtual stringset_t getSlerpKeys() const { return stringset_t(); }

    // Calculate any custom settings that may need to be cached.
    virtual void updateSettings() { mDirty = false; };

    virtual stringset_t getSkipApplyKeys() const { return stringset_t(); }
    // Apply any settings that need special handling. 
    virtual void applySpecial(void *) { };

    LLSD    mSettings;

private:
    bool    mDirty;

    LLSD    combineSDMaps(const LLSD &first, const LLSD &other) const;
    LLSD    interpolateSDMap(const LLSD &settings, const LLSD &other, F32 mix) const;

};


#endif

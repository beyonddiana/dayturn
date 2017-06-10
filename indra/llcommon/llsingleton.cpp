/** 
 * @file llsingleton.cpp
 * @author Brad Kittenbrink
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

#include "linden_common.h"
#include "llsingleton.h"

#include "llerror.h"
#include "llerrorcontrol.h"         // LLError::is_available()
#include "lldependencies.h"

namespace {
    void log(LLError::ELevel level,
             const char* p1, const char* p2, const char* p3, const char* p4);
    
    void logdebugs(const char* p1="", const char* p2="", const char* p3="", const char* p4="");
    
    bool oktolog();
}


//static
void LLSingletonBase::log_initializing(const char* verb, const char* name)
{
    if (oktolog())
    {
        LL_DEBUGS("LLSingleton") << verb << ' ' << demangle(name) << ';';
        list_t& list(get_initializing());
        for (list_t::const_reverse_iterator ri(list.rbegin()), rend(list.rend());
             ri != rend; ++ri)
        {
            LLSingletonBase* sb(*ri);
            LL_CONT << ' ' << demangle(typeid(*sb).name());
        }
        LL_ENDL;
    }
}



/*---------------------------- Logging helpers -----------------------------*/
namespace {
bool oktolog()
{
   // See comments in log() below.
   return LLError::is_available();
}
 
void log(LLError::ELevel level,
         const char* p1, const char* p2, const char* p3, const char* p4)
{
     // The is_available() test below ensures that we'll stop logging once
     // LLError has been cleaned up. If we had a similar portable test for
     // std::cerr, this would be a good place to use it.
 
     // Check LLError::is_available() because some of LLError's infrastructure
     // is itself an LLSingleton. If that LLSingleton has not yet been
     // initialized, trying to log will engage LLSingleton machinery... and
     // around and around we go.
     if (LLError::is_available())
     {
         LL_VLOGS(level, "LLSingleton") << p1 << p2 << p3 << p4 << LL_ENDL;
     }
     else
     {
         // Caller may be a test program, or something else whose stderr is
         // visible to the user.
         std::cerr << p1 << p2 << p3 << p4 << std::endl;
     }
 }
 
 void logdebugs(const char* p1, const char* p2, const char* p3, const char* p4)
 {
     log(LLError::LEVEL_DEBUG, p1, p2, p3, p4);
 }
 } // anonymous namespace 
 
  //static
 void LLSingletonBase::logwarns(const char* p1, const char* p2, const char* p3, const char* p4)
 {
     log(LLError::LEVEL_WARN, p1, p2, p3, p4);
 }
 
 //static
 void LLSingletonBase::logerrs(const char* p1, const char* p2, const char* p3, const char* p4)
 {
     log(LLError::LEVEL_ERROR, p1, p2, p3, p4);
     // The other important side effect of LL_ERRS() is
     // https://www.youtube.com/watch?v=OMG7paGJqhQ (emphasis on OMG)
     LLError::crashAndLoop(std::string());
 }




 std::string LLSingletonBase::demangle(const char* mangled)
 {
     return LLError::Log::demangle(mangled);
 }

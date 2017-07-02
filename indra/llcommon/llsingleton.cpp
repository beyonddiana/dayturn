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
#include "llcoro_get_id.h"
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <algorithm>
#include <iostream>                 // std::cerr in dire emergency
#include <sstream>
#include <stdexcept>


namespace {
    void log(LLError::ELevel level,
             const char* p1, const char* p2, const char* p3, const char* p4);
    
    void logdebugs(const char* p1="", const char* p2="", const char* p3="", const char* p4="");
    
    bool oktolog();
}

// Our master list of all LLSingletons is itself an LLSingleton. We used to
// store it in a function-local static, but that could get destroyed before
// the last of the LLSingletons -- and ~LLSingletonBase() definitely wants to
// remove itself from the master list. Since the whole point of this master
// list is to help track inter-LLSingleton dependencies, and since we have
// this implicit dependency from every LLSingleton to the master list, make it
// an LLSingleton.
class LLSingletonBase::MasterList:
public LLSingleton<LLSingletonBase::MasterList>
{
    LLSINGLETON_EMPTY_CTOR(MasterList);
    
public:
    // No need to make this private with accessors; nobody outside this source
    // file can see it.
    
    // This is the master list of all instantiated LLSingletons (save the
    // MasterList itself) in arbitrary order. You MUST call dep_sort() before
    // traversing this list.
    LLSingletonBase::list_t mMaster;
    
    // We need to maintain a stack of LLSingletons currently being
    // initialized, either in the constructor or in initSingleton(). However,
    // managing that as a stack depends on having a DISTINCT 'initializing'
    // stack for every C++ stack in the process! And we have a distinct C++
    // stack for every running coroutine. It would be interesting and cool to
    // implement a generic coroutine-local-storage mechanism and use that
    // here. The trouble is that LLCoros is itself an LLSingleton, so
    // depending on LLCoros functionality could dig us into infinite
    // recursion. (Moreover, when we reimplement LLCoros on top of
    // Boost.Fiber, that library already provides fiber_specific_ptr -- so
    // it's not worth a great deal of time and energy implementing a generic
    // equivalent on top of boost::dcoroutine, which is on its way out.)
    // Instead, use a map of llcoro::id to select the appropriate
    // coro-specific 'initializing' stack. llcoro::get_id() is carefully
    // implemented to avoid requiring LLCoros.
    typedef boost::unordered_map<llcoro::id, LLSingletonBase::list_t> InitializingMap;
    InitializingMap mInitializing;
    
    // non-static method, cf. LLSingletonBase::get_initializing()
    list_t& get_initializing_()
    {
        // map::operator[] has find-or-create semantics, exactly what we need
        // here. It returns a reference to the selected mapped_type instance.
        return mInitializing[llcoro::get_id()];
    }
    
    void cleanup_initializing_()
    {
        InitializingMap::iterator found = mInitializing.find(llcoro::get_id());
        if (found != mInitializing.end())
        {
            mInitializing.erase(found);
        }
    }
};

//static
LLSingletonBase::list_t& LLSingletonBase::get_master()
{
    return LLSingletonBase::MasterList::instance().mMaster;
}

void LLSingletonBase::add_master()
{
    // As each new LLSingleton is constructed, add to the master list.
    get_master().push_back(this);
}

void LLSingletonBase::remove_master()
{
    // When an LLSingleton is destroyed, remove from master list.
    // add_master() used to capture the iterator to the newly-added list item
    // so we could directly erase() it from the master list. Unfortunately
    // that runs afoul of destruction-dependency order problems. So search the
    // master list, and remove this item IF FOUND. We have few enough
    // LLSingletons, and they are so rarely destroyed (once per run), that the
    // cost of a linear search should not be an issue.
    get_master().remove(this);
}

//static
LLSingletonBase::list_t& LLSingletonBase::get_initializing()
{
    return LLSingletonBase::MasterList::instance().get_initializing_();
}

//static
LLSingletonBase::list_t& LLSingletonBase::get_initializing_from(MasterList* master)
{
    return master->get_initializing_();
}

LLSingletonBase::~LLSingletonBase() {}

void LLSingletonBase::push_initializing(const char* name)
{
    // log BEFORE pushing so logging singletons don't cry circularity
    log_initializing("Pushing", name);
    get_initializing().push_back(this);
}

void LLSingletonBase::pop_initializing()
{
    list_t& list(get_initializing());
    
    if (list.empty())
    {
        logerrs("Underflow in stack of currently-initializing LLSingletons at ",
                demangle(typeid(*this).name()).c_str(), "::getInstance()");
    }
    
    // Now we know list.back() exists: capture it
    LLSingletonBase* back(list.back());
    // and pop it
    list.pop_back();
    
    // The viewer launches an open-ended number of coroutines. While we don't
    // expect most of them to initialize LLSingleton instances, our present
    // get_initializing() logic could lead to an open-ended number of map
    // entries. So every time we pop the stack back to empty, delete the entry
    // entirely.
    if (list.empty())
    {
        MasterList::instance().cleanup_initializing_();
    }
    
    // Now validate the newly-popped LLSingleton.
    if (back != this)
    {
        logerrs("Push/pop mismatch in stack of currently-initializing LLSingletons: ",
                demangle(typeid(*this).name()).c_str(), "::getInstance() trying to pop ",
                demangle(typeid(*back).name()).c_str());
    }
    
    // log AFTER popping so logging singletons don't cry circularity
    log_initializing("Popping", typeid(*back).name());
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

/**
 * @file   lleventfilter.h
 * @author Nat Goodspeed
 * @date   2009-03-05
 * @brief  Define LLEventFilter: LLEventStream subclass with conditions
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

#if ! defined(LL_LLEVENTFILTER_H)
#define LL_LLEVENTFILTER_H

#include "llevents.h"
#include "stdtypes.h"
#include "lltimer.h"
#include <boost/function.hpp>

/**
 * Generic base class
 */
class LL_COMMON_API LLEventFilter: public LLEventStream
{
public:
    /// construct a standalone LLEventFilter
    LLEventFilter(const std::string& name="filter", bool tweak=true):
        LLEventStream(name, tweak)
    {}
    /// construct LLEventFilter and connect it to the specified LLEventPump
    LLEventFilter(LLEventPump& source, const std::string& name="filter", bool tweak=true);

    /// Post an event to all listeners
    virtual bool post(const LLSD& event) = 0;
};

/**
 * Pass through only events matching a specified pattern
 */
class LLEventMatching: public LLEventFilter
{
public:
    /// Pass an LLSD map with keys and values the incoming event must match
    LLEventMatching(const LLSD& pattern);
    /// instantiate and connect
    LLEventMatching(LLEventPump& source, const LLSD& pattern);

    /// Only pass through events matching the pattern
    virtual bool post(const LLSD& event);

private:
    LLSD mPattern;
};

/**
 * Wait for an event to be posted. If no such event arrives within a specified
 * time, take a specified action. See LLEventTimeout for production
 * implementation.
 *
 * @NOTE This is an abstract base class so that, for testing, we can use an
 * alternate "timer" that doesn't actually consume real time.
 */
class LL_COMMON_API LLEventTimeoutBase: public LLEventFilter
{
public:
    /// construct standalone
    LLEventTimeoutBase();
    /// construct and connect
    LLEventTimeoutBase(LLEventPump& source);

    /// Callable, can be constructed with boost::bind()
    typedef boost::function<void()> Action;

    /**
     * Start countdown timer for the specified number of @a seconds. Forward
     * all events. If any event arrives before timer expires, cancel timer. If
     * no event arrives before timer expires, take specified @a action.
     *
     * This is a one-shot timer. Once it has either expired or been canceled,
     * it is inert until another call to actionAfter().
     *
     * Calling actionAfter() while an existing timer is running cheaply
     * replaces that original timer. Thus, a valid use case is to detect
     * idleness of some event source by calling actionAfter() on each new
     * event. A rapid sequence of events will keep the timer from expiring;
     * the first gap in events longer than the specified timer will fire the
     * specified Action.
     *
     * Any post() call cancels the timer. To be satisfied with only a
     * particular event, chain on an LLEventMatching that only passes such
     * events:
     *
     * @code
     * event                                                 ultimate
     * source ---> LLEventMatching ---> LLEventTimeout  ---> listener
     * @endcode
     *
     * @NOTE
     * The implementation relies on frequent events on the LLEventPump named
     * "mainloop".
     */
    void actionAfter(F32 seconds, const Action& action);

    /**
     * Like actionAfter(), but where the desired Action is LL_ERRS
     * termination. Pass the timeout time and the desired LL_ERRS @a message.
     *
     * This method is useful when, for instance, some async API guarantees an
     * event, whether success or failure, within a stated time window.
     * Instantiate an LLEventTimeout listening to that API and call
     * errorAfter() on each async request with a timeout comfortably longer
     * than the API's time guarantee (much longer than the anticipated
     * "mainloop" granularity).
     *
     * Then if the async API breaks its promise, the program terminates with
     * the specified LL_ERRS @a message. The client of the async API can
     * therefore assume the guarantee is upheld.
     *
     * @NOTE
     * errorAfter() is implemented in terms of actionAfter(), so all remarks
     * about calling actionAfter() also apply to errorAfter().
     */
    void errorAfter(F32 seconds, const std::string& message);

    /**
     * Like actionAfter(), but where the desired Action is a particular event
     * for all listeners. Pass the timeout time and the desired @a event data.
     * 
     * Suppose the timeout should only be satisfied by a particular event, but
     * the ultimate listener must see all other incoming events as well, plus
     * the timeout @a event if any:
     * 
     * @code
     * some        LLEventMatching                           LLEventMatching
     * event  ---> for particular  ---> LLEventTimeout  ---> for timeout
     * source      event                                     event \
     *       \                                                      \ ultimate
     *        `-----------------------------------------------------> listener
     * @endcode
     * 
     * Since a given listener can listen on more than one LLEventPump, we can
     * set things up so it sees the set union of events from LLEventTimeout
     * and the original event source. However, as LLEventTimeout passes
     * through all incoming events, the "particular event" that satisfies the
     * left LLEventMatching would reach the ultimate listener twice. So we add
     * an LLEventMatching that only passes timeout events.
     *
     * @NOTE
     * eventAfter() is implemented in terms of actionAfter(), so all remarks
     * about calling actionAfter() also apply to eventAfter().
     */
    void eventAfter(F32 seconds, const LLSD& event);

    /// Pass event through, canceling the countdown timer
    virtual bool post(const LLSD& event);

    /// Cancel timer without event
    void cancel();

    /// Is this timer currently running?
    bool running() const;

protected:
    virtual void setCountdown(F32 seconds) = 0;
    virtual bool countdownElapsed() const = 0;

private:
    bool tick(const LLSD&);

    LLBoundListener mMainloop;
    Action mAction;
};

/// Production implementation of LLEventTimoutBase
class LL_COMMON_API LLEventTimeout: public LLEventTimeoutBase
{
public:
    LLEventTimeout();
    LLEventTimeout(LLEventPump& source);

protected:
    virtual void setCountdown(F32 seconds);
    virtual bool countdownElapsed() const;

private:
    LLTimer mTimer;
};

/**
 * LLEventBatch: accumulate post() events (LLSD blobs) into an LLSD Array
 * until the array reaches a certain size, then call listeners with the Array
 * and clear it back to empty.
 */
class LL_COMMON_API LLEventBatch: public LLEventFilter
{
public:
    // pass batch size
    LLEventBatch(std::size_t size);
    // construct and connect
    LLEventBatch(LLEventPump& source, std::size_t size);

    // force out the pending batch
    void flush();

    // accumulate an event and flush() when big enough
    virtual bool post(const LLSD& event);

private:
    LLSD mBatch;
    std::size_t mBatchSize;
};

/**
 * LLEventThrottle: construct with a time interval. Regardless of how
 * frequently you call post(), LLEventThrottle will pass on an event to
 * its listeners no more often than once per specified interval.
 *
 * A new event after more than the specified interval will immediately be
 * passed along to listeners. But subsequent events will be delayed until at
 * least one time interval since listeners were last called. Consider the
 * sequence below. Suppose we have an LLEventThrottle constructed with an
 * interval of 3 seconds. The numbers on the left are timestamps in seconds
 * relative to an arbitrary reference point.
 *
 *  1: post(): event immediately passed to listeners, next no sooner than 4
 *  2: post(): deferred: waiting for 3 seconds to elapse
 *  3: post(): deferred
 *  4: no post() call, but event delivered to listeners; next no sooner than 7
 *  6: post(): deferred
 *  7: no post() call, but event delivered; next no sooner than 10
 * 12: post(): immediately passed to listeners, next no sooner than 15
 * 17: post(): immediately passed to listeners, next no sooner than 20
 *
 * For a deferred event, the LLSD blob delivered to listeners is from the most
 * recent deferred post() call. However, you may obtain the previous event
 * blob by calling pending(), modify it however you want and post() the new
 * value. Each time an event is delivered to listeners, the pending() value is
 * reset to isUndefined().
 *
 * You may also call flush() to immediately pass along any deferred events to
 * all listeners.
 */
class LL_COMMON_API LLEventThrottle: public LLEventFilter
{
public:
    // pass time interval
    LLEventThrottle(F32 interval);
    // construct and connect
    LLEventThrottle(LLEventPump& source, F32 interval);

    // force out any deferred events
    void flush();

    // retrieve (aggregate) deferred event since last event sent to listeners
    LLSD pending() const;

    // register an event, may be either passed through or deferred
    virtual bool post(const LLSD& event);

private:
    // remember throttle interval
    F32 mInterval;
    // count post() calls since last flush()
    std::size_t mPosts;
    // pending event data from most recent deferred event
    LLSD mPending;
    // use this to arrange a deferred flush() call
    LLEventTimeout mAlarm;
    // use this to track whether we're within mInterval of last flush()
    LLTimer mTimer;
};

/**
 * LLEventBatchThrottle: like LLEventThrottle, it refuses to pass events to
 * listeners more often than once per specified time interval.
 * Like LLEventBatch, it accumulates pending events into an LLSD Array.
 */
class LLEventBatchThrottle: public LLEventThrottle
{
public:
    // pass time interval
    LLEventBatchThrottle(F32 interval);
    // construct and connect
    LLEventBatchThrottle(LLEventPump& source, F32 interval);

    // append a new event to current batch
    virtual bool post(const LLSD& event);
};

#endif /* ! defined(LL_LLEVENTFILTER_H) */

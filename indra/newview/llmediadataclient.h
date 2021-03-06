/** 
 * @file llmediadataclient.h
 * @brief class for queueing up requests to the media service
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLMEDIADATACLIENT_H
#define LL_LLMEDIADATACLIENT_H

#include "llhttpclient.h"
#include <set>
#include "llrefcount.h"
#include "llpointer.h"
#include "lleventtimer.h"


// Link seam for LLVOVolume
class LLMediaDataClientObject : public LLRefCount
{
public:
	// Get the number of media data items
	virtual U8 getMediaDataCount() const = 0;
	// Get the media data at index, as an LLSD
	virtual LLSD getMediaDataLLSD(U8 index) const = 0;
	// Return true if the current URL for the face in the media data matches the specified URL.
	virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const = 0;
	// Get this object's UUID
	virtual LLUUID getID() const = 0;
	// Navigate back to previous URL
	virtual void mediaNavigateBounceBack(U8 index) = 0;
	// Does this object have media?
	virtual bool hasMedia() const = 0;
	// Update the object's media data to the given array
	virtual void updateObjectMediaData(LLSD const &media_data_array, const std::string &version_string) = 0;
	// Return the total "interest" of the media (on-screen area)
	virtual F64 getMediaInterest() const = 0;
	// Return the given cap url
	virtual std::string getCapabilityUrl(const std::string &name) const = 0;
	// Return whether the object has been marked dead
	virtual bool isDead() const = 0;
	// Returns a media version number for the object
	virtual U32 getMediaVersion() const = 0;
	// Returns whether the object is "interesting enough" to fetch
	virtual bool isInterestingEnough() const = 0;
	// Returns whether we've seen this object yet or not
	virtual bool isNew() const = 0;

	// smart pointer
	typedef LLPointer<LLMediaDataClientObject> ptr_t;
};


// This object creates a priority queue for requests.
// Abstracts the Cap URL, the request, and the responder
class LLMediaDataClient : public LLRefCount
{
protected:
    LOG_CLASS(LLMediaDataClient);
public:
    
    const static F32 QUEUE_TIMER_DELAY;// = 1.0; // seconds(s)
	const static F32 UNAVAILABLE_RETRY_TIMER_DELAY;// = 5.0; // secs
	const static U32 MAX_RETRIES;// = 4;
	const static U32 MAX_SORTED_QUEUE_SIZE;// = 10000;
	const static U32 MAX_ROUND_ROBIN_QUEUE_SIZE;// = 10000;

	// Constructor
	LLMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
					  F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
		              U32 max_retries = MAX_RETRIES,
					  U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
					  U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE);
	
	F32 getRetryTimerDelay() const { return mRetryTimerDelay; }
	
	// Returns true iff the queue is empty
	virtual bool isEmpty() const;
	
	// Returns true iff the given object is in the queue
	virtual bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
	
	// Remove the given object from the queue. Returns true iff the given object is removed.
	virtual void removeFromQueue(const LLMediaDataClientObject::ptr_t &object);
	
	// Called only by the Queue timer and tests (potentially)
	virtual bool processQueueTimer();
	
protected:
	// Destructor
	virtual ~LLMediaDataClient(); // use unref
    
	class Responder;
	
	// Request (pure virtual base class for requests in the queue)
	class Request : public LLRefCount
	{
	public:
		// Subclasses must implement this to build a payload for their request type.
		virtual LLSD getPayload() const = 0;
		// and must create the correct type of responder.
		virtual Responder *createResponder() = 0;

		virtual std::string getURL() { return ""; }

        enum Type {
            GET,
            UPDATE,
            NAVIGATE,
			ANY
        };
        
	protected:
		// The only way to create one of these is through a subclass.
		Request(Type in_type, LLMediaDataClientObject *obj, LLMediaDataClient *mdc, S32 face = -1);
	public:
		LLMediaDataClientObject *getObject() const { return mObject; }

        U32 getNum() const { return mNum; }
		U32 getRetryCount() const { return mRetryCount; }
		void incRetryCount() { mRetryCount++; }
        Type getType() const { return mType; }
		F64 getScore() const { return mScore; }
		
		// Note: may return empty string!
		std::string getCapability() const;
		const char *getCapName() const;
		const char *getTypeAsString() const;
		
		// Re-enqueue thyself
		void reEnqueue();
		
		F32 getRetryTimerDelay() const;
		U32 getMaxNumRetries() const;
		
		bool isObjectValid() const { return mObject.notNull() && (!mObject->isDead()); }
		bool isNew() const { return isObjectValid() && mObject->isNew(); }
		void updateScore();
		
		void markDead();
		bool isDead();
		void startTracking();
		void stopTracking();
		
		friend std::ostream& operator<<(std::ostream &s, const Request &q);
		
		const LLUUID &getID() const { return mObjectID; }
		S32 getFace() const { return mFace; }
		
		bool isMatch (const Request* other, Type match_type = ANY) const 
		{ 
			return ((match_type == ANY) || (mType == other->mType)) && 
					(mFace == other->mFace) && 
					(mObjectID == other->mObjectID); 
		}
	protected:
		LLMediaDataClientObject::ptr_t mObject;
	private:
		Type mType;
		// Simple tracking
		U32 mNum;
		static U32 sNum;
        U32 mRetryCount;
		F64 mScore;
		
		LLUUID mObjectID;
		S32 mFace;

		// Back pointer to the MDC...not a ref!
		LLMediaDataClient *mMDC;
	};
	typedef LLPointer<Request> request_ptr_t;

	// Responder
	class Responder : public LLHTTPClient::Responder
	{
		LOG_CLASS(Responder);
	public:
		Responder(const request_ptr_t &request);
		request_ptr_t &getRequest() { return mRequest; }

	protected:
		//If we get back an error (not found, etc...), handle it here
		virtual void httpFailure();
		//If we get back a normal response, handle it here.	 Default just logs it.
		virtual void httpSuccess();

	private:
		request_ptr_t mRequest;
	};

	class RetryTimer : public LLEventTimer
	{
	public:
		RetryTimer(F32 time, request_ptr_t);
		virtual BOOL tick();
	private:
		// back-pointer
		request_ptr_t mRequest;
	};
		
	
protected:
	typedef std::list<request_ptr_t> request_queue_t;
	typedef std::set<request_ptr_t> request_set_t;

	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const = 0;

	// Puts the request into a queue, appropriately handling duplicates, etc.
	virtual void enqueue(Request*) = 0;
	
	virtual void serviceQueue();

	virtual request_queue_t *getQueue() { return &mQueue; };

	// Gets the next request, removing it from the queue
	virtual request_ptr_t dequeue();
	
	virtual bool canServiceRequest(request_ptr_t request) { return true; };

	// Returns a request to the head of the queue (should only be used for requests that came from dequeue
	virtual void pushBack(request_ptr_t request);
	
	void trackRequest(request_ptr_t request);
	void stopTrackingRequest(request_ptr_t request);
	
	request_queue_t mQueue;

	const F32 mQueueTimerDelay;
	const F32 mRetryTimerDelay;
	const U32 mMaxNumRetries;
	const U32 mMaxSortedQueueSize;
	const U32 mMaxRoundRobinQueueSize;
	
	// Set for keeping track of requests that aren't in either queue.  This includes:
	//	Requests that have been sent and are awaiting a response (pointer held by the Responder)
	//  Requests that are waiting for their retry timers to fire (pointer held by the retry timer)
	request_set_t mUnQueuedRequests;

	void startQueueTimer();
	void stopQueueTimer();

private:
	
	static F64 getObjectScore(const LLMediaDataClientObject::ptr_t &obj);
    
	friend std::ostream& operator<<(std::ostream &s, const Request &q);
	friend std::ostream& operator<<(std::ostream &s, const request_queue_t &q);

	class QueueTimer : public LLEventTimer
	{
	public:
		QueueTimer(F32 time, LLMediaDataClient *mdc);
		virtual BOOL tick();
	private:
		// back-pointer
		LLPointer<LLMediaDataClient> mMDC;
	};
	
	void setIsRunning(bool val) { mQueueTimerIsRunning = val; }
		
	bool mQueueTimerIsRunning;

	template <typename T> friend typename T::iterator find_matching_request(T &c, const LLMediaDataClient::Request *request, LLMediaDataClient::Request::Type match_type = LLMediaDataClient::Request::ANY);
	template <typename T> friend typename T::iterator find_matching_request(T &c, const LLUUID &id, LLMediaDataClient::Request::Type match_type = LLMediaDataClient::Request::ANY);
	template <typename T> friend void remove_matching_requests(T &c, const LLUUID &id, LLMediaDataClient::Request::Type match_type = LLMediaDataClient::Request::ANY);

};

// MediaDataClient specific for the ObjectMedia cap
class LLObjectMediaDataClient : public LLMediaDataClient
{
protected:
    LOG_CLASS(LLObjectMediaDataClient);
public:
    LLObjectMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
							F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
							U32 max_retries = MAX_RETRIES,
							U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
							U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries),
		  mCurrentQueueIsTheSortedQueue(true)
		{}
    
	void fetchMedia(LLMediaDataClientObject *object); 
    void updateMedia(LLMediaDataClientObject *object);

	class RequestGet: public Request
	{
	public:
		RequestGet(LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
		/*virtual*/ LLSD getPayload() const;
		/*virtual*/ Responder *createResponder();
	};

	class RequestUpdate: public Request
	{
	public:
		RequestUpdate(LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
		/*virtual*/ LLSD getPayload() const;
		/*virtual*/ Responder *createResponder();
	};

	// Returns true iff the queue is empty
	virtual bool isEmpty() const;
	
	// Returns true iff the given object is in the queue
	virtual bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
	    
	// Remove the given object from the queue. Returns true iff the given object is removed.
	virtual void removeFromQueue(const LLMediaDataClientObject::ptr_t &object);

	virtual bool processQueueTimer();

	virtual bool canServiceRequest(request_ptr_t request);

protected:
	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const;
	
	virtual request_queue_t *getQueue();

	// Puts the request into the appropriate queue
	virtual void enqueue(Request*);
		    
    class Responder : public LLMediaDataClient::Responder
    {
        LOG_CLASS(Responder);
    public:
        Responder(const request_ptr_t &request)
            : LLMediaDataClient::Responder(request) {}
    protected:
        virtual void httpSuccess();
    };
private:
	// The Get/Update data client needs a second queue to avoid object updates starving load-ins.
	void swapCurrentQueue();
	
	request_queue_t mRoundRobinQueue;
	bool mCurrentQueueIsTheSortedQueue;

	// Comparator for sorting
	static bool compareRequestScores(const request_ptr_t &o1, const request_ptr_t &o2);
	void sortQueue();
};


// MediaDataClient specific for the ObjectMediaNavigate cap
class LLObjectMediaNavigateClient : public LLMediaDataClient
{
protected:
    LOG_CLASS(LLObjectMediaNavigateClient);
public:
	// NOTE: from llmediaservice.h
	static const int ERROR_PERMISSION_DENIED_CODE = 8002;
	
    LLObjectMediaNavigateClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
								F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
								U32 max_retries = MAX_RETRIES,
								U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
								U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries)
		{}
    
    void navigate(LLMediaDataClientObject *object, U8 texture_index, const std::string &url);

	// Puts the request into the appropriate queue
	virtual void enqueue(Request*);

	class RequestNavigate: public Request
	{
	public:
		RequestNavigate(LLMediaDataClientObject *obj, LLMediaDataClient *mdc, U8 texture_index, const std::string &url);
		/*virtual*/ LLSD getPayload() const;
		/*virtual*/ Responder *createResponder();
		/*virtual*/ std::string getURL() { return mURL; }
	private:
		std::string mURL;
	};
    
protected:
	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const;

    class Responder : public LLMediaDataClient::Responder
    {
        LOG_CLASS(Responder);
    public:
        Responder(const request_ptr_t &request)
            : LLMediaDataClient::Responder(request) {}
    protected:
        virtual void httpFailure();
        virtual void httpSuccess();
    private:
        void mediaNavigateBounceBack();
    };

};


#endif // LL_LLMEDIADATACLIENT_H

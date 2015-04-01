/** 
 * @file llcorehttputil.cpp
 * @date 2014-08-25
 * @brief Implementation of adapter and utility classes expanding the llcorehttp interfaces.
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include <sstream>

#include "llcorehttputil.h"
#include "llhttpconstants.h"
#include "llsdserialize.h"

using namespace LLCore;


namespace LLCoreHttpUtil
{

// *TODO:  Currently converts only from XML content.  A mode
// to convert using fromBinary() might be useful as well.  Mesh
// headers could use it.
bool responseToLLSD(HttpResponse * response, bool log, LLSD & out_llsd)
{
	// Convert response to LLSD
	BufferArray * body(response->getBody());
	if (! body || ! body->size())
	{
		return false;
	}

	LLCore::BufferArrayStream bas(body);
	LLSD body_llsd;
	S32 parse_status(LLSDSerialize::fromXML(body_llsd, bas, log));
	if (LLSDParser::PARSE_FAILURE == parse_status){
		return false;
	}
	out_llsd = body_llsd;
	return true;
}


HttpHandle requestPostWithLLSD(HttpRequest * request,
							   HttpRequest::policy_t policy_id,
							   HttpRequest::priority_t priority,
							   const std::string & url,
							   const LLSD & body,
							   const HttpOptions::ptr_t &options,
                               const HttpHeaders::ptr_t &headers,
							   HttpHandler * handler)
{
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	BufferArray * ba = new BufferArray();
	BufferArrayStream bas(ba);
	LLSDSerialize::toXML(body, bas);

	handle = request->requestPost(policy_id,
								  priority,
								  url,
								  ba,
								  options,
								  headers,
								  handler);
	ba->release();
	return handle;
}

HttpHandle requestPostWithLLSD(HttpRequest::ptr_t & request,
	HttpRequest::policy_t policy_id,
	HttpRequest::priority_t priority,
	const std::string & url,
	const LLSD & body,
	const HttpOptions::ptr_t & options,
	const HttpHeaders::ptr_t & headers,
	HttpHandler * handler)
{
	return requestPostWithLLSD(request.get(), policy_id, priority,
		url, body, options, headers, handler);
}

HttpHandle requestPutWithLLSD(HttpRequest * request,
							   HttpRequest::policy_t policy_id,
							   HttpRequest::priority_t priority,
							   const std::string & url,
							   const LLSD & body,
							   const HttpOptions::ptr_t &options,
                               const HttpHeaders::ptr_t &headers,
							   HttpHandler * handler)
{
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	BufferArray * ba = new BufferArray();
	BufferArrayStream bas(ba);
	LLSDSerialize::toXML(body, bas);

	handle = request->requestPut(policy_id,
		priority,
		url,
		ba,
		options,
		headers,
		handler);
	ba->release();
	return handle;
}

HttpHandle requestPutWithLLSD(HttpRequest::ptr_t & request,
	HttpRequest::policy_t policy_id,
	HttpRequest::priority_t priority,
	const std::string & url,
	const LLSD & body,
	const HttpOptions::ptr_t & options,
	const HttpHeaders::ptr_t & headers,
	HttpHandler * handler)
{
	return requestPutWithLLSD(request.get(), policy_id, priority,
		url, body, options, headers, handler);
}

std::string responseToString(LLCore::HttpResponse * response)
{
	static const std::string empty("[Empty]");

	if (! response)
	{
		return empty;
	}

	BufferArray * body(response->getBody());
	if (! body || ! body->size())
	{
		return empty;
	}

	// Attempt to parse as LLSD regardless of content-type
	LLSD body_llsd;
	if (responseToLLSD(response, false, body_llsd))
	{
		std::ostringstream tmp;

		LLSDSerialize::toPrettyNotation(body_llsd, tmp);
		std::size_t temp_len(tmp.tellp());
		
		if (temp_len)
		{
			return tmp.str().substr(0, std::min(temp_len, std::size_t(1024)));
		}
	}
	else
	{
		// *TODO:  More elaborate forms based on Content-Type as needed.
		char content[1024];

		size_t len(body->read(0, content, sizeof(content)));
		if (len)
		{
			return std::string(content, 0, len);
		}
	}

	// Default
	return empty;
}

HttpCoroHandler::HttpCoroHandler(LLEventStream &reply) :
    mReplyPump(reply)
{
}

void HttpCoroHandler::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
    LLSD result;

    LLCore::HttpStatus status = response->getStatus();

    if (!status)
    {
        result = LLSD::emptyMap();
        LL_WARNS()
            << "\n--------------------------------------------------------------------------\n"
            << " Error[" << status.toULong() << "] cannot access url '" << response->getRequestURL() 
            << "' because " << status.toString()
            << "\n--------------------------------------------------------------------------"
            << LL_ENDL;

    }
    else
    {
        const bool emit_parse_errors = false;

        bool parsed = !((response->getBodySize() == 0) ||
            !LLCoreHttpUtil::responseToLLSD(response, emit_parse_errors, result));

        if (!parsed)
        {
            // Only emit a warning if we failed to parse when 'content-type' == 'application/llsd+xml'
            LLCore::HttpHeaders::ptr_t headers(response->getHeaders());
            const std::string *contentType = (headers) ? headers->find(HTTP_IN_HEADER_CONTENT_TYPE) : NULL;

            if (contentType && (HTTP_CONTENT_LLSD_XML == *contentType))
            {
                std::string thebody = LLCoreHttpUtil::responseToString(response);
                LL_WARNS() << "Failed to deserialize . " << response->getRequestURL() << " [status:" << response->getStatus().toString() << "] "
                    << " body: " << thebody << LL_ENDL;

                // Replace the status with a new one indicating the failure.
                status = LLCore::HttpStatus(499, "Failed to deserialize LLSD.");
            }
        }

        if (result.isUndefined())
        {
            // If we've gotten to this point and the result LLSD is still undefined 
            // either there was an issue deserializing the body or the response was
            // blank.  Create an empty map to hold the result either way.
            result = LLSD::emptyMap();
        }
    }

    buildStatusEntry(response, status, result);
    mReplyPump.post(result);
}

void HttpCoroHandler::buildStatusEntry(LLCore::HttpResponse *response, LLCore::HttpStatus status, LLSD &result)
{
    LLSD httpresults = LLSD::emptyMap();

    httpresults["success"] = static_cast<LLSD::Boolean>(status);
    httpresults["type"] = static_cast<LLSD::Integer>(status.getType());
    httpresults["status"] = static_cast<LLSD::Integer>(status.getStatus());
    httpresults["message"] = static_cast<LLSD::String>(status.getMessage());
    httpresults["url"] = static_cast<LLSD::String>(response->getRequestURL());

    LLSD httpHeaders = LLSD::emptyMap();
    LLCore::HttpHeaders::ptr_t hdrs = response->getHeaders();

    if (hdrs)
    {
        for (LLCore::HttpHeaders::iterator it = hdrs->begin(); it != hdrs->end(); ++it)
        {
            if (!(*it).second.empty())
            {
                httpHeaders[(*it).first] = (*it).second;
            }
            else
            {
                httpHeaders[(*it).first] = static_cast<LLSD::Boolean>(true);
            }
        }
    }

    httpresults["headers"] = httpHeaders;
    result["http_result"] = httpresults;
}


} // end namespace LLCoreHttpUtil


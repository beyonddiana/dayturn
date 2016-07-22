/** 
 * @file llimagej2ckdu.h
 * @brief This is an implementation of JPEG2000 encode/decode using Kakadu
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLIMAGEJ2CKDU_H
#define LL_LLIMAGEJ2CKDU_H

#include "llimagej2c.h"

//
// KDU core header files
//
#define KDU_NO_THREADS
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"

// don't *really* want to rebuild KDU so turn off specific warnings for this header
#if LL_DARWIN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#include "kdu_compressed.h"
#pragma clang diagnostic pop
#else
#include "kdu_compressed.h"
#endif

#include "kdu_sample_processing.h"

class LLKDUDecodeState;
class LLKDUMemSource;

class LLImageJ2CKDU : public LLImageJ2CImpl
{	
public:
	enum ECodeStreamMode 
	{
		MODE_FAST = 0,
		MODE_RESILIENT = 1,
		MODE_FUSSY = 2
	};
	LLImageJ2CKDU();
	virtual ~LLImageJ2CKDU();
	
protected:
	virtual bool getMetadata(LLImageJ2C &base);
	virtual bool decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count);
	virtual bool encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time=0.0,
								bool reversible=false);
	virtual bool initDecode(LLImageJ2C &base, LLImageRaw &raw_image, int discard_level = -1, int* region = NULL);
	virtual bool initEncode(LLImageJ2C &base, LLImageRaw &raw_image, int blocks_size = -1, int precincts_size = -1, int levels = 0);
	virtual std::string getEngineInfo() const;
	void findDiscardLevelsBoundaries(LLImageJ2C &base);

private:
	bool initDecode(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, ECodeStreamMode mode, S32 first_channel, S32 max_channel_count, int discard_level = -1, int* region = NULL);
	void setupCodeStream(LLImageJ2C &base, bool keep_codestream, ECodeStreamMode mode);
	void cleanupCodeStream();

	// Encode variable
	LLKDUMemSource *mInputp;
	kdu_codestream *mCodeStreamp;
	kdu_coords *mTPosp; // tile position
	kdu_dims *mTileIndicesp;
	int mBlocksSize;
	int mPrecinctsSize;
	int mLevels;

	// Temporary variables for in-progress decodes...
	LLImageRaw *mRawImagep;
	LLKDUDecodeState *mDecodeState;
};

#endif

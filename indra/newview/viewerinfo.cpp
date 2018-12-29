/**
 * @file viewerinfo.cpp
 * @brief Functions for querying the viewer name and version.
 * @author Jacek Antonelli
 *
 * Copyright (c) 2010, Jacek Antonelli
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "llviewerprecompiledheaders.h"

#include "viewerinfo.h"

namespace ViewerInfo
{

	// These are intentionally defined here instead of in the header,
	// because they should NOT be read directly. Use the functions.
	const std::string NAME  = "Dayturn";
	const S32         MAJOR = 3;
	const S32         MINOR = 6;
	const S32         PATCH = 4;
	const S32         BUILD = 28144;
	const std::string EXTRA = "";


	const std::string& viewerName()
	{
		return NAME;
	}

	S32 versionMajor()
	{
		return MAJOR;
	}

	S32 versionMinor()
	{
		return MINOR;
	}

	S32 versionPatch()
	{
		return PATCH;
	}

	const std::string& versionExtra()
	{
		return EXTRA;
	}

	const std::string& versionNumber()
	{
		static std::string s = llformat("%d.%d.%d.%d", MAJOR, MINOR, PATCH, BUILD);
		return s;
	}

	const std::string& versionFull()
	{
		static std::string s;
		if (s.length() > 0)
		{
			return s;
		}

		s = versionNumber();

		if (EXTRA.length() > 0)
		{
			s += " " + EXTRA;
		}

		return s;
	}

	const std::string& fullInfo()
	{
		static std::string s = NAME + " " + versionFull();
		return s;
	}

}

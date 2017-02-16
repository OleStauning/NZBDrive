/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef SEGMENTSTATE_HPP
#define SEGMENTSTATE_HPP

namespace ByteFountain
{
	enum class SegmentState
	{
		None,
		Loading,
		HasData,
		MissingSegment,
		DownloadFailed
	};
}

#endif
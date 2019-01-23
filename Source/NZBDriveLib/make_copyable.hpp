/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef MAKE_COPYABLE
#define MAKE_COPYABLE

#include <memory>
#include <functional>

namespace ByteFountain
{
	template <class F> auto make_copyable(F&& f)
	{
		auto spf = std::make_shared<std::decay_t<F>>(std::forward<F>(f));
		return [spf](auto&&... args)->decltype(auto) { return (*spf)(args...); };
	}
}

#endif

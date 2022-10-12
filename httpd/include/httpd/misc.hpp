﻿//
// misc.hpp
// ~~~~~~~~
//
// Copyright (c) 2022 Jack (jack.arain at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>
#include <version>


#ifdef __linux__
#  include <sys/resource.h>

# ifndef HAVE_UNAME
#  define HAVE_UNAME
# endif

#elif _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <fcntl.h>
#  include <io.h>
#  include <windows.h>

#endif // _WIN32

#ifdef HAVE_UNAME
#  include <sys/utsname.h>
#endif


//////////////////////////////////////////////////////////////////////////

inline bool is_space(const char c)
{
	if (c == ' ' ||
		c == '\f' ||
		c == '\n' ||
		c == '\r' ||
		c == '\t' ||
		c == '\v')
		return true;
	return false;
}


//////////////////////////////////////////////////////////////////////////

inline std::string_view string_trim(std::string_view sv)
{
	const char* b = sv.data();
	const char* e = b + sv.size();

	for (; b != e; b++)
	{
		if (!is_space(*b))
			break;
	}

	for (; e != b; )
	{
		if (!is_space(*(--e)))
		{
			++e;
			break;
		}
	}

	return std::string_view(b, e - b);
}


//////////////////////////////////////////////////////////////////////////

inline bool parse_endpoint_string(std::string_view str,
	std::string& host, std::string& port, bool& ipv6only)
{
	ipv6only = false;

	auto address_string = string_trim(str);
	auto it = address_string.begin();

	bool is_ipv6_address = *it == '[';
	if (is_ipv6_address)
	{
		auto host_end = std::find(it, address_string.end(), ']');
		if (host_end == address_string.end())
			return false;

		it++;
		for (auto first = it; first != host_end; first++)
			host.push_back(*first);

		std::advance(it, host_end - it);
		it++;
	}
	else
	{
		auto host_end = std::find(it, address_string.end(), ':');
		if (host_end == address_string.end())
			return false;

		for (auto first = it; first != host_end; first++)
			host.push_back(*first);

		// Skip host.
		std::advance(it, host_end - it);
	}

	if (*it != ':')
		return false;

	it++;
	for (; it != address_string.end(); it++)
	{
		if (*it >= '0' && *it <= '9')
		{
			port.push_back(*it);
			continue;
		}

		break;
	}

	if (it != address_string.end())
	{
#ifdef __cpp_lib_to_address
		auto opt = std::string_view(
			std::to_address(it), address_string.end() - it);
#else
		auto opt = std::string(it, address_string.end());
#endif
		if (opt == "ipv6only" || opt == "-ipv6only")
			ipv6only = true;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////

inline int platform_init()
{
#if defined(WIN32) || defined(_WIN32)
	/* Disable the "application crashed" popup. */
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
		SEM_NOOPENFILEERRORBOX);

#if defined(DEBUG) ||defined(_DEBUG)
	//	_CrtDumpMemoryLeaks();
	// 	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	// 	flags |= _CRTDBG_LEAK_CHECK_DF;
	// 	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	// 	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	// 	_CrtSetDbgFlag(flags);
#endif

#if !defined(__MINGW32__)
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	_setmode(0, _O_BINARY);
	_setmode(1, _O_BINARY);
	_setmode(2, _O_BINARY);

	/* Disable stdio output buffering. */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* Enable minidump when application crashed. */
#elif defined(__linux__)
	rlimit of = { 50000, 100000 };
	setrlimit(RLIMIT_NOFILE, &of);

	struct rlimit core_limit;
	core_limit.rlim_cur = RLIM_INFINITY;
	core_limit.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limit);

	/* Set the stack size programmatically with setrlimit */
	rlimit rl;
	int result = getrlimit(RLIMIT_STACK, &rl);
	if (result == 0)
	{
		const rlim_t stack_size = 100 * 1024 * 1024;
		if (rl.rlim_cur < stack_size)
		{
			rl.rlim_cur = stack_size;
			setrlimit(RLIMIT_STACK, &rl);
		}
	}
#endif

	std::ios::sync_with_stdio(false);

	return 0;
}


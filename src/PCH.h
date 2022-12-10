#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <ClibUtil/simpleINI.hpp>
#include <ankerl/unordered_dense.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <srell.hpp>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace ini = clib_util::ini;

using namespace std::literals;

template <class K, class D>
using Map = ankerl::unordered_dense::map<K, D>;
template <class K>
using Set = ankerl::unordered_dense::set<K>;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_vfunc(REL::ID a_src)
	{
		REL::Relocation<std::uintptr_t> vtbl{ a_src };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}
}

#include "Version.h"

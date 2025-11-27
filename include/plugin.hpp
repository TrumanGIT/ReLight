#pragma once

#include <RE/Skyrim.h>
#include <RE/V/VirtualMachine.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

#pragma warning(push)

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;
using namespace std;

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
}

#define DLLEXPORT __declspec(dllexport)

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()


#pragma once

#include "logger.hpp"

struct BSLightingShader_SetupGeometry
		{
			static void thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags);
			static inline REL::Relocation<decltype(thunk)> func;

			static void Install();
		};

struct BSShaderPropertyLightData_AttachLight
{
	static __int64 thunk(RE::BSShaderPropertyLightData* This, RE::BSLight* BSLight);
	static inline std::uintptr_t func;  // changed this
	static void Install();
};
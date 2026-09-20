#pragma once

#include <cstdint>

namespace Random {

	inline uint32_t xorshift32(uint32_t& state)
	{
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}

	inline float rand(uint32_t& state)
	{
		uint32_t r = xorshift32(state);
		return (r >> 8) * (1.0f / 16777216.0f);
	}

	inline float getRandomFloat(const float& min, const float& max)
	{
		static std::mt19937 rng{ std::random_device{}() };
		std::uniform_real_distribution<float> dist(min, max);
		return dist(rng);
	}

}
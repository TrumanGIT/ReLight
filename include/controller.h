#pragma once

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "random.h"

enum class INTERPOLATION : std::uint8_t
{
	kStep,
	kLinear,
	kCubic
};

template <class T, std::uint32_t index = 0>
struct Keyframe
{
	float time{};
	T     value{};
	T     forward{};
	T     backward{};
};

template <class T, std::uint32_t index = 0>
class KeyframeSequence
{
public:
	void clear() { keys = {}; }
	bool empty() const { return keys.empty(); }
	explicit operator bool() const { return !empty(); }

	float GetDuration() const { return keys.empty() ? 0.0f : keys.back().time - keys.front().time; }

	T GetValue(const float a_time, std::uint32_t& a_lastIndex) const
	{
		for (auto i = a_lastIndex; i < keys.size() - 1; ++i) {
			const auto& currKeyframe = keys[i];
			const auto& nextKeyframe = keys[i + 1];

			if (a_time >= currKeyframe.time && a_time <= nextKeyframe.time) {
				a_lastIndex = i;
				return Interpolate(a_time, currKeyframe, nextKeyframe);
			}
		}

		a_lastIndex = 0;
		return keys.front().value;
	}

	INTERPOLATION                   interpolation{ INTERPOLATION::kLinear };
	std::vector<Keyframe<T, index>> keys{};

private:
	T Interpolate(float a_time, const Keyframe<T, index>& a_start, const Keyframe<T, index>& a_end) const
	{
		float dt = a_end.time - a_start.time;

		if (dt <= 0.0f) {
			return a_start.value;
		}

		float t = (a_time - a_start.time) / dt;

		switch (interpolation) {
		case INTERPOLATION::kStep:
			return a_start.value;
		case INTERPOLATION::kLinear:
			return (1 - t) * a_start.value + t * a_end.value;
		case INTERPOLATION::kCubic:
		{
			// Hermite interpolation formula
			float t2 = t * t;
			float t3 = t2 * t;
			float h1 = 2 * t3 - 3 * t2 + 1;
			float h2 = -2 * t3 + 3 * t2;
			float h3 = t3 - 2 * t2 + t;
			float h4 = t3 - t2;

			return h1 * a_start.value +
				h2 * a_end.value +
				h3 * a_start.forward * dt +
				h4 * a_end.backward * dt;
		}
		default:
			return T();
		}
	}
};

template <class T, std::uint32_t index = 0>
class LightController
{
public:
	LightController() = default;
	LightController(const KeyframeSequence<T, index>* a_sequence, bool a_randomAnimStart) :
		sequence(a_sequence),
		duration(a_sequence->GetDuration())
	{
		if (a_randomAnimStart && duration > 0.0f) {
			currentTime = Random::getRandomFloat(0.0f, duration);  
		}
	}

	T GetValue(const float a_delta)
	{
		currentTime += a_delta;

		if (currentTime >= duration) {
			currentTime = duration;
			return sequence->keys.back().value;
		}

		return sequence->GetValue(currentTime, lastIndex);
	}

	bool empty() const { return !sequence || sequence->empty(); }
	explicit operator bool() const { return !empty(); }

private:
	const KeyframeSequence<T, index>* sequence{ nullptr };
	std::uint32_t                     lastIndex{ 0 };
	float                             currentTime{ 0.0f };
	float                             duration{ 0.0f };
};

using FloatKeyframe = Keyframe<float>;
using FloatKeyframeSequence = KeyframeSequence<float>;
using FloatController = LightController<float>;

// Per-light playback state
//
// The config (shared by every light using it) owns the FloatKeyframeSequence.
// LightController<float> owns currentTime/lastIndex and points at that sequence,
// so there must be one controller per light. Keyed by NiLight address.
namespace FadeState
{
	struct Entry
	{
		FloatController              controller{};
		const FloatKeyframeSequence* sequence{ nullptr };  // definition the controller was built from
		std::uint32_t                lastFrame{ 0 };
	};

	inline std::unordered_map<const void*, Entry> g_entries;
	inline std::uint32_t                          g_frame = 0;

	// once per frame, from the hook
	inline void NextFrame() { ++g_frame; }

	// ~once per second: drop entries for lights not updated recently
	// (unloaded cells, lights past the distance cutoff, ...)
	inline void Prune()
	{
		std::erase_if(g_entries, [](const auto& kv) {
			return (g_frame - kv.second.lastFrame) > 120;
			});
	}

	inline void Remove(const void* a_light) { g_entries.erase(a_light); }

	inline void Clear() { g_entries.clear(); }

	// Returns the animated multiplier for this light.
	// Sequences with < 2 keys or zero duration can't animate (and fmod by 0 would give NaN),
	// so they're treated as a constant.
	inline float GetValue(const void* a_light, const FloatKeyframeSequence& a_sequence, float a_delta, bool a_randomStart)
	{
		if (a_sequence.keys.size() < 2 || a_sequence.GetDuration() <= 0.0f) {
			return a_sequence.empty() ? 1.0f : a_sequence.keys.front().value;
		}

		auto& entry = g_entries[a_light];

		// new light, or this NiLight address was reused by a light with a different config
		if (entry.sequence != &a_sequence) {
			entry.controller = FloatController(&a_sequence, a_randomStart);
			entry.sequence = &a_sequence;
		}

		entry.lastFrame = g_frame;
		return entry.controller.GetValue(a_delta);
	}
}
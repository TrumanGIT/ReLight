#include "raycast.hpp"


namespace raycast {

	bool TESRayHitStatic(RE::bhkWorld* world, RE::NiPoint3 start, RE::NiPoint3 end)
	{
		RE::bhkPickData pickData{};
		const float scale = RE::bhkWorld::GetWorldScale();
		pickData.rayInput.from = start * scale;
		pickData.rayInput.to = end * scale;
		pickData.rayInput.enableShapeCollectionFilter = true;
		RE::CFilter filter{};
		filter.SetCollisionLayer(RE::COL_LAYER::kLOS);
		static const std::uint32_t sSystemGroup =
			RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup();
		filter.SetSystemGroup(sSystemGroup);
		pickData.rayInput.filterInfo = filter;
		{
			RE::BSReadLockGuard lock(world->worldLock);
			world->PickObject(pickData);
		}
		if (!pickData.rayOutput.HasHit())
			return false;
		auto* collidable = pickData.rayOutput.rootCollidable;
		if (!collidable)
			return false;
		auto layer = collidable->GetCollisionLayer();
		if (layer != RE::COL_LAYER::kStatic &&
			layer != RE::COL_LAYER::kTerrain &&
			layer != RE::COL_LAYER::kGround)
			return false;
		auto* niObj = RE::TES::GetSingleton()->Pick(pickData);
		if (!niObj || niObj->name.empty())
			return false;
		logger::debug("TES::Pick hit node: {}", niObj->name.c_str());
		return (niObj->name.contains("wall") || niObj->name.contains("floor") || niObj->name.contains("intcor") || niObj->name.contains("farmintinnend")) &&
			!niObj->name.contains("shelf");
	}

	bool HasAnythingBetween(RE::NiPoint3 start, RE::NiPoint3 end)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player) return false;

		auto* cell = player->GetParentCell();
		if (!cell) return false;
		auto* world = cell->GetbhkWorld();
		if (!world) return false;

		bool hitLow = TESRayHitStatic(world, start + RE::NiPoint3(0, 0, 35.0f), end + RE::NiPoint3(0, 0, 35.0f));
		bool hitMid = TESRayHitStatic(world, start + RE::NiPoint3(0, 0, 70.0f), end + RE::NiPoint3(0, 0, 70.0f));
		bool hitHigh = TESRayHitStatic(world, start + RE::NiPoint3(0, 0, 105.0f), end + RE::NiPoint3(0, 0, 105.0f));

		logger::debug("Ray results: low {} mid {} high {}", hitLow, hitMid, hitHigh);
		return hitLow && hitMid && hitHigh;
	}

}
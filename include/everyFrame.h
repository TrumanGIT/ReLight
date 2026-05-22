#pragma once

#include "logger.hpp"
#include "random.h"
#include "LightData.h"
#include "disableLights.h"
#include "LightManager.h"
#include "utility.h"
#include "global.h"

inline float NiSinQImpl(float a_value)
{
	static constexpr std::array<float, 512> sineTable = {
		0.0f,
		0.012271538f,
		0.024541229f,
		0.036807224f,
		0.049067676f,
		0.06132074f,
		0.07356457f,
		0.08579731f,
		0.098017134f,
		0.1102222f,
		0.12241066f,
		0.1345807f,
		0.14673047f,
		0.15885815f,
		0.1709619f,
		0.1830399f,
		0.19509034f,
		0.2071114f,
		0.21910127f,
		0.23105815f,
		0.24298023f,
		0.2548657f,
		0.2667128f,
		0.27851975f,
		0.29028472f,
		0.302006f,
		0.3136818f,
		0.32531038f,
		0.33688992f,
		0.34841877f,
		0.3598951f,
		0.3713173f,
		0.38268352f,
		0.39399213f,
		0.40524143f,
		0.41642967f,
		0.4275552f,
		0.43861637f,
		0.44961146f,
		0.46053883f,
		0.47139686f,
		0.48218387f,
		0.4928983f,
		0.5035384f,
		0.51410276f,
		0.5245897f,
		0.53499764f,
		0.545325f,
		0.5555702f,
		0.56573176f,
		0.5758081f,
		0.5857978f,
		0.5956992f,
		0.6055109f,
		0.61523145f,
		0.62485933f,
		0.6343931f,
		0.6438313f,
		0.6531726f,
		0.6624155f,
		0.6715587f,
		0.6806007f,
		0.68954027f,
		0.69837594f,
		0.7071065f,
		0.7157305f,
		0.72424674f,
		0.7326539f,
		0.74095076f,
		0.74913603f,
		0.75720847f,
		0.7651669f,
		0.7730101f,
		0.7807368f,
		0.788346f,
		0.79583645f,
		0.8032071f,
		0.81045675f,
		0.81758434f,
		0.82458884f,
		0.8314692f,
		0.83822423f,
		0.8448531f,
		0.8513548f,
		0.85772824f,
		0.8639725f,
		0.87008667f,
		0.8760698f,
		0.881921f,
		0.88763934f,
		0.89322406f,
		0.89867425f,
		0.9039891f,
		0.90916777f,
		0.9142096f,
		0.91911376f,
		0.9238794f,
		0.928506f,
		0.9329927f,
		0.93733895f,
		0.941544f,
		0.9456073f,
		0.94952816f,
		0.953306f,
		0.95694035f,
		0.9604305f,
		0.9637761f,
		0.9669765f,
		0.97003126f,
		0.97293997f,
		0.97570217f,
		0.9783174f,
		0.98078537f,
		0.98310554f,
		0.98527765f,
		0.98730147f,
		0.98917663f,
		0.99090266f,
		0.99247956f,
		0.99390703f,
		0.9951848f,
		0.9963127f,
		0.9972905f,
		0.99811816f,
		0.9987955f,
		0.9993224f,
		0.9996988f,
		0.9999247f,
		1.0f,
		0.9999247f,
		0.99969876f,
		0.99932235f,
		0.9987954f,
		0.9981181f,
		0.9972904f,
		0.99631256f,
		0.99518466f,
		0.99390686f,
		0.9924794f,
		0.9909025f,
		0.98917633f,
		0.98730123f,
		0.9852775f,
		0.98310524f,
		0.980785f,
		0.97831714f,
		0.9757018f,
		0.97293967f,
		0.9700309f,
		0.96697617f,
		0.96377563f,
		0.9604301f,
		0.9569399f,
		0.9533056f,
		0.9495276f,
		0.94560677f,
		0.94154346f,
		0.9373384f,
		0.93299216f,
		0.9285054f,
		0.9238788f,
		0.9191131f,
		0.914209f,
		0.90916723f,
		0.9039885f,
		0.89867365f,
		0.8932234f,
		0.8876387f,
		0.88192034f,
		0.87606907f,
		0.87008595f,
		0.86397177f,
		0.85772747f,
		0.851354f,
		0.8448523f,
		0.83822346f,
		0.83146834f,
		0.824588f,
		0.81758344f,
		0.8104558f,
		0.8032061f,
		0.79583544f,
		0.7883449f,
		0.7807357f,
		0.7730088f,
		0.76516557f,
		0.75720716f,
		0.74913466f,
		0.74094933f,
		0.7326524f,
		0.7242452f,
		0.7157289f,
		0.7071048f,
		0.6983742f,
		0.6895385f,
		0.68059886f,
		0.6715568f,
		0.66241354f,
		0.6531705f,
		0.64382917f,
		0.6343909f,
		0.624857f,
		0.61522907f,
		0.6055085f,
		0.5956967f,
		0.5857952f,
		0.5758055f,
		0.565729f,
		0.5555674f,
		0.5453221f,
		0.5349947f,
		0.52458674f,
		0.5140997f,
		0.50353533f,
		0.49289507f,
		0.4821806f,
		0.47139347f,
		0.4605354f,
		0.44960797f,
		0.43861282f,
		0.42755163f,
		0.41642603f,
		0.40523773f,
		0.3939884f,
		0.38267976f,
		0.37131345f,
		0.35989127f,
		0.34841484f,
		0.336886f,
		0.32530636f,
		0.31367776f,
		0.30200192f,
		0.2902806f,
		0.27851558f,
		0.26670858f,
		0.25486144f,
		0.2429759f,
		0.2310538f,
		0.21909688f,
		0.20710696f,
		0.19508587f,
		0.18303539f,
		0.17095734f,
		0.15885356f,
		0.14672585f,
		0.13457604f,
		0.12240596f,
		0.11021745f,
		0.09801234f,
		0.085792474f,
		0.07355969f,
		0.061315827f,
		0.04906273f,
		0.03680224f,
		0.024536205f,
		0.012266479f,
		-5.094213e-06f,
		-0.012276667f,
		-0.02454639f,
		-0.036812417f,
		-0.0490729f,
		-0.061325993f,
		-0.07356986f,
		-0.08580263f,
		-0.09802249f,
		-0.110227585f,
		-0.12241608f,
		-0.13458614f,
		-0.14673592f,
		-0.15886362f,
		-0.17096739f,
		-0.18304542f,
		-0.19509587f,
		-0.20711695f,
		-0.21910682f,
		-0.23106371f,
		-0.2429858f,
		-0.2548713f,
		-0.26671842f,
		-0.27852535f,
		-0.29029036f,
		-0.30201164f,
		-0.31368744f,
		-0.325316f,
		-0.33689559f,
		-0.3484244f,
		-0.35990077f,
		-0.37132293f,
		-0.38268918f,
		-0.3939978f,
		-0.40524706f,
		-0.4164353f,
		-0.42756084f,
		-0.43862197f,
		-0.44961706f,
		-0.46054444f,
		-0.47140247f,
		-0.48218948f,
		-0.4929039f,
		-0.50354403f,
		-0.5141084f,
		-0.5245953f,
		-0.53500324f,
		-0.54533064f,
		-0.55557585f,
		-0.5657374f,
		-0.5758138f,
		-0.5858034f,
		-0.59570485f,
		-0.60551655f,
		-0.61523706f,
		-0.62486494f,
		-0.6343987f,
		-0.643837f,
		-0.6531782f,
		-0.6624211f,
		-0.6715643f,
		-0.68060625f,
		-0.68954575f,
		-0.6983814f,
		-0.70711195f,
		-0.715736f,
		-0.72425216f,
		-0.73265934f,
		-0.7409561f,
		-0.74914134f,
		-0.7572138f,
		-0.7651721f,
		-0.77301526f,
		-0.780742f,
		-0.7883511f,
		-0.7958416f,
		-0.80321217f,
		-0.81046176f,
		-0.8175893f,
		-0.8245937f,
		-0.831474f,
		-0.838229f,
		-0.8448578f,
		-0.85135937f,
		-0.8577328f,
		-0.86397696f,
		-0.870091f,
		-0.876074f,
		-0.8819251f,
		-0.8876434f,
		-0.893228f,
		-0.8986781f,
		-0.9039929f,
		-0.90917146f,
		-0.9142132f,
		-0.9191172f,
		-0.92388284f,
		-0.92850924f,
		-0.9329959f,
		-0.93734205f,
		-0.941547f,
		-0.94561017f,
		-0.94953096f,
		-0.9533087f,
		-0.9569429f,
		-0.960433f,
		-0.9637785f,
		-0.9669788f,
		-0.9700334f,
		-0.972942f,
		-0.9757041f,
		-0.9783193f,
		-0.98078704f,
		-0.9831072f,
		-0.9852792f,
		-0.9873029f,
		-0.9891778f,
		-0.99090385f,
		-0.99248064f,
		-0.99390805f,
		-0.9951857f,
		-0.99631345f,
		-0.9972912f,
		-0.99811864f,
		-0.99879587f,
		-0.9993227f,
		-0.99969906f,
		-0.99992484f,
		-1.0f,
		-0.99992466f,
		-0.9996986f,
		-0.999322f,
		-0.998795f,
		-0.9981175f,
		-0.9972898f,
		-0.9963118f,
		-0.99518377f,
		-0.9939059f,
		-0.9924784f,
		-0.9909013f,
		-0.9891751f,
		-0.98729986f,
		-0.9852759f,
		-0.9831037f,
		-0.98078334f,
		-0.97831535f,
		-0.9756999f,
		-0.9729376f,
		-0.97002876f,
		-0.9669739f,
		-0.96377337f,
		-0.9604277f,
		-0.9569373f,
		-0.9533029f,
		-0.94952494f,
		-0.94560397f,
		-0.94154054f,
		-0.9373354f,
		-0.932989f,
		-0.9285022f,
		-0.9238755f,
		-0.9191097f,
		-0.91420543f,
		-0.90916353f,
		-0.9039847f,
		-0.8986697f,
		-0.8932195f,
		-0.88763463f,
		-0.88191617f,
		-0.87606484f,
		-0.8700816f,
		-0.8639673f,
		-0.85772294f,
		-0.8513494f,
		-0.8448476f,
		-0.8382186f,
		-0.8314634f,
		-0.82458293f,
		-0.8175783f,
		-0.81045055f,
		-0.8032008f,
		-0.79583f,
		-0.7883394f,
		-0.78073007f,
		-0.77300316f,
		-0.76515985f,
		-0.7572013f,
		-0.7491287f,
		-0.7409433f,
		-0.73264635f,
		-0.724239f,
		-0.7157226f,
		-0.7070985f,
		-0.6983678f,
		-0.689532f,
		-0.6805923f,
		-0.67155015f,
		-0.6624068f,
		-0.6531638f,
		-0.6438224f,
		-0.634384f,
		-0.62485003f,
		-0.61522204f,
		-0.60550135f,
		-0.59568954f,
		-0.58578795f,
		-0.57579815f,
		-0.5657217f,
		-0.55556f,
		-0.5453146f,
		-0.53498715f,
		-0.5245791f,
		-0.514092f,
		-0.5035276f,
		-0.4928873f,
		-0.48217276f,
		-0.47138563f,
		-0.46052748f,
		-0.4496f,
		-0.4386048f,
		-0.42754358f,
		-0.41641793f,
		-0.4052296f,
		-0.39398023f,
		-0.38267154f,
		-0.3713052f,
		-0.35988295f,
		-0.3484065f,
		-0.33687758f,
		-0.32529795f,
		-0.3136693f,
		-0.30199343f,
		-0.2902721f,
		-0.278507f,
		-0.2667f,
		-0.25485283f,
		-0.24296726f,
		-0.23104513f,
		-0.21908818f,
		-0.20709825f,
		-0.19507714f,
		-0.18302663f,
		-0.17094857f,
		-0.15884475f,
		-0.14671703f,
		-0.1345672f,
		-0.122397125f,
		-0.1102086f,
		-0.098003484f,
		-0.08578361f,
		-0.07355081f,
		-0.061306935f,
		-0.049053825f,
		-0.036793333f,
		-0.024527298f,
		-0.01225757f,
	};
	return sineTable[static_cast<std::uint32_t>(a_value) & 511];
}

inline float NiSinQ(float a_radians)
{
	return NiSinQImpl((512.0f / (2.0f * std::numbers::pi_v<float>)) * a_radians);
}
// hook into player update so we can update light flicker data every fram
struct PlayerCharacter_Update {

    static void thunk(RE::PlayerCharacter* player, float delta);

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install();
};


// used in flicker calcs
inline float getRandomFloat(const float& min, const float& max, uint32_t rngState)
{
	return min + (max - min) * Random::rand(rngState);
}

inline void HandleQueuedLights(const RE::NiPointer<RE::BSLight>& light)
{
	if (!light || !light->light) {
		return;
	}

	auto* niLight = light->light.get();
	if (!niLight) {
		return;
	}

	if (globals::magicLightQueued.load()) {
		if (niLight->parent && niLight->parent == globals::magicLightAttachNode) {
			light->unk060 = 4;

			logger::info("set magic light unk060 to 4 so it will skip the islightaffectingsurface hook");

			globals::magicLightQueued.store(false);
			globals::magicLightAttachNode = nullptr;
		}
	}

	std::vector<size_t> indicesToRemove;

	for (size_t i = 0; i < globals::torchLightAttachNodes.size(); ++i) {
		auto& attachLightNode = globals::torchLightAttachNodes[i];

		if (niLight->parent && niLight->parent == attachLightNode) {
			light->unk060 = 4;

			std::string torchName = "torch";

			auto cfgs = findConfigsForMeshPath(torchName, globals::currentCellIsInterior);
			if (cfgs.empty()) {
				continue;
			}

			niLight->name = "RL" + torchName;
			niLight->unk138 = cfgs[0].configID;

			LightData::setNiPointLightDataFromCfg(niLight, cfgs[0], 1.0f);

			indicesToRemove.push_back(i);
		}
	}

	for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it) {
		globals::torchLightAttachNodes.erase(globals::torchLightAttachNodes.begin() + *it);
	}
}

// generic type argument probly not needed both shadow light list and non shadow light list same array type proboblly
template <class T>
static void ApplyLightFlicker(T& lights, float delta, bool shadowLights, RE::NiPoint3 playerPos)
{

	constexpr float maxDist = 5000.0f;
	constexpr float maxDistSq = maxDist * maxDist;

    std::vector<RE::NiPointer<RE::NiLight>> toRemove;  

    for (auto& light : lights) {
        if (!light || !light->light)
			continue;

		HandleQueuedLights(light); 

        auto name = std::string_view(light->light->name.c_str());
        if (name.size() < 2 || name[0] != 'R' || name[1] != 'L')
            continue;

		auto diff = light->light->world.translate - playerPos;
		float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		if (distSq > maxDistSq)
			continue;

        // this is to remove lights from the scene otherwise they stay after mesh unloads
        if (!light->light->parent) {
            toRemove.push_back(light->light);
            continue;
        }

		// scale I use as a free float used as flicker timer
		auto& scale = light->light->local.scale;

		auto& rt = light->light->GetLightRuntimeData();

		auto* pointLight = netimmerse_cast<RE::NiPointLight*>(light->light.get());
		if (!pointLight)
			continue;

		// seems like everyone throws ambient away like Community shaders for example
		// so we will do the same to act as 3 free floats we can store values in for flicker amplitude.
		auto& pos = light->light->local.translate;

		auto it = LightData::configIDToJsonCfg.find(rt.unk138);
		if (it == LightData::configIDToJsonCfg.end())
			continue;

		const auto& dataExt = it->second;

		uint32_t seed =
			static_cast<uint32_t>(
				reinterpret_cast<std::uintptr_t>(light.get()) & 0xFFFFFFFF);

        // r could add more randomness but we already do that with the seed so I dont use r actually lmao
		const float r = getRandomFloat(-0.1f, 0.1f, seed);

		//pulsing
		scale += delta * (1.0f - r) * std::numbers::pi_v<float>;
		rt.fade =
			(dataExt.startingFade +
				NiSinQ(scale * dataExt.flickersPerSecond) * dataExt.flickerIntensity)
			* globals::brightnessModifier;

		// oscillation
		if (pointLight->constAttenuation == 0.0f &&
			pointLight->linearAttenuation == 0.0f &&
			pointLight->quadraticAttenuation == 0.0f) {

			pointLight->constAttenuation = getRandomFloat(0.0f, RE::NI_TWO_PI, seed);
			pointLight->linearAttenuation = getRandomFloat(0.0f, RE::NI_TWO_PI, seed + 1);
			pointLight->quadraticAttenuation = getRandomFloat(0.0f, RE::NI_TWO_PI, seed + 2);
		}

		const float speedBase = dataExt.flickersPerSecond * std::numbers::pi_v<float>;
		const float amp = dataExt.flickerAmplitude;

		pointLight->constAttenuation = std::fmod(pointLight->constAttenuation + delta * speedBase * 0.91f, RE::NI_TWO_PI);
		pointLight->linearAttenuation = std::fmod(pointLight->linearAttenuation + delta * speedBase * 1.13f, RE::NI_TWO_PI);
		pointLight->quadraticAttenuation = std::fmod(pointLight->quadraticAttenuation + delta * speedBase * 1.37f, RE::NI_TWO_PI);

		const float sx = NiSinQ(pointLight->constAttenuation);
		const float sy = NiSinQ(pointLight->linearAttenuation);
		const float sz = NiSinQ(pointLight->quadraticAttenuation);

		const auto& base = pointLight->local.translate;  // or whatever your config field is called

		pos.x = base[0] + sx * amp;
		pos.y = base[1] + sy * amp;
		pos.z = base[2] + sz * (amp * 0.5f);

		RE::NiUpdateData updateData{};
		updateData.time = 0.0f;
		updateData.flags = RE::NiUpdateData::Flag::kDirty;

		auto a_root = light->light->parent;
		if (!a_root) {
			continue;
		}

		a_root->UpdateTransformAndBounds(updateData);

    }
        //shadow lights are persistance even if mesh dissapers, so if we want light to go away with mesh, must do this
        if (!toRemove.empty()) {
            auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
            if (!ssNode) {
                logger::warn("ShadowSceneNode[0] is null!");
                return;
            }

            for (auto& light : toRemove) {
                if (!light.get()) continue; 
                ssNode->RemoveLight(light.get()); 
            }
        }
}

inline void handlePendingMerges() {
    std::lock_guard lock(LightManager::pendingMergesMutex);
    if (LightManager::pendingMerges.empty()) return;

    auto now = std::chrono::steady_clock::now();

    std::vector<RE::ObjectRefHandle> reprocessQueue{};

    LightManager::pendingMerges.erase(
        std::remove_if(LightManager::pendingMerges.begin(), LightManager::pendingMerges.end(),
            [&](LightManager::PendingMerge& entry) {

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.registeredAt).count();
                if (elapsed < 250) return false;

                auto refA = entry.refA.get();
                if (!refA) return true;

                std::vector<RE::ObjectRefHandle> validCandidates;

                for (const auto& handle : entry.candidateHandles) {
                    auto refB = handle.get();
                    if (!refB) continue;

                    // ray cast to ensure no walls in between
                    if (HasAnythingBetween(refA.get()->GetPosition(), refB.get()->GetPosition())) {
                        reprocessQueue.emplace_back(handle);
                        continue;
                    }
              
                    validCandidates.push_back(handle);
                }

                LightManager::finalizeMerge(entry, validCandidates);
                return true;
            }),
        LightManager::pendingMerges.end());

    while (!reprocessQueue.empty()) {

        auto retryRefAHandle = reprocessQueue.front();
        reprocessQueue.erase(reprocessQueue.begin());

        auto retryRefA = retryRefAHandle.get();
        if (!retryRefA) continue;

        std::vector<RE::ObjectRefHandle> nextQueue;
        std::vector<RE::ObjectRefHandle> mergeGroup;

        for (const auto& handle : reprocessQueue) {

            auto otherRef = handle.get();
            if (!otherRef) continue;;
          
            if (HasAnythingBetween(retryRefA.get()->GetPosition(), otherRef.get()->GetPosition())) {
                nextQueue.emplace_back(handle);
            }
            else {
                mergeGroup.emplace_back(handle);
            }
        }

        auto root = retryRefA->Get3D();
        if (!root) { reprocessQueue = std::move(nextQueue); continue; }

        auto rootNode = root->AsNode();
        if (!rootNode) { reprocessQueue = std::move(nextQueue); continue; }

        auto base = retryRefA->GetBaseObject();
        auto model = base ? base->As<RE::TESModel>() : nullptr;
        if (!model) { reprocessQueue = std::move(nextQueue); continue; }

        std::string meshName = extractMeshName(model->GetModel());
        std::string match = std::string(findPriorityMatch(meshName));
        if (match.empty()) { reprocessQueue = std::move(nextQueue); continue; }

        auto cell = retryRefA->GetParentCell();
        if (!cell) { reprocessQueue = std::move(nextQueue); continue; }

        auto cfgs = findConfigsForMeshPath(match, cell->IsInteriorCell());
        if (cfgs.empty()) {
            logger::warn("Dropping ref {:08X} no configs found", retryRefA->GetFormID());
            // keep all other refs for retry
            nextQueue.insert(nextQueue.end(), mergeGroup.begin(), mergeGroup.end());
            reprocessQueue = std::move(nextQueue);
            continue;
        }

        uint32_t flags = cfgs[0].flags;
  
        if (cfgs.size() == 1 && !(flags & static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging))) {
            auto cloneLight = LightManager::cloneNiPointLight(LightData::masterNiPointLight.light.get());
            if (!cloneLight) { reprocessQueue = std::move(nextQueue); continue; }

            logger::debug("RE processing ref {:08X} with light {}", retryRefA->GetFormID(), match);

            LightManager::attachLightUsingAttachPath(cfgs[0], rootNode, cloneLight, retryRefA->GetFormID());

            LightManager::PendingMerge p;
            p.refA = retryRefAHandle;
            p.refARoot = rootNode;
            p.candidateHandles = mergeGroup;
            p.light = RE::NiPointer<RE::NiPointLight>(cloneLight);
            p.refALightName = match;
            p.winningConfig = cfgs[0]; 

            LightManager::finalizeMerge(p, mergeGroup);
        }
     
        else {
            bool debugMarkerAttached = false; 

            for (const auto& cfg : cfgs) {
                LightManager::AttachLight(cfg, rootNode, retryRefA.get(), cfg.menuName, retryRefA->GetFormID(), debugMarkerAttached);
            }

            // multi-light can't merge, retry others
            nextQueue.insert(nextQueue.end(), mergeGroup.begin(), mergeGroup.end());
        }

        reprocessQueue = std::move(nextQueue);
    }
}

inline bool OneSecondPassed(const std::chrono::steady_clock::time_point& timerStart)
{

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timerStart).count();

    return elapsed >= 1;
}

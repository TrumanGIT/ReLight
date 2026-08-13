#pragma once

#include "logger.hpp"
#include "random.h"
#include "LightData.h"
#include "disableLights.h"
#include "LightManager.h"
#include "utility.h"
#include "global.h"
#include "raycast.hpp"
#include <CLibUtilsQTR/DrawDebug.hpp>


inline float NiSinQImpl(float a_value)
{
	// power of 3 
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

inline float NiCosQImpl(float a_value)
{
	static constexpr std::array<float, 512> cosineTable = {
		1.0,
		0.99992466f,
		0.9996988f,
		0.99932235f,
		0.9987955f,
		0.9981181f,
		0.9972905f,
		0.9963126f,
		0.9951848f,
		0.9939069f,
		0.99247956f,
		0.9909026f,
		0.9891765f,
		0.9873014f,
		0.98527765f,
		0.98310554f,
		0.98078525f,
		0.9783174f,
		0.97570217f,
		0.97293997f,
		0.97003126f,
		0.9669765f,
		0.96377605f,
		0.9604305f,
		0.95694035f,
		0.953306f,
		0.94952816f,
		0.9456073f,
		0.941544f,
		0.93733895f,
		0.93299276f,
		0.928506f,
		0.9238795f,
		0.9191138f,
		0.9142097f,
		0.90916795f,
		0.9039892f,
		0.89867437f,
		0.8932243f,
		0.8876396f,
		0.8819212f,
		0.87607f,
		0.8700869f,
		0.8639727f,
		0.8577286f,
		0.85135514f,
		0.8448535f,
		0.83822477f,
		0.83146966f,
		0.8245894f,
		0.8175848f,
		0.8104572f,
		0.80320764f,
		0.79583704f,
		0.7883465f,
		0.78073746f,
		0.7730107f,
		0.7651675f,
		0.757209f,
		0.74913657f,
		0.7409514f,
		0.7326545f,
		0.72424734f,
		0.7157312f,
		0.7071071f,
		0.6983766f,
		0.68954086f,
		0.6806013f,
		0.6715594f,
		0.6624162f,
		0.65317327f,
		0.6438321f,
		0.6343938f,
		0.62486f,
		0.6152321f,
		0.60551155f,
		0.59569997f,
		0.5857985f,
		0.57580876f,
		0.5657326f,
		0.55557096f,
		0.5453257f,
		0.5349983f,
		0.5245904f,
		0.5141034f,
		0.503539f,
		0.4928988f,
		0.48218435f,
		0.47139725f,
		0.4605392f,
		0.44961178f,
		0.43861666f,
		0.4275555f,
		0.41642994f,
		0.40524167f,
		0.39399236f,
		0.38268372f,
		0.37131745f,
		0.35989526f,
		0.34841886f,
		0.33689f,
		0.3253104f,
		0.31368184f,
		0.302006f,
		0.2902847f,
		0.2785197f,
		0.26671273f,
		0.2548656f,
		0.24298008f,
		0.23105797f,
		0.21910107f,
		0.20711116f,
		0.19509009f,
		0.1830396f,
		0.17096157f,
		0.1588578f,
		0.1467301f,
		0.13458028f,
		0.12241022f,
		0.11022172f,
		0.09801662f,
		0.08579675f,
		0.07356397f,
		0.06132011f,
		0.049067013f,
		0.036806528f,
		0.024540495f,
		0.01227077f,
		-8.026785e-07f,
		-0.012272376f,
		-0.0245421f,
		-0.03680813f,
		-0.049068615f,
		-0.06132171f,
		-0.07356557f,
		-0.08579836f,
		-0.09801822f,
		-0.110223316f,
		-0.12241182f,
		-0.13458188f,
		-0.14673169f,
		-0.15885939f,
		-0.17096317f,
		-0.18304119f,
		-0.19509166f,
		-0.20711274f,
		-0.21910264f,
		-0.23105954f,
		-0.24298164f,
		-0.25486714f,
		-0.26671427f,
		-0.27852124f,
		-0.29028624f,
		-0.30200756f,
		-0.31368336f,
		-0.32531196f,
		-0.33689153f,
		-0.34842038f,
		-0.35989678f,
		-0.37131894f,
		-0.3826852f,
		-0.39399382f,
		-0.40524313f,
		-0.4164314f,
		-0.42755696f,
		-0.43861812f,
		-0.44961324f,
		-0.46054062f,
		-0.47139868f,
		-0.4821857f,
		-0.49290013f,
		-0.50354034f,
		-0.5141047f,
		-0.5245917f,
		-0.5349996f,
		-0.545327f,
		-0.5555723f,
		-0.56573385f,
		-0.57581025f,
		-0.58579993f,
		-0.5957014f,
		-0.60551316f,
		-0.6152337f,
		-0.6248616f,
		-0.6343954f,
		-0.64383364f,
		-0.65317494f,
		-0.6624179f,
		-0.67156106f,
		-0.68060315f,
		-0.68954265f,
		-0.6983784f,
		-0.7071089f,
		-0.71573293f,
		-0.7242492f,
		-0.73265636f,
		-0.74095327f,
		-0.7491385f,
		-0.75721097f,
		-0.7651694f,
		-0.7730125f,
		-0.7807393f,
		-0.7883485f,
		-0.79583895f,
		-0.80320954f,
		-0.8104592f,
		-0.81758684f,
		-0.8245913f,
		-0.83147156f,
		-0.8382267f,
		-0.84485555f,
		-0.85135716f,
		-0.85773057f,
		-0.86397475f,
		-0.8700889f,
		-0.876072f,
		-0.8819231f,
		-0.88764143f,
		-0.8932261f,
		-0.8986762f,
		-0.90399104f,
		-0.9091697f,
		-0.91421145f,
		-0.91911554f,
		-0.9238812f,
		-0.9285077f,
		-0.93299437f,
		-0.9373405f,
		-0.94154555f,
		-0.94560874f,
		-0.9495295f,
		-0.9533074f,
		-0.9569416f,
		-0.9604318f,
		-0.9637773f,
		-0.9669777f,
		-0.97003233f,
		-0.97294104f,
		-0.9757031f,
		-0.9783184f,
		-0.9807862f,
		-0.9831064f,
		-0.9852785f,
		-0.9873022f,
		-0.9891772f,
		-0.99090326f,
		-0.99248016f,
		-0.9939076f,
		-0.99518526f,
		-0.996313f,
		-0.99729085f,
		-0.99811846f,
		-0.9987957f,
		-0.9993226f,
		-0.99969894f,
		-0.9999247f,
		-1.0f,
		-0.99992466f,
		-0.9996987f,
		-0.9993222f,
		-0.9987952f,
		-0.9981178f,
		-0.99729013f,
		-0.99631214f,
		-0.9951842f,
		-0.9939064f,
		-0.9924789f,
		-0.9909018f,
		-0.9891757f,
		-0.9873005f,
		-0.9852767f,
		-0.98310447f,
		-0.9807842f,
		-0.9783162f,
		-0.97570086f,
		-0.9729386f,
		-0.97002983f,
		-0.966975f,
		-0.96377444f,
		-0.96042883f,
		-0.9569386f,
		-0.95330423f,
		-0.9495263f,
		-0.9456054f,
		-0.941542f,
		-0.9373369f,
		-0.93299055f,
		-0.92850375f,
		-0.9238771f,
		-0.9191114f,
		-0.9142072f,
		-0.9091653f,
		-0.9039866f,
		-0.8986716f,
		-0.8932214f,
		-0.8876366f,
		-0.8819182f,
		-0.8760669f,
		-0.8700837f,
		-0.86396945f,
		-0.85772514f,
		-0.8513516f,
		-0.84484994f,
		-0.83822095f,
		-0.8314657f,
		-0.8245854f,
		-0.8175808f,
		-0.81045306f,
		-0.80320334f,
		-0.7958326f,
		-0.788342f,
		-0.78073275f,
		-0.7730059f,
		-0.7651626f,
		-0.7572041f,
		-0.74913156f,
		-0.7409462f,
		-0.73264927f,
		-0.724242f,
		-0.7157256f,
		-0.70710146f,
		-0.6983709f,
		-0.6895351f,
		-0.68059546f,
		-0.6715533f,
		-0.66241f,
		-0.653167f,
		-0.64382565f,
		-0.63438725f,
		-0.6248534f,
		-0.61522543f,
		-0.6055048f,
		-0.59569293f,
		-0.5857914f,
		-0.5758017f,
		-0.5657252f,
		-0.55556357f,
		-0.54531825f,
		-0.5349908f,
		-0.52458274f,
		-0.5140957f,
		-0.5035313f,
		-0.492891f,
		-0.4821765f,
		-0.4713894f,
		-0.4605313f,
		-0.44960383f,
		-0.43860868f,
		-0.42754745f,
		-0.41642183f,
		-0.4052335f,
		-0.39398417f,
		-0.3826755f,
		-0.3713092f,
		-0.35988694f,
		-0.34841052f,
		-0.33688164f,
		-0.325302f,
		-0.31367338f,
		-0.3019975f,
		-0.29027617f,
		-0.27851114f,
		-0.26670414f,
		-0.25485697f,
		-0.24297144f,
		-0.2310493f,
		-0.21909237f,
		-0.20710245f,
		-0.19508134f,
		-0.18303084f,
		-0.1709528f,
		-0.158849f,
		-0.14672127f,
		-0.13457146f,
		-0.12240139f,
		-0.11021287f,
		-0.09800775f,
		-0.085787885f,
		-0.07355509f,
		-0.061311215f,
		-0.049058113f,
		-0.03679762f,
		-0.02453159f,
		-0.012261862f,
		9.711589e-06f,
		0.012281284f,
		0.024551008f,
		0.036817033f,
		0.049077515f,
		0.061330605f,
		0.07357445f,
		0.08580723f,
		0.09802708f,
		0.11023217f,
		0.12242065f,
		0.13459072f,
		0.1467405f,
		0.15886818f,
		0.17097194f,
		0.18304995f,
		0.1951004f,
		0.20712146f,
		0.21911134f,
		0.23106821f,
		0.24299029f,
		0.25487575f,
		0.26672286f,
		0.2785298f,
		0.29029477f,
		0.30201605f,
		0.31369182f,
		0.32532036f,
		0.3368999f,
		0.34842873f,
		0.35990506f,
		0.37132722f,
		0.38269344f,
		0.39400202f,
		0.4052513f,
		0.4164395f,
		0.427565f,
		0.43862614f,
		0.4496212f,
		0.46054855f,
		0.47140655f,
		0.48219353f,
		0.4929079f,
		0.5035481f,
		0.5141124f,
		0.5245993f,
		0.5350072f,
		0.5453345f,
		0.5555797f,
		0.56574124f,
		0.5758176f,
		0.5858072f,
		0.59570855f,
		0.60552025f,
		0.61524075f,
		0.6248686f,
		0.6344023f,
		0.6438405f,
		0.65318173f,
		0.6624246f,
		0.6715677f,
		0.6806097f,
		0.68954915f,
		0.69838476f,
		0.70711523f,
		0.7157392f,
		0.7242554f,
		0.73266244f,
		0.7409592f,
		0.74914443f,
		0.7572168f,
		0.7651751f,
		0.77301824f,
		0.7807449f,
		0.788354f,
		0.7958444f,
		0.8032149f,
		0.8104645f,
		0.81759197f,
		0.82459635f,
		0.83147657f,
		0.83823156f,
		0.84486026f,
		0.85136175f,
		0.8577351f,
		0.8639792f,
		0.8700932f,
		0.8760762f,
		0.88192725f,
		0.8876455f,
		0.8932301f,
		0.8986801f,
		0.9039948f,
		0.90917337f,
		0.91421497f,
		0.919119f,
		0.9238845f,
		0.9285109f,
		0.9329975f,
		0.9373436f,
		0.94154847f,
		0.9456116f,
		0.94953233f,
		0.95331f,
		0.9569442f,
		0.96043426f,
		0.9637797f,
		0.96697986f,
		0.97003454f,
		0.97294307f,
		0.97570515f,
		0.9783202f,
		0.980788f,
		0.983108f,
		0.98528004f,
		0.9873036f,
		0.98917854f,
		0.9909045f,
		0.99248123f,
		0.9939085f,
		0.9951861f,
		0.9963138f,
		0.9972915f,
		0.99811894f,
		0.9987961f,
		0.99932295f,
		0.9996992f,
		0.9999249f,
	};
	return cosineTable[static_cast<std::uint32_t>(a_value) & 511];
}

// power of 3 
inline float NiSinQ(float a_radians)
{
	return NiSinQImpl((512.0f / (2.0f * std::numbers::pi_v<float>)) * a_radians);
}

//power of 3
inline float NiCosQ(float a_radians)
{
	return NiCosQImpl((512.0f / RE::NI_TWO_PI) * a_radians);
}

// hook into player update so we can update light flicker data every fram
struct PlayerCharacter_Update {

    static void thunk(RE::PlayerCharacter* player, float delta);

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install();
};

struct NiLightFlickerHook {
	static void Install();
	static void thunk(RE::TESObjectLIGH* a_lightTemplate, RE::REFR_LIGHT* a_refrLight, RE::TESObjectREFR* a_ref, float a_value);

	static inline REL::Relocation<decltype(thunk)> func;
};

inline float getRandomFloat(const float& min, const float& max)
{
	static std::mt19937 rng{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(min, max);
	return dist(rng);
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

	// spells like candle light ect, we mark them unk060 = 4; so that light flicker prevention can identify them without much work
	if (globals::magicLightQueued.load()) {
		if (niLight->parent && niLight->parent == globals::magicLightAttachNode) {
			light->light->fadeAmount = 4; 

			logger::debug("set magic light fadeAmount to 4 so it will work with light flicker prevention mode");

			globals::magicLightQueued.store(false);
			globals::magicLightAttachNode = nullptr;
		}
	}
}

//power of three light placer
inline void UpdateRegionEmittance(RE::NiColor& a_color, RE::TESRegion* a_region)
{
	auto weather = a_region->currentWeather;
	if (!weather) {
		weather = a_region->SelectWeather();
		if (weather) {
			a_region->SetCurrentWeather(weather);
		}
	}
	if (!weather) {
		if (auto defaultWeather = RE::TESForm::LookupByID<RE::TESWeather>(0x15E)) {
			weather = defaultWeather;
		}
	}
	if (weather) {
		RE::Sky::COLOR_BLEND      colorBlend{};
		RE::TESWeather::ColorTime time1{};
		RE::TESWeather::ColorTime time2{};

		auto sky = RE::Sky::GetSingleton();
		sky->FillColorBlend(colorBlend, weather, 1.0f, time1, time2);
		sky->FillColorBlendColors(colorBlend, weather, nullptr, RE::TESWeather::ColorType::kEffectLighting, time1, time2);
		auto* setting = RE::GameSettingCollection::GetSingleton()->GetSetting("fWeatherFlashDirectional");
		float flashDirectional = setting ? setting->data.f : 1.0f;
		sky->SetColor(a_color, &colorBlend, sky->flash * flashDirectional);
		logger::debug("Updated Region Emittance Color");
	}
}

// generic type argument probly not needed both shadow light list and non shadow light list same array type proboblly
template <class T>
static void updateLights(T& lights, float delta, bool shadowLights, RE::NiPoint3 playerPos, bool updateExternalEmittance)
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

		// dont update lights far from player
		auto diff = light->light->world.translate - playerPos;
		float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		if (distSq > maxDistSq)
			continue;

		auto& rt = light->light->GetLightRuntimeData();

		auto it = LightData::configIDToJsonCfg.find(rt.unk138);
		if (it == LightData::configIDToJsonCfg.end())
			continue;

		const auto& config = it->second;

		// lights from a esp already covered by vanilla's flicker update
		if (config.isPluginLight) continue; 

		// this is to remove lights from the scene otherwise they stay after mesh unloads
		auto a_root = light->light->parent;
		if (!a_root) {
			toRemove.push_back(light->light);
			continue;
		}

		if (config.emittanceRegion && updateExternalEmittance) {

			auto emittance = config.emittanceRegion->emittanceColor;

			if (emittance == globals::COLOR_BLACK) {
				UpdateRegionEmittance(emittance, config.emittanceRegion);
			}
			rt.diffuse.red = (config.diffuseColor[0] / 255.0f) * emittance.red;
			rt.diffuse.green = (config.diffuseColor[1] / 255.0f) * emittance.green;
			rt.diffuse.blue = (config.diffuseColor[2] / 255.0f) * emittance.blue;
		}

		if (config.flickersPerSecond <= 0.0f)
			continue; // static light, no flicker/pulse

		auto* pointLight = netimmerse_cast<RE::NiPointLight*>(light->light.get());
		if (!pointLight)
			continue;

		auto& pos = light->light->local.translate;

		RE::NiPoint3 basePos = light->light->worldBound.center; 

		float flickerDelta = delta * config.flickersPerSecond;

		const auto* ref = light->light->GetUserData();

		//object reference scale 
		float scale = ref ? ref->GetScale() : 1.0f;

		//dont increase gets to messy, only handle mini objects like mini fires in windhelm otherwise too bright
		if (scale >= 1.0f) scale = 1.0f;

		if (!LightData::HasRelightFlag(config.flags, RELIGHT_FLAGS::kPulse)) {

			// ---- Flicker (matches vanilla kFlicker / kFlickerSlow) ----
			auto constAttenOffset = std::fmod(pointLight->constAttenuation + getRandomFloat(1.1f, 13.1f) * flickerDelta, RE::NI_TWO_PI);
			auto linearAttenOffset = std::fmod(pointLight->linearAttenuation + getRandomFloat(1.2f, 13.2f) * flickerDelta, RE::NI_TWO_PI);
			auto quadraticAttenOffset = std::fmod(pointLight->quadraticAttenuation + getRandomFloat(1.3f, 19.3f) * flickerDelta, RE::NI_TWO_PI);

			pointLight->constAttenuation = constAttenOffset;
			pointLight->linearAttenuation = linearAttenOffset;
			pointLight->quadraticAttenuation = quadraticAttenOffset;

			const auto constAttenSine = NiSinQ(constAttenOffset + 1.7f);
			const auto linearAttenSine = NiSinQ(linearAttenOffset + 0.5f);

			float flickerMovementMult = (config.flickerAmplitude * constAttenSine) * linearAttenSine * 0.5f;
			if ((flickerMovementMult + config.flickerAmplitude) <= 0.0f) {
				flickerMovementMult = 0.0f;
			}

			pos.x = basePos.x + flickerMovementMult * constAttenSine;
			pos.y = basePos.y + flickerMovementMult * linearAttenSine;
			pos.z = basePos.z + flickerMovementMult * NiSinQ(quadraticAttenOffset + 0.3f);

			const float halfIntensityAmplitude = config.flickerIntensity * 0.5f;
			const float flickerIntensityVal = std::clamp(
				(NiSinQImpl(linearAttenOffset * 1.3f * (512.0f / RE::NI_TWO_PI) + 52.966763f) + 1.0f) * 0.5f *
				(NiSinQImpl(constAttenOffset * 1.1f * (512.0f / RE::NI_TWO_PI) + 152.38132f) + 1.0f) * 0.5f * 0.33333331f +
				NiSinQImpl(quadraticAttenOffset * 3.0f * (512.0f / RE::NI_TWO_PI) + 73.3386f) * 0.2f,
				-1.0f, 1.0f);

			const float baseFade = config.startingFade * globals::brightnessModifier * scale;
			rt.fade = ((halfIntensityAmplitude * flickerIntensityVal) + (1.0f - halfIntensityAmplitude)) * baseFade;
		}
		else {

			flickerDelta *= 5; 

			// ---- Pulse (matches vanilla kPulse / kPulse) ----
			auto constAttenuation = std::fmod(pointLight->constAttenuation + flickerDelta, RE::NI_TWO_PI);
			pointLight->constAttenuation = constAttenuation;

			const auto constAttenCosine = NiCosQ(constAttenuation);
			const auto constAttenSine = NiSinQ(constAttenuation);

			const float halfIntensityAmplitude = config.flickerIntensity * 0.5f;
			const float baseFade = config.startingFade * globals::brightnessModifier * scale;

			rt.fade = ((constAttenCosine * halfIntensityAmplitude) + (1.0f - halfIntensityAmplitude)) * baseFade;

			pos.x = basePos.x + config.flickerAmplitude * constAttenCosine;
			pos.y = basePos.y + config.flickerAmplitude * constAttenSine;
			pos.z = basePos.z + config.flickerAmplitude * (constAttenSine * constAttenCosine);
		}

		RE::NiUpdateData updateData{};
		updateData.time = 0.0f;
		updateData.flags = RE::NiUpdateData::Flag::kDirty;
		a_root->UpdateTransformAndBounds(updateData);
	}

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
		toRemove.clear();
	}
}

//TODO:: Redo whatever the fuck this is 
inline void handlePendingMerges() {

	constexpr int maxPerFrame = 2;
	int processedThisFrame = 0;

    std::lock_guard lock(LightManager::pendingMergesMutex);
    if (LightManager::pendingMerges.empty()) return;

    auto now = std::chrono::steady_clock::now();

    std::vector<RE::ObjectRefHandle> reprocessQueue{};

    LightManager::pendingMerges.erase(
        std::remove_if(LightManager::pendingMerges.begin(), LightManager::pendingMerges.end(),
            [&](LightManager::PendingMerge& entry) {

				// dont wanmt to hold up the hook
				if (processedThisFrame >= maxPerFrame)
					return false;

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.registeredAt).count();
                if (elapsed < 250) return false;

                auto refA = entry.refA.get();
                if (!refA) return true;

                std::vector<RE::ObjectRefHandle> validCandidates;

                for (const auto& handle : entry.candidateHandles) {
                    auto refB = handle.get();
                    if (!refB) continue;

                    // ray cast to ensure no walls in between
                    if (raycast::HasAnythingBetween(refA.get()->GetPosition(), refB.get()->GetPosition())) {
                        reprocessQueue.emplace_back(handle);
                        continue;
                    }
              
                    validCandidates.push_back(handle);
                }
				processedThisFrame++;
                LightManager::finalizeMerge(entry, validCandidates);
                return true;
            }),
        LightManager::pendingMerges.end());

    while (!reprocessQueue.empty() && processedThisFrame < maxPerFrame) {

        auto retryRefAHandle = reprocessQueue.front();
        reprocessQueue.erase(reprocessQueue.begin());

        auto retryRefA = retryRefAHandle.get();
        if (!retryRefA) continue;

        std::vector<RE::ObjectRefHandle> nextQueue;
        std::vector<RE::ObjectRefHandle> mergeGroup;

        for (const auto& handle : reprocessQueue) {

            auto otherRef = handle.get();
            if (!otherRef) continue;;
          
            if (raycast::HasAnythingBetween(retryRefA.get()->GetPosition(), otherRef.get()->GetPosition())) {
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

		const auto allowLightMerge = cfgs[0].shadowLight ? globals::enableShadowLightMerging : globals::enableLightMerging;
		
		const auto isMutliLightConfig = cfgs.size() > 1;
  
		if (!isMutliLightConfig &&
			allowLightMerge &&
			!LightData::HasRelightFlag(cfgs[0].flags, RELIGHT_FLAGS::kNoMerging)) {
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
			processedThisFrame++;
            LightManager::finalizeMerge(p, mergeGroup);
        }
     
        else {
            bool debugMarkerAttached = false; 

            for (const auto& cfg : cfgs) {
                LightManager::AttachLight(cfg, rootNode, retryRefA.get(), cfg.menuName, retryRefA->GetFormID(), debugMarkerAttached);
            }

			// This ref can't act as a merge source bc of flag or global or is multi light, so attach its normal light(s).
		 // Keep the remaining refs queued so they can try to merge with each other.
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

template <class T>
static void DrawLightDebugSpheres(T& lights, RE::NiPoint3 playerPos, uint32_t configID)
{

	const int maxDistSq = globals::distanceForDrawDebugLines * globals::distanceForDrawDebugLines;

	auto* api = DebugAPI_IMPL::DebugAPI::GetSingleton();
	if (!api) return;

	for (auto& light : lights) {
		if (!light || !light->light) continue;

		// only draw the selected light
		if (light->light->unk138 != configID) continue;

		// Distance check
		auto diff = light->light->world.translate - playerPos;
		float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		if (distSq > maxDistSq) continue;

		RE::NiPoint3 worldPos = light->light->world.translate;
		float radius = light->light->radius.x;

		// 3 circles for a full sphere wireframe
		api->DrawCircle(worldPos, radius, RE::NiPoint3(0.0f, 0.0f, 0.0f), 1); // XY
		api->DrawCircle(worldPos, radius, RE::NiPoint3(RE::NI_HALF_PI, 0.0f, 0.0f), 1); // XZ
		api->DrawCircle(worldPos, radius, RE::NiPoint3(0.0f, RE::NI_HALF_PI, 0.0f), 1); // YZ
	}
}




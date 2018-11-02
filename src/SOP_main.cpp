#if OPERATOR_TYPE_SOP == 1

#include "CPlusPlus_Common.h"
#include "SOP_CPlusPlusBase.h"

#include <assert.h>

#include <vector>
#include <random>

#include <linalg.h>

using vec3 = linalg::vec<float, 3>;

////

class PROJECT_NAME : public SOP_CPlusPlusBase
{
public:

	std::uniform_real_distribution<float> rand01{ 0.0f, 1.0f };

	PROJECT_NAME(const OP_NodeInfo* info)
	{}

	virtual ~PROJECT_NAME()
	{}

	void setupParameters(OP_ParameterManager* manager) override
	{
		{
			OP_NumericParameter	np;

			np.name = "Maxpoints";
			np.label = "Max Points";
			np.defaultValues[0] = 100.0;
			np.minSliders[0] = 0.0;
			np.maxSliders[0] = 10000.0;

			OP_ParAppendResult res = manager->appendInt(np);
			assert(res == OP_ParAppendResult::Success);
		}

		{
			OP_NumericParameter	np;

			np.name = "Globalseed";
			np.label = "Global Seed";
			np.defaultValues[0] = 0.0;
			np.minSliders[0] = 0.0;
			np.maxSliders[0] = 10000.0;

			OP_ParAppendResult res = manager->appendInt(np);
			assert(res == OP_ParAppendResult::Success);
		}
	}

	void getGeneralInfo(SOP_GeneralInfo* ginfo) override
	{
		ginfo->cookEveryFrameIfAsked = false;
		ginfo->directToGPU = false;
	}

	void execute(SOP_Output* output, OP_Inputs* inputs, void* reserved) override
	{
		if (inputs->getNumInputs() == 0)
			return;

		const OP_SOPInput* input = inputs->getInputSOP(0);

		if (input == nullptr)
			return;

		size_t numpt = input->getNumPoints();
		size_t numprims = input->getNumPrimitives();

		if (numprims == 0)
			return;

		const vec3* positions = (const vec3*)input->getPointPositions();

		float area_sum = 0;
		std::vector<float> areas;

		size_t SEED = inputs->getParInt("Globalseed");
		std::mt19937 mt(SEED);

		// pass1
		for (size_t primnum = 0; primnum < numprims; primnum++)
		{
			const auto& prim_info = input->getPrimitive(primnum);

			if (prim_info.numVertices != 3)
				return;

			const int i0 = prim_info.pointIndices[0];
			const int i1 = prim_info.pointIndices[1];
			const int i2 = prim_info.pointIndices[2];

			const vec3& p0 = positions[i0];
			const vec3& p1 = positions[i1];
			const vec3& p2 = positions[i2];

			float area = linalg::length(linalg::cross((p0 - p1), (p0 - p2))) / 2;

			area_sum += area;

			areas.push_back(area_sum);
		}

		size_t num_scatter_points = inputs->getParInt("Maxpoints");

		// pass2
		for (int i = 0; i < num_scatter_points; i++)
		{
			float r = area_sum * rand01(mt);

			auto it = std::upper_bound(areas.begin(), areas.end(), r);
			size_t primnum = std::distance(areas.begin(), it);
			primnum = std::min(primnum, numprims - 1);

			const auto& prim_info = input->getPrimitive(primnum);

			const int i0 = prim_info.pointIndices[0];
			const int i1 = prim_info.pointIndices[1];
			const int i2 = prim_info.pointIndices[2];

			const vec3& p0 = positions[i0];
			const vec3& p1 = positions[i1];
			const vec3& p2 = positions[i2];

			vec3 p = random_point(p0, p1, p2, mt);

			output->addPoint(p.x, p.y, p.z);
		}
	}

	inline vec3 random_point(const vec3& a, const vec3& b, const vec3& c, std::mt19937& mt)
	{
		const float u = rand01(mt);
		const float v = rand01(mt);

		const float m1 = std::min(u, v);
		const float m2 = 1.0f - std::max(u, v);
		const float m3 = std::max(u, v) - std::min(u, v);

		const float x = m1 * a.x + m2 * b.x + m3 * c.x;
		const float y = m1 * a.y + m2 * b.y + m3 * c.y;
		const float z = m1 * a.z + m2 * b.z + m3 * c.z;

		return vec3(x, y, z);
	}

	void executeVBO(SOP_VBOOutput* output, OP_Inputs*, void* reserved) override
	{}
};

////

extern "C"
{

	DLLEXPORT int32_t GetSOPAPIVersion(void)
	{
		return SOP_CPLUSPLUS_API_VERSION;
	}

	DLLEXPORT SOP_CPlusPlusBase* CreateSOPInstance(const OP_NodeInfo* info)
	{
		return new PROJECT_NAME(info);
	}

	DLLEXPORT void DestroySOPInstance(SOP_CPlusPlusBase* instance)
	{
		delete (PROJECT_NAME*)instance;
	}
};

#endif
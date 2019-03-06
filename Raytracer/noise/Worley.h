#pragma once

#include "Noise.h"
#include "../Common.h"
#include <memory>
namespace re
{
	class Worley : public Noise
	{
	public:
		Worley(real domainSize, int divisions);
		virtual real SampleNormalized(const Vector3& point) override;
	private:

		int GetArrayIndex(real v) const;
		int WrapIndex(int index) const;
		Vector3 GetPointAt(int x, int y, int z);

		std::vector<Vector3> m_Points;
		int m_Divisions;
		real m_Step;
	};
}
#pragma once
#include "Noise.h"
#include <memory>

namespace re
{
	class Perlin;

	class Marble : public Noise
	{
	public:
		Marble(real domainSize, real frequency = 50, real turbolence = 4);
		virtual real SampleNormalized(const Vector3& point) override;
	private:
		std::shared_ptr<Perlin> m_Perlin;
		real m_Frequency, m_Turbolence;
	};
}
#pragma once

#include "Noise.h"

namespace re
{
	class CheckerBoard : public Noise
	{
	public:
		CheckerBoard(real domainSize) : Noise::Noise(domainSize) {};
		virtual real SampleNormalized(const Vector3& point) override;
	};
}
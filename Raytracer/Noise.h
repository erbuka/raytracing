#pragma once
#include "Common.h"

namespace re
{
	class Noise
	{
	public:
		Noise(real domainSize) : m_DomainSize(domainSize) {}
		real GetDomainSize() { return m_DomainSize; }
		real Sample(const Vector3& point);
		virtual real SampleNormalized(const Vector3& point) = 0;
	private:
		real m_DomainSize;
	};
}
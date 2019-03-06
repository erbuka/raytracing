#include "Noise.h"

re::real re::Noise::Sample(const Vector3 & point)
{
	static auto normalize = [](real x, real domainSize) -> real
	{
		x = std::fmodf(x, domainSize) / domainSize;
		return x < 0 ? 1.0 + x : x;
	};

	Vector3 normalizedPoint = Vector3(
		normalize(point.X, m_DomainSize),
		normalize(point.Y, m_DomainSize),
		normalize(point.Z, m_DomainSize)
		);

	return SampleNormalized(normalizedPoint);

}

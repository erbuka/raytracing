#include "Marble.h"
#include "Perlin.h"
re::Marble::Marble(real domainSize, real frequency, real turbolence) : 
	Noise(domainSize), 
	m_Frequency(frequency),
	m_Turbolence(turbolence)
{
	m_Perlin = std::shared_ptr<Perlin>(new Perlin(domainSize));
}

re::real re::Marble::SampleNormalized(const Vector3 & point)
{
	auto f = m_Perlin->SampleNormalized(point) * m_Turbolence;
	return std::fabs(std::sin(f * m_Frequency * 3.141592f));
}

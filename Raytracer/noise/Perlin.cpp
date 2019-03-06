#include "Perlin.h"
#include <random>
#include <cassert>

re::Perlin::Perlin(real domainSize) : 
	Noise::Noise(domainSize)
{
	m_Seed = new real[SeedSize];
	NewSeed();
}

re::Perlin::~Perlin()
{
	delete[] m_Seed;
}

void re::Perlin::NewSeed()
{
	for (int i = 0; i < SeedSize; i++)
	{
		m_Seed[i] = (rand() % SeedSize);
	}
}

re::real re::Perlin::SampleNormalized(const Vector3 & position)
{
	real amplitude = 1.0f;
	real amplitudeAcc = 0.0f;
	real result = 0.0f;
	unsigned int frequency = SeedSize;

	// Remap the vector to seed space
	Vector3 rpos = position * (real)SeedSize;

	for (unsigned int i = 0; i < m_Octaves; i++)
	{
		result += amplitude * SampleAtFrequency(rpos, frequency);
		frequency = frequency >> 1;
		amplitudeAcc += amplitude;
		amplitude *= m_Persistance;
	}

	result /= amplitudeAcc;

	return result;

}

re::real re::Perlin::GetSeed(size_t x, size_t y, size_t z) const
{
	size_t ix = m_Seed[x % SeedSize];
	size_t iy = m_Seed[(ix + y) % SeedSize];
	size_t iz = m_Seed[(iy + z) % SeedSize];
	return (real)iz / SeedSize;
}

re::real re::Perlin::SampleAtFrequency(const Vector3 & position, unsigned int frequency)
{
	static auto fade = [](real t) -> real
	{
		return t * t * t * (t * (t * 6 - 15) + 10);
	};

	Vector3 min = (position / frequency).Floor() * frequency;
	Vector3 max = min + Vector3(frequency, frequency, frequency);

	real a = GetSeed(min.X, min.Y, min.Z);
	real b = GetSeed(max.X, min.Y, min.Z);
	real c = GetSeed(min.X, max.Y, min.Z);
	real d = GetSeed(max.X, max.Y, min.Z);

	real e = GetSeed(min.X, min.Y, max.Z);
	real f = GetSeed(max.X, min.Y, max.Z);
	real g = GetSeed(min.X, max.Y, max.Z);
	real h = GetSeed(max.X, max.Y, max.Z);

	real u = fade((position.X - min.X) / frequency);
	real v = fade((position.Y - min.Y) / frequency);
	real w = fade((position.Z - min.Z) / frequency);

	// Trilinear interpolation
	return Lerp(Lerp(Lerp(a, b, u), Lerp(c, d, u), v), Lerp(Lerp(e, f, u), Lerp(g, h, u), v), w);
}

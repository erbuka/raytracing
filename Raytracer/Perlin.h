#pragma once
#include "Noise.h"

namespace re
{

	class Perlin : public Noise
	{
	public:
		Perlin(real domainSize);
		~Perlin();
		void NewSeed();
		void SetPersistance(real persistance) { m_Persistance = persistance; }
		void SetOctaves(unsigned int octaves) { m_Octaves = octaves; }

		virtual real SampleNormalized(const Vector3& position) override;

	private:

		static constexpr size_t SeedSize = 256;
		static constexpr size_t MaxOctaves = 8;

		real m_Persistance = 0.5f;
		unsigned int m_Octaves = MaxOctaves;

		real * m_Seed;

		real GetSeed(size_t x, size_t y, size_t z) const;
		real SampleAtFrequency(const Vector3& position, unsigned int fequency);
	};
}
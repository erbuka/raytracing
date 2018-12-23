#include "Worley.h"

re::Worley::Worley(real domainSize, int divisions) :  
	Noise(domainSize),
	m_Divisions(divisions)
{
	m_Step = 1.0 / divisions;

	auto randomPoint = [&](real x, real y, real z) -> Vector3 {
		return{
			Random(x * m_Step, (x + 1) * m_Step),
			Random(y * m_Step, (y + 1) * m_Step),
			Random(z * m_Step, (z + 1) * m_Step)
		};
	};

	for (int z = 0; z < divisions; z++)
	{
		for (int y = 0; y < divisions; y++)
		{
			for (int x = 0; x < divisions; x++)
			{
				m_Points.push_back(randomPoint(x, y, z));
			}
		}
	}
}

re::real re::Worley::SampleNormalized(const Vector3 & point)
{
	real maxDistance = m_Step * std::sqrt(3.0f);

	int x = GetArrayIndex(point.X);
	int y = GetArrayIndex(point.Y);
	int z = GetArrayIndex(point.Z);

	real distance = std::numeric_limits<float>::max();
		
	for (int dx = -1; dx <= 1; dx++)
	{
		for (int dy = -1; dy <= 1; dy++)
		{
			for (int dz = -1; dz <= 1; dz++)
			{
				auto& p = GetPointAt(x + dx, y + dy, z + dz);
				real d = (p - point).SquaredLength();
				if (d < distance)
				{
					distance = d;
				}
			}
		}
	}

	return distance / maxDistance * 10.0f;

}

int re::Worley::GetArrayIndex(real v) const
{
	return (int)(v * m_Divisions);
}

int re::Worley::WrapIndex(int index) const
{
	while (index < 0) index += m_Divisions;
	return index % m_Divisions;
}

re::Vector3 re::Worley::GetPointAt(int x, int y, int z)
{
	static auto offset = [&](int v, int divisions) -> int {
		if (v < 0) return -1;
		else if (v >= divisions) return 1;
		else return 0;
	};

	int offsetX = offset(x, m_Divisions),
		offsetY = offset(y, m_Divisions),
		offsetZ = offset(z, m_Divisions);

	return m_Points[WrapIndex(z) * m_Divisions * m_Divisions + WrapIndex(y) * m_Divisions + WrapIndex(x)] +
		Vector3(offsetX, offsetY, offsetZ);
}

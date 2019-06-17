#pragma once
#include "Common.h"
#include "noise/CheckerBoard.h"
#include <memory>


namespace re
{

	class Material
	{
	public:
		virtual Color GetAbsorbedColor(const Vector3& point) = 0;
		virtual real GetAbsorptance(const Vector3& point) = 0;
		virtual real GetReflectance(const Vector3& point);
	};

	class UniformMaterial : public Material
	{
	public:
		static const UniformMaterial OpaqueWhite;
		static const UniformMaterial OpaqueBlack;

		UniformMaterial(Color absorbedColor, real absorptance) :
			m_AbsorbedColor(absorbedColor), m_Absorptance(absorptance) {}

		virtual Color GetAbsorbedColor(const Vector3& point) override { return m_AbsorbedColor; }
		virtual real GetAbsorptance(const Vector3& point) override { return m_Absorptance; }
	private:
		Color m_AbsorbedColor;
		real m_Absorptance;
	};


	class InterpolatedMaterial : public Material
	{
	public:
		InterpolatedMaterial(std::shared_ptr<Noise> noise, std::shared_ptr<Material> material0, std::shared_ptr<Material> material1);

		virtual Color GetAbsorbedColor(const Vector3& point) override;
		virtual real GetAbsorptance(const Vector3& point) override;

	private:
		std::shared_ptr<Material> m_Material0, m_Material1;
		std::shared_ptr<Noise> m_Noise;

	};

}
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
		bool m_Shiny;
	};


	class ChekerBoardMaterial : public Material 
	{
	public:
		ChekerBoardMaterial(Color color0, Color color1, real size, real absorptance0 = 1.0f, real absorptance1 = 1.0f);

		virtual Color GetAbsorbedColor(const Vector3& point) override;
		virtual real GetAbsorptance(const Vector3& point) override;
	
	private:
		std::unique_ptr<CheckerBoard> m_CheckedBoard;
		Color m_Color0, m_Color1;
		real m_Absorptance0, m_Absorptance1;
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
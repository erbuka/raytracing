#include "Material.h"

const re::UniformMaterial re::UniformMaterial::OpaqueWhite = 
	re::UniformMaterial(re::Color::White, 1.0);

const re::UniformMaterial re::UniformMaterial::OpaqueBlack =
	re::UniformMaterial(re::Color::Black, 1.0);

re::real re::Material::GetReflectance(const Vector3 & point)
{
	return (real)(1.0 - GetAbsorptance(point));
}

re::InterpolatedMaterial::InterpolatedMaterial(std::shared_ptr<Noise> noise, std::shared_ptr<Material> material0, std::shared_ptr<Material> material1) :
	m_Noise(noise),
	m_Material0(material0),
	m_Material1(material1)
{
}

re::Color re::InterpolatedMaterial::GetAbsorbedColor(const Vector3 & point)
{
	return Lerp(m_Material0->GetAbsorbedColor(point), m_Material1->GetAbsorbedColor(point), m_Noise->Sample(point));
}

re::real re::InterpolatedMaterial::GetAbsorptance(const Vector3 & point)
{
	return Lerp(m_Material0->GetAbsorptance(point), m_Material1->GetAbsorptance(point), m_Noise->Sample(point));
}

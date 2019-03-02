#pragma once
#include "Common.h"
#include "Scene.h"


#define RE_DEBUG

namespace re
{
	class Scene;
	class Shape;
	class Material;

	class AbstractRaycaster : public Renderer
	{
	public:

		AbstractRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 6.0f);
		~AbstractRaycaster();

		virtual void Render(Scene * scene, std::promise<RenderStatus> p) override;
		virtual void Interrupt() override;
		virtual RenderStatus GetStatus() override;

		bool SuperSampling;

	protected:

		virtual Color Raycast(Scene * m_Scene, const Ray& ray) = 0;

		unsigned int * m_Pixels;
		unsigned int m_NumThreads = 4;

	private:
		Ray CreateScreenRay(Scene * m_Scene, real x, real y);
		void DoRaytraceThread(Scene * m_Scene, unsigned int minX, unsigned int maxX);

		RenderStatus m_Status = { true, true, 0, m_Pixels };

	};

	class Raytracer : public AbstractRaycaster
	{
	public:

		unsigned int MaxRecursion = 3;

		Raytracer(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 6.0f) :
			AbstractRaycaster(viewWidth, viewHeight, fovY) {}

	protected:
		virtual Color Raycast(Scene * m_Scene, const Ray& ray) override;

	private:
		Color RecursiveRaytrace(Scene * m_Scene, const Ray& ray, int recursion = 0);
		bool CastShadowRay(Scene * m_Scene, const Ray& shadowRay);

	};

	class DebugRaycaster : public AbstractRaycaster
	{
	public:

		enum class Modes { Normal };

		Modes Mode = Modes::Normal;

		DebugRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 6.0f) :
			AbstractRaycaster(viewWidth, viewHeight, fovY) {}
	protected:
		virtual Color Raycast(Scene * m_Scene, const Ray& ray) override;
	
	};
}
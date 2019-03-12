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

		enum class AAMode : int
		{
			None = 0,
			SSAA = 1
		};

		AbstractRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 6.0f);
		~AbstractRaycaster();

		virtual void Render(Scene * scene, std::promise<RenderStatus> p) override;
		virtual void Interrupt() override;
		virtual RenderStatus GetStatus() override;

		AAMode Antialiasing = AAMode::None;

		unsigned int NumThreads = 4;

	protected:

		virtual Color Raycast(Scene * scene, const Ray& ray) = 0;

		Color *m_ColorBuffer0;

		unsigned int *m_Pixels;

	private:

		unsigned int m_CurrentRenderSlice;
		std::mutex m_RenderMutex;
		RenderStatus m_Status = { true, true, 0, m_Pixels };

		Ray CreateScreenRay(Scene * m_Scene, real x, real y);
		void DoRaytraceThread(Scene * m_Scene, unsigned int minX, unsigned int maxX);

		void ColorsToPixels(Color *cb, unsigned int *pixels);

		bool NextRenderSlice(unsigned int& sliceX);


	};

	class Raytracer : public AbstractRaycaster
	{
	public:

		unsigned int MaxRecursion = 3;

		Raytracer(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 4.0f) :
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

		enum class Modes { Normal, Color };

		Modes Mode = Modes::Normal;

		DebugRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 6.0f) :
			AbstractRaycaster(viewWidth, viewHeight, fovY) {}
	protected:
		virtual Color Raycast(Scene * m_Scene, const Ray& ray) override;
	
	};
}
#pragma once
#include "Common.h"
#include "Scene.h"


#define RE_DEBUG

namespace re
{
	class Scene;
	class Shape;
	class Material;

	/// A generic 3D renderer. By default the rendering process is asynchronous and
	/// should be performed with multiple threads
	class Renderer
	{
	public:

		/// Structure that represents the renderer status
		struct RenderStatus {
			bool Finished;
			bool Interruped;
			float Percent;
			unsigned int * Pixels;
		};

		Renderer(size_t viewWidth, size_t viewHeight, real fovY = PI / 4) :
			m_ViewWidth(viewWidth), m_ViewHeight(viewHeight), m_FoVY(fovY) {}

		virtual ~Renderer() { }
		
		/// Renders the scene in synchronous mode and returns a pixel buffer containing the renderered scene
		virtual unsigned int * RenderSync(Scene * scene);

		/// Renders the scene
		virtual void Render(Scene * scene, std::promise<RenderStatus> promise) = 0;

		/// Returns the current status
		virtual RenderStatus GetStatus() = 0;

		/// Interrupts the rendering process
		virtual void Interrupt() = 0;

		virtual size_t GetViewWidth() { return m_ViewWidth; }
		virtual size_t GetViewHeight() { return m_ViewHeight; }

	protected:
		size_t m_ViewWidth, m_ViewHeight;
		real m_FoVY;
	};

	/// A multitrheaded raycaster that renders the scene casting rays from the camera to the viewport.
	/// Subclasses must implement the Raycast function
	class AbstractRaycaster : public Renderer
	{
	public:

		enum class AAMode : int
		{
			None = 0,
			SSAA = 1 
		};

		AbstractRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 4.0f);
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
	/// Raytracer implementation
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

	/// A raycaster for debugging and fast rendering the scene
	class DebugRaycaster : public AbstractRaycaster
	{
	public:

		enum class Modes : unsigned int { 
			Normal = 0, 
			Color = 1
		};

		Modes Mode = Modes::Normal;

		DebugRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY = PI / 4.0f) :
			AbstractRaycaster(viewWidth, viewHeight, fovY) {}
	protected:
		virtual Color Raycast(Scene * m_Scene, const Ray& ray) override;
	
	};
}
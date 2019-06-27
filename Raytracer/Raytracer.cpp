#include "Raytracer.h"
#include "Scene.h"
#include <cmath>
#include <thread>
#include <vector>
#include <cassert>
#include <limits>
#include <chrono>
#include <future>
#include <array>

unsigned int * re::Renderer::RenderSync(Scene * scene)
{
	std::promise<RenderStatus> p;
	auto f = p.get_future();
	Render(scene, std::move(p));
	return f.get().Pixels;
}

re::AbstractRaycaster::AbstractRaycaster(unsigned int viewWidth, unsigned int viewHeight, real fovY) :
	Renderer::Renderer(viewWidth, viewHeight, fovY)
{
	m_ColorBuffer0 = new Color[m_ViewWidth * m_ViewHeight];
	m_Pixels = new unsigned int[m_ViewWidth * m_ViewHeight];
}

re::AbstractRaycaster::~AbstractRaycaster()
{
	delete[] m_ColorBuffer0;
	delete[] m_Pixels;
}


void re::AbstractRaycaster::Render(Scene * scene, std::promise<RenderStatus> p)
{
	m_Status = { false, false, 0, m_Pixels };

	// We create a main thread that runs the rendering process
	auto threadFunc = [scene, this](std::promise<RenderStatus> p) {
		
		// Start by compiling there scene (fast operation)
		scene->Compile();

		// We subdivide the viewport in vertical scanlines (1 pixel wide)
		// "NumThreads" threads are created. Eachone will process a scanline
		// at time and the query for a new scanline. A mutex is used to access
		// the available scanlines (see function DoRayTracethread)

		std::vector<std::function<void()>> functions;
		m_CurrentRenderScanline = 0;

		for (unsigned int i = 0; i < NumThreads; i++) 
		{
			functions.push_back(std::bind(
				&AbstractRaycaster::DoRaytraceThread,
				this,
				scene
			));
		}
		
		std::vector<std::future<void>> futures;

		for (auto func : functions)
		{
			futures.push_back(std::async(std::launch::async, func));
		}

		// Wait for all threads to complete their execution
		for (auto &f : futures)
		{
			f.wait();
		}

		ColorsToPixels(m_ColorBuffer0, m_Pixels);
		
		m_Status.Pixels = m_Pixels;
		m_Status.Finished = true;

		p.set_value(m_Status);


	};

	std::thread th(threadFunc, std::move(p));
	
	// Thread must be detached otherwise is will get destroyed at the end of the scope
	th.detach();

}

void re::AbstractRaycaster::Interrupt()
{
m_Status.Interruped = true;
}

re::Renderer::RenderStatus re::AbstractRaycaster::GetStatus()
{
	return m_Status;
}


re::Ray re::AbstractRaycaster::CreateScreenRay(Scene * scene, real x, real y)
{
	// GetSeed axis base
	Vector3 forward = scene->Camera.Direction.Normalized();
	Vector3 right = Cross(forward, Vector3::Up).Normalized();
	Vector3 up = Cross(right, forward).Normalized();

	real h2 = std::atan(m_FoVY / 2);
	real w2 = h2 * (real)m_ViewWidth / (real)m_ViewHeight;

	real xFactor = ((real)x / m_ViewWidth) * 2.0f - 1.0f;
	real yFactor = 1.0f - ((real)y / m_ViewHeight) * 2.0f;

	Ray result;

	result.Origin = scene->Camera.Position;
	result.Direction = (forward + right * xFactor * w2 + up * yFactor * h2).Normalized();

	return result;
}

void re::AbstractRaycaster::DoRaytraceThread(Scene * m_Scene)
{
	unsigned int x;
	// The "NextRenderScanline" function gives us the scanline we have to render in this thread. Uses a mutex since
	// the threads are racing for scanlines
	while (NextRenderScanline(x))
	{
		for (int y = 0; y < m_ViewHeight; y++)
		{
			if (Antialiasing == AAMode::SSAA) {
				Color result(Color::Black);
				for (int dx = -1; dx <= 1; dx++)
				{
					for (int dy = -1; dy <= 1; dy++)
					{
						Ray ray = CreateScreenRay(m_Scene, x + dx * .5f, y + dy * .5f);
						result += (Raycast(m_Scene, ray) * (1.0f / 9.0f));
					}
				}
				m_ColorBuffer0[y * m_ViewWidth + x] = result;
			}
			else
			{
				Ray ray = CreateScreenRay(m_Scene, x, y);
				m_ColorBuffer0[y * m_ViewWidth + x] = Raycast(m_Scene, ray);
			}
			// Update the current status
			m_Status.Percent += 1.0f / (m_ViewWidth * m_ViewHeight);

			// If the process has been interrupted, just return and and this thread
			if (m_Status.Interruped)
				return;

		}
	}
}

void re::AbstractRaycaster::ColorsToPixels(Color * cb, unsigned int * pixels)
{
	for (unsigned int x = 0; x < m_ViewWidth; x++)
	{
		for (unsigned int y = 0; y < m_ViewHeight; y++)
		{
			unsigned int idx = y * m_ViewWidth + x;
			pixels[idx] = m_ColorBuffer0[idx].GetHexValue();
		}
	}
}

bool re::AbstractRaycaster::NextRenderScanline(unsigned int& sliceX)
{
	std::lock_guard<std::mutex> lock(m_RenderMutex);

	if (m_CurrentRenderScanline == m_ViewWidth)
	{
		return false;
	}
	else
	{
		sliceX = m_CurrentRenderScanline++;
		return true;
	}
}

re::Color re::Raytracer::RecursiveRaytrace(Scene * scene, const Ray & ray, int recursion)
{
	// Find the closest intersection if any
	Scene::RaycastResult raycastResult = scene->CastRay(ray);

	Color directLighting = Color::Black;
	Color indirectLighting = Color::Black;

	if (raycastResult.Hit)
	{

		Vector3 worldPoint = raycastResult.Point;
		Vector3 localPoint = raycastResult.LocalPoint;
		auto shape = raycastResult.Node->GetComponentOfType<Shape>();
		Vector3 normal = raycastResult.Normal;
		Material * material = shape->Material;
		// Handle direct lighting

		real absorptance = material->GetAbsorptance(localPoint);
		real reflectance = material->GetReflectance(localPoint);

		if (absorptance > 0) 
		{

			for (auto light : scene->Lights)
			{
				if (!light->Enabled)
				{
					continue;
				}

				// Cast shadow ray
				bool shadowRayHit = false;
				Ray shadowRay;

				switch (light->Type)
				{
				case LightType::Directional:
					shadowRay.Origin = worldPoint;
					shadowRay.Direction = light->Direction;
					shadowRayHit = CastShadowRay(scene, shadowRay);
					break;
				case LightType::Point:
					shadowRay.Origin = worldPoint;
					shadowRay.Direction = (light->Position - worldPoint).Normalized();
					shadowRayHit = CastShadowRay(scene, shadowRay);
					break;
				case LightType::Ambient:
					// Ambient light always passes trough
					shadowRayHit = false;
					break;
				default:
					break;
				}

				// Handle lighting if hit

				if (!shadowRayHit) {
					real diffuseFactor = 0.0f;
					real distance, attenuation;
					Vector3 direction, lightVector;

					switch (light->Type)
					{
					case LightType::Directional:
						diffuseFactor = std::fmaxf(0.0f, normal ^ light->Direction);
						break;
					case LightType::Point:
						lightVector = light->Position - worldPoint;
						direction = lightVector.Normalized();
						distance = lightVector.Length();
						attenuation = 1.0f - ((distance * distance) / (light->Attenuation * light->Attenuation));
						diffuseFactor = std::fmaxf(0.0f, direction ^ normal) * attenuation;
						break;
					case LightType::Ambient:
						// Ambient light lights everything with the same factor
						diffuseFactor = 1.0f;
						break;
					default:
						break;
					}
					
					directLighting +=
						light->Color * material->GetAbsorbedColor(localPoint) * diffuseFactor * absorptance;
				}
			}

		}

		if (reflectance > 0 && recursion < MaxRecursion)
		{
			Ray reflectedRay;

			reflectedRay.Origin = worldPoint;
			reflectedRay.Direction = ray.Direction.Reflect(normal);

			indirectLighting = RecursiveRaytrace(scene, reflectedRay, recursion + 1) * reflectance;
		}

		return directLighting + indirectLighting;

	} 
	else
	{
		// If we dont hit anything, return the background
		if (scene->Background != nullptr)
		{
			return scene->Background->GetColor(ray.Direction);
		}
		else
		{
			return Color::Black;
		}
	}


}

bool re::Raytracer::CastShadowRay(Scene * m_Scene, const Ray & shadowRay)
{
	constexpr real epsilon = std::numeric_limits<real>::epsilon();
	Scene::RaycastResult result = m_Scene->CastRay(shadowRay);
	return result.Hit;
}

re::Color re::Raytracer::Raycast(Scene * scene, const Ray & ray)
{
	return RecursiveRaytrace(scene, ray, 0);
}

re::Color re::DebugRaycaster::Raycast(Scene * scene, const Ray & ray)
{
	Scene::RaycastResult result = scene->CastRay(ray);


	switch (Mode)
	{
	case Modes::Normal:
		if (result.Hit)
		{
			return Color(
				(result.Normal.X + 1) / 2,
				(result.Normal.Y + 1) / 2,
				1 - (result.Normal.Z + 1) / 2
			);
		}
		break;
	case Modes::Color:
		if (result.Hit)
		{
			return result.Node->GetComponentOfType<Shape>()->Material->GetAbsorbedColor(result.LocalPoint);
		}
		else
		{
			if (scene->Background != nullptr)
			{
				return scene->Background->GetColor(ray.Direction);
			}
		}

	}

	return Color::Black;

}

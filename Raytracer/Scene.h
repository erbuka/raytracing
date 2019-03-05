#pragma once
#include "Common.h"
#include "Material.h"
#include "Perlin.h"
#include <vector>
#include <future>

namespace re
{
	class Shape;
	class Scene;
	class SceneNode;

	enum class LightType { Directional, Ambient, Point };

	class Light
	{
	public:

		LightType Type = LightType::Directional;
		Vector3 Direction = Vector3::Up;
		Vector3 Position = Vector3::Zero;
		Color Color = Color::White;
		real Attenuation = 5.0f;
		bool Enabled = true;

		Light() {}

	};

	class Renderer
	{
	public:

		struct RenderStatus {
			bool Finished;
			bool Interruped;
			float Percent;
			unsigned int * Pixels;
		};

		Renderer(size_t viewWidth, size_t viewHeight, real fovY = PI / 6) :
			m_ViewWidth(viewWidth), m_ViewHeight(viewHeight), m_FoVY(fovY) {}
		
		virtual ~Renderer() { }

		virtual unsigned int * RenderSync(Scene * scene);
		virtual void Render(Scene * scene, std::promise<RenderStatus> promise) = 0;
		virtual RenderStatus GetStatus() = 0;
		virtual void Interrupt() = 0;

		virtual size_t GetViewWidth() { return m_ViewWidth; }
		virtual size_t GetViewHeight() { return m_ViewHeight; }

	protected:
		size_t m_ViewWidth, m_ViewHeight;
		real m_FoVY;
	};

	class Background
	{
	public:
		virtual Color GetColor(const Vector3& direction) const = 0;
	};

	class SkyBox : public Background
	{
	public:
		SkyBox(Color color0, Color color1, Light* sun = nullptr);

		virtual Color GetColor(const Vector3& direction) const override;

		float SunFactor = 128.0f;

	private:
		std::shared_ptr<Perlin> m_Perlin;
		Color m_SkyColor0, m_SkyColor1;
		Light* m_Sun;
	};

	class Scene
	{
	public:

		struct RaycastResult
		{
			bool Hit = false;
			Vector3 Point = Vector3::Zero;
			Vector3 LocalPoint = Vector3::Zero;
			Vector3 Normal = Vector3::Zero;
			SceneNode * Node = nullptr;
		};

		Vector3 CameraPosition = Vector3::Zero;
		Vector3 LookDirection = Vector3::Forward;
		std::vector<Light*> Lights;
		Background * Background = nullptr;

		Scene();
		~Scene();

		void Compile();

		SceneNode * GetRoot() { return m_Root; }

		RaycastResult CastRay(const Ray &ray);

	private:

		RaycastResult CastRayRecursive(const Ray& ray, SceneNode * currentNode);

		SceneNode * m_Root;
	};

	class Component {
	public:
		Component(SceneNode* owner) { m_Owner = owner; }
		virtual void Compile() = 0;
	protected:
		SceneNode * m_Owner;
	};

	class Transform : public Component
	{
	public:

		Transform(SceneNode* owner) : Component(owner) {}

		Vector3 Position, Rotation, Scale = Vector3::One;
		virtual void Compile() override;

		void GetTransform(Matrix4& result) { result = m_Transform; }
		void GetInverseTransform(Matrix4& result) { result = m_InverseTransform; }

	private:
		Matrix4 m_Transform, m_InverseTransform;
	};

	class Shape : public Component
	{
	public:
		Shape(SceneNode * owner);
		
		re::Material * Material = (re::Material*)&(UniformMaterial::OpaqueWhite);

		unsigned int GetID() { return m_ID; }

		virtual void Compile() override {}
		virtual RayHitResult Intersect(const Ray& ray) = 0;
		virtual Vector3 GetNormal(const Vector3& point) const = 0;
	protected:
		unsigned int m_ID;
	};


	class Sphere : public Shape
	{
	public:

		Sphere(SceneNode * owner) : Shape(owner) {}

		virtual RayHitResult Intersect(const Ray& ray) override; // Ray is in local coordinates
		virtual Vector3 GetNormal(const Vector3& point) const override; // Point is in local coordinates
	};


	class Plane : public Shape
	{
	public:
		Vector3 Normal = Vector3::Up;

		Plane(SceneNode * owner) : Shape(owner) {}

		virtual RayHitResult Intersect(const Ray& ray) override;
		virtual Vector3 GetNormal(const Vector3& point) const override { return Normal; }

	};

	class Triangle : public Shape
	{
	public:

		Vector3 Vertices[3];

		Triangle(SceneNode * owner) : Shape(owner) {}

		virtual RayHitResult Intersect(const Ray& ray) override;
		virtual Vector3 GetNormal(const Vector3& point) const override;
	};

	class SceneNode
	{
	public:
		SceneNode();
		~SceneNode();

		virtual bool IsLeaf() const { return m_Children.size() == 0; }
		virtual std::vector<SceneNode*>& GetChildren() { return m_Children; };
		SceneNode * AddChild();
		SceneNode * GetParent() const { return m_Parent; }

		template<typename T> T* AddComponent()
		{
			T * c = dynamic_cast<T*>(new T(this));
			m_Components.push_back(c);
			return c;
		}

		template<typename T> T* GetComponentOfType() 
		{
			for (auto component : m_Components)
			{
				T* ccast = dynamic_cast<T*>(component);
				if (ccast != nullptr)
				{
					return ccast;
				}
			}
			return nullptr;
		}
		template<typename T> std::vector<T*> GetComponentsOfType() 
		{
			std::vector<T*> result;
			for (auto component : m_Components)
			{
				auto ccast = dynamic_cast<T*>(component);
				if (ccast != nullptr)
				{
					result.push_back(ccast);
				}
			}
			return result;
		}

		void Compile();

	protected:
		SceneNode * m_Parent = nullptr;
		std::vector<SceneNode*> m_Children;
		std::vector<Component*> m_Components;
	};

}
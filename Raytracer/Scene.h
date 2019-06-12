#pragma once
#include "Common.h"
#include "Material.h"
#include "noise/Perlin.h"
#include <vector>
#include <future>
#include <array>

namespace re
{
	class KDTreeTriangle;
	class Shape;
	class Scene;
	class SceneNode;

	/// Light types
	enum class LightType { Directional, Ambient, Point };
	enum class NormalModes { Face, Vertex };

	/// Class representing a light
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


	class Background
	{
	public:
		virtual Color GetColor(const Vector3& direction) const = 0;
	};

	class SkyBox : public Background
	{
	public:
		SkyBox(Color color0, Color color1, std::shared_ptr<Light> sun);

		virtual Color GetColor(const Vector3& direction) const override;

		float SunFactor = 128.0f;

	private:
		std::shared_ptr<Perlin> m_Perlin;
		Color m_SkyColor0, m_SkyColor1;
		
		std::shared_ptr<Light> m_Sun;
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

		struct
		{
			Vector3 Position = Vector3::Zero;
			Vector3 Direction = { 0,0,-1 };
			void LookAt(const Vector3& target) { Direction = (target - Position).Normalized(); }
		} Camera;

		std::vector<std::shared_ptr<Light>> Lights;
		std::shared_ptr<Background> Background = nullptr;

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
		void GetNormalTransform(Matrix4& result) { result = m_NormalTransform; }

	private:
		Matrix4 m_Transform, m_InverseTransform, m_NormalTransform;
	};

	class Shape : public Component
	{
	public:
		Shape(SceneNode * owner);
		
		re::Material * Material = (re::Material*)&(UniformMaterial::OpaqueWhite);

		unsigned int GetID() { return m_ID; }

		virtual void Compile() override {}
		virtual RayHitResult Intersect(const Ray& ray) = 0;
	protected:
		unsigned int m_ID;
	};


	class Sphere : public Shape
	{
	public:

		Sphere(SceneNode * owner) : Shape(owner) {}

		virtual RayHitResult Intersect(const Ray& ray) override; // Ray is in local coordinates
	};


	class Plane : public Shape
	{
	public:
		Vector3 Normal = Vector3::Up;

		Plane(SceneNode * owner) : Shape(owner) {}

		virtual RayHitResult Intersect(const Ray& ray) override;

	};

	struct Triangle
	{
	public:

		Triangle() {}

		std::array<Vector3, 3> Vertices, Normals;

		Vector3 FaceNormal;
		std::array<Vector3, 3> Edges;


		Vector3 Baricentric(const Vector3& point) const;

		void Update();
	private:
		real m_D00, m_D01, m_D11;
		real m_InvDen;

	};

	class Mesh : public Shape
	{
	public:

		NormalModes NormalMode = NormalModes::Face;

		Mesh(SceneNode * owner) : Shape(owner) { }
		virtual RayHitResult Intersect(const Ray& ray) override;
		
		Triangle& AddTriangle();

		void Compile() override;

		void Invalidate();

	private:

		bool m_Invalidated = true;

		RayHitResult IntersectTriangle(const Ray& ray, const Triangle& triangle) const;
		void IntersectInternal(const Ray& ray, KDTreeTriangle * node, RayHitResult & result, real & distance) const;


		KDTreeTriangle* m_KdTree = nullptr;

		std::vector<Triangle> m_Triangles;
		re::BoundingBox m_BoundingBox;
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
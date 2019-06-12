#include "Scene.h"
#include <cassert>

namespace re {
	class KDTreeTriangle
	{
	public:

		~KDTreeTriangle()
		{
			if (Left)
				delete Left;

			if (Right)
				delete Right;
		}

		KDTreeTriangle(const std::vector<Triangle>& triangles, const BoundingBox& bounds, int depth) :
			Bounds(bounds),
			Depth(depth)
		{


			// Stop condition
			if (triangles.size() <= 1 || Depth == 100)
			{
				for (auto &t : triangles)
					Triangles.push_back(t);
				return;
			}


			
			// Select the split axis (round-robin)
			unsigned int uAxis = depth % 3;


			struct
			{
				BoundingBox LeftBounds, RightBounds;
				std::vector<Triangle> LeftTris, RightTris;
			} result;

			// Take the median of all points as split point
			real median = 0;

			for (auto &t : triangles)
			{
				median += t.Vertices[0].Elements[uAxis];
				median += t.Vertices[1].Elements[uAxis];
				median += t.Vertices[2].Elements[uAxis];
			}

			median /= 3 * triangles.size();

			Bounds.Split(static_cast<Axis>(uAxis), median, result.LeftBounds, result.RightBounds);

			// Test every triangle in both left and right bounding boxes
			for (auto& t : triangles)
			{

				if (t.Vertices[0].Elements[uAxis] <= median || t.Vertices[1].Elements[uAxis] <= median || t.Vertices[2].Elements[uAxis] <= median)
				{
					result.LeftTris.push_back(t);
				}

				if (t.Vertices[0].Elements[uAxis] >= median || t.Vertices[1].Elements[uAxis] >= median || t.Vertices[2].Elements[uAxis] >= median)
				{
					result.RightTris.push_back(t);
				}
			}


			// Check that not too many triangles are in common (> 50%)
			// between the subdivisions 
			if (result.LeftTris.size() + result.RightTris.size() > 1.5 * triangles.size())
			{
				// If so, subdiving is not efficent anymore
				for (auto &t : triangles)
					Triangles.push_back(t);

			}
			else
			{
				// Subidivide

				if (result.LeftTris.size() > 0)
					Left = new KDTreeTriangle(result.LeftTris, result.LeftBounds, depth + 1);

				if (result.RightTris.size() > 0)
					Right = new KDTreeTriangle(result.RightTris, result.RightBounds, depth + 1);
			}

			
		}

		const unsigned int Depth;
		const BoundingBox Bounds;
		KDTreeTriangle *Left = nullptr, *Right = nullptr;
		std::vector<Triangle> Triangles;

	};
}

re::SkyBox::SkyBox(Color color0, Color color1, std::shared_ptr<Light> sun)
	: m_SkyColor0(color0), m_SkyColor1(color1), m_Sun(sun)
{
	m_Perlin = std::make_shared<Perlin>(1);
}

re::Color re::SkyBox::GetColor(const Vector3 & direction) const
{
	
	constexpr float thresold = 0.98f;
	constexpr float border = (1.0f - thresold) / 2.0f;
	constexpr float thresoldPlusBorder = thresold + border;

	// Get the current sky color
	//auto color = Lerp(m_SkyTop, m_SkyBottom, std::fmaxf(0, 1.0 - (direction ^ Vector3::Up)));
	
	real sample = m_Perlin->Sample((direction + Vector3::One) / 2);
	auto color = Mix(m_SkyColor0, m_SkyColor1, sample);

	if (m_Sun != nullptr && m_Sun->Type == LightType::Directional)
	{
		float f = std::fmaxf(0, direction ^ m_Sun->Direction);

		if (f < thresold)
		{
			return color;
		}
		else if (f < thresoldPlusBorder)
		{
			f = std::powf((f - thresold) / border, 2.0f);
			return Mix(color, m_Sun->Color, f);
		}
		else
		{
			return m_Sun->Color;
		}

	}
	else
	{
		return color;
	}
	



}

re::Scene::Scene()
{
	m_Root = new SceneNode();
}

re::Scene::~Scene()
{
	delete m_Root;
}

void re::Scene::Compile()
{
	m_Root->Compile();
}

re::Scene::RaycastResult re::Scene::CastRay(const Ray & ray)
{
	return CastRayRecursive(ray, m_Root);
}

re::Scene::RaycastResult re::Scene::CastRayRecursive(const Ray & ray, SceneNode * currentNode)
{
	Transform * transform = currentNode->GetComponentOfType<Transform>();
	Shape * shape = currentNode->GetComponentOfType<Shape>();

	assert(transform != nullptr);

	RaycastResult raycastResult;

	real hitDistance = std::numeric_limits<real>::max();

	if (shape != nullptr) {
		Ray transformedRay;
		Matrix4 itmat;

		transform->GetInverseTransform(itmat);

		transformedRay.Origin = itmat * Vector4(ray.Origin, 1);
		transformedRay.Direction = Vector3(itmat * Vector4(ray.Direction, 0)).Normalized();

		RayHitResult result = shape->Intersect(transformedRay);

		if (result.Hit)
		{
			Matrix4 tmat;
			transform->GetTransform(tmat);
			
			// Calculate point in world coordinates
			Vector3 worldPoint = tmat * Vector4(result.Point, 1);

			real distance = (ray.Origin - worldPoint).SquaredLength();

			if (distance >= std::numeric_limits<real>::epsilon())
			{
				Matrix4 tnorm;
				transform->GetNormalTransform(tnorm);

				raycastResult.Hit = true;
				raycastResult.Point = worldPoint;
				raycastResult.LocalPoint = result.Point;
				raycastResult.Normal = Vector3(tnorm * Vector4(result.Normal, 0)).Normalized();
				raycastResult.Node = currentNode;
				hitDistance = distance;
			}
		}

	}


	for (auto child : currentNode->GetChildren())
	{
		RaycastResult childResult = CastRayRecursive(ray, child);

		if (childResult.Hit)
		{
			real distance = (ray.Origin - childResult.Point).SquaredLength();

			if (distance < hitDistance)
			{
				raycastResult = childResult;
				hitDistance = distance;
			}

		}
		
	}

	return raycastResult;

}

void re::Transform::Compile()
{
	SceneNode * parent = m_Owner->GetParent();

	Matrix4 prev = Matrix4::Identity, prevInverse = Matrix4::Identity;

	if (parent != nullptr)
	{
		auto parentTransform = parent->GetComponentOfType<Transform>();
		parentTransform->GetTransform(prev);
		parentTransform->GetInverseTransform(prevInverse);
	}


	auto t = Matrix4::GetTranslation(Position);
	auto r = Matrix4::GetRotation(Rotation);
	auto s = Matrix4::GetScale(Scale);

	auto it = Matrix4::GetTranslation(Vector3::Zero - Position);
	auto ir = r.Transpose();
	auto is = Matrix4::GetScale(Vector3::One / Scale);

	m_Transform = prev * t * r * s;
	m_InverseTransform = is * ir * it * prevInverse;
	m_NormalTransform = m_InverseTransform.Transpose();
}

re::SceneNode::SceneNode()
{
	AddComponent<re::Transform>();
}

re::SceneNode::~SceneNode()
{
	for (auto component : m_Components)
	{
		delete component;
	}

	for (auto child : m_Children)
	{
		delete child;
	}

}

re::SceneNode * re::SceneNode::AddChild()
{
	SceneNode * child = new SceneNode();
	child->m_Parent = this;
	m_Children.push_back(child);
	return child;
}

void re::SceneNode::Compile()
{

	for (auto component : m_Components)
	{
		component->Compile();
	};

	for (auto child : m_Children)
	{
		child->Compile();
	}

}

re::RayHitResult re::Sphere::Intersect(const Ray & ray)
{
	RayHitResult result;

	// real squaredRadius = 1.0f;
	//Vector3 l = ray.Origin;

	real projection = (Vector3::Zero - ray.Origin) ^ ray.Direction;
	real squaredDistance = (ray.Origin ^ ray.Origin) - projection * projection;

	if (squaredDistance > 1.0f)
	{
		// No hit
		return result;
	}

	real offset = std::sqrt(1.0f - squaredDistance);

	real t1 = projection - offset;
	real t2 = projection + offset;

	if (t1 < 0 && t2 < 0)
	{
		// No intersection. The ray is going in the opposite direction
		return result;
	}

	// It could be 1 or 2 intersections
	// t1 it's the closest, but might be negative if the ray origin is
	// inside the sphere

	result.Hit = true;
	result.Point = ray.Origin + ray.Direction * (t1 >= 0.0f ? t1 : t2);
	result.Normal = result.Point.Normalized();

	return result;
}

re::RayHitResult re::Plane::Intersect(const Ray & ray)
{
	RayHitResult result;

	real distance = ray.Origin ^ Normal;

	// We also check if we're facing the plane or not
	// If not facing, we cull the plane
	if (distance < 0)
	{
		// Negative distance means that the ray origin is
		// "below" the plane, so we'are not facing it
		return result;
	}

	real cosine = Normal ^ ray.Direction;

	if (cosine >= 0)
	{
		// a positive cosine means that the ray will never intersect
		// the plane
		return result;
	}

	// The plane has been hit
	result.Hit = true;
	result.Point = ray.Origin + ray.Direction * (distance / -cosine);
	result.Normal = Normal;

	return result;
}


re::Shape::Shape(SceneNode * owner) : Component(owner)
{
	static unsigned int nextID = 1;
	m_ID = nextID;
	nextID = std::max(1u, nextID + 1);
}

re::RayHitResult re::Mesh::Intersect(const Ray & ray)
{
	/*
	RayHitResult result;
	
	// Check against the mesh bounding box first
	if (!m_BoundingBox.Intersect(ray).Hit)
		return result;

	real distance = std::numeric_limits<real>::max();

	// Find, if extits, the closest triangle that is intersected
	// by the ray
	for (auto &t : m_Triangles)
	{
		auto r = IntersectTriangle(ray, t);
		auto d = (r.Point - ray.Origin).SquaredLength();
		if (r.Hit && d < distance)
		{
			result.Hit = true;
			result.Point = r.Point;
			result.Normal = r.Normal;
			distance = d;
		}
	}

	return result;
	*/
	
	RayHitResult result;
	real distance = std::numeric_limits<real>::max();
	IntersectInternal(ray, m_KdTree, result, distance);
	return result;
	
}

re::Triangle& re::Mesh::AddTriangle()
{
	m_Triangles.push_back({});
	return m_Triangles.back();
}

void re::Mesh::Compile()
{
	if (m_Invalidated)
	{

		auto& min = m_BoundingBox.Min;
		auto& max = m_BoundingBox.Max;

		for (auto &t : m_Triangles) {
			t.Update();

			for (auto &v : t.Vertices)
			{
				min.X = std::min(min.X, v.X);
				min.Y = std::min(min.Y, v.Y);
				min.Z = std::min(min.Z, v.Z);

				max.X = std::max(max.X, v.X);
				max.Y = std::max(max.Y, v.Y);
				max.Z = std::max(max.Z, v.Z);
			}

		}

		m_KdTree = new KDTreeTriangle(m_Triangles, m_BoundingBox, 0);

		m_Invalidated = false;
	}
}

void re::Mesh::Invalidate()
{
	m_Invalidated = true;
}

re::RayHitResult re::Mesh::IntersectTriangle(const Ray & ray, const Triangle & t) const
{
	RayHitResult result;
	//Vector3 t0 = (Vertices[1] - Vertices[0]).Normalized();
	//Vector3 t1 = (Vertices[2] - Vertices[0]).Normalized();

	Vector3 l = ray.Origin - t.Vertices[0];
	real distance = l ^ t.FaceNormal;

	if (distance < 0)
	{
		// Ray origin "behind" the triangle plane
		return result;
	}

	real cosine = ray.Direction ^ t.FaceNormal;

	// Check if the ray is never intersecting the triangle plane
	if (cosine >= 0)
	{
		return result;
	}


	// Project the ray on the triangle plane 
	Vector3 projection = ray.Origin + ray.Direction * (distance / -cosine);

	// Use baricentric coordinates to check if the ray projection
	// is contained in the triangle
	Vector3 bar = t.Baricentric(projection);

	if (bar.X >= 0 && bar.Y >= 0 && bar.Z >= 0)
	{
		result.Hit = true;
		result.Point = projection;
		if (NormalMode == NormalModes::Face)
		{
			result.Normal = t.FaceNormal;
		}
		else
		{
			result.Normal = (t.Normals[0] * bar.X + t.Normals[1] * bar.Y + t.Normals[2] * bar.Z).Normalized();
		}
	}

	return result;
}

void re::Mesh::IntersectInternal(const Ray& ray, KDTreeTriangle * node, RayHitResult & result, real & distance) const
{
	if (node->Bounds.Intersect(ray).Hit)
	{
		for (auto & t : node->Triangles)
		{
			auto r = IntersectTriangle(ray, t);
			auto d = (ray.Origin - r.Point).SquaredLength();

			if (d < distance && r.Hit)
			{
				result = r;
				distance = d;
			}
		}

		if (node->Left)
			IntersectInternal(ray, node->Left, result, distance);

		if (node->Right)
			IntersectInternal(ray, node->Right, result, distance);

	}
}

re::Vector3 re::Triangle::Baricentric(const Vector3 & point) const
{
	// Fast baricentric coordinates:
	// https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
	real v, w, u;
	Vector3 v2 = point - Vertices[0];
	real d20 = v2 ^ Edges[0];
	real d21 = v2 ^ Edges[1];
	v = (m_D11 * d20 - m_D01 * d21) * m_InvDen;
	w = (m_D00 * d21 - m_D01 * d20) * m_InvDen;
	u = 1.0f - v - w;
	return { u, v, w };
}

void re::Triangle::Update()
{
	const_cast<Vector3&>(FaceNormal) = Cross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[1]).Normalized();
	const_cast<std::array<Vector3, 3>&>(Edges) = {
		Vertices[1] - Vertices[0],
		Vertices[2] - Vertices[0],
		Vertices[2] - Vertices[1]
	};

	m_D00 = Edges[0] ^ Edges[0];
	m_D01 = Edges[0] ^ Edges[1];
	m_D11 = Edges[1] ^ Edges[1];

	m_InvDen = 1.0 / (m_D00 * m_D11 - m_D01 * m_D01);
}

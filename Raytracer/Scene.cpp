#include "Scene.h"
#include <cassert>

re::SkyBox::SkyBox(Color color0, Color color1, Light * sun)
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

			real distance = (ray.Origin - worldPoint).Length();

			if (distance >= std::numeric_limits<float>::epsilon())
			{
				raycastResult.Hit = true;
				raycastResult.Point = worldPoint;
				raycastResult.LocalPoint = result.Point;
				// Calculate normal in world coordinates
				raycastResult.Normal = Vector3(tmat * Vector4(shape->GetNormal(result.Point), 0)).Normalized();
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

	return result;
}

re::Vector3 re::Sphere::GetNormal(const Vector3 & point) const
{
	return point.Normalized();
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

	return result;
}


re::RayHitResult re::Triangle::Intersect(const Ray & ray)
{
	RayHitResult result;
	Vector3 normal = GetNormal(Vector3::Zero);
	Vector3 t0 = (Vertices[1] - Vertices[0]).Normalized();
	Vector3 t1 = (Vertices[2] - Vertices[0]).Normalized();

	Vector3 l = ray.Origin - Vertices[0];
	real distance = l ^ normal;

	if (distance < 0)
	{
		// Ray origin "behind" the triangle plane
		return result;
	}

	real cosine = ray.Direction ^ normal;

	// Check if the ray is never intersecting the triangle plane
	if (cosine >= 0)
	{
		return result;
	}


	// Project the ray on the triangle plane 
	Vector3 projection = ray.Origin + ray.Direction * (distance / -cosine);

	// "Inside-Outside" method
	if ((normal ^ (Cross(Vertices[1] - Vertices[0], projection - Vertices[0]))) > 0 &&
		(normal ^ (Cross(Vertices[2] - Vertices[1], projection - Vertices[1]))) > 0 &&
		(normal ^ (Cross(Vertices[0] - Vertices[2], projection - Vertices[2]))) > 0)
	{
		result.Hit = true;
		result.Point = projection;
	}

	return result;


}

re::Vector3 re::Triangle::GetNormal(const Vector3 & point) const
{
	// CCW Normal computation
	return Cross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[1]).Normalized();
}

unsigned int * re::Renderer::RenderSync(Scene * scene)
{
	std::promise<RenderStatus> p;
	auto f = p.get_future();
	Render(scene, std::move(p));
	return f.get().Pixels;
}

re::Shape::Shape(SceneNode * owner) : Component(owner)
{
	static unsigned int nextID = 1;
	m_ID = nextID;
	nextID = std::max(1u, nextID + 1);
}

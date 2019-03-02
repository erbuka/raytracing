#include "Common.h"
#include <cmath>
#include <array>

const re::Vector3 re::Vector3::Forward = re::Vector3(0, 0, 1);
const re::Vector3 re::Vector3::Up = re::Vector3(0, 1, 0);
const re::Vector3 re::Vector3::Right = re::Vector3(1, 0, 0);
const re::Vector3 re::Vector3::Zero = re::Vector3(0, 0, 0);
const re::Vector3 re::Vector3::One = re::Vector3(1, 1, 1);

const re::Matrix4 re::Matrix4::Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

const re::Matrix4 re::Matrix4::Zero = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f
};

const re::Color re::Color::White = re::Color(1.0f, 1.0f, 1.0f);
const re::Color re::Color::Black = re::Color(0.0f, 0.0f, 0.0f);

re::Vector3::Vector3(const Vector4 & source)
{
	X = source.X;
	Y = source.Y;
	Z = source.Z;
}

re::real re::Vector3::SquaredLength() const
{
	return X*X + Y*Y + Z*Z;
}

re::real re::Vector3::Length() const
{
	return std::sqrt(X*X + Y*Y + Z*Z);
}

re::Vector3 re::Vector3::Normalized() const
{
	real length = Length();
	if (length > 0)
	{
		return Vector3(X / length, Y / length, Z / length);
	}
	else
	{
		return Vector3();
	}
}

re::Vector3 re::Vector3::Reflect(const Vector3 & normal) const
{
	return *this - normal * (normal ^ *this) * (real)2.0f;
}

re::Vector3 re::Vector3::Floor() const
{
	return Vector3(std::floor(X), std::floor(Y), std::floor(Z));
}

re::Vector3 re::Vector3::Ceil() const
{
	return Vector3(std::ceil(X), std::ceil(Y), std::ceil(Z));
}

re::Vector3 & re::Vector3::operator+=(const Vector3 & other)
{
	X += other.X;
	Y += other.Y;
	Z += other.Z;
	return *this;
}


re::Vector3 & re::Vector3::operator-=(const Vector3 & other)
{
	X -= other.X;
	Y -= other.Y;
	Z -= other.Z;
	return *this;
}

re::Vector3 & re::Vector3::operator*=(const Vector3 & other)
{
	X *= other.X;
	Y *= other.Y;
	Z *= other.Z;
	return *this;
}

re::Vector3 & re::Vector3::operator*=(real t)
{
	X *= t;
	Y *= t;
	Z *= t;
	return *this;
}

re::Vector3 re::Vector3::operator-() const
{
	return Vector3(-X, -Y, -Z);	
}

std::ostream & re::operator<<(std::ostream & os, const Vector3 & v)
{
	os << "(" << v.X << ", " << v.Y << ", " << v.Z << ")";
	return os;
}

re::Vector3 re::operator+(const Vector3 & lhs, const Vector3 & rhs)
{
	return Vector3(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
}

re::Vector3 re::operator-(const Vector3 & lhs, const Vector3 & rhs)
{
	return Vector3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
}

re::Vector3 re::operator*(const Vector3 & lhs, const Vector3 & rhs)
{
	return Vector3(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
}

re::Vector3 re::operator/(const Vector3 & lhs, const Vector3 & rhs)
{
	return Vector3(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z);
}

re::real re::operator^(const Vector3 & lhs, const Vector3 & rhs)
{
	return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
}

re::Vector3 re::Cross(const Vector3 & lhs, const Vector3 & rhs)
{
	return Vector3(
		lhs.Z * rhs.Y - lhs.Y * rhs.Z,
		lhs.X * rhs.Z - lhs.Z * rhs.X,
		lhs.Y * rhs.X - lhs.X * rhs.Y
		);
}

re::Matrix4 re::operator*(const Matrix4 & lhs, const Matrix4 & rhs)
{
	Matrix4 result = Matrix4::Zero;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				result[i * 4 + j] += lhs[i * 4 + k] * rhs[k * 4 + j];
			}
		}
	}
	return result;
}

re::Vector4 re::operator*(const Matrix4 & m, const Vector4 & p)
{
	Vector4 result(Vector3::Zero, 0);
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.Elements[i] += m[i * 4 + j] * p.Elements[j];
		}
	}
	return result;
}

re::Vector2 re::operator+(const Vector2 & lhs, const Vector2 & rhs)
{
	return Vector2(lhs.X + rhs.X, lhs.Y + rhs.Y);
}

re::Vector2 re::operator-(const Vector2 & lhs, const Vector2 & rhs)
{
	return Vector2(lhs.X - rhs.X, lhs.Y - rhs.Y);
}

re::Vector2 re::operator*(const Vector2 & lhs, const Vector2 & rhs)
{
	return Vector2(lhs.X * rhs.X, lhs.Y * rhs.Y);
}

re::Vector2 re::operator*(const Vector2 & lhs, real t)
{
	return Vector2(lhs.X * t, lhs.Y * t);
}

re::Vector2 re::operator*(real t, const Vector2 & rhs)
{
	return rhs * t;
}

re::real re::operator^(const Vector2& lhs, const Vector2& rhs)
{
	return lhs.X * rhs.X + rhs.Y * rhs.Y;
}

re::Color re::operator+(const Color & lhs, const Color & rhs)
{
	return Color(
		Clamp(lhs.R + rhs.R, (real)0.0f, (real)1.0f),
		Clamp(lhs.G + rhs.G, (real)0.0f, (real)1.0f),
		Clamp(lhs.B + rhs.B, (real)0.0f, (real)1.0f)
		);
}

re::Color re::operator*(const Color & lhs, const Color & rhs)
{
	return Color(
		Clamp(lhs.R * rhs.R, (real)0.0f, (real)1.0f),
		Clamp(lhs.G * rhs.G, (real)0.0f, (real)1.0f),
		Clamp(lhs.B * rhs.B, (real)0.0f, (real)1.0f)
		);
}

re::Color re::operator*(const Color & lhs, real k)
{
	return Color(
		Clamp(lhs.R * k, (real)0.0f, (real)1.0f),
		Clamp(lhs.G * k, (real)0.0f, (real)1.0f),
		Clamp(lhs.B * k, (real)0.0f, (real)1.0f)
		);
}

re::Color re::Mix(const Color & a, const Color & b, real t)
{
	real _t = 1 - t;
	return { _t * a.R + t * b.R, _t * a.G + t * b.G, _t * a.B + t * b.B };
}

re::Vector3 re::operator*(const Vector3 & v, real t)
{
	return Vector3(v.X * t, v.Y * t, v.Z * t);
}

re::Vector3 re::operator/(const Vector3 & v, real t)
{
	return Vector3(v.X / t, v.Y / t, v.Z / t);
}

re::Color::Color(real r, real g, real b) :
	R(Clamp(r, (real)0.0f, (real)1.0f)),
	G(Clamp(g, (real)0.0f, (real)1.0f)),
	B(Clamp(b, (real)0.0f, (real)1.0f))
{
}

re::Color::Color(unsigned int color)
{
	constexpr real factor = 1.0f / 255.0f;
	B = (color & 0x000000ff) * factor;
	G = ((color & 0x0000ff00) >> 8) * factor;
	R = ((color & 0x00ff0000) >> 16) * factor;
}

unsigned int re::Color::GetHexValue() const
{
	return
		0xff000000 |
		(((unsigned int)(R * (real)255.0f))) |
		(((unsigned int)(G * (real)255.0f)) << 8) |
		(((unsigned int)(B * (real)255.0f)) << 16);
}

re::Color & re::Color::operator+=(const Color & other)
{
	R = Clamp(R + other.R, (real)0.0f, (real)1.0f);
	G = Clamp(G + other.G, (real)0.0f, (real)1.0f);
	B = Clamp(B + other.B, (real)0.0f, (real)1.0f);
	return *this;
}

re::Color & re::Color::operator*=(const Color & other)
{
	R = Clamp(R * other.R, (real)0.0f, (real)1.0f);
	G = Clamp(G * other.G, (real)0.0f, (real)1.0f);
	B = Clamp(B * other.B, (real)0.0f, (real)1.0f);
	return *this;
}

re::Color & re::Color::operator*=(real t)
{
	R = Clamp(R * t, (real)0.0f, (real)1.0f);
	G = Clamp(G * t, (real)0.0f, (real)1.0f);
	B = Clamp(B * t, (real)0.0f, (real)1.0f);
	return *this;
}

re::RayHitResult re::BoundingBox::Intersect(const Ray & ray)
{
	RayHitResult result;

	real t1 = (Min.X - ray.Origin.X) / ray.Direction.X;
	real t2 = (Max.X - ray.Origin.X) / ray.Direction.X;
	real t3 = (Min.Y - ray.Origin.Y) / ray.Direction.Y;
	real t4 = (Max.Y - ray.Origin.Y) / ray.Direction.Y;
	real t5 = (Min.Z - ray.Origin.Z) / ray.Direction.Z;
	real t6 = (Max.Z - ray.Origin.Z) / ray.Direction.Z;
	real t7 = std::fmax(std::fmax(std::fmin(t1, t2), std::fmin(t3, t4)), std::fmin(t5, t6));
	real t8 = std::fmin(std::fmin(std::fmax(t1, t2), std::fmax(t3, t4)), std::fmax(t5, t6));
	real t9 = (t8 < 0 || t7 > t8) ? -1 : t7;
	if (t9 != -1) {
		result.Point = ray.Direction * t9 + ray.Origin;
		result.Hit = true;
	}
	return result;
}

re::Matrix4 re::Matrix4::GetTranslation(const re::Vector3& translation)
{
	Matrix4 result = Matrix4::Identity;

	result[3] = translation.X;
	result[7] = translation.Y;
	result[11] = translation.Z;

	return result;
}


re::Matrix4 re::Matrix4::GetScale(const re::Vector3& scale)
{
	Matrix4 result = Matrix4::Identity;

	result[0] = scale.X;
	result[5] = scale.Y;
	result[10] = scale.Z;

	return result;
}

re::Matrix4 re::Matrix4::Transpose() const
{
	Matrix4 result;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result[i * 4 + j] = Elements[j * 4 + i];
		}
	}
	return result;
}

re::Matrix4 re::Matrix4::GetRotation(const re::Vector3& rotation)
{
	Matrix4 result = Matrix4::Identity;

	real cyaw = std::cosf(rotation.Z), syaw = std::sinf(rotation.Z);
	real cpitch = std::cosf(rotation.Y), spicth = std::sinf(rotation.Y);
	real croll = std::cosf(rotation.X), sroll = std::sinf(rotation.X);

	result[0] = cyaw * cpitch;
	result[1] = cyaw * spicth * sroll - syaw * croll;
	result[2] = cyaw * spicth * croll + syaw * sroll;
	
	result[4] = syaw * cpitch;
	result[5] = syaw * spicth * sroll + cyaw * croll;
	result[6] = syaw * spicth * croll - cyaw * sroll;

	result[8] = -spicth;
	result[9] = cpitch * sroll;
	result[10] = cpitch * croll;

	return result;

}

re::Vector4::Vector4(const Vector3 & source, real w)
{
	X = source.X;
	Y = source.Y;
	Z = source.Z;
	W = w;
}

#pragma once
#include <algorithm>
#include <iostream>
#include <vector>
namespace re
{
	typedef double real;

	constexpr real PI = 3.14159265359f;

	inline real Map(real value, real sa, real sb, real da, real db)
	{
		return da + (value - sa) / (sb - sa)*(db - da);
	}

	template<typename T> inline T Random(T min = 0, T max = 1)
	{
		return (T)(min + (max - min) * ((double)rand() / RAND_MAX));
	}

	template<typename T> inline T Clamp(T value, T min, T max)
	{
		return std::max(min, std::min(max, value));
	}

	template<typename T, typename V> inline T Lerp(T a, T b, V t)
	{
		return a * (1 - t) + b * t;
	}

	template<typename T> inline T MaxOf(const T& val)
	{
		return val;
	}

	template<typename T, typename ... Args> inline T MaxOf(const T& first, const Args& ... args)
	{
		T result = MaxOf(args...);
		return first > result ? first : result;
	}

	template<typename T> inline T MinOf(const T& val)
	{
		return val;
	}

	template<typename T, typename ... Args> inline T MinOf(const T& first, const Args& ... args)
	{
		T result = MinOf(args...);
		return first < result ? first : result;
	}

	union Vector2;
	union Vector3;
	union Vector4;

	union Vector2
	{
		static const Vector2 Zero;

		Vector2() : X(0), Y(0) {}
		Vector2(real x, real y) : X(x), Y(y) {}

		struct { real X, Y; };
		struct { real R, G; };
		struct { real U, V; };
	};

	union Vector3
	{
		static const Vector3 Forward;
		static const Vector3 Up;
		static const Vector3 Right;

		static const Vector3 Zero;
		static const Vector3 One;

		real Elements[3];
		struct { real X, Y, Z; };
		struct { real R, G, B; };
		struct { real U, V, S; };

		Vector3() : X(0), Y(0), Z(0) {}
		Vector3(real x, real y, real z) : X(x), Y(y), Z(z) {}
		Vector3(const Vector4& source);
		
		real SquaredLength() const;
		real Length() const;
		Vector3 Normalized() const;
		Vector3 Reflect(const Vector3& normal) const;

		Vector3 Floor() const;
		Vector3 Ceil() const;

		Vector3& operator+=(const Vector3& other);
		Vector3& operator-=(const Vector3& other);
		Vector3& operator*=(const Vector3& other);
		Vector3& operator*=(real t);

		Vector3 operator-() const;

	};

	union Vector4
	{
		real Elements[4];
		struct { real X, Y, Z, W; };
		struct { real R, G, B, A; };
		struct { real U, V, S, T; };

		Vector4(const Vector3& source, real w);
	};

	struct Matrix4
	{
	public:
		static const Matrix4 Zero;
		static const Matrix4 Identity;

		real Elements[16];

		real& operator[](int index) { return Elements[index]; }
		real operator[](int index) const { return Elements[index]; }

		Matrix4 Transpose() const;

		static Matrix4 GetRotation(const Vector3& rotation);
		static Matrix4 GetTranslation(const Vector3& translation);
		static Matrix4 GetScale(const Vector3& translation);
	};



	std::ostream& operator<<(std::ostream& os, const Vector3& v);

	Vector3 operator+(const Vector3& lhs, const Vector3& rhs);
	Vector3 operator-(const Vector3& lhs, const Vector3& rhs);
	Vector3 operator*(const Vector3& lhs, const Vector3& rhs); // Hardmond product
	Vector3 operator/(const Vector3& lhs, const Vector3& rhs); // Hardmond division
	Vector3 operator*(const Vector3& v, real t);
	Vector3 operator/(const Vector3& v, real t);
	real operator^(const Vector3& lhs, const Vector3& rhs); // Scalar product
	Vector3 Cross(const Vector3& lhs, const Vector3& rhs);

	Matrix4 operator*(const Matrix4& lhs, const Matrix4& rhs);
	Vector4 operator*(const Matrix4& m, const Vector4& p);


	Vector2 operator+(const Vector2& lhs, const Vector2& rhs);
	Vector2 operator-(const Vector2& lhs, const Vector2& rhs);
	Vector2 operator*(const Vector2& lhs, const Vector2& rhs);
	Vector2 operator*(const Vector2& lhs, real t);
	Vector2 operator*(real t, const Vector2& rhs);
	real operator^(const Vector2& lhs, const Vector2& rhs);


	struct Color
	{
		static const Color White;
		static const Color Black;

		real R, G, B;
		Color() : R(0), G(0), B(0) {}
		Color(real r, real g, real b);
		Color(unsigned int color);
		unsigned int GetHexValue() const;

		Color& operator+=(const Color& other);
		Color& operator*=(const Color& other);
		Color& operator*=(real t);

		real Luma() const;

	};

	Color operator+(const Color& lhs, const Color& rhs);
	Color operator*(const Color& lhs, const Color& rhs);
	Color operator*(const Color& color, real k);

	Color Mix(const Color& a, const Color& b, real t);

	struct Ray
	{
		Vector3 Origin = Vector3::Zero, Direction = Vector3::Forward;
	};

	struct RayHitResult
	{
		bool Hit = false;
		Vector3 Point = Vector3::Zero;
		Vector3 Normal = Vector3::Zero;
	};

	class Raytraceable
	{
	public:
		virtual RayHitResult Intersect(const Ray& ray) = 0;
	};

	class BoundingBox : public Raytraceable
	{
	public:
		
		Vector3 Min, Max;

		BoundingBox() : Min(), Max() {}
		BoundingBox(Vector3 min, Vector3 max) : Min(min), Max(max) {}

		virtual RayHitResult Intersect(const Ray& ray) override;
	};



}
#pragma once
#include <concepts>

namespace utils::graphics::opengl
{
	class Drawable
	{
	public:
		virtual void draw() const = 0;
	};

	template <class T>
	concept is_drawable = std::derived_from<T, Drawable>;

	class Component
	{
	public:
		virtual void use() const = 0;
	};
}
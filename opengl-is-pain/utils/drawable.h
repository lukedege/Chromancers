#pragma once

namespace utils::graphics::opengl
{
	class Drawable
	{
	public:
		virtual void draw() const = 0;
	};
}
#pragma once

namespace utils::oop
{
	// Simple classes to inherit to quickly remove copiability or movability properties
	struct non_copyable
	{
		inline  non_copyable() {}
		inline ~non_copyable() {}
		inline       non_copyable(const non_copyable& copy) = delete;
		inline const non_copyable& operator=(const non_copyable& copy) = delete;
	};
	struct non_movable
	{
		inline  non_movable() {}
		inline ~non_movable() {}
		inline       non_movable(non_movable&& move) noexcept = delete;
		inline const non_movable& operator=(non_movable&& move) noexcept = delete;
	};
}
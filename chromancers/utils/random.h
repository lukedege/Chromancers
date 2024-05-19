#pragma once

#include <random>
#include <limits>

namespace utils::random
{
	// Simple random generator class
	class generator
	{
		std::random_device random_device; // create object for seeding
		std::mt19937 random_engine{random_device()}; // create engine and seed it
		std::uniform_int_distribution<unsigned int> uint_distribution{0, std::numeric_limits<unsigned int>::max()}; // create distribution for int  with given range
		std::uniform_real_distribution<float> float_distribution{0, 1}; // create distribution for integers with given range

	public:

		float get_float(float min, float max)
		{
			std::uniform_real_distribution<float> float_distribution{min, max};
			return float_distribution(random_engine);
		}

		// Returns a float between 0 and 1
		float get_float()
		{
			return float_distribution(random_engine);
		}

		// Returns an unsigned int between 0 and uint::max
		unsigned int get_uint()
		{
			return uint_distribution(random_engine);
		}
	};
	
}
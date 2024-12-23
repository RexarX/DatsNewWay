#pragma once

#include "Core.h"

namespace Snake {
	class Timestep {
	public:
		Timestep(double time = 0.0) noexcept : m_Time(time) {}

		inline operator float() const noexcept { return static_cast<float>(m_Time); }

		inline double GetSeconds() const noexcept { return m_Time; }
		inline double GetMilliseconds() const noexcept { return m_Time * 1000.0; }
		inline double GetFramerate() const noexcept { return 1.0 / m_Time; }

	private:
		double m_Time = 0.0;
	};
}
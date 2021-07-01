#ifndef _VKS_COMMON_MATH_H_
#define _VKS_COMMON_MATH_H_ 1
#include <cmath>
#include<cstdint>
/**
 *
 */
class Math {
  public:
	template <class T> inline constexpr static T clamp(T a, T min, T max) {
		static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value,
					  "Must be a decimal type(float/double/half) or integar.");
		return Math::max<T>(min, Math::min<T>(max, a));
	}

	/**
	 *	Get max value of a and b.
	 */
	template <typename T> inline constexpr static T max(T a, T b) {
		return (static_cast<T>(a) < static_cast<T>(b)) ? static_cast<T>(b) : static_cast<T>(a);
	}

	/**
	 *	Get min value of a and b.
	 */
	template <typename T> inline constexpr static T min(T a, T b) noexcept {
		return (static_cast<T>(b) < static_cast<T>(a)) ? static_cast<T>(b) : static_cast<T>(a);
	}

	/**
	 *	Get float modular.
	 */
	template <typename T> inline static float mod(T a) {
		static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value,
					  "Must be a decimal type(float/double/half) or integar.");
		if constexpr (std::is_floating_point<T>::value) {
			T part;
			return ::modf(a, &part);
		}
	}

	/**
	 *	Convert degree to radian.
	 */
	template <class T> inline static T deg2Rad(T deg) { return (deg * (T)Math::PI) / 180.0f; }

	/**
	 *	Convert radian to degree.
	 */
	template <class T> inline static T radToDeg(T deg) { return (deg * (T)180) / (T)Math::PI; }

	template <typename T> inline constexpr static T lerp(T a, T b, T t) noexcept {
		static_assert(std::is_floating_point<T>::value, "Must be a decimal type(float/double/half).");
		return (a + (b - a) * t);
	}

	/*	*/
	inline static constexpr double E = 2.718281828459045235;
	inline static constexpr double PI = 3.141592653589793238462643383279502884;
	inline static constexpr double PI_half = PI / 2.0;
	inline static constexpr double PI_2 = PI * 2.0;
	inline static constexpr double Infinite = 0;
	inline static constexpr double Deg2Rad = Math::PI / 180.0;
	inline static constexpr double Rad2Deg = 180 / Math::PI;
	inline static constexpr double NegativeInfinity = 0;

	/*	*/
	template <typename T> static T NextPowerOfTwo(T v) {
		T res = 1;
		while (res < v)
			res <<= 1;
		return res;
	}
	template <typename T> static T ClosestPowerOfTwo(T v) noexcept {
		T n = NextPowerOfTwo(v);
		T p = 0;
		return 0;
	}
	template <typename T> static bool IsPowerOfTwo(T v) noexcept { return (v && ((v - 1) & v)); }

	template <typename T> constexpr static T dotGridGradient(T ix, T iy, T x, T y) noexcept {

		T gradient[] = {0, 0};
		randomGradient<T>(ix, iy, gradient);

		// Compute the distance vector
		T dx = x - (T)ix;
		T dy = y - (T)iy;

		// Compute the dot-product
		return (dx * gradient[0] + dy * gradient[1]);
	}

	template <typename T> constexpr static void randomGradient(int ix, int iy, T *gradient) noexcept {
		// Random float. No precomputed gradients mean this works for any number of grid coordinates
		if constexpr (std::is_same<T, float>::value) {
			T random =
				2920.f * sinf(ix * 21942.f + iy * 171324.f + 8912.f) * cosf(ix * 23157.f * iy * 217832.f + 9758.f);
			gradient[0] = cosf(random);
			gradient[1] = sinf(random);
		} else {
			T random = 2920.f * sin(ix * 21942.f + iy * 171324.f + 8912.f) * cos(ix * 23157.f * iy * 217832.f + 9758.f);
			gradient[0] = cos(random);
			gradient[1] = sin(random);
		}
	}

	template <typename T> constexpr static T perlinNoise(T x, T y, int octave) noexcept {

		T result = 0;
		T totalAmplitude = 0.0f;
		T amplitude = 2.0f;
		T persistance = 0.50f;
		const uint32_t wrap = 255;

		// Determine grid cell coordinates
		const int32_t x0 = (int32_t)std::floor(x) % wrap;
		const int32_t x1 = x0 + 1;
		const int32_t y0 = (int32_t)std::floor(y) % wrap;
		const int32_t y1 = y0 + 1;

		T sx = x - (T)x0;
		T sy = y - (T)y0;

		for (int z = 0; z < octave; z++) {
			// Interpolate between grid point gradients
			T n0, n1, ix0, ix1, value;

			// x0 *= 1.0f;
			// x1 *= 1.0f;
			// y0 *= 1.0f;
			// y1 *= 1.0f;

			n0 = dotGridGradient<T>(x0, y0, x, y);
			n1 = dotGridGradient<T>(x1, y0, x, y);
			ix0 = lerp<T>(n0, n1, sx);

			n0 = dotGridGradient<T>(x0, y1, x, y);
			n1 = dotGridGradient<T>(x1, y1, x, y);
			ix1 = lerp<T>(n0, n1, sx);

			value = lerp<T>(ix0, ix1, sy);

			// amplitude *= persistance;
			// totalAmplitude += amplitude;

			// /*	*/
			// const unsigned int samplePeriod = (1 << z);
			// const float sampleFrquency = 1.0f / (float)samplePeriod;

			// /*	*/
			// const float xpos = ((float)x / (float)samplePeriod) * sampleFrquency;
			// const float ypos = ((float)y / (float)samplePeriod) * sampleFrquency;
			result += value;
		}
		return result * 0.5 + 0.5;
	}

	//	static float PerlinNoise(float x, float y, float z, int octave);
};

#endif
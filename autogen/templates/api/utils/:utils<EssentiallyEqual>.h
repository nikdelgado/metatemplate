#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

namespace {{ns_tpl}}
{
	// This namespace is the minimal code required to use the
	// gtest ALMOST_EQUAL without pulling in the rest of that code
	// as an include. For ref, this was pulled from
	// googletest/googletest/include/gtest/internal/gtest-port.h @ 96683ee6680e433a53e7deda976640ae3268012d
	// googletest/googletest/include/gtest/internal/gtest-internal.h @ d66ce585109eed6d2d891b7ed7ab3ca96e854483
	namespace gtest_clone {
		// Copyright 2005, Google Inc.
		// All rights reserved.
		//
		// Redistribution and use in source and binary forms, with or without
		// modification, are permitted provided that the following conditions are
		// met:
		//
		//     * Redistributions of source code must retain the above copyright
		// notice, this list of conditions and the following disclaimer.
		//     * Redistributions in binary form must reproduce the above
		// copyright notice, this list of conditions and the following disclaimer
		// in the documentation and/or other materials provided with the
		// distribution.
		//     * Neither the name of Google Inc. nor the names of its
		// contributors may be used to endorse or promote products derived from
		// this software without specific prior written permission.
		//
		// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
		// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
		// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
		// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
		// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
		// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
		// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
		// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
		// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
		// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
		// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
		//
		// Authors: wan@google.com (Zhanyong Wan), eefacm@gmail.com (Sean Mcafee)
		//
		// The Google C++ Testing Framework (Google Test)

		// This template class serves as a compile-time function from size to
		// type.  It maps a size in bytes to a primitive type with that
		// size. e.g.
		//
		//   TypeWithSize<4>::UInt
		//
		// is typedef-ed to be unsigned int (unsigned integer made up of 4
		// bytes).
		//
		// Such functionality should belong to STL, but I cannot find it
		// there.
		//
		// Google Test uses this class in the implementation of floating-point
		// comparison.
		//
		// For now it only handles UInt (unsigned int) as that's all Google Test
		// needs.  Other types can be easily added in the future if need
		// arises.
		template <size_t size>
		class TypeWithSize {
		public:
		// This prevents the user from using TypeWithSize<N> with incorrect
		// values of N.
		using UInt = void;
		};

		// The specialization for size 4.
		template <>
		class TypeWithSize<4> {
		public:
		using Int = std::int32_t;
		using UInt = std::uint32_t;
		};

		// The specialization for size 8.
		template <>
		class TypeWithSize<8> {
		public:
		using Int = std::int64_t;
		using UInt = std::uint64_t;
		};

		// This template class represents an IEEE floating-point number
		// (either single-precision or double-precision, depending on the
		// template parameters).
		//
		// The purpose of this class is to do more sophisticated number
		// comparison.  (Due to round-off error, etc, it's very unlikely that
		// two floating-points will be equal exactly.  Hence a naive
		// comparison by the == operation often doesn't work.)
		//
		// Format of IEEE floating-point:
		//
		//   The most-significant bit being the leftmost, an IEEE
		//   floating-point looks like
		//
		//     sign_bit exponent_bits fraction_bits
		//
		//   Here, sign_bit is a single bit that designates the sign of the
		//   number.
		//
		//   For float, there are 8 exponent bits and 23 fraction bits.
		//
		//   For double, there are 11 exponent bits and 52 fraction bits.
		//
		//   More details can be found at
		//   http://en.wikipedia.org/wiki/IEEE_floating-point_standard.
		//
		// Template parameter:
		//
		//   RawType: the raw floating-point type (either float or double)
		template <typename RawType>
		class FloatingPoint {
		public:
			// Defines the unsigned integer type that has the same size as the
			// floating point number.
			using Bits = typename TypeWithSize<sizeof(RawType)>::UInt;

			// Constants.

			// # of bits in a number.
			static const size_t kBitCount = 8 * sizeof(RawType);

			// # of fraction bits in a number.
			static const size_t kFractionBitCount =
				std::numeric_limits<RawType>::digits - 1;

			// # of exponent bits in a number.
			static const size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

			// The mask for the sign bit.
			static const Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

			// The mask for the fraction bits.
			static const Bits kFractionBitMask = ~static_cast<Bits>(0) >>
												(kExponentBitCount + 1);

			// The mask for the exponent bits.
			static const Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

			// How many ULP's (Units in the Last Place) we want to tolerate when
			// comparing two numbers.  The larger the value, the more error we
			// allow.  A 0 value means that two numbers must be exactly the same
			// to be considered equal.
			//
			// The maximum error of a single floating-point operation is 0.5
			// units in the last place.  On Intel CPU's, all floating-point
			// calculations are done with 80-bit precision, while double has 64
			// bits.  Therefore, 4 should be enough for ordinary use.
			//
			// See the following article for more details on ULP:
			// http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
			static const uint32_t kMaxUlps = 4;

			// Constructs a FloatingPoint from a raw floating-point number.
			//
			// On an Intel CPU, passing a non-normalized NAN (Not a Number)
			// around may change its bits, although the new value is guaranteed
			// to be also a NAN.  Therefore, don't expect this constructor to
			// preserve the bits in x when x is a NAN.
			explicit FloatingPoint(const RawType& x) { u_.value_ = x; }
			
			// Returns the exponent bits of this number.
			Bits exponent_bits() const { return kExponentBitMask & u_.bits_; }

			// Returns the fraction bits of this number.
			Bits fraction_bits() const { return kFractionBitMask & u_.bits_; }

			// Returns the sign bit of this number.
			Bits sign_bit() const { return kSignBitMask & u_.bits_; }

			// Returns true if and only if this is NAN (not a number).
			bool is_nan() const {
				// It's a NAN if the exponent bits are all ones and the fraction
				// bits are not entirely zeros.
				return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
			}

			// Returns true if and only if this number is at most kMaxUlps ULP's away
			// from rhs.  In particular, this function:
			//
			//   - returns false if either number is (or both are) NAN.
			//   - treats really large numbers as almost equal to infinity.
			//   - thinks +0.0 and -0.0 are 0 DLP's apart.
			bool AlmostEquals(const FloatingPoint& rhs) const {
				// The IEEE standard says that any comparison operation involving
				// a NAN must return false.
				if (is_nan() || rhs.is_nan()) return false;

				return DistanceBetweenSignAndMagnitudeNumbers(u_.bits_, rhs.u_.bits_) <=
					kMaxUlps;
			}

		private:
			// The data type used to store the actual floating-point number.
			union FloatingPointUnion {
				RawType value_;  // The raw floating-point number.
				Bits bits_;      // The bits that represent the number.
			};

			// Converts an integer from the sign-and-magnitude representation to
			// the biased representation.  More precisely, let N be 2 to the
			// power of (kBitCount - 1), an integer x is represented by the
			// unsigned number x + N.
			//
			// For instance,
			//
			//   -N + 1 (the most negative number representable using
			//          sign-and-magnitude) is represented by 1;
			//   0      is represented by N; and
			//   N - 1  (the biggest number representable using
			//          sign-and-magnitude) is represented by 2N - 1.
			//
			// Read http://en.wikipedia.org/wiki/Signed_number_representations
			// for more details on signed number representations.
			static Bits SignAndMagnitudeToBiased(const Bits& sam) {
				if (kSignBitMask & sam) {
				// sam represents a negative number.
				return ~sam + 1;
				} else {
				// sam represents a positive number.
				return kSignBitMask | sam;
				}
			}

			// Given two numbers in the sign-and-magnitude representation,
			// returns the distance between them as an unsigned number.
			static Bits DistanceBetweenSignAndMagnitudeNumbers(const Bits& sam1,
																const Bits& sam2) {
				const Bits biased1 = SignAndMagnitudeToBiased(sam1);
				const Bits biased2 = SignAndMagnitudeToBiased(sam2);
				return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
			}

			FloatingPointUnion u_;
		}; // class FloatingPoint
	} // namespace detail
	/**
	 * @brief Checks if two floating point values are essentially equal
	 *
	 * @param v1: the first value
	 * @param v2: the second value
	 * @returns bool true if equal
	 */
	template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
	[[nodiscard]] bool EssentiallyEqual(T v1, T v2) noexcept
	{
		const gtest_clone::FloatingPoint<T> fp1(v1);
		const gtest_clone::FloatingPoint<T> fp2(v2);
		return fp1.AlmostEquals(fp2);
	}

	/**
	 * @brief Checks if two optional floating point values are essentially equal
	 *
	 * @param v1: the first value
	 * @param v2: the second value
	 * @returns bool true if equal
	 */
	template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
	[[nodiscard]] bool EssentiallyEqual(const std::optional<T>& v1, const std::optional<T>& v2) noexcept
	{
		return v1.has_value() && v2.has_value() ? EssentiallyEqual(v1.value(), v2.value()) : !(v1.has_value() || v2.has_value());
	}

    /**
     * @brief Checks if two optional floating point vectors are essentially equal
     *
     * @param v1: the first vector
     * @param v2: the second vector
     * @returns bool true if equal
     */
    template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    [[nodiscard]] bool EssentiallyEqual(const std::vector<T>& vec1, const std::vector<T>& vec2) noexcept
    {
            return vec1.size() == vec2.size()
                       && std::equal(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(),
                                                     [](const auto& v1, const auto& v2) { return EssentiallyEqual(v1, v2); });
    }
} // namespace {{ns_tpl}}
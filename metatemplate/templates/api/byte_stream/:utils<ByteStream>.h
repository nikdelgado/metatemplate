// Original: https://github.com/mohitmv/quick @ include/quick/byte_stream.hpp
// Author: Mohit Saini (mohitsaini1196@gmail.com)

/**
MIT License

Copyright (c) 2022 Mohit Saini

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
// ByteStream is super intuitive, safe, reliable and easy to use utility for
// binary serialisation and deserialization of complex and deeply nested C++
// objects.
// Learn more at README.md and byte_stream_test.cpp

// Sample Use Case:
//
// int x1 = 4;
// vector<std::set<std::string>> v1 = {"Q"};
// quick::OByteStream obs;
// obs << x1 << v1;
// std::vector<std::byte> content = obs.Buffer();

// quick::IByteStream ibs(content);
// int x2;
// vector<std::set<std::string>> v2;
// ibs >> x2 >> v2;
// assert(x1 == x2 && v1 == v2);

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "{{path_package}}/utils/Clock.h"
#include "{{path_package}}/utils/UUID.h"

#include "{{path_package}}/types/AbstractFactory.h"

namespace {{ns_tpl}}
{
	namespace bytestream_impl
	{
		enum class Status
		{
			OK,
			INVALID_READ
		};

		inline void writePrimitiveType(void* dst, const void* value, size_t size)
		{
			std::copy_n((const std::byte*)value, size, (std::byte*)dst);
		}

		inline void writeBuffer(void* dst, const void* buffer_ptr, size_t buffer_size)
		{
			std::copy_n((const std::byte*)buffer_ptr, buffer_size, (std::byte*)dst);
		}

		template <typename T, typename = void>
		class HasIterator : public std::false_type
		{
		};
		template <typename T>
		class HasIterator<T, std::void_t<typename T::iterator>> : public std::true_type
		{
		};

		template <typename T, typename = void>
		class HasToBytestream : public std::false_type
		{
		};
		template <typename T>
		class HasToBytestream<T, std::void_t<decltype(&T::toByteStream)>> : public std::true_type
		{
		};

		template <typename T, typename = void>
		class HasFromBytestream : public std::false_type
		{
		};
		template <typename T>
		class HasFromBytestream<T, std::void_t<decltype(&T::fromByteStream)>> : public std::true_type
		{
		};

		template <typename T, typename = void>
		class CanInsert : public std::false_type
		{
		};
		template <typename T>
		class CanInsert<T, std::void_t<decltype(std::declval<T>().insert(std::declval<typename T::value_type&&>()))>> : public std::true_type
		{
		};

		template <typename T, typename = void>
		class CanPushBack : public std::false_type
		{
		};
		template <typename T>
		class CanPushBack<T, std::void_t<decltype(std::declval<T>().push_back(std::declval<typename T::value_type&&>()))>> : public std::true_type
		{
		};

		template <typename T>
		class ConstCastValueType
		{
		public:
			using type = T;
		};
		template <typename T1, typename T2>
		class ConstCastValueType<std::pair<const T1, T2>>
		{
		public:
			using type = std::pair<T1, T2>;
		};

	} // namespace bytestream_impl

	class OByteStream
	{
	public:
		OByteStream(size_t capacity = 0)
		{
			if(capacity > 0)
			{
				outputBytes_.reserve(capacity);
			}
		}

		const std::vector<std::byte>& getBytes() const
		{
			return outputBytes_;
		}
		std::vector<std::byte>& getMutableBytes()
		{
			return outputBytes_;
		}

		template <typename T>
		OByteStream& operator<<(const T& input)
		{
			write(input);
			return *this;
		}

		const std::vector<std::byte>& buffer() const
		{
			return outputBytes_;
		}
		std::vector<std::byte>& buffer()
		{
			return outputBytes_;
		}

	private:
		void write(const std::string_view& input)
		{
			const size_t size0 = outputBytes_.size();
			const size_t size1 = input.size();
			outputBytes_.resize(size0 + sizeof(size_t) + size1);
			bytestream_impl::writePrimitiveType(&outputBytes_[size0], &size1, sizeof(size_t));
			bytestream_impl::writeBuffer(&outputBytes_[size0 + sizeof(size_t)], input.data(), input.size());
		}

		template <typename T, std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, int> = 0>
		void write(const std::vector<T>& input)
		{
			const size_t size0 = outputBytes_.size();
			const size_t size1 = input.size();
			const size_t realInputSize = sizeof(T) * input.size();
			outputBytes_.resize(size0 + sizeof(size_t) + realInputSize);
			bytestream_impl::writePrimitiveType(&outputBytes_[size0], &size1, sizeof(size_t));
			if(realInputSize > 0)
			{
				bytestream_impl::writeBuffer(&outputBytes_[size0 + sizeof(size_t)], input.data(), realInputSize);
			}
		}

		template <typename T, std::enable_if_t<bytestream_impl::HasIterator<T>::value, int> = 0>
		void write(const T& container)
		{
			write(container.size());
			for(auto& item : container)
			{
				write(item);
			}
		}

		template <typename T, std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, int> = 0>
		void write(T input)
		{
			size_t size0 = outputBytes_.size();
			outputBytes_.resize(size0 + sizeof(T));
			bytestream_impl::writePrimitiveType(&outputBytes_[size0], &input, sizeof(T));
		}

		template <typename... Ts>
		void writeTuple(const std::tuple<Ts...>&, std::index_sequence<sizeof...(Ts)>)
		{
		}

		template <std::size_t I, typename... Ts>
		std::enable_if_t<(I < sizeof...(Ts))> writeTuple(const std::tuple<Ts...>& input, std::index_sequence<I>)
		{
			write(std::get<I>(input));
			writeTuple(input, std::index_sequence<I + 1>());
		}

		template <typename... Ts>
		void write(const std::tuple<Ts...>& input)
		{
			writeTuple(input, std::index_sequence<0>());
		}

		template <typename T1, typename T2>
		void write(const std::pair<T1, T2>& input)
		{
			write(input.first);
			write(input.second);
		}

		template <typename T>
		void write(const std::optional<T>& input)
		{
			bool hasValue = input.has_value();
			write(hasValue);
			if(hasValue)
			{
				write(input.value());
			}
		}

		template <typename T>
		void write(const std::shared_ptr<T>& input)
		{
			types::AbstractFactory<T>::type::to_stream(input, *this);
		}

		void write(const utils::UUID& input)
		{
			size_t size0 = outputBytes_.size();
			outputBytes_.resize(size0 + sizeof(input.data));
			bytestream_impl::writeBuffer(&outputBytes_[size0], input.data, sizeof(input.data));
		}

		void write(const utils::Duration& input)
		{
			size_t size0 = outputBytes_.size();
			outputBytes_.resize(size0 + sizeof(input));
			bytestream_impl::writeBuffer(&outputBytes_[size0], &input, sizeof(input));
		}

		void write(const utils::TimePoint& input)
		{
			size_t size0 = outputBytes_.size();
			outputBytes_.resize(size0 + sizeof(input));
			bytestream_impl::writeBuffer(&outputBytes_[size0], &input, sizeof(input));
		}

		template <typename T, std::enable_if_t<bytestream_impl::HasToBytestream<T>::value, int> = 0>
		void write(const T& input)
		{
			input.toByteStream(*this);
		}

		std::vector<std::byte> outputBytes_;
	};

	class IByteStream
	{
	public:
		using Status = bytestream_impl::Status;

		IByteStream(const std::byte* buffer, size_t len) : buffer_(buffer), bufferLen_(len)
		{
		}

		IByteStream(const std::vector<std::byte>& bufferVec) : buffer_(bufferVec.data()), bufferLen_(bufferVec.size())
		{
		}

		IByteStream(const std::string_view& str_view) : buffer_(reinterpret_cast<const std::byte*>(str_view.data())), bufferLen_(str_view.size())
		{
		}

		template <typename T>
		IByteStream& operator>>(T& output)
		{
			if(read(output))
				return *this;
			status_ = Status::INVALID_READ;
			return *this;
		}

		Status getStatus() const
		{
			return status_;
		}
		bool ok() const
		{
			return status_ == Status::OK;
		}
		bool end() const
		{
			return readPtr_ == bufferLen_;
		}

	private:
		bool read(std::string& output)
		{
			size_t stringSize;
			if(!read(stringSize))
				return false;
			if(readPtr_ + stringSize > bufferLen_)
				return false;
			output.resize(stringSize);
			std::copy_n(buffer_ + readPtr_, stringSize, (std::byte*)output.data());
			readPtr_ += stringSize;
			return true;
		}

		// NOTE: reading to std::string_view is not supported
		// a string_view does not own data, which means once the bytestream
		// is out of scope, the string_view becomes invalid.

		template <typename T>
		bool read(std::vector<T>& output)
		{
			size_t vecSize;
			if(!read(vecSize))
				return false;
			output.resize(vecSize);
			if constexpr(std::is_fundamental<T>::value || std::is_enum<T>::value)
			{
				// std::copy_n at once is faster than for-loop on individual item.
				size_t to_copy = vecSize * sizeof(T);
				if(readPtr_ + to_copy > bufferLen_)
					return false;
				if(vecSize > 0)
				{
					std::copy_n(buffer_ + readPtr_, to_copy, (std::byte*)output.data());
					readPtr_ += to_copy;
				}
			}
			else
			{
				for(size_t i = 0; i < vecSize; ++i)
				{
					if(!read(output[i]))
						return false;
				}
			}
			return true;
		}

		template <typename T>
		std::enable_if_t<(std::is_fundamental<T>::value || std::is_enum<T>::value), bool> read(T& output)
		{
			if(readPtr_ + sizeof(T) > bufferLen_)
				return false;
			std::copy_n(buffer_ + readPtr_, sizeof(T), (std::byte*)&output);
			readPtr_ += sizeof(T);
			return true;
		}

		template <typename... Ts>
		bool readTuple(std::tuple<Ts...>&, std::index_sequence<sizeof...(Ts)>)
		{
			return true;
		}

		template <std::size_t I, typename... Ts>
		std::enable_if_t<(I < sizeof...(Ts)), bool> readTuple(std::tuple<Ts...>& output, std::index_sequence<I>)
		{
			return read(std::get<I>(output)) && readTuple(output, std::index_sequence<I + 1>());
		}

		template <typename... Ts>
		bool read(std::tuple<Ts...>& output)
		{
			return readTuple(output, std::index_sequence<0>());
		}

		template <typename T1, typename T2>
		bool read(std::pair<T1, T2>& output)
		{
			return read(output.first) && read(output.second);
		}

		template <typename T>
		std::enable_if_t<bytestream_impl::HasFromBytestream<T>::value, bool> read(T& output)
		{
			output.fromByteStream(*this);
			return status_ == Status::OK;
		}

		template <typename T>
		std::enable_if_t<bytestream_impl::CanInsert<T>::value, bool> read(T& output)
		{
			return readContainer(output, [](T& container, typename T::value_type&& value_type) { container.insert(std::move(value_type)); });
		}

		template <typename T>
		std::enable_if_t<bytestream_impl::CanPushBack<T>::value, bool> read(T& output)
		{
			return readContainer(output, [&](T& container, typename T::value_type&& valueType) { container.push_back(std::move(valueType)); });
		}

		template <typename T, typename Inserter>
		bool readContainer(T& output, Inserter inserter)
		{
			size_t containerSize;
			if(!read(containerSize))
				return false;
			for(size_t i = 0; i < containerSize; ++i)
			{
				typename bytestream_impl::ConstCastValueType<typename T::value_type>::type valueType;
				if(!read(valueType))
					return false;
				inserter(output, std::move(valueType));
			}
			return true;
		}

		template <typename T>
		bool read(std::optional<T>& output)
		{
			bool hasValue;
			bool result = read(hasValue);
			if(result && hasValue)
			{
				T value;
				result = read(value);
				if(result)
				{
					output = value;
				}
			}
			else
			{
				output = std::nullopt;
			}
			return result;
		}

		template <typename T>
		bool read(std::shared_ptr<T>& output)
		{
			output = std::move(types::AbstractFactory<T>::type::from_stream(*this));
			return true;
		}

		bool read(utils::UUID& output)
		{
			const auto size = output.size();
			if(readPtr_ + size > bufferLen_)
				return false;

			std::memcpy(&output, buffer_ + readPtr_, size);
			readPtr_ += size;
			return true;
		}

		bool read(utils::Duration& output)
		{
			constexpr auto size = sizeof(output);
			if(readPtr_ + size > bufferLen_)
				return false;
			std::memcpy(&output, buffer_ + readPtr_, size);
			readPtr_ += size;
			return true;
		}

		bool read(utils::TimePoint& output)
		{
			const auto size = sizeof(output);
			if(readPtr_ + size > bufferLen_)
				return false;
			std::memcpy(&output, buffer_ + readPtr_, size);
			readPtr_ += size;
			return true;
		}

		Status status_ = Status::OK;
		const std::byte* buffer_ = nullptr;
		size_t readPtr_ = 0;
		size_t bufferLen_ = 0;
	};

} // namespace {{ns_tpl}}
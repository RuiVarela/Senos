#pragma once

#include "../core/Lang.hpp"

#include <array>
#include <cstddef>
#include <iterator>

namespace sns {

	template <typename T = float, int N = 2048> 
	class CircularBuffer {
	public:
		using Type = T;

		CircularBuffer() 
			:m_size(0), m_head(0), m_tail(0) { 
				clear(); 
		}

		void clear() {
			m_size = 0;
			m_head = 0;
			m_tail = 0;
		}

		void pop_front() {
			assert(!empty());

			m_head = (m_head + 1) % capacity();
			--m_size;
		}

		void push_back(Type v) {
			m_data[m_tail] = v;
			m_tail = (m_tail + 1) % capacity();

			if (m_size == capacity()) {
				// We always accept data. when full and lose the front()
				m_head = (m_head + 1) % capacity();
			} else {
				++m_size;
			}
		}

		T &operator[](int index) {
			assert(index >= 0);
			assert(index < size());
			return m_data[(m_head + index) % capacity()];
		}

		T operator[](int index) const {
			assert(index >= 0);
			assert(index < size());
			return m_data[(m_head + index) % capacity()];
		}

		bool empty() const { return m_size == 0; }
		size_t size() const { return m_size; }
		size_t capacity() const { return m_data.size(); }

		Type front() {
			assert(!empty());
			return m_data[m_head];
		}
	private:
		std::array<Type, N> m_data;
		size_t m_size;
		size_t m_head;
		size_t m_tail;
	};
} // namespace sns
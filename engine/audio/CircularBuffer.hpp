#pragma once

#include "../core/Lang.hpp"

#include <array>


namespace sns {

	template<typename T = float, int N = 2048>
	class CircularBuffer {
	public:
		using Type = T;

		CircularBuffer()
			:m_size(0), m_head(0), m_tail(0)
		{
			clear();
		}

		void clear() {
			m_size = 0;
			m_head = 0;
			m_tail = 0;
		}

		void pop_front() {
			assert(!empty());

			m_head = (m_head + 1) % m_data.size();
			--m_size;
		}

		void push_back(Type v) {
			m_data[m_tail] = v;
			m_tail = (m_tail + 1) % m_data.size();

			if (m_size == m_data.size()) {
				// We always accept data. when full and lose the front()
				m_head = (m_head + 1) % m_data.size();
			}
			else {
				++m_size;
			}
		}

		bool empty() const {
			return m_size == 0;
		}

		size_t size() const {
			return m_size;
		}

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
}
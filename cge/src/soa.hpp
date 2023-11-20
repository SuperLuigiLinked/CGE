/**
 * @file cge/soa.hpp
 * @brief Helper functions for Data-oriented Design (Struct of Arrays).
 */

#pragma once

#include <cstdlib>

namespace soa
{
    using addr_t = decltype(sizeof(int));

    template <typename T>
    static inline constexpr addr_t alloc_one(addr_t& addr, const addr_t count) noexcept
    {
        const addr_t realign{ (alignof(T) - (addr % alignof(T))) % alignof(T) };
        const addr_t base{ addr + realign };
        addr = base + count * sizeof(T);
        return base;
    }

    template <typename T>
    static inline void offset_one(const addr_t addr, const addr_t*& offset, T*& ptr) noexcept
    {
        ptr = reinterpret_cast<T*>(addr + *offset);
        ++offset;
    };
}

namespace soa
{
    /**
     * @see C:
     * - https://en.cppreference.com/w/cpp/memory/c/realloc
     */
    template <typename data_t, typename count_t, typename... Types>
    extern bool realloc(data_t*& data, count_t& count, const count_t new_count, Types*&... ptrs) noexcept
    {
        if (new_count == count)
        {
            return true;
        }
        else if (new_count == 0)
        {
            count = 0;
            return true;
        }
        else if constexpr (sizeof...(Types) == 0)
        {
            const addr_t alloc_size{ new_count * sizeof(data_t) };

            void* const old_data{ static_cast<void*>(data) };
            void* const new_data{ std::realloc(old_data, alloc_size) };
            if (new_data == nullptr) return false;

            data = static_cast<data_t*>(new_data);
            count = new_count;

            return true;
        }
        else
        {
            addr_t alloc_size{};
            const addr_t offsets[sizeof...(Types)] { soa::alloc_one<Types>(alloc_size, addr_t(new_count))... };

            void* const old_data{ static_cast<void*>(data) };
            void* const new_data{ std::realloc(old_data, alloc_size) };
            if (new_data == nullptr) return false;

            data = static_cast<data_t*>(new_data);
            count = new_count;

            const addr_t* offset{ offsets };
            (..., soa::offset_one(addr_t(data), offset, ptrs));

            return true;
        }
    }

    /**
     * @see C:
     * - https://en.cppreference.com/w/cpp/memory/c/free
     */
    template <typename data_t, typename count_t>
    static inline void dealloc(data_t*& data, count_t& count) noexcept
    {
        std::free(static_cast<void*>(data));
        data = nullptr;
        count = 0;
    }

}

/**
 * @file cge/soa.hpp
 * @brief Helper functions for Data-oriented Design (Struct of Arrays).
 */

#pragma once

#include <cstdlib>

namespace soa
{
    using Address = std::size_t;

    template <typename T>
    static inline constexpr Address suballoc(Address& addr, const Address count) noexcept
    {
        const Address realign{ (alignof(T) - (addr % alignof(T))) % alignof(T) };
        const Address base{ addr + realign };
        addr = base + count * sizeof(T);
        return base;
    }

    template <>
    inline constexpr Address suballoc<void>(Address& addr, const Address count [[maybe_unused]]) noexcept
    {
        return addr;
    }

    template <typename T>
    static inline void offset(const Address addr, const Address*& offset, T*& ptr) noexcept
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
    template <typename Count, typename Data, typename... Subdata>
    extern bool realloc(const Count new_count, Count& count, Data*& data, Subdata*&... subdata) noexcept
    {
        if (new_count <= count)
        {
            count = new_count;
            return true;
        }
        else
        {
            Address alloc_size{};
            const Address offsets[]{
                soa::suballoc<Data>(alloc_size, Address(new_count)),
                soa::suballoc<Subdata>(alloc_size, Address(new_count))...
            };
            if (alloc_size == 0) return false;

            void* const old_data{ static_cast<void*>(data) };
            void* const new_data{ std::realloc(old_data, alloc_size) };
            if (new_data == nullptr) return false;

            count = new_count;
            data = static_cast<Data*>(new_data);

            if constexpr (sizeof...(Subdata) > 0)
            {
                const Address* offset{ offsets + 1 };
                (..., soa::offset(Address(data), offset, subdata));
            }

            return true;
        }
    }

    /**
     * @see C:
     * - https://en.cppreference.com/w/cpp/memory/c/free
     */
    template <typename Count, typename Data>
    static inline void dealloc(Count& count, Data*& data) noexcept
    {
        std::free(static_cast<void*>(data));
        data = nullptr;
        count = 0;
    }

}

#pragma once
#include <assert.h>
#include <cstring>
#include <new>
#include <type_traits>

namespace kod {
template <typename T>
struct array_allocator;

template <typename T>
using has_destructor = std::disjunction<std::has_virtual_destructor<T>, std::conjunction<std::is_destructible<T>, std::negation<std::is_trivially_destructible<T>>>>;
template <typename T>
_INLINE_VAR constexpr bool has_destructor_v = has_destructor<T>::value;

template <typename T>
struct array {
    using allocator = array_allocator<T>;

private:
    T* mdata{ nullptr };
    size_t msize{ 0 }, mcapacity{ 0 };

public:
    inline array() {}
    inline array(size_t capacity)
        : mdata{ static_cast<T*>(malloc(sizeof(T) * capacity)) }
        , mcapacity{ mdata ? capacity : 0 }
    {
    }

public:
    inline constexpr size_t size() const { return msize; }
    inline constexpr size_t capacity() const { return mcapacity; }
    inline constexpr T& front() const
    {
        assert(msize > 1);
        return mdata[0];
    }
    inline constexpr T& back() const
    {
        assert(msize > 1);
        return mdata[msize - 1];
    }

public:
    template <typename... Args>
    inline constexpr bool emplace(size_t i, Args&&... args)
    {
        /*if (i > msize) {
            return false;
        }*/

        assert(i <= msize);

        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
                if (i != (++msize) - 1) {
                    memcpy(mdata + (i + 1), mdata + i, sizeof(T) * (msize - i - 1));
                }
            } else {
                --mcapacity;
                return false;
            }
        }
        new (mdata + i) T(args...);
        return true;
    }

    template <typename... Args>
    inline constexpr bool emplace_back(Args&&... args)
    {
        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
            } else {
                --mcapacity;
                return false;
            }
        }
        new (mdata + ((++msize) - 1)) T(args...);
        return true;
    }

    template <bool unsafe = std::enable_if_t<std::is_copy_constructible_v<T>, std::is_trivially_copy_constructible<T>>::value>
    inline constexpr bool push(size_t i, const T& arg)
    {
        /*if (i > msize) {
            return false;
        }*/

        assert(i <= msize);

        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
                if (i != (++msize) - 1) {
                    memcpy(mdata + (i + 1), mdata + i, sizeof(T) * (msize - i - 1));
                }
            } else {
                --mcapacity;
                return false;
            }
        }
        //new (mdata + i) T{ arg };
        allocator::copy<unsafe>(mdata + i, arg);
        return true;
    }

    template <bool unsafe = std::enable_if_t<std::is_move_constructible_v<T>, std::is_trivially_move_constructible<T>>::value>
    inline constexpr bool push(size_t i, T&& arg)
    {
        /*if (i > msize) {
            return false;
        }*/

        assert(i <= msize);

        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
                if (i != (++msize) - 1) {
                    memcpy(mdata + (i + 1), mdata + i, sizeof(T) * (msize - i - 1));
                }
            } else {
                --mcapacity;
                return false;
            }
        }
        //new (mdata + i) T{ std::move(arg) };
        allocator::move<unsafe>(mdata + i, arg);
        return true;
    }

    template <bool unsafe = std::enable_if_t<std::is_copy_constructible_v<T>, std::is_trivially_copy_constructible<T>>::value>
    inline constexpr bool push_back(const T& arg)
    {
        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
            } else {
                --mcapacity;
                return false;
            }
        }
        //new (mdata + ((++msize) - 1)) T{ arg };
        allocator::copy<unsafe>(mdata + ((++msize) - 1), arg);
        return true;
    }

    template <bool unsafe = std::enable_if_t<std::is_move_constructible_v<T>, std::is_trivially_move_constructible<T>>::value>
    inline constexpr bool push_back(T&& arg)
    {
        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
            } else {
                --mcapacity;
                return false;
            }
        }
        //new (mdata + ((++msize) - 1)) T{ std::move(arg) };
        allocator::move<unsafe>(mdata + ((++msize) - 1), arg);
        return true;
    }

    inline constexpr T&& pop_back()
    {
        assert(msize > 1);
        return std::move(mdata[--msize]);
    }

    template <bool identity = true>
    inline constexpr size_t find(const T& arg)
    {
        for (size_t i = 0; i < msize; i++) {
            if ((mdata + i) == &arg) {
                return i;
            }
        }
        return -1;
    }

    template <>
    inline constexpr size_t find<false>(const T& arg)
    {
        for (size_t i = 0; i < msize; i++) {
            if (mdata[i] == arg) {
                return i;
            }
        }
        return -1;
    }

    template <bool sorted = true>
    inline constexpr bool remove(const T& arg)
    {
        for (size_t i = 0; i < msize; i++) {
            T* ptr = mdata + i;
            if (ptr == &arg) {
                allocator::free(ptr);
                if (i != --msize) {
                    memcpy(mdata + i, mdata + (i + 1), sizeof(T) * (msize - i));
                }
                return true;
            }
        }
        return false;
    }

    template <>
    inline constexpr bool remove<false>(const T& arg)
    {
        for (size_t i = 0; i < msize; i++) {
            T* ptr = mdata + i;
            if (ptr == &arg) {
                allocator::free(ptr);
                if (i != --msize) {
                    memcpy(mdata + i, mdata + msize, sizeof(T));
                }
                return true;
            }
        }
        return false;
    }

    template <bool sorted = true>
    inline constexpr bool removeAt(size_t i)
    {
        if (i < msize) {
            allocator::free(mdata[i]);
            if (i != --msize) {
                memcpy(mdata + i, mdata + (i + 1), sizeof(T) * (msize - i));
            }
            return true;
        }

        return false;
    }

    template <>
    inline constexpr bool removeAt<false>(size_t i)
    {
        if (i < msize) {
            allocator::free(mdata[i]);
            if (i != --msize) {
                //new (mdata + i) T{ std::move(mdata[msize]) };
                memcpy(mdata + i, mdata + msize, sizeof(T));
            }
            return true;
        }

        return false;
    }

    inline constexpr bool removeRange(size_t i, size_t j)
    {
        //if (i > j) // TODO ASSERT?
        //    return false;

        assert(i <= j);
        assert(i < msize && j < msize);

        //if (i < msize && j < msize) { // TODO ASSERT?
        if (i < msize) {
            allocator::free(mdata, i, j);
            if (i != (msize -= ((j - i) + 1))) {
                memcpy(mdata + i, mdata + (j + 1), sizeof(T) * (msize - i));
            }
            return true;
        }
        //}

        return false;
    }

    inline constexpr bool reserve(size_t value)
    {
        if ((value += msize) > mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * value)) {
                mdata = static_cast<T*>(data);
                mcapacity = value;
            } else {
                return false;
            }
        }

        return true;
    }

    inline constexpr void clear()
    {
        allocator::free(mdata, 0, msize - 1);
        msize = 0;
    }

    inline constexpr bool fit()
    {
        if (msize) {
            if (void* data = realloc(mdata, sizeof(T) * msize)) {
                mdata = static_cast<T*>(data);
                mcapacity = msize;
                return true;
            }
        } else {
            free(mdata);
            mdata = nullptr;
            return true;
        }
        return false;
    }

    inline constexpr void reset()
    {
        allocator::free(mdata, 0, msize - 1);
        free(mdata);
        mdata = nullptr;
        msize = mcapacity = 0;
    }

    inline constexpr T& operator[](size_t i) const
    {
        assert(i < msize);
        return mdata[i];
    }
    inline constexpr bool operator==(const array& arg) const { return mdata == arg.mdata; }
    inline constexpr bool operator!=(const array& arg) const { return mdata != arg.mdata; }

    inline ~array()
    {
        allocator::free(mdata, 0, msize - 1);
        free(mdata);
    }
};

template <typename T>
struct array_allocator {
    template <bool /* unsafe */>
    inline constexpr static void copy(T* const& ptr, const T& value) { new (ptr) T{ value }; }
    template <>
    inline constexpr static void copy<true>(T* const& ptr, const T& value) { memcpy(ptr, &value, sizeof(T)); }

    template <bool /* unsafe */>
    inline constexpr static void move(T* const& ptr, const T& value) { new (ptr) T{ std::move(value) }; }
    template <>
    inline constexpr static void move<true>(T* const& ptr, const T& value) { memcpy(ptr, &value, sizeof(T)); }

    template <bool = has_destructor_v<T>>
    inline constexpr static void free(T* const& data, const size_t& start, const size_t& end)
    {
        for (T* i = data + start; i <= data + end; ++i)
            i->~T();
    }
    template <>
    inline constexpr static void free<false>(T* const& data, const size_t& start, const size_t& end) {}

    template <bool = has_destructor_v<T>>
    inline constexpr static void free(T* const& data) { data->~T(); }
    template <>
    inline constexpr static void free<false>(T* const& data) {}
};
}

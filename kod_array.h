#pragma once
#include <assert.h>
#include <new>
#include <type_traits>

namespace kod {
template <typename T>
struct array_destructor {
    template <bool = std::disjunction_v<std::has_virtual_destructor<T>, std::conjunction<std::is_destructible<T>, std::negation<std::is_trivially_destructible<T>>>>>
    inline constexpr static void destroy(T* const& data)
    {
        if (data)
            data->~T();
    }
    template <>
    inline constexpr static void destroy<false>(T* const& data) {}
    template <bool = std::disjunction_v<std::has_virtual_destructor<T>, std::conjunction<std::is_destructible<T>, std::negation<std::is_trivially_destructible<T>>>>>
    inline constexpr static void destroy(T* const& data, const size_t& start, const size_t& end)
    {
        if (data) {
            for (T* i = data + start; i <= data + end; ++i) {
                i->~T();
            }
        }
    }
    template <>
    inline constexpr static void destroy<false>(T* const& data, const size_t& start, const size_t& end) {}
};
template <typename T, typename Destructor = array_destructor<T>>
struct array {
private:
    T* mdata{ nullptr };
    size_t msize{ 0 }, mcapacity{ 0 };
    //-----
public:
    inline array() {}
    inline array(size_t capacity)
        : mdata{ static_cast<T*>(malloc(sizeof(T) * capacity)) }
        , mcapacity{ mdata ? capacity : 0 }
    {
    }
    inline array(const array& arg)
        : mdata{ arg.mdata }
        , msize{ arg.msize }
        , mcapacity{ arg.mcapacity }
    {
    }
    //-----
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
    //-----
    template <typename... Args, std::enable_if_t<std::is_same_v<decltype(T{ Args{}... }), T>, int> = 0>
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
        ::new (mdata + i) T{ args... };
        return true;
    }
    template <std::enable_if_t<std::is_copy_constructible_v<T>, int> = 0>
    inline constexpr bool push(size_t i, const T& arg)
    {
        return emplace(i, arg);
    }
    template <typename... Args, std::enable_if_t<std::is_same_v<decltype(T{ Args{}... }), T>, int> = 0>
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
        ::new (mdata + ((++msize) - 1)) T{ args... };
        return true;
    }
    template <std::enable_if_t<std::is_copy_constructible_v<T>, int> = 0>
    inline constexpr bool push_back(const T& arg)
    {
        return emplace_back(arg);
    }
    //-----
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
                Destructor::destroy(ptr);
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
                Destructor::destroy(ptr);
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
            Destructor::destroy(mdata[i]);
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
            Destructor::destroy(mdata[i]);
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
            Destructor::destroy(mdata, i, j);
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
        Destructor::destroy(mdata, 0, msize - 1);
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
        Destructor::destroy(mdata, 0, msize - 1);
        free(mdata);
        mdata = nullptr;
        msize = mcapacity = 0;
    }

    inline constexpr T* const& operator+(size_t i) const { return mdata + i; }
    inline constexpr T* const& operator-(size_t i) const { return mdata - i; }

    inline constexpr T& operator[](size_t i) const
    {
        assert(i < msize);
        return mdata[i];
    }
    inline constexpr bool operator==(const array& arg) const { return mdata == arg.mdata; }
    inline constexpr bool operator!=(const array& arg) const { return mdata != arg.mdata; }
    //-----
    ~array()
    {
        Destructor::destroy(mdata, 0, msize - 1);
        free(mdata);
    }
};
}
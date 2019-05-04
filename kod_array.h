#pragma once
#include <assert.h>
#include <cstring>
#include <new>
#include <type_traits>

namespace kod {
template <typename T, typename = typename std::conditional_t<std::is_trivially_destructible_v<T>, nullptr_t, T>>
struct array {
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
                return false;
            }
        }
        new (mdata + ((++msize) - 1)) T(args...);
        return true;
    }

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
                return false;
            }
        }

        memcpy(mdata + i, &arg, sizeof(T));
        return true;
    }

    //template <typename = typename std::enable_if_t<std::is_move_constructible_v<T>, T>>
    inline constexpr bool push_back(const T& arg)
    {
        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
            } else {
                return false;
            }
        }
        memcpy(mdata + ((++msize) - 1), &arg, sizeof(T));
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
                ptr->~T();
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
                ptr->~T();
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
            mdata[i].~T();
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
            mdata[i].~T();
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
            for (size_t k = i; k <= j; ++k) {
                mdata[k].~T();
            }
            if (i != (msize -= ((j - i) + 1))) {
                memcpy(mdata + i, mdata + (j + 1), sizeof(T) * (msize - i));
            }
            return true;
        }
        //}

        return false;
    }

    inline constexpr void clear()
    {
        for (T* i = mdata + (msize - 1); i >= mdata; --i)
            i->~T();
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
        for (T* i = mdata + (msize - 1); i >= mdata; --i)
            i->~T();
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
        for (size_t i = 0; i < msize; ++i)
            mdata[i].~T();
        free(mdata);
    }
};

template <typename T>
struct array<T, nullptr_t> {
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
                return false;
            }
        }
        new (mdata + ((++msize) - 1)) T(args...);
        return true;
    }

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
                return false;
            }
        }

        memcpy(mdata + i, &arg, sizeof(T));
        return true;
    }

    //template <typename = typename std::enable_if_t<std::is_move_constructible_v<T>, T>>
    inline constexpr bool push_back(const T& arg)
    {
        if (msize == mcapacity) {
            if (void* data = realloc(mdata, sizeof(T) * (++mcapacity))) {
                mdata = static_cast<T*>(data);
            } else {
                return false;
            }
        }
        memcpy(mdata + ((++msize) - 1), &arg, sizeof(T));
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
            if (i != (msize -= ((j - i) + 1))) {
                memcpy(mdata + i, mdata + (j + 1), sizeof(T) * (msize - i));
            }
            return true;
        }
        //}

        return false;
    }

    inline constexpr void clear()
    {
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
        free(mdata);
    }
};
}
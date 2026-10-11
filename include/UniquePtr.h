#pragma once
#include <cstddef>
#include <type_traits>

template <typename T> class UniquePtr {
    private:
        T* ptr;
    public:
        UniquePtr(T* p = nullptr) : ptr(p) {}
        ~UniquePtr() { delete ptr; }

        UniquePtr(const UniquePtr& other) : ptr(copy_object(other.ptr)) {}

        UniquePtr& operator=(const UniquePtr& other) {
            if (this == &other) {
                return *this;
            }

            T* copy = copy_object(other.ptr);

            delete ptr;
            ptr = copy;
            return *this;
        }

        UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {
            other.ptr = nullptr;
        }

        UniquePtr& operator=(UniquePtr&& other) noexcept {
            if (this != &other) {
                delete ptr;
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }

        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }
        T* get() const { return ptr; }

        T* release() {
            T* tmp = ptr;
            ptr = nullptr;
            return tmp;
        }

        void reset(T* p = nullptr) {
            if (ptr != p) {
                delete ptr;
                ptr = p;
            }
        }

        template <typename U> UniquePtr(UniquePtr<U>&& other) {
            static_assert(std::is_convertible_v<U*, T*>);
            ptr = other.release();
        }

        template <typename U> UniquePtr(const UniquePtr<U>& other) : ptr(nullptr) {
            static_assert(std::is_convertible_v<U*, T*>);
            ptr = copy_object(other.get());
        }

        template <typename U> UniquePtr& operator=(UniquePtr<U>&& other) {
            static_assert(std::is_convertible_v<U*, T*>);
            reset(other.release());
            return *this;
        }

        template <typename U> static T* copy_object(const U* source) {
            if (source == nullptr) {
                return nullptr;
            }

            if constexpr (std::is_polymorphic_v<U>) {
                return source->clone();
            } else {
                return new U(*source);
            }
        }
};

// У полиморфных классов, которые нужно копировать должен быть clone

template <typename T> class UniquePtr<T[]> {
    private:
        T* ptr;

    public:
        UniquePtr(T* p = nullptr) : ptr(p) {}
        ~UniquePtr() { delete[] ptr; }

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr(UniquePtr&& other) noexcept : ptr(other.release()) {}

        UniquePtr& operator=(UniquePtr&& other) noexcept {
            if (this != &other) {
                reset(other.release());
            }
            return *this;
        }

        T& operator[](std::size_t index) const { return ptr[index]; }
        T* get() const { return ptr; }

        T* release() {
            T* tmp = ptr;
            ptr = nullptr;
            return tmp;
        }

        void reset(T* p = nullptr) {
            if (ptr != p) {
                delete[] ptr;
                ptr = p;
            }
        }
};

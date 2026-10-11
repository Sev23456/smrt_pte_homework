#pragma once
#include <cstddef>
#include <type_traits>

template <typename T> class SharedPtr {
        template <typename> friend class SharedPtr;
        T* ptr;
        std::size_t* ref_count;

        static std::size_t* make_ref_count(T* p) {
            if (!p) {
                return nullptr;
            }
            try {
                return new std::size_t(1);
            } catch (...) {
                delete p;
                throw;
            }
        }

        void release_current(T* next_ptr = nullptr, std::size_t* next_count = nullptr) {
            if (ref_count && --(*ref_count) == 0) {
                delete ptr;
                delete ref_count;
            }
            ptr = next_ptr;
            ref_count = next_count;
        }
    public:
        SharedPtr(T* p = nullptr) : ptr(p), ref_count(make_ref_count(p)) {}

        SharedPtr(const SharedPtr &other) : ptr(other.ptr), ref_count(other.ref_count) {
            if (ref_count) {
                ++(*ref_count);
            }
        }

        SharedPtr& operator=(const SharedPtr &other) {
            if (this != &other) {
                if (other.ref_count) {
                    ++(*other.ref_count);
                }
                release_current(other.ptr, other.ref_count);
            }
            return *this;
        }

        SharedPtr(SharedPtr&& other) noexcept : ptr(other.ptr), ref_count(other.ref_count) {
            other.ptr = nullptr;
            other.ref_count = nullptr;
        }

        SharedPtr& operator=(SharedPtr&& other) noexcept {
            if (this != &other) {
                release_current(other.ptr, other.ref_count);
                other.ptr = nullptr;
                other.ref_count = nullptr;
            }
            return *this;
        }

        template <typename U> SharedPtr(SharedPtr<U>&& other) noexcept
            : ptr(other.ptr), ref_count(other.ref_count) {
            static_assert(std::is_convertible_v<U*, T*>);
            other.ptr = nullptr;
            other.ref_count = nullptr;
        }

        template <typename U> SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
            static_assert(std::is_convertible_v<U*, T*>);
            release_current(other.ptr, other.ref_count);
            other.ptr = nullptr;
            other.ref_count = nullptr;
            return *this;
        }

        template <typename U> SharedPtr& operator=(const SharedPtr<U> &other) {
            static_assert(std::is_convertible_v<U*, T*>);

            if (other.ref_count) {
                ++(*other.ref_count);
            }
            release_current(other.ptr, other.ref_count);
            return *this;
        }

        ~SharedPtr() { release_current(); }

        template <typename U> SharedPtr(const SharedPtr<U>& other) : ptr(other.ptr), ref_count(other.ref_count) {
            static_assert(std::is_convertible_v<U*, T*>);

            if (ref_count) {
                ++(*ref_count);
            }
        }

        T& operator*() const { return *ptr; }
        T* operator->() const {return ptr; }
        T* get() const { return ptr; }

        void reset(T* p = nullptr) {
            if (p == ptr) {
                return;
            }
            std::size_t* next_count = make_ref_count(p);
            release_current(p, next_count);
        }
};


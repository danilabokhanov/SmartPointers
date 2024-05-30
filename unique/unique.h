#pragma once

#include "compressed_pair.h"
#include <cstddef>

template <class T>
class DefaultDeleter {
public:
    DefaultDeleter() = default;

    DefaultDeleter(const DefaultDeleter&) = default;

    template <class W>
    requires std::is_base_of_v<T, W> DefaultDeleter(DefaultDeleter<W>&&)
    noexcept {
    }

    DefaultDeleter& operator=(const DefaultDeleter&) = default;

    template <class W>
    requires std::is_base_of_v<T, W> DefaultDeleter& operator=(DefaultDeleter<W>&&) noexcept {
        return *this;
    }

    ~DefaultDeleter() = default;

    void operator()(T* p) const {
        static_assert(sizeof(T) > 0);
        static_assert(!std::is_void<T>::value);
        delete p;
    }
};

template <class T>
class DefaultDeleter<T[]> {
public:
    DefaultDeleter() = default;

    DefaultDeleter(const DefaultDeleter&) = default;

    template <class W>
    requires std::is_base_of_v<T, W> DefaultDeleter(DefaultDeleter<W>&&)
    noexcept {
    }

    DefaultDeleter& operator=(const DefaultDeleter&) = default;

    template <class W>
    requires std::is_base_of_v<T, W> DefaultDeleter& operator=(DefaultDeleter<W>&&) noexcept {
        return *this;
    }

    ~DefaultDeleter() = default;

    void operator()(T* p) const {
        static_assert(sizeof(T) > 0);
        static_assert(!std::is_void<T>::value);
        delete[] p;
    }
};

template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }
    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::move(deleter)) {
    }

    template <typename W, typename D>
    UniquePtr(UniquePtr<W, D>&& other) noexcept {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = other.Release();
        data_.GetSecond() = std::move(other.GetDeleter());
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <typename W, typename D>
    UniquePtr& operator=(UniquePtr<W, D>&& other) noexcept {
        if (Get() == static_cast<T*>(other.Get())) {
            return *this;
        }
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = other.Release();
        data_.GetSecond() = std::move(other.GetDeleter());
        return *this;
    }

    UniquePtr& operator=(const UniquePtr& other) = delete;
    UniquePtr& operator=(std::nullptr_t) {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = nullptr;
        return *this;
    }

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    T* Release() {
        T* ptr = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return ptr;
    }
    void Reset(T* ptr = nullptr) {
        T* prev_ptr = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(prev_ptr);
    }

    void Swap(UniquePtr& other) {
        std::swap(data_, other.data_);
    }

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }
    explicit operator bool() const {
        return data_.GetFirst();
    }

    typename std::add_lvalue_reference<T>::type operator*() const {
        return *data_.GetFirst();
    }
    T* operator->() const {
        return data_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> data_;
};

template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }
    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::move(deleter)) {
    }

    template <typename W, typename D>
    UniquePtr(UniquePtr<W, D>&& other) noexcept {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = other.Release();
        data_.GetSecond() = std::move(other.GetDeleter());
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <typename W, typename D>
    UniquePtr& operator=(UniquePtr<W, D>&& other) noexcept {
        if (Get() == static_cast<T*>(other.Get())) {
            return *this;
        }
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = other.Release();
        data_.GetSecond() = std::move(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(const UniquePtr& other) = delete;
    UniquePtr& operator=(std::nullptr_t) {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = nullptr;
        return *this;
    }

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    T* Release() {
        T* ptr = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return ptr;
    }

    void Reset(T* ptr = nullptr) {
        T* prev_ptr = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(prev_ptr);
    }

    void Swap(UniquePtr& other) {
        std::swap(data_, other.data_);
    }

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }
    explicit operator bool() const {
        return data_.GetFirst();
    }

    T& operator[](size_t index) const {
        return data_.GetFirst()[index];
    }

private:
    CompressedPair<T*, Deleter> data_;
};

namespace std {
template <typename T, typename Deleter = DefaultDeleter<T>>
void swap(UniquePtr<T, Deleter>& p, UniquePtr<T, Deleter>& q) {  // NOLINT
    p.Swap(q);
}
}  // namespace std

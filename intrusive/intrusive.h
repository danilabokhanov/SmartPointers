#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        return ++count_;
    }
    size_t DecRef() {
        return --count_;
    }
    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.

    RefCounted() = default;

    RefCounted(RefCounted&) {
    }

    RefCounted(RefCounted&&) {
    }

    RefCounted& operator=(RefCounted&) {
        return *this;
    }

    RefCounted& operator=(RefCounted&&) {
        return *this;
    }

    ~RefCounted() = default;

    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (!counter_.DecRef()) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() {
    }
    IntrusivePtr(std::nullptr_t) {
    }

    IntrusivePtr(T* p) : ptr_(p) {
        IncRef();
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) : ptr_(other.ptr_) {
        IncRef();
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) : ptr_(other.ptr_) {
        IncRef();
    }
    IntrusivePtr(IntrusivePtr&& other) : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (this == &other) {
            return *this;
        }
        DecRef();
        ptr_ = other.ptr_;
        IncRef();
        return *this;
    }

    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (this == &other) {
            return *this;
        }
        DecRef();
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        return *this;
    }
    // Destructor
    ~IntrusivePtr() {
        DecRef();
    }

    // Modifiers
    void Reset() {
        DecRef();
        ptr_ = nullptr;
    }

    void Reset(T* ptr) {
        DecRef();
        ptr_ = ptr;
        IncRef();
    }
    void Swap(IntrusivePtr& other) {
        std::swap(ptr_, other.ptr_);
    }

    // Observers
    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        return RefCount();
    }
    explicit operator bool() const {
        return ptr_;
    }

    template <typename Y, typename... Args>
    friend IntrusivePtr<Y> MakeIntrusive(Args&&... args);

private:
    T* ptr_ = nullptr;

    void IncRef() {
        if (ptr_) {
            ptr_->IncRef();
        }
    }

    void DecRef() {
        if (ptr_) {
            ptr_->DecRef();
        }
    }
    size_t RefCount() const {
        if (ptr_) {
            return ptr_->RefCount();
        }
        return 0;
    }
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    IntrusivePtr<T> res(new T(args)...);
    return res;
}

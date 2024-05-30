#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class ControlBlock {
public:
    virtual void IncreaseCounter() = 0;
    virtual void DecreaseCounter() = 0;
    virtual void* GetPointer() = 0;
    virtual size_t GetCounter() const = 0;
    ControlBlock() = default;
    virtual ~ControlBlock() {
    }
};

template <typename T>
class ControlBlockPointer : public ControlBlock {
public:
    ControlBlockPointer(T* ptr) : ControlBlock(), cnt_(1), ptr_(ptr) {
    }

    void IncreaseCounter() override {
        ++cnt_;
    }

    void DecreaseCounter() override {
        if (!--cnt_) {
            delete ptr_;
            delete this;
        }
    }

    void* GetPointer() override {
        return ptr_;
    }

    size_t GetCounter() const override {
        return cnt_;
    }

private:
    size_t cnt_ = 0;
    T* ptr_ = nullptr;
};

template <typename T>
class ControlBlockObject : public ControlBlock {
public:
    template <typename... Args>
    ControlBlockObject(Args&&... args)
        : ControlBlock(), cnt_(1), obj_(std::forward<Args>(args)...) {
    }

    void IncreaseCounter() override {
        ++cnt_;
    }

    void DecreaseCounter() override {
        if (!--cnt_) {
            delete this;
        }
    }

    void* GetPointer() override {
        return &obj_;
    }

    size_t GetCounter() const override {
        return cnt_;
    }

private:
    size_t cnt_ = 0;
    T obj_;
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
    }
    SharedPtr(std::nullptr_t) {
    }
    explicit SharedPtr(T* ptr) : cb_(new ControlBlockPointer(ptr)), observed_(ptr) {
    }

    template <class Y>
    requires std::is_base_of_v<T, Y>
    explicit SharedPtr(Y* ptr) : cb_(new ControlBlockPointer(ptr)), observed_(ptr) {
    }

    SharedPtr(const SharedPtr& other) : cb_(other.cb_), observed_(other.observed_) {
        IncreaseCBCounter();
    }

    SharedPtr(SharedPtr&& other) : cb_(other.cb_), observed_(other.observed_) {
        other.cb_ = nullptr;
        other.observed_ = nullptr;
    }

    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : cb_(other.cb_), observed_(other.observed_) {
        IncreaseCBCounter();
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) : cb_(other.cb_), observed_(other.observed_) {
        other.cb_ = nullptr;
        other.observed_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : cb_(other.cb_), observed_(ptr) {
        IncreaseCBCounter();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        DecreaseCBCounter();
        cb_ = other.cb_;
        observed_ = other.observed_;
        IncreaseCBCounter();
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }
        DecreaseCBCounter();
        cb_ = other.cb_;
        observed_ = other.observed_;
        other.cb_ = nullptr;
        other.observed_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        DecreaseCBCounter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        DecreaseCBCounter();
        cb_ = nullptr;
        observed_ = nullptr;
    }

    void Reset(T* ptr) {
        DecreaseCBCounter();
        cb_ = new ControlBlockPointer(ptr);
        observed_ = ptr;
    }

    template <typename Y>
    void Reset(Y* ptr) {
        DecreaseCBCounter();
        cb_ = new ControlBlockPointer(ptr);
        observed_ = ptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(cb_, other.cb_);
        std::swap(observed_, other.observed_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return reinterpret_cast<T*>(observed_);
    }
    T& operator*() const {
        return *reinterpret_cast<T*>(observed_);
    }

    T* operator->() const {
        return reinterpret_cast<T*>(observed_);
    }
    size_t UseCount() const {
        return GetCBCounter();
    }
    explicit operator bool() const {
        return reinterpret_cast<T*>(observed_);
    }

    template <typename W, typename... Args>
    friend SharedPtr<W> MakeShared(Args&&... args);

private:
    void IncreaseCBCounter() const {
        if (cb_) {
            cb_->IncreaseCounter();
        }
    }

    void DecreaseCBCounter() const {
        if (cb_) {
            cb_->DecreaseCounter();
        }
    }

    size_t GetCBCounter() const {
        if (cb_) {
            return cb_->GetCounter();
        }
        return 0;
    }

    ControlBlock* cb_ = nullptr;
    void* observed_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename W, typename... Args>
SharedPtr<W> MakeShared(Args&&... args) {
    SharedPtr<W> res{};
    res.cb_ = new ControlBlockObject<W>(std::forward<Args>(args)...);
    res.observed_ = reinterpret_cast<W*>(res.cb_->GetPointer());
    return res;
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};

#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class ControlBlock {
public:
    virtual void IncreaseSharedCounter() = 0;
    virtual void DecreaseSharedCounter() = 0;
    virtual void IncreaseWeakCounter() = 0;
    virtual void DecreaseWeakCounter() = 0;
    virtual void* GetPointer() = 0;
    virtual size_t GetSharedCounter() const = 0;
    ControlBlock() = default;
    virtual ~ControlBlock() {
    }
};

template <typename T>
class ControlBlockPointer : public ControlBlock {
public:
    ControlBlockPointer(T* ptr) : ControlBlock(), shared_cnt_(1), weak_cnt_(0), ptr_(ptr) {
    }

    void IncreaseSharedCounter() override {
        ++shared_cnt_;
    }

    void DecreaseSharedCounter() override {
        if (!--shared_cnt_) {
            delete ptr_;
        }
        if (!weak_cnt_ && !shared_cnt_) {
            delete this;
        }
    }

    void IncreaseWeakCounter() override {
        ++weak_cnt_;
    }

    void DecreaseWeakCounter() override {
        if (!--weak_cnt_ && !shared_cnt_) {
            delete this;
        }
    }

    void* GetPointer() override {
        return ptr_;
    }

    size_t GetSharedCounter() const override {
        return shared_cnt_;
    }

private:
    size_t shared_cnt_ = 0, weak_cnt_ = 0;
    T* ptr_ = nullptr;
};

template <typename T>
class ControlBlockObject : public ControlBlock {
public:
    template <typename... Args>
    ControlBlockObject(Args&&... args) : ControlBlock(), shared_cnt_(1), weak_cnt_(0) {
        new (&obj_) T(std::forward<Args>(args)...);
    }

    void IncreaseSharedCounter() override {
        ++shared_cnt_;
    }

    void DecreaseSharedCounter() override {
        if (!--shared_cnt_) {
            reinterpret_cast<T*>(&obj_)->~T();
        }
        if (!weak_cnt_ && !shared_cnt_) {
            delete this;
        }
    }

    void IncreaseWeakCounter() override {
        ++weak_cnt_;
    }

    void DecreaseWeakCounter() override {
        if (!--weak_cnt_ && !shared_cnt_) {
            delete this;
        }
    }

    void* GetPointer() override {
        return &obj_;
    }

    size_t GetSharedCounter() const override {
        return shared_cnt_;
    }

private:
    size_t shared_cnt_, weak_cnt_ = 0;
    std::aligned_storage_t<sizeof(T), alignof(T)> obj_;
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

    template <typename W>
    friend class WeakPtr;

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
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        cb_ = other.cb_;
        observed_ = other.observed_;
        IncreaseCBCounter();
    }

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
            cb_->IncreaseSharedCounter();
        }
    }

    void DecreaseCBCounter() const {
        if (cb_) {
            cb_->DecreaseSharedCounter();
        }
    }

    size_t GetCBCounter() const {
        if (cb_) {
            return cb_->GetSharedCounter();
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

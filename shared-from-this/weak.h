#pragma once

#include "shared.h"
#include "sw_fwd.h"  // Forward declaration

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
    }

    WeakPtr(const WeakPtr& other) : cb_(other.cb_), observed_(other.observed_) {
        IncreaseCBCounter();
    }

    WeakPtr(WeakPtr&& other) : cb_(other.cb_), observed_(other.observed_) {
        other.cb_ = nullptr;
        other.observed_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) : cb_(other.cb_), observed_(other.observed_) {
        if (other.operator bool()) {
            IncreaseCBCounter();
        }
    }

    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other) : cb_(other.cb_), observed_(other.observed_) {
        IncreaseCBCounter();
    }

    template <typename Y>
    WeakPtr(WeakPtr<Y>&& other) : cb_(other.cb_), observed_(other.observed_) {
        other.cb_ = nullptr;
        other.observed_ = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        DecreaseCBCounter();
        cb_ = other.cb_;
        observed_ = other.observed_;
        IncreaseCBCounter();
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
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

    WeakPtr& operator=(const SharedPtr<T>& other) {
        if (cb_ == other.cb_) {
            return *this;
        }
        DecreaseCBCounter();
        cb_ = other.cb_;
        observed_ = other.observed_;
        IncreaseCBCounter();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        DecreaseCBCounter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        DecreaseCBCounter();
        cb_ = nullptr;
        observed_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(cb_, other.cb_);
        std::swap(observed_, other.observed_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        return GetCBCounter();
    }

    bool Expired() const {
        return !GetCBCounter();
    }

    SharedPtr<T> Lock() const {
        SharedPtr<T> res{};
        if (Expired()) {
            return res;
        }
        res.cb_ = cb_;
        res.observed_ = observed_;
        res.IncreaseCBCounter();
        return res;
    }

    template <typename Y>
    friend class SharedPtr;

    template <typename W>
    friend class WeakPtr;

private:
    ControlBlock* cb_ = nullptr;
    void* observed_ = nullptr;

    void IncreaseCBCounter() const {
        if (cb_) {
            cb_->IncreaseWeakCounter();
        }
    }

    void DecreaseCBCounter() const {
        if (cb_) {
            cb_->DecreaseWeakCounter();
        }
    }

    size_t GetCBCounter() const {
        if (cb_) {
            return cb_->GetSharedCounter();
        }
        return 0;
    }
};

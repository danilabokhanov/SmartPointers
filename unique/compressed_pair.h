#pragma once
#include <utility>
#include <type_traits>

template <class T>
concept compressed = !std::is_final_v<T> && std::is_empty_v<T>;

template <typename F, typename S, typename W = void>
class CompressedPair;

template <typename F, typename S>
class CompressedPair<
    F, S,
    typename std::enable_if<(compressed<F> && compressed<S> && !std::is_same_v<F, S>) ||
                            (std::is_same_v<F, S> && !compressed<F>)>::type> : F,
                                                                               S {
public:
    CompressedPair() : F(), S() {
    }
    template <typename P, typename Q>
    CompressedPair(P&& first, Q&& second) : F(std::forward<P>(first)), S(std::forward<Q>(second)) {
    }

    F& GetFirst() {
        return *this;
    }

    S& GetSecond() {
        return *this;
    };

    const F& GetFirst() const {
        return *this;
    }

    const S& GetSecond() const {
        return *this;
    };
};

template <typename F, typename S>
class CompressedPair<
    F, S,
    typename std::enable_if<(!compressed<F> && compressed<S> && !std::is_same_v<F, S>) ||
                            (std::is_same_v<F, S> && compressed<S>)>::type> : S {
public:
    CompressedPair() : S(), first_() {
    }
    template <typename P, typename Q>
    CompressedPair(P&& first, Q&& second)
        : S(std::forward<Q>(second)), first_(std::forward<P>(first)) {
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return *this;
    };

    const S& GetSecond() const {
        return *this;
    };

private:
    F first_;
};

template <typename F, typename S>
class CompressedPair<
    F, S, typename std::enable_if<compressed<F> && !compressed<S> && !std::is_same_v<F, S>>::type>
    : F {
public:
    CompressedPair() : F(), second_() {
    }
    template <typename P, typename Q>
    CompressedPair(P&& first, Q&& second)
        : F(std::forward<P>(first)), second_(std::forward<Q>(second)) {
    }

    F& GetFirst() {
        return *this;
    }

    const F& GetFirst() const {
        return *this;
    }

    S& GetSecond() {
        return second_;
    };

    const S& GetSecond() const {
        return second_;
    };

private:
    S second_;
};

template <typename F, typename S>
class CompressedPair<
    F, S,
    typename std::enable_if<!compressed<F> && !compressed<S> && !std::is_same_v<F, S>>::type> {
public:
    CompressedPair() : first_(), second_() {
    }
    template <typename P, typename Q>
    CompressedPair(P&& first, Q&& second)
        : first_(std::forward<P>(first)), second_(std::forward<Q>(second)) {
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return second_;
    };

    const S& GetSecond() const {
        return second_;
    };

private:
    F first_;
    S second_;
};

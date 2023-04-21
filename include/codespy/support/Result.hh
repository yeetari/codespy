#pragma once

#include <codespy/support/Utility.hh>
#include <codespy/support/Variant.hh>

#include <cassert>
#include <cstdlib>
#include <utility>

// TODO: Find a way to avoid using statement-expressions?

#define CODESPY_ASSUME(expr, ...)                                                                                      \
    codespy::maybe_unwrap(({                                                                                           \
        auto _result_assume = (expr);                                                                                  \
        assert(!_result_assume.is_error() __VA_OPT__(, ) __VA_ARGS__);                                                 \
        _result_assume.disown_value();                                                                                 \
    }))

#define CODESPY_EXPECT(expr, ...)                                                                                      \
    codespy::maybe_unwrap(({                                                                                           \
        auto _result_expect = (expr);                                                                                  \
        if (_result_expect.is_error()) {                                                                               \
            std::abort();                                                                                              \
        }                                                                                                              \
        _result_expect.disown_value();                                                                                 \
    }))

#define CODESPY_TRY(expr)                                                                                              \
    codespy::maybe_unwrap(({                                                                                           \
        auto _result_try = (expr);                                                                                     \
        if (_result_try.is_error()) {                                                                                  \
            return _result_try.error();                                                                                \
        }                                                                                                              \
        _result_try.disown_value();                                                                                    \
    }))

namespace codespy {

template <typename T, typename... Es>
class [[nodiscard]] Result {
    static constexpr bool is_void = is_same<decay<T>, void>;
    using storage_t = conditional<is_void, char, conditional<is_ref<T>, RefWrapper<remove_ref<T>>, T>>;
    Variant<storage_t, Es...> m_value;

public:
    // clang-format off
    // NOLINTNEXTLINE
    Result() requires (is_void) : m_value(storage_t{}) {}
    // clang-format on

    template <ContainsType<T, Es...> U>
    Result(const U &value) : m_value(value) {}
    template <ContainsType<T, Es...> U>
    Result(U &&value) : m_value(std::move(value)) {}

    Result(Variant<Es...> &&error) : m_value(std::move(error)) {}

    template <typename U>
    Result(U &ref)
        requires(is_ref<T> && !is_void)
        : m_value(codespy::ref(ref)) {}
    template <typename U>
    Result(const U &ref)
        requires(is_ref<T> && !is_void)
        : m_value(codespy::cref(ref)) {}

    Result(const Result &) = delete;
    Result(Result &&) = delete;
    ~Result() = default;

    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

    bool is_error() const { return m_value.index() > 0; }
    auto disown_value() {
        assert(!is_error());
        return std::move(m_value.template get<storage_t>());
    }
    auto error() const {
        assert(is_error());
        return m_value.template downcast<Es...>();
    }
    auto &value() {
        assert(!is_error());
        return codespy::maybe_unwrap(m_value.template get<storage_t>());
    }
    const auto &value() const {
        assert(!is_error());
        return codespy::maybe_unwrap(m_value.template get<storage_t>());
    }
};

} // namespace codespy

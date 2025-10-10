#ifndef ALIAS_HPP
#define ALIAS_HPP

#include "Global.h"
#include <string_view>

namespace vfspp
{

class Alias
{
public:
    Alias();
    explicit Alias(std::string_view alias);

    static Alias Root();

    const std::string& String() const noexcept;
    std::string_view View() const noexcept;
    size_t Length() const noexcept;

    bool operator==(const Alias& other) const noexcept;
    bool operator!=(const Alias& other) const noexcept;

    struct Hash
    {
        size_t operator()(const Alias& alias) const noexcept
        {
            return alias.HashValue();
        }
    };

private:
    static std::string Normalize(std::string_view alias);
    size_t HashValue() const noexcept;

private:
    std::string m_Value;
};

inline Alias::Alias()
    : m_Value(Normalize("/"))
{
}

inline Alias::Alias(std::string_view alias)
    : m_Value(Normalize(alias))
{
}

inline Alias Alias::Root()
{
    return Alias("/");
}

inline const std::string& Alias::String() const noexcept
{
    return m_Value;
}

inline std::string_view Alias::View() const noexcept
{
    return m_Value;
}

inline size_t Alias::Length() const noexcept
{
    return m_Value.length();
}

inline bool Alias::operator==(const Alias& other) const noexcept
{
    return m_Value == other.m_Value;
}

inline bool Alias::operator!=(const Alias& other) const noexcept
{
    return !(*this == other);
}

inline std::string Alias::Normalize(std::string_view alias)
{
    std::string normalized(alias);

    const char* whitespace = " \t\n\r";
    size_t begin = normalized.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        normalized.clear();
    } else if (begin > 0) {
        normalized.erase(0, begin);
    }

    if (!normalized.empty()) {
        size_t end = normalized.find_last_not_of(whitespace);
        if (end != std::string::npos && end + 1 < normalized.size()) {
            normalized.erase(end + 1);
        }
    }

    if (normalized.empty()) {
        normalized = "/";
    }

    if (normalized.front() != '/') {
        normalized.insert(normalized.begin(), '/');
    }

    while (normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }

    if (normalized.back() != '/') {
        normalized.push_back('/');
    }

    return normalized;
}

inline size_t Alias::HashValue() const noexcept
{
    return std::hash<std::string>{}(m_Value);
}

} // namespace vfspp

#endif // ALIAS_HPP

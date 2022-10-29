#pragma once
#include "SmallVector.h"
#include "Try.h"

namespace Ty {

template <typename Key, typename Value, u32 capacity = 16>
struct SmallMap {
    constexpr SmallMap() = default;

    constexpr ErrorOr<void> append(Key key, Value value) requires(
        std::is_trivially_copyable_v<Key>and
            std::is_trivially_copyable_v<Value>)
    {
        TRY(m_keys.append(key));
        TRY(m_values.append(value));

        return {};
    }

    constexpr ErrorOr<void> append(Key&& key, Value value) requires(
        !std::is_trivially_copyable_v<
            Key> and std::is_trivially_copyable_v<Value>)
    {
        TRY(m_keys.append(std::move(key)));
        TRY(m_values.append(value));

        return {};
    }

    constexpr ErrorOr<void> append(Key key, Value&& value) requires(
        std::is_trivially_copyable_v<
            Key> and !std::is_trivially_copyable_v<Value>)
    {
        TRY(m_keys.append(key));
        TRY(m_values.append(std::move(value)));

        return {};
    }

    constexpr ErrorOr<void>
    append(Key&& key, Value&& value) requires(
        !std::is_trivially_copyable_v<
            Key> and !std::is_trivially_copyable_v<Value>)
    {
        TRY(m_keys.append(std::move(key)));
        TRY(m_values.append(std::move(value)));

        return {};
    }

    constexpr Id<Value> find(Key const& key) const
    {
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return Id<Value>(i);
        }
        return Id<Value>::invalid();
    }

    constexpr Value const& operator[](Id<Value> id) const
    {
        return m_values[id];
    }

    constexpr Value& operator[](Id<Value> id)
    {
        return m_values[id];
    }

private:
    SmallVector<Key, capacity> m_keys {};
    SmallVector<Value, capacity> m_values {};
};

}

using namespace Ty;

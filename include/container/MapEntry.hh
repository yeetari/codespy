#pragma once

#include <support/Hash.hh>

namespace jamf {

template <typename K, typename V>
struct MapEntry {
    K key;
    V value;

    bool operator==(const MapEntry &other) const { return key == other.key; }
};

template <typename K, typename V>
struct Hash<MapEntry<K, V>> {
    hash_t operator()(const MapEntry<K, V> &entry, hash_t seed) const { return hash_of(entry.key, seed); }
};

} // namespace jamf

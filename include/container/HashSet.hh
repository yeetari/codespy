#pragma once

#include <container/Array.hh>
#include <vull/maths/Common.hh>
#include <support/Hash.hh>
#include <support/Optional.hh>
#include <support/Utility.hh>

namespace jamf {

template <typename T>
class HashSet {
    struct Bucket {
        Bucket *next{nullptr};
        AlignedStorage<T> storage;

        Bucket() = default;
        Bucket(const Bucket &) = delete;
        Bucket(Bucket &&) = delete;
        ~Bucket() {
            delete next;
            storage.release();
        }

        Bucket &operator=(const Bucket &) = delete;
        Bucket &operator=(Bucket &&) = delete;

        void append(T &&elem) {
            if (next != nullptr) {
                next->append(move(elem));
                return;
            }
            do_append(move(elem));
        }
        void do_append(T &&elem) {
            assert(next == nullptr);
            next = new Bucket;
            next->storage.set(move(elem));
        }
    };

    template <bool IsConst>
    class Iterator {
        friend HashSet;

    private:
        Bucket *m_root_bucket;
        Bucket *m_bucket;
        Bucket *m_end_bucket;

        Iterator(Bucket *bucket, Bucket *end_bucket)
            : m_root_bucket(bucket), m_bucket(bucket), m_end_bucket(end_bucket) {}

        void skip_to_next() {
            if (m_bucket == nullptr) [[unlikely]] {
                return;
            }
            m_bucket = m_bucket->next;
            if (m_bucket == nullptr) {
                m_bucket = ++m_root_bucket;
                if (m_bucket == m_end_bucket) {
                    return;
                }
                skip_to_next();
            }
        }

    public:
        bool operator==(const Iterator &other) const { return m_bucket == other.m_bucket; }
        Iterator &operator++() {
            skip_to_next();
            return *this;
        }
        T &operator*() const { return m_bucket->storage.get(); }
    };

    Bucket *m_buckets{nullptr};
    size_t m_capacity{0};
    size_t m_size{0};

    void insert(T &&elem);

public:
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    HashSet() = default;
    HashSet(const HashSet &) = delete;
    HashSet(HashSet &&);
    ~HashSet();

    HashSet &operator=(const HashSet &) = delete;
    HashSet &operator=(HashSet &&) = delete;

    void clear();
    bool ensure_capacity(size_t capacity);
    void rehash(size_t capacity);

    Optional<T &> add(const T &elem);
    Optional<T &> add(T &&elem);
    bool contains(const T &elem) const;
    template <typename EqualFn>
    Optional<T &> find_hash(hash_t hash, EqualFn equal_fn) const;

    iterator begin() { return ++iterator(m_buckets, m_buckets + m_capacity); }
    iterator end() { return {m_buckets + m_capacity, nullptr}; }
    const_iterator begin() const { return ++const_iterator(m_buckets, m_buckets + m_capacity); }
    const_iterator end() const { return {m_buckets + m_capacity, nullptr}; }

    bool empty() const { return m_size == 0; }
    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }
};

template <typename T>
HashSet<T>::HashSet(HashSet &&other) {
    m_buckets = exchange(other.m_buckets, nullptr);
    m_capacity = exchange(other.m_capacity, 0u);
    m_size = exchange(other.m_size, 0u);
}

template <typename T>
HashSet<T>::~HashSet() {
    clear();
}

template <typename T>
void HashSet<T>::insert(T &&elem) {
    auto &bucket = m_buckets[hash_of(elem) % m_capacity];
    bucket.append(move(elem));
}

template <typename T>
void HashSet<T>::clear() {
    m_size = 0;
    m_capacity = 0;
    delete[] exchange(m_buckets, nullptr);
}

template <typename T>
bool HashSet<T>::ensure_capacity(size_t capacity) {
    if (capacity > m_capacity) {
        rehash(max(m_capacity * 2 + 1, capacity));
        return true;
    }
    return false;
}

template <typename T>
void HashSet<T>::rehash(size_t capacity) {
    assert(capacity >= m_size);
    auto *old_buckets = exchange(m_buckets, new Bucket[capacity]);
    auto old_capacity = exchange(m_capacity, capacity);

    // Default construct root bucket elements.
    if constexpr (!is_trivially_copyable<T> || !is_trivially_destructible<T>) {
        for (size_t i = 0; i < m_capacity; i++) {
            m_buckets[i].storage.emplace();
        }
    }

    // Move old elements;
    for (size_t i = 0; i < old_capacity; i++) {
        for (auto *bucket = old_buckets[i].next; bucket != nullptr; bucket = bucket->next) {
            insert(move(bucket->storage.get()));
        }
    }
    delete[] old_buckets;
}

template <typename T>
Optional<T &> HashSet<T>::add(const T &elem) {
    return add(T(elem));
}

template <typename T>
Optional<T &> HashSet<T>::add(T &&elem) {
    Bucket *bucket = nullptr;
    if (!empty()) {
        bucket = &m_buckets[hash_of(elem) % m_capacity];
        while (true) {
            if (bucket->next == nullptr) {
                break;
            }
            bucket = bucket->next;
            if (elem == bucket->storage.get()) {
                return bucket->storage.get();
            }
        }
    }
    if (ensure_capacity(m_size + 1)) {
        // Rehash took place.
        insert(move(elem));
    } else {
        assert(bucket != nullptr);
        bucket->do_append(move(elem));
    }
    m_size++;
    return {};
}

template <typename T>
bool HashSet<T>::contains(const T &elem) const {
    return !!find_hash(hash_of(elem), [&](const T &other) {
        return elem == other;
    });
}

template <typename T>
template <typename EqualFn>
Optional<T &> HashSet<T>::find_hash(hash_t hash, EqualFn equal_fn) const {
    if (empty()) {
        return {};
    }
    auto &root_bucket = m_buckets[hash % m_capacity];
    for (auto *bucket = root_bucket.next; bucket != nullptr; bucket = bucket->next) {
        if (auto &elem = bucket->storage.get(); equal_fn(elem)) {
            return elem;
        }
    }
    return {};
}

} // namespace jamf

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
#include <cassert>
#include <unordered_set>
#include <algorithm>

#define TABLE_SIZE 32768
#define WRITE_RATIO 0.75
#define READ_RATIO 0.5
#define WRITE_SIZE (size_t)(TABLE_SIZE * WRITE_RATIO)
#define TYPE int64_t

TYPE test[TABLE_SIZE];

void fill_test_array() {
    for (size_t i = 0; i < TABLE_SIZE; i++) {
        test[i] = rand() % (1 << 31);
    }
}

enum class EntryState : uint8_t {
    EMPTY,
    OCCUPIED,
    DELETED
};

template<typename T>
class Entry {
public:
    T key;
    EntryState state;

    Entry() : key(0), state(EntryState::EMPTY) {}
    Entry(T key) : key(key), state(EntryState::OCCUPIED) {}
};

template<typename T, size_t N>
class OpenAddressingTable {
public:
    size_t occupied_count;
    Entry<T>* entries;
    std::hash<T> hasher;

    inline size_t hash_function(const T& key, size_t count) {
        return (hasher(key) + count) % N;
    }

    OpenAddressingTable() : occupied_count(0) {
        entries = new Entry<T>[N];
    }

    ~OpenAddressingTable() {
        delete[] entries;
    }

    inline size_t remaining_size() {
        return N - occupied_count;
    }

    T& get(const T& key) {
        for (size_t i = 0; i < N; i++) {
            size_t hash = hash_function(key, i);

            if (entries[hash].state == EntryState::EMPTY || entries[hash].state == EntryState::DELETED)
                throw std::runtime_error("key not found");

            if (entries[hash].state == EntryState::OCCUPIED && entries[hash].key == key)
                return entries[hash].key;
        }

        throw std::runtime_error("key not found");
    }

    void insert(const T& key) {
        if (occupied_count == N)
            throw std::runtime_error("table is full");

        for (size_t i = 0; i < N; i++) {
            size_t hash = hash_function(key, i);

            if (entries[hash].state == EntryState::OCCUPIED && entries[hash].key == key)
                throw std::runtime_error("key already exists");

            if (entries[hash].state == EntryState::EMPTY || entries[hash].state == EntryState::DELETED) {
                entries[hash].key = key;
                entries[hash].state = EntryState::OCCUPIED;
                occupied_count++;
                return;
            }
        }

        throw std::runtime_error("table is full");
    }

    void remove(const T& key) {
        for (size_t i = 0; i < N; i++) {
            size_t hash = hash_function(key, i);

            if (entries[hash].state == EntryState::EMPTY)
                throw std::runtime_error("key not found");

            if (entries[hash].state == EntryState::OCCUPIED && entries[hash].key == key) {
                entries[hash].state = EntryState::DELETED;
                occupied_count--;
                return;
            }
        }

        throw std::runtime_error("key not found");
    }
};

// const size_t MAIN_SIZE = 1 << 13;
const size_t SUB_SIZE = 2;

template<typename T, size_t N>
class TinyStorage {
public:
    Entry<T>* fast_table;
    OpenAddressingTable<T, N> overflow_data;
    size_t fast_table_size;

    std::hash<T> hasher;

    TinyStorage() {
        size_t target_bits = ceil(log2(N)) - 1;

        fast_table_size = 1 << target_bits;
        fast_table = new Entry<T>[fast_table_size];
    }

    ~TinyStorage() {
        delete[] fast_table;
    }

    inline size_t hash_function(const T& key) {
        return (hasher(key)) % fast_table_size;
    }

    T& get(const T& key) {
        size_t bucket_idx = hash_function(key);
        for (size_t slot_idx = 0; slot_idx < SUB_SIZE; slot_idx++) {
            if (fast_table[bucket_idx].key == key) {
                return fast_table[bucket_idx].key;
            }
        }
        
        return overflow_data.get(key);
    }

    void insert(const T& key) {
        size_t bucket_idx = hash_function(key);
        for (size_t slot_idx = 0; slot_idx < SUB_SIZE; slot_idx++) {
            if (fast_table[bucket_idx].state == EntryState::OCCUPIED) {
                continue;
            }

            fast_table[bucket_idx].key = key;
            fast_table[bucket_idx].state = EntryState::OCCUPIED;
            return;
        }

        overflow_data.insert(key);
    }

    void remove(const T& key) {
        size_t bucket_idx = hash_function(key);
        for (size_t slot_idx = 0; slot_idx < SUB_SIZE; slot_idx++) {
            if (fast_table[bucket_idx].state == EntryState::OCCUPIED && fast_table[bucket_idx].key == key) {
                fast_table[bucket_idx].state = EntryState::DELETED;
                return;
            }
        }

        overflow_data.remove(key);
    }
};

void test_open_addressing_table() {
    OpenAddressingTable<TYPE, TABLE_SIZE> table{};

    for (size_t i = 0; i < WRITE_SIZE; i++)
        table.insert(test[i]);

    for (size_t i = 0; i < WRITE_SIZE * READ_RATIO; i++)
        assert(table.get(test[i]) == test[i]);
}

void test_open_addressing_2x_table() {
    OpenAddressingTable<TYPE, TABLE_SIZE * 2> table{};

    for (size_t i = 0; i < WRITE_SIZE; i++)
        table.insert(test[i]);

    for (size_t i = 0; i < WRITE_SIZE * READ_RATIO; i++)
        assert(table.get(test[i]) == test[i]);
}


void test_tiny_storage() {
    TinyStorage<TYPE, TABLE_SIZE> table{};

    for (size_t i = 0; i < WRITE_SIZE; i++)
        table.insert(test[i]);

    for (size_t i = 0; i < WRITE_SIZE * READ_RATIO; i++)
        assert(table.get(test[i]) == test[i]);
}

void test_unordered_set() {
    std::unordered_set<TYPE> table { test, test + WRITE_SIZE };

    for (size_t i = 0; i < WRITE_SIZE * READ_RATIO; i++)
        assert(table.find(test[i]) != table.end());
}

long long measure_time(std::function<void()> func, size_t times) {
    fill_test_array();

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < times; i++) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

void stat_time(std::function<void()> func, size_t batches, size_t times) {
    std::vector<long long> stats;
    for (size_t i = 0; i < batches; i++) {
        stats.push_back(measure_time(func, times));
    }
    std::sort(stats.begin(), stats.end());
    
    size_t min_time = stats[0];
    size_t max_time = stats[stats.size() - 1];
    size_t median_index = stats.size() / 2;
    size_t median_time = stats[median_index];
    size_t average_time = std::accumulate(stats.begin(), stats.end(), 0) / stats.size();

    std::cout << "Min/Max/Median/Average time: "
              << min_time << "/"
              << max_time << "/"
              << median_time << "/"
              << average_time << "us" << std::endl;
}

int main() {
    // fill the test array with random numbers
    srand(42);

    std::cout << "Open Addressing Table" << std::endl;
    stat_time(test_open_addressing_table, 100, 1000);

    std::cout << "Open Addressing 2x Table" << std::endl;
    stat_time(test_open_addressing_2x_table, 100, 1000);

    std::cout << "Tiny Storage" << std::endl;
    stat_time(test_tiny_storage, 100, 1000);
    return 0;
}

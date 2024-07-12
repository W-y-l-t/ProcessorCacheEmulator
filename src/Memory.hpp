#include "MemoryUtils.hpp"
#include "CacheUtils.hpp"
#include "EmulatorUtils.hpp"

class Memory {
 public:
    Memory() {
        data_.resize(MEMORY_SIZE);
    }

    void Read(size_t address, std::vector<uint8_t>& data) {
        for (size_t i = 0; i < CACHE_LINE_SIZE; ++i) {
            data[i] = data_[i + address];
        }
    }

    void Write(size_t address, std::vector<uint8_t> data) {
        for (size_t i = 0; i < CACHE_LINE_SIZE; ++i) {
            data_[i + address] = data[i];
        }
    }

 private:
    std::vector<uint8_t> data_{};
};

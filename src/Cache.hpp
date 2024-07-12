#include "CacheUtils.hpp"
#include "EmulatorUtils.hpp"
#include "Memory.hpp"

class CacheLine {
public:
    CacheLine() {
        data_.resize(CACHE_LINE_SIZE);
    };

    bool valid_{};
    bool dirty_{};

    size_t recent_{};
    bool mru_{};

    size_t set_{};

    size_t tag_{};
    std::vector<uint8_t> data_{};
};

class Cache {
public:
    Cache() {
        cache_.resize(CACHE_LINE_COUNT);
        for (size_t i = 0; i < CACHE_LINE_COUNT; ++i) {
            cache_[i].set_ = i / (size_t)CACHE_WAY;
        }
        policy_ = Replacement::LRU;
    };

    void SetPolicy(Replacement policy) {
        policy_ = policy;
    }

    bool Write(size_t address, std::vector<uint8_t> data) {
        size_t index = ((address >> CACHE_OFFSET_LEN) & ((1 << CACHE_INDEX_LEN) - 1)) * CACHE_WAY;
        size_t tag = address >> (CACHE_OFFSET_LEN + CACHE_INDEX_LEN);
        size_t offset = (address & ((1 << CACHE_OFFSET_LEN) - 1));
        size_t address_without_offset = (address & ((1 << (CACHE_INDEX_LEN + CACHE_TAG_LEN)) - 1));

        size_t byte_count = data.size();

        for (size_t i = index; i < index + CACHE_WAY; ++i) {
            if (cache_[i].valid_ && cache_[i].tag_ == tag) {

                for (size_t j = 0; j < byte_count; ++j) {
                    cache_[i].data_[j + offset] = data[j];
                }

                Update(i, cache_[i].set_);
                return true;
            }
        }

        memory_read_ += 1;

        for (size_t i = index; i < index + CACHE_WAY; ++i){
            if (!cache_[i].valid_){
                cache_[i].valid_ = true;
                cache_[i].tag_ = tag;
                cache_[i].dirty_ = true;

                memory_.Read(address_without_offset, cache_[i].data_);

                for (size_t j = 0; j < byte_count; ++j) {
                    cache_[i].data_[j + offset] = data[j];
                }

                Update(i, cache_[i].set_);
                return false;
            }
        }

        size_t best_index = FindBest(index);

        size_t best_line_address_without_offset = 0;
        best_line_address_without_offset |= (cache_[best_index].tag_ << (CACHE_INDEX_LEN + CACHE_OFFSET_LEN));
        best_line_address_without_offset |= (cache_[best_index].set_ << (CACHE_OFFSET_LEN));

        if (cache_[best_index].dirty_){
            memory_write_ += 1;

            memory_.Write(best_line_address_without_offset, cache_[best_index].data_);
        }

        memory_.Read(address_without_offset, cache_[best_index].data_);

        cache_[best_index].tag_ = tag;
        cache_[best_index].dirty_ = true;

        for (size_t j = 0; j < byte_count; ++j) {
            cache_[best_index].data_[j + offset] = data[j];
        }

        Update(best_index, cache_[best_index].set_);

        return false;
    }

    bool Read(size_t address, std::vector<uint8_t>& data) {
        size_t index = ((address >> CACHE_OFFSET_LEN) & ((1 << CACHE_INDEX_LEN) - 1)) * CACHE_WAY;
        size_t tag = address >> (CACHE_OFFSET_LEN + CACHE_INDEX_LEN);
        size_t offset = (address & ((1 << CACHE_OFFSET_LEN) - 1));
        size_t address_without_offset = (address & ((1 << (CACHE_INDEX_LEN + CACHE_TAG_LEN)) - 1));

        size_t byte_count = data.size();

        for (size_t i = index; i < index + CACHE_WAY; ++i) {
            if (cache_[i].valid_ && cache_[i].tag_ == tag) {

                for (size_t j = 0; j < byte_count; ++j) {
                    data[j] = cache_[i].data_[j + offset];
                }

                Update(i, cache_[i].set_);
                return true;
            }
        }

        memory_read_ += 1;

        for (size_t i = index; i < index + CACHE_WAY; ++i) {
            if (!cache_[i].valid_) {
                cache_[i].valid_ = true;
                cache_[i].tag_ = tag;
                cache_[i].dirty_ = false;

                memory_.Read(address_without_offset, cache_[i].data_);

                for (size_t j = 0; j < byte_count; ++j) {
                    data[j] = cache_[i].data_[j + offset];
                }

                Update(i, cache_[i].set_);
                return false;
            }
        }

        size_t best_index = FindBest(index);

        size_t best_line_address_without_offset = 0;
        best_line_address_without_offset |= (cache_[best_index].tag_ << (CACHE_INDEX_LEN + CACHE_OFFSET_LEN));
        best_line_address_without_offset |= (cache_[best_index].set_ << (CACHE_OFFSET_LEN));

        if (cache_[best_index].dirty_){
            memory_write_ += 1;

            memory_.Write(best_line_address_without_offset, cache_[best_index].data_);
        }

        memory_.Read(address_without_offset, cache_[best_index].data_);

        cache_[best_index].tag_ = tag;
        cache_[best_index].dirty_ = false;

        for (size_t j = 0; j < byte_count; ++j) {
             data[j] = cache_[best_index].data_[j + offset];
        }

        Update(best_index, cache_[best_index].set_);

        return false;
    }

    void Read(size_t address, size_t byte_count, std::vector<uint8_t>& data) {
        size_t offset = (address & ((1 << CACHE_OFFSET_LEN) - 1));

        bool is_miss = false;
        if (offset + byte_count > CACHE_LINE_SIZE) {
            std::vector<uint8_t> first;
            std::vector<uint8_t> second;

            size_t index = 0;
            while (index < CACHE_LINE_SIZE - offset) {
                first.push_back(data[index]);
                index += 1;
            }
            while (index < byte_count) {
                second.push_back(data[index]);
                index += 1;
            }

            is_miss |= !Read(address, first);
            is_miss |= !Read(address + first.size(), second);

            index = 0;
            while (index < CACHE_LINE_SIZE - offset) {
                data[index] = first[index];
                index += 1;
            }
            while (index < byte_count) {
                data[index] = second[index];
                index += 1;
            }
        } else {
            is_miss |= !Read(address, data);
        }

        if (is_miss) {
            cache_miss_ += 1;
        } else {
            cache_hit_ += 1;
        }
    }

    void Write(size_t address, size_t byte_count, std::vector<uint8_t>& data) {
        size_t offset = (address & ((1 << CACHE_OFFSET_LEN) - 1));

        bool is_miss = false;
        if (offset + byte_count > CACHE_LINE_SIZE) {
            std::vector<uint8_t> first;
            std::vector<uint8_t> second;

            size_t index = 0;
            while (index < CACHE_LINE_SIZE - offset) {
                first.push_back(data[index]);
                index += 1;
            }
            while (index < byte_count) {
                second.push_back(data[index]);
                index += 1;
            }

            is_miss |= !Write(address, first);
            is_miss |= !Write(address + first.size(), second);
        } else {
            is_miss |= !Write(address, data);
        }

        if (is_miss) {
            cache_miss_ += 1;
        } else {
            cache_hit_ += 1;
        }
    }

    void Update(size_t index, size_t set) {
        if (policy_ == Replacement::LRU) {
            for (size_t i = set * CACHE_WAY; i < set * CACHE_WAY + CACHE_WAY; ++i) {
                cache_[i].recent_ += 1;
            }
            cache_[index].recent_ = 0;
        } else {
            cache_[index].mru_ = true;

            bool zero = false;

            for (size_t i = set * CACHE_WAY; i < set * CACHE_WAY + CACHE_WAY; ++i) {
                zero |= (!cache_[i].mru_);
            }

            if (!zero) {
                for (size_t i = set * CACHE_WAY; i < set * CACHE_WAY + CACHE_WAY; ++i) {
                    cache_[i].mru_ = false;
                }
                cache_[index].mru_ = true;
            }
        }
    }

    size_t FindBest(size_t index) {
        if (policy_ == Replacement::LRU) {
            size_t best_index = index;
            size_t highest = 0;

            for (size_t i = index; i < index + CACHE_WAY; ++i){
                if (cache_[i].recent_ > highest){
                    highest = cache_[i].recent_;
                    best_index = i;
                }
            }

            return best_index;
        } else {
            size_t best_index = index;

            for (size_t i = index; i < index + CACHE_WAY; ++i){
                if (!cache_[i].mru_){
                    best_index = i;
                    break;
                }
            }

            return best_index;
        }
    }

 public:
    size_t memory_read_{};
    size_t memory_write_{};
    size_t cache_hit_{};
    size_t cache_miss_{};

    std::vector<std::string> stats;

 private:
    std::vector<CacheLine> cache_;
    Memory memory_{};
    Replacement policy_{};
};

#pragma once

#include "Cache.hpp"

#include <regex>
#include <string>
#include <limits>
#include <vector>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <unordered_map>

const std::regex value_type(R"((\-)?((0x([(A-F|a-f)]|\d)*)|\d*))");

class Emulator {
 public:
    Emulator();

    void Emulate(const std::string& file_name);

    void Run(const std::string& file_name);

    static std::vector<std::string> ParseInstruction(const std::string& instruction);

    static int32_t ParseOffset(const std::string& off);

    void SetPolicy(Replacement policy);

    static int32_t ReturnSignExtended(int32_t value, size_t bits_cnt);

 private:
    Replacement policy_{};
    Cache lru_cache_;
    Cache bit_p_lru_cache_;

    std::vector<std::vector<std::string>> instructions_;
    std::unordered_map<std::string, int32_t> registers_;
    std::unordered_map<std::string, int64_t> labels_;

    size_t current_line_{};
    size_t current_instruction_{};
    std::unordered_map<size_t, size_t> instruction_to_line_;
    std::unordered_map<size_t, size_t> line_to_instruction_;
};

#include "Emulator.hpp"

Emulator::Emulator() {
    lru_cache_.SetPolicy(Replacement::LRU);
    bit_p_lru_cache_.SetPolicy(Replacement::BIT_P_LRU);
};

void Emulator::Emulate(const std::string& file_name) {
    std::ifstream asm_file;
    asm_file.open(file_name);

    if (!asm_file.is_open()) {
        std::cerr << "Error: File not found" << std::endl;
        exit(0);
    }

    std::string line;
    while (std::getline(asm_file, line)) {
        instructions_.push_back(ParseInstruction(line));
    }

    asm_file.close();

    for (const auto &i : instructions_) {
        auto instruction = i;

        if (instruction.empty()) {
            continue;
        }

        if (instruction[0].back() == ':') {
            instruction[0].pop_back();
            labels_[instruction[0]] = (int64_t)current_instruction_;
            continue;
        }

        current_instruction_ += 1;
    }

    current_instruction_ = 0;

    while (current_line_ < instructions_.size()) {
        std::vector<std::string> instruction = instructions_[current_line_];

        if (instruction.empty() || instruction[0].back() == ':') {
            current_line_ += 1;
            continue;
        }

        current_instruction_ += 1;

        if (!instruction_to_line_.contains(current_instruction_)) {
            instruction_to_line_[current_instruction_] = current_line_;
        }
        line_to_instruction_[current_line_] = current_instruction_;

        current_line_ += 1;
    }

    current_line_ = 0;
    current_instruction_ = 0;

    while (current_line_ < instructions_.size()) {
        std::vector<std::string> instruction = instructions_[current_line_];

        if (instruction.empty() || instruction[0].back() == ':') {
            current_line_ += 1;
            continue;
        }

        current_instruction_ += 1;

        if (instruction[0] == "lb") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(1);

            lru_cache_.Read(address, 1, data);
            bit_p_lru_cache_.Read(address, 1, data);

            registers_[instruction[1]] = ReturnSignExtended((int32_t)(data[0]), 8);
        }

        if (instruction[0] == "sb") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(1);
            data[0] = (registers_[instruction[1]] & 0xFF);

            lru_cache_.Write(address, 1, data);
            bit_p_lru_cache_.Write(address, 1, data);
        }

        if (instruction[0] == "lh") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(2);

            lru_cache_.Read(address, 2, data);
            bit_p_lru_cache_.Read(address, 2, data);

            registers_[instruction[1]] = ReturnSignExtended((int32_t)(data[1] << 8 | data[0]), 16);
        }

        if (instruction[0] == "sh") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(2);
            data[0] = (registers_[instruction[1]] & 0xFF);
            data[1] = ((registers_[instruction[1]] >> 8) & 0xFF);

            lru_cache_.Write(address, 2, data);
            bit_p_lru_cache_.Write(address, 2, data);
        }

        if (instruction[0] == "lw") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(4);

            lru_cache_.Read(address, 4, data);
            bit_p_lru_cache_.Read(address, 4, data);

            registers_[instruction[1]] = ReturnSignExtended((int32_t)((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0])), 32);
        }

        if (instruction[0] == "sw") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(4);
            data[0] = (registers_[instruction[1]] & 0xFF);
            data[1] = ((registers_[instruction[1]] >> 8) & 0xFF);
            data[2] = ((registers_[instruction[1]] >> 16) & 0xFF);
            data[3] = ((registers_[instruction[1]] >> 24) & 0xFF);

            lru_cache_.Write(address, 4, data);
            bit_p_lru_cache_.Write(address, 4, data);
        }

        if (instruction[0] == "lbu") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(1);

            lru_cache_.Read(address, 1, data);
            bit_p_lru_cache_.Read(address, 1, data);

            registers_[instruction[1]] = (int32_t)(data[0]);
        }

        if (instruction[0] == "lhu") {
            int32_t offset = ParseOffset(instruction[2]);
            size_t address = registers_[instruction[3]] + ReturnSignExtended(offset, 12);

            std::vector<uint8_t> data(2);

            lru_cache_.Read(address, 2, data);
            bit_p_lru_cache_.Read(address, 2, data);

            registers_[instruction[1]] = (int32_t)((data[1] << 8) | (data[0]));
        }

        if (instruction[0] == "addi") {
            int32_t offset = ParseOffset(instruction[3]);
            registers_[instruction[1]] = (registers_[instruction[2]] + ReturnSignExtended(offset, 12));
        }

        if (instruction[0] == "jal") {
            std::string unknown = instruction[2];
            size_t address;
            if (std::regex_match(unknown, value_type)) {
                address = (unknown.starts_with("0x") ?
                           std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                           std::stoi(unknown)) / 4 - 1;
            } else {
                address = labels_[unknown];
            }

            if (instruction[1] == "zero" || instruction[1] == "x0") {
                current_line_ += 1;
                continue;
            }

            registers_[instruction[1]] = (int32_t)current_instruction_ + 1;
            current_line_ = instruction_to_line_[address];
            current_instruction_ = address;
        }

        if (instruction[0] == "bge") {
            std::string unknown = instruction[3];
            if (registers_[instruction[1]] >= registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += (unknown.starts_with("0x") ?
                                             std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                             std::stoi(unknown)) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        if (instruction[0] == "lui") {
            int32_t imm = ParseOffset(instruction[2]);
            registers_[instruction[1]] = (int32_t)(imm & 0xFFFFF000);
        }

        if (instruction[0] == "auipc") {
            int32_t imm = ParseOffset(instruction[2]);
            registers_[instruction[1]] = (int32_t)(current_instruction_ + (imm & 0xFFFFF000));
        }

        if (instruction[0] == "stli") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = (registers_[instruction[2]] < ReturnSignExtended(imm, 12)) ? 1 : 0;
        }

        if (instruction[0] == "stliu") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = ((uint32_t)registers_[instruction[2]] < (uint32_t)ReturnSignExtended(imm, 12)) ? 1 : 0;
        }

        if (instruction[0] == "xori") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = (registers_[instruction[2]] ^ ReturnSignExtended(imm, 12));
        }

        if (instruction[0] == "ori") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = (registers_[instruction[2]] | ReturnSignExtended(imm, 12));
        }

        if (instruction[0] == "andi") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = (registers_[instruction[2]] & ReturnSignExtended(imm, 12));
        }

        if (instruction[0] == "slli") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = registers_[instruction[2]] << (imm & 0x1F);
        }

        if (instruction[0] == "srli") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = registers_[instruction[2]] / (1 << ((imm & 0x1F) - 1));
        }

        if (instruction[0] == "srai") {
            int32_t imm = ParseOffset(instruction[3]);
            registers_[instruction[1]] = registers_[instruction[2]] >> (imm & 0x1F);
        }

        if (instruction[0] == "add") {
            registers_[instruction[1]] = registers_[instruction[2]] + registers_[instruction[3]];
        }

        if (instruction[0] == "sub") {
            registers_[instruction[1]] = registers_[instruction[2]] - registers_[instruction[3]];
        }

        if (instruction[0] == "mul") {
            registers_[instruction[1]] = registers_[instruction[2]] * registers_[instruction[3]];
        }

        if (instruction[0] == "div") {
            registers_[instruction[1]] = registers_[instruction[2]] / registers_[instruction[3]];
        }

        if (instruction[0] == "rem") {
            registers_[instruction[1]] = registers_[instruction[2]] % registers_[instruction[3]];
        }

        if (instruction[0] == "mulh") {
            registers_[instruction[1]] = (int32_t)(((int64_t)registers_[instruction[2]] * (int64_t)registers_[instruction[3]]) >> 32);
        }

        if (instruction[0] == "mulhu") {
            registers_[instruction[1]] = (int32_t)(((uint64_t)registers_[instruction[2]] * (uint64_t)registers_[instruction[3]]) >> 32);
        }

        if (instruction[0] == "mulhsu") {
            registers_[instruction[1]] = (int32_t)(((int64_t)registers_[instruction[2]] * (uint64_t)registers_[instruction[3]]) >> 32);
        }

        if (instruction[0] == "divu") {
            registers_[instruction[1]] = (int32_t)((uint32_t)registers_[instruction[2]] / (uint32_t)registers_[instruction[3]]);
        }

        if (instruction[0] == "remu") {
            registers_[instruction[1]] = (int32_t)((uint32_t)registers_[instruction[2]] % (uint32_t)registers_[instruction[3]]);
        }

        if (instruction[0] == "sll") {
            registers_[instruction[1]] = (registers_[instruction[2]] << (registers_[instruction[3]] & 0x1F));
        }

        if (instruction[0] == "slt") {
            registers_[instruction[1]] = (registers_[instruction[2]] < registers_[instruction[3]] ? 1 : 0);
        }

        if (instruction[0] == "sltu") {
            registers_[instruction[1]] = ((uint32_t)registers_[instruction[2]] < (uint32_t)registers_[instruction[3]] ? 1 : 0);
        }

        if (instruction[0] == "xor") {
            registers_[instruction[1]] = (registers_[instruction[2]] ^ registers_[instruction[3]]);
        }

        if (instruction[0] == "srl") {
            registers_[instruction[1]] = (registers_[instruction[2]] / (1 << ((registers_[instruction[3]] & 0x1F) - 1)));
        }

        if (instruction[0] == "sra") {
            registers_[instruction[1]] = (registers_[instruction[2]] >> (registers_[instruction[3]] & 0x1F));
        }

        if (instruction[0] == "or") {
            registers_[instruction[1]] = (registers_[instruction[2]] | registers_[instruction[3]]);
        }

        if (instruction[0] == "and") {
            registers_[instruction[1]] = (registers_[instruction[2]] & registers_[instruction[3]]);
        }

        if (instruction[0] == "jalr") {
            int64_t off;
            if (std::regex_match(instruction[2], value_type)) {
                off = (instruction[2].starts_with("0x") ?
                       std::stoi(instruction[2].substr(2, instruction[2].size()),nullptr, 16) :
                       std::stoi(instruction[2]));
            } else {
                off = labels_[instruction[2]];
            }

            int64_t address = registers_[instruction[3]] + ReturnSignExtended((int32_t)off, 12);

            if (instruction[1] == "zero" || instruction[1] == "x0") {
                current_line_ += 1;
                continue;
            }

            registers_[instruction[1]] = (int32_t)current_instruction_ + 1;
            current_line_ = instruction_to_line_[address];
            current_instruction_ = address;
        }

        if (instruction[0] == "beq") {
            std::string unknown = instruction[3];
            if (registers_[instruction[1]] == registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += ReturnSignExtended(unknown.starts_with("0x") ?
                                             std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                             std::stoi(unknown), 12) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        if (instruction[0] == "bne") {
            std::string unknown = instruction[3];
            if (registers_[instruction[1]] != registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += ReturnSignExtended(unknown.starts_with("0x") ?
                                            std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                            std::stoi(unknown), 12) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        if (instruction[0] == "blt") {
            std::string unknown = instruction[3];
            if (registers_[instruction[1]] < registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += ReturnSignExtended(unknown.starts_with("0x") ?
                                             std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                             std::stoi(unknown), 12) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        if (instruction[0] == "bge") {
            std::string unknown = instruction[3];
            if (registers_[instruction[1]] >= registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += ReturnSignExtended(unknown.starts_with("0x") ?
                                            std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                            std::stoi(unknown), 12) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        if (instruction[0] == "bltu") {
            std::string unknown = instruction[3];
            if ((uint32_t)registers_[instruction[1]] < (uint32_t)registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += ReturnSignExtended(unknown.starts_with("0x") ?
                                             std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                             std::stoi(unknown), 12) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        if (instruction[0] == "bgeu") {
            std::string unknown = instruction[3];
            if ((uint32_t)registers_[instruction[1]] >= (uint32_t)registers_[instruction[2]]) {
                if (std::regex_match(unknown, value_type)) {
                    current_instruction_ += ReturnSignExtended(unknown.starts_with("0x") ?
                                             std::stoi(unknown.substr(2, unknown.size()),nullptr, 16) :
                                             std::stoi(unknown), 12) / 4 - 1;
                    current_line_ = instruction_to_line_[current_instruction_];
                } else {
                    current_instruction_ = labels_[unknown];
                    current_line_ = instruction_to_line_[current_instruction_];
                }
            }
        }

        current_line_ += 1;
    }
}

std::vector<std::string> Emulator::ParseInstruction(const std::string &instruction) {
    std::vector<std::string> match;

    if (instruction.starts_with("#") || instruction.starts_with("//")) {
        return match;
    }

    std::istringstream iss(instruction);
    std::string token;
    while (std::getline(iss, token, ' ')) {
        while (token.find('\t') != std::string::npos) {
            token.erase(token.find('\t'), 1);
        }
        while (token.find('\n') != std::string::npos) {
            token.erase(token.find('\n'), 1);
        }
        while (token.find(' ') != std::string::npos) {
            token.erase(token.find(' '), 1);
        }

        if ((instruction.starts_with("#") || instruction.starts_with("//")) &&
             match.empty()) {

            return match;
        }

        while (token.find(',') != std::string::npos) {
            token.erase(token.find(','), 1);
        }
        while (token.find('#') != std::string::npos) {
            token.erase(token.find('#'), 1);
        }
        while (token.find('/') != std::string::npos) {
            token.erase(token.find('/'), 1);
        }

        if (!token.empty()) {
            match.push_back(token);
        }
    }

    return match;
}

int32_t Emulator::ParseOffset(const std::string &off) {
    return (off.starts_with("0x") ?
            std::stoi(off.substr(2, off.size()),nullptr, 16) :
            std::stoi(off));
}

void Emulator::Run(const std::string &file_name) {
    Emulate(file_name);

    float lru;
    float bit_p_lru;

    if (lru_cache_.memory_read_ + lru_cache_.memory_write_ == 0) {
        lru = std::numeric_limits<float>::quiet_NaN();
        bit_p_lru = std::numeric_limits<float>::quiet_NaN();
    } else {
        lru = (float)lru_cache_.cache_hit_ * 100 / (float)(lru_cache_.cache_hit_ + lru_cache_.cache_miss_);
        bit_p_lru = (float)bit_p_lru_cache_.cache_hit_ * 100 / (float)(bit_p_lru_cache_.cache_hit_ + bit_p_lru_cache_.cache_miss_);
    }

    switch (policy_) {
        case Replacement::LRU:
            printf("LRU\thit rate: %3.4f%%\n", lru);
            break;
        case Replacement::BIT_P_LRU:
            printf("pLRU\thit rate: %3.4f%%\n", bit_p_lru);
            break;
        case Replacement::BOTH:
            printf("LRU\thit rate: %3.4f%%\npLRU\thit rate: %3.4f%%\n", lru, bit_p_lru);
            break;
        default:
            break;
    }
}

void Emulator::SetPolicy(Replacement policy) {
    policy_ = policy;
}

int32_t Emulator::ReturnSignExtended(int32_t value, size_t bits_cnt) {
    if (value & (1 << (bits_cnt - 1))) {
        value |= (((1 << (32 - bits_cnt)) - 1) << bits_cnt);
    } else {
        value &= ((1 << bits_cnt) - 1);
    }

    return value;
}

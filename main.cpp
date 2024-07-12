#include "bin/ASMtoMC.hpp"
#include "src/Emulator.hpp"

int32_t main(int32_t argc, char* argv[]) {
    Arguments args{};
    Emulator emulator;

    for (int32_t i = 1; i < argc; ++i) {
        if (std::string{argv[i]} == "--asm") {
            args.asm_file = argv[++i];
        } else if (std::string{argv[i]} == "--bin") {
            args.mc_file = argv[++i];
        } else if (std::string{argv[i]} == "--replacement") {
            if (std::string{argv[i + 1]} == "0") {
                args.mode = ReplacementMode::BOTH;
                emulator.SetPolicy(Replacement::BOTH);
            } else if (std::string{argv[i + 1]} == "1") {
                args.mode = ReplacementMode::LRU;
                emulator.SetPolicy(Replacement::LRU);
            } else if (std::string{argv[i + 1]} == "2") {
                args.mode = ReplacementMode::bit_pLRU;
                emulator.SetPolicy(Replacement::BIT_P_LRU);
            }
        }
    }

    if (!args.mc_file.empty()) {
        ASMtoMC asm_to_mc{args.asm_file, args.mc_file};
        asm_to_mc.Convert();
    }

    if (args.mode != ReplacementMode::NONE) {
        emulator.Run(args.asm_file);
    }

    return 0;
}

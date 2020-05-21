//
// Created by flo on 09.05.20.
//

#include "translate.hpp"
#include <iostream>
#include <asmjit/asmjit.h>
#include <sys/mman.h>
#include "register.h"
#include "translate_arithmetic.hpp"
#include "translate_controlflow.h"

#include "util.h"
#include "parser.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <unordered_map>


using namespace asmjit;


//instruction translation
void translate_risc_instr(t_risc_instr instr, register_info &r_info);
void generate_strlen();

/**
 * The AsmJit code holder for our generated x86 machine code.
 */
CodeHolder code;

/**
 * The Assembly emitter that writes to the CodeBuffer in the CodeHolder.
 */
x86::Assembler *a;

/**
 * Quick example to test code generation using AsmJit.
 */
void test_generation() {
    std::cout << "Testing code generation with a String length example...\n";
    generate_strlen();
}

/**
 * Initializes a new translatable block of code.
 * Call this before translating any instructions that belong together in the same execution run
 * (e.g., before translating every basic block).
 */
void init_block() {
    code.init(CodeInfo(ArchInfo::kIdHost));
    a = new x86::Assembler(&code);
}

/**
 * Finalize the translated block.
 * This emits the ret instruction in order to have the translated basic block return to the main loop.
 * It will, after performing cleanup, prepare the code for relocation and allocate an executable page for it.
 * @return the starting address of the function block, or the nullptr in case of error
 */
t_cache_loc finalize_block() {
    //emit ret instruction as the final instruction in the block
    a->ret();

    //flatten and resolve open links to prepare for relocation
    code.flatten();
    code.resolveUnresolvedLinks();

    //get the worst case size estimate for memory allocation
    size_t size = code.codeSize();

    //allocate executable page for determined worst case code size, initialized to 0
    void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (!ptr) {
        std::cout << "Bad. Memory allocation fault.\n";
        return nullptr;
    }

    //relocate code to allocated memory area, size may have changed
    code.relocateToBase(reinterpret_cast<uint64_t>(ptr));
    size = code.codeSize();

    //copy the .text section to the allocated page
    CodeBuffer buffer = code.sectionById(0)->buffer();
    memcpy(ptr, buffer.data(), buffer.size());

    //prevents gcc from optimizing the assumed dead store away
    __builtin___clear_cache(static_cast<char *>(ptr), static_cast<char *>(ptr) + size - 1);

    return ptr;
}

/**
 * Generates a basic strlen loop in machine code and executes it
 * by calling the generated code in memory.
 */
void generate_strlen() {
    //initialize new block
    init_block();

    //calling convention: first argument in rdi, ret value in rax
    x86::Gp str = x86::rdi;
    x86::Gp len = x86::rax;

    Label loop_cond = a->newLabel();
    Label loop_end = a->newLabel();

    a->xor_(len, len);
    a->bind(loop_cond);
    a->cmp(x86::byte_ptr(str), 0);
    a->jz(loop_end);
    a->inc(len);
    a->inc(str);
    a->jmp(loop_cond);
    a->bind(loop_end);

    //finalize block and get cached location
    t_cache_loc executable = finalize_block();

    //call the assembled function on a string
    std::string hello_world = "Hello, World!"; //len = 13
    char *string = const_cast<char *>(hello_world.c_str());
    typedef int (*str_len_asm)(char *);
    int ret = ((str_len_asm) executable)(string);

    std::cout << "Length of string " << string << " is " << ret;
}

/**
 * Translate the passed instruction and add the output
 * to the current x86 block.
 * @param instr the RISC instruction to translate
 */
void translate_risc_instr(t_risc_instr instr, register_info &r_info) {
    //todo once the optype is finalized in t_risc_instr->optype, extract multiple dispatch layers here

    switch (instr.mnem) {
        case LUI:
            translate_lui(instr, r_info);
            break;
        case AUIPC:
            break;
        case JAL:
            translate_JAL(instr, r_info);
            break;
        case JALR:
            translate_JALR(instr, r_info);
            break;
        case BEQ:
            translate_BEQ(instr, r_info);
            break;
        case BNE:
            translate_BNE(instr, r_info);
            break;
        case BLT:
            translate_BLT(instr, r_info);
            break;
        case BGE:
            translate_BGE(instr, r_info);
            break;
        case BLTU:
            translate_BLTU(instr, r_info);
            break;
        case BGEU:
            translate_BGEU(instr, r_info);
            break;
        case LB:
            break;
        case LBU:
            break;
        case LH:
            break;
        case LW:
            break;
        case LHU:
            break;
        case SB:
            break;
        case SH:
            break;
        case SW:
            break;
        case ADDI:
            translate_addi(instr, r_info);
            break;
        case SLTI:
            break;
        case SLTIU:
            break;
        case XORI:
            break;
        case ORI:
            break;
        case ANDI:
            break;
        case SLLI:
            translate_slli(instr, r_info);
            break;
        case SRLI:
            break;
        case SRAI:
            break;
        case ADD:
            break;
        case SUB:
            break;
        case SLL:
            break;
        case SLT:
            break;
        case SLTU:
            break;
        case XOR:
            break;
        case SRL:
            break;
        case SRA:
            break;
        case OR:
            break;
        case AND:
            break;
        case FENCE:
            break;
        case FENCE_I:
            break;
        case LWU:
            break;
        case LD:
            break;
        case SD:
            break;
        case ADDIW:
            translate_addiw(instr, r_info);
            break;
        case SLLIW:
            break;
        case SRLIW:
            break;
        case SRAIW:
            break;
        case ADDW:
            break;
        case SUBW:
            break;
        case SLLW:
            break;
        case SRLW:
            break;
        case SRAW:
            break;
        case MUL:
            break;
        case MULH:
            break;
        case MULHSU:
            break;
        case MULHU:
            break;
        case DIV:
            break;
        case DIVU:
            break;
        case REM:
            break;
        case REMU:
            break;
        case MULW:
            break;
        case DIVW:
            break;
        case DIVUW:
            break;
        case REMW:
            break;
        case REMUW:
            break;
        case ECALL:
            break;
        case EBREAK:
            break;
        case CSRRW:
            break;
        case CSRRS:
            break;
        case CSRRC:
            break;
        case CSRRWI:
            break;
        case CSRRSI:
            break;
        case CSRRCI:
            break;
    }
}

//NEITHER FINISHED NOR TESTED



void translate_risc_JAL_onlylink(t_risc_instr risc_instr);

void load_risc_registers(register_info r_info);

void save_risc_registers(register_info r_info);


t_cache_loc translate_block(t_risc_addr risc_addr) {

    t_risc_instr risc_instr = {};

    /// get memory for structs
    //what was the problem with 2 standard libraries again?
    //I could use a custom class or arrays instead,
    //but that would be less elegant.
    std::vector<t_risc_instr> block_cache;

    ///count register usage
    uint32_t reg_count[N_REG];
    //std::vector<uint32_t> reg_count(N_REG);

    //parse structs
    while (true) {

        risc_instr.addr = risc_addr;

        block_cache.push_back(risc_instr);

        parse_instruction(&block_cache.back(), reg_count);
        //parse_instruction(&risc_instr,reg_count);

        switch (block_cache.back().optype) {

            ///branch?
            case BRANCH : {    ///BEQ, BNE, BLT, BGE, BLTU, BGEU
                ///destination address unknown at translate time, stop parsing
                goto PARSE_DONE;
            }
                break;

                ///unconditional jump? -> follow
            case JUMP : {    ///JAL, JALR
                switch (block_cache.back().mnem) {
                    case JAL : {
                        ///link
                        ///replace [JAL rd, offset] with [AUIPC rd, 4]
                        ///(4 because next risc_addr)
                        //this AUIPC would not be possible on actual Risc V,
                        //because the lower 12 bits would always be zero.
                        //we can do this here, because the immediate parsing
                        //is done before this step: in parse_instruction()
                        //where AUIPC is parsed as IMMEDIADE instead of UPPER_IMMEDIATE
                        block_cache.back() = t_risc_instr{
                                risc_addr,
                                AUIPC,
                                IMMEDIATE,
                                x0,
                                x0,
                                block_cache.back().reg_dest,
                                4
                        };

                        ///calculate address of jump destination
                        risc_addr += block_cache.back().imm;//(signed long) (parse_jump_immediate(block_cache)); //left shift???

                    }
                        break;

                    case JALR : {
                        ///destination address unknown at translate time, stop parsing
                        goto PARSE_DONE;
                    }

                    default: {
                        ///should not get here
                        std::cerr << "Oops: line " << __LINE__ << " in " __FILE__ << std::endl;
                    }
                }
            }
                break;

                ///no jump or branch -> continue fetching
            default: {
                ///next instruction address
                risc_addr += 4;
            }
        }

    }

    PARSE_DONE:

    ///REGISTER ALLOCATION:

    /*
    *The idea is to allocate the most used Risc V registers to real x86_64 registers
    *but we can not use all the available x86_64 registers because we need some of them for
    *the translated instructions that expand to multiple x86_64 instructions which probably
    *have to store some temporary values.
    *
    *at the end of the translated basic block all the Risc V registers allocated to real x86_64
    *registers will be copied back to their respective in-memory register representation fields.
    *this is necessary because the binary translator also uses the x86_64 registers in between basic blocks
    *and also because the allocation will probably be different for the next basic block.
    */


    //Again, I dont remember the exact problematic with using the standard library anymore.
    //The unordered_map /*could be*/ is replaced by (multiple) arrays,
    //and we could of course implement the sorting algorithm or whatever we'll use later on ourselves.

    ///rank registers by usage
    std::vector<int> indices(N_REG);
    std::iota(indices.begin(), indices.end(), 0);
    std::stable_sort(indices.begin(),
                     indices.end(),
                     [&](int a, int b) { return reg_count[a] > reg_count[b]; }
    );

    ///create allocation MAPping
    //std::unordered_map<t_risc_reg, asmjit::x86::Gp> register_map;
    asmjit::x86::Gp register_map[N_REG];
    bool mapped[N_REG];



    //insert register pairs here, example:
    std::vector<asmjit::x86::Gp> x86_64_registers = {asmjit::x86::r8, asmjit::x86::r9,
                                                     asmjit::x86::r10, asmjit::x86::r11,
                                                     asmjit::x86::r12, asmjit::x86::r13,
                                                     asmjit::x86::r14, asmjit::x86::r15};

    for (int i = 0; i < x86_64_registers.size(); i++) {
        if (indices[i] != t_risc_reg::x0 && reg_count[indices[i]] > 2) {
            register_map[indices[i]] = x86_64_registers[i];
            mapped[indices[i]] = true;

            /*
            register_map.insert(
                    std::pair<t_risc_reg, asmjit::x86::Gp>(
                            static_cast<t_risc_reg>(indices[i]), x86_64_registers[i]
                    )
            );
             */
        } else {
            //I'm not sure if it's zero initialized...
            mapped[indices[i]] = false;
        }
    }
    //notice: risc reg x0 will need special treatment

    ///create info struct
    register_info r_info = {
            register_map,
            mapped,
            reinterpret_cast<uint64_t>(get_reg_data())
    };


    ///initialize new block
    init_block();

    ///save the x86_64 registers
    //???

    ///load registers
    load_risc_registers(r_info);

    /// translate structs
    for (t_risc_instr &i : block_cache) {
        translate_risc_instr(i, r_info);
    }

    ///save registers
    save_risc_registers(r_info);

    ///load the saved x86_64 registers
    //???

    std::cout << "Translated Block: " << block_cache.size() << " instructions" << std::endl;

    ///finalize block and return cached location
    return finalize_block();
}

///writes rd but doesn't actually jump
void translate_risc_JAL_onlylink(t_risc_instr risc_instr) {
    not_yet_implemented("single-instruction JAL onlylink translator not implemented yet");
}

///loads the Risc V registers into their allocated x86_64 registers
void load_risc_registers(register_info r_info) {
    for (int i = t_risc_reg::x0; i <= t_risc_reg::pc; i++) {
        if (r_info.mapped[i]) {
            a->mov(r_info.map[i], x86::ptr(r_info.base + 8 * i));
        }
    }
}

///saves the Risc V registers into their respective memory fields
void save_risc_registers(register_info r_info) {
    for (int i = t_risc_reg::x0; i <= t_risc_reg::pc; i++) {
        if (r_info.mapped[i]) {
            a->mov(x86::ptr(r_info.base + 8 * i), r_info.map[i]);
        }
    }
}

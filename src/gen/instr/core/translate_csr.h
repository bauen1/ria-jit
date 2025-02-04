//
// Created by flo on 21.05.20.
//

#ifndef DYNAMICBINARYTRANSLATORRISCV64_X86_64_TRANSLATE_CSR_H
#define DYNAMICBINARYTRANSLATORRISCV64_X86_64_TRANSLATE_CSR_H

#include "util/log.h"
#include "gen/translate.h"
#include <util/typedefs.h>

void translate_CSRRW(const t_risc_instr *instr, const register_info *r_info);

void translate_CSRRS(const t_risc_instr *instr, const register_info *r_info);

void translate_CSRRC(const t_risc_instr *instr, const register_info *r_info);

void translate_CSRRWI(const t_risc_instr *instr, const register_info *r_info);

void translate_CSRRSI(const t_risc_instr *instr, const register_info *r_info);

void translate_CSRRCI(const t_risc_instr *instr, const register_info *r_info);

void translate_MANUAL_CSRR(const t_risc_instr *instr, const register_info *r_info, const context_info *c_info);

#endif //DYNAMICBINARYTRANSLATORRISCV64_X86_64_TRANSLATE_CSR_H

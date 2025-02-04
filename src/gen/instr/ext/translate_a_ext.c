//
// Created by flo on 14.07.20.
//

#include "translate_a_ext.h"
#include <fadec/fadec-enc.h>
#include <util/util.h>

/*
 * I imagine this can be refactored into something more elegant without using three replacement registers.
 * However, I keep running into segmentation faults, so this works for now.
 */
#define SCRATCH_REG FE_SP

/*
 * Implemented without account for the atomicity of the instructions.
 * The purpose is solely to implement the required functionality in order to support RV64IMA(FD).
 */

/**
 * Translate the LRW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_LRW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate LRW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regDest = getRd(instr, r_info);

    //not doing anything for fencing yet
    err |= fe_enc64(&current, FE_MOVSXr64m32, regDest, FE_MEM(regSrc1, 0, 0, 0));
}

/**
 * Translate the SCW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_SCW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate SCW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);

    // rs2 -> [rs1] 0 in rd if succeed
    err |= fe_enc64(&current, FE_MOV32mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);

    // we are not using any memory fences here, just simulate a successfull write (0 in rd)
    err |= fe_enc64(&current, FE_MOV64ri, regDest, 0);
}

/**
 * Translate the AMOSWAPW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOSWAPW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOSWAPW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    //While the following is "more" atomic, since we don't support full atomicity (yet) anyways, it is not worth the way
    // slower execution, because of the memory barrier.
//    err |= fe_enc64(&current, FE_MOV32rr, SCRATCH_REG, regSrc2);
//    err |= fe_enc64(&current, FE_XCHG32mr, FE_MEM(regSrc1, 0, 0, 0), SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_MOV32mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOADDW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOADDW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOADDW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_ADD32mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOXORW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOXORW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOXORW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_XOR32mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOANDW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOANDW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOANDW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_AND32mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOORW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOORW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOORW...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_OR32mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMINW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMINW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMINW...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP32rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore.
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVL32rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV32mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMAXW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMAXW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMAXW...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP32rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore.
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVG32rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV32mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMINUW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMINUW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMINUW...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP32rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore. (CMOVC = CMOVB and there is only one mnemonic available.)
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVC32rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV32mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMAXUW instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMAXUW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMAXUW...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV32rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP32rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore.
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVA32rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV32mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the LRD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_LRD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate LRD...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regDest = getRd(instr, r_info);

    //not doing anything for fencing yet
    err |= fe_enc64(&current, FE_MOV64rm, regDest, FE_MEM(regSrc1, 0, 0, 0));

    //not doing anything for fencing yet
}

/**
 * Translate the SCD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_SCD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate SCD...\n");
    // rs2 -> [rs1] 0 in rd if succeed

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);

    // rs2 -> [rs1] 0 in rd if succeed
    err |= fe_enc64(&current, FE_MOV64mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);

    // we are not using any memory fences here, just simulate a successfull write (0 in rd)
    err |= fe_enc64(&current, FE_MOV64ri, regDest, 0);

}

/**
 * Translate the AMOSWAPD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOSWAPD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOSWAPD...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_MOV64mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOADDD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOADDD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOADDD...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_ADD64mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOXORD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOXORD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOXORD...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_XOR64mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOANDD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOANDD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOANDD...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_AND64mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOORD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOORD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOORD...\n");

    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2(instr, r_info);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_OR64mr, FE_MEM(regSrc1, 0, 0, 0), regSrc2);
    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMIND instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMIND(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMIND...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP64rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore.
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVL64rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV64mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMAXD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMAXD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMAXD...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP64rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore.
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVG64rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV64mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMINUD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMINUD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMINUD...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);

    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP64rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore. (CMOVC = CMOVB and there is only one mnemonic available.)
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVC64rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV64mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

/**
 * Translate the AMOMAXUD instruction.
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_AMOMAXUD(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate AMOMAXUD...\n");
    FeReg regSrc1 = getRs1(instr, r_info);
    FeReg regSrc2 = getRs2Into(instr, r_info, SECOND_REG);
    FeReg regDest = getRd(instr, r_info);
    saveScratchReg(SCRATCH_REG);

    //load [rs1] --> rd
    //apply binary operator rd • rs2 --> [rs1]

    //load into rd
    //note: this was likely wrong in the previous implementation. we want to set rd := [rs1], not rd := rs1
    err |= fe_enc64(&current, FE_MOV64rm, SCRATCH_REG, FE_MEM(regSrc1, 0, 0, 0));
    err |= fe_enc64(&current, FE_CMP64rr, SCRATCH_REG, regSrc2);
    ///Can use SECOND_REG as a second scratch reg here since it is not stored back if used before and its old value is
    /// only overwritten if not needed anymore.
    if (regDest != regSrc2) {
        invalidateReplacement(r_info, SECOND_REG, true);
    }
    err |= fe_enc64(&current, FE_CMOVA64rr, SECOND_REG, SCRATCH_REG);
    err |= fe_enc64(&current, FE_MOV64mr, FE_MEM(regSrc1, 0, 0, 0), SECOND_REG);

    if (instr->reg_dest != x0) {
        err |= fe_enc64(&current, FE_MOV64rr, regDest, SCRATCH_REG);
    }

    loadScratchReg(SCRATCH_REG);
}

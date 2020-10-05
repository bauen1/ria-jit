//
// Created by Flo Schmidt on 02.09.20.
//

#include "translate_f_ext.h"
#include <fadec/fadec-enc.h>

/* https://www.felixcloutier.com */

#define SIGN_BIT_MASK (0x1 << 31) //left most bit

//TODO if enabled fused mul add functions need to be reworked because they will overwrite operands if they are mapped
#undef AVX_SUPPORT

/**
 * Translate the FLW instruction.
 * Description: load a single-precision floating-point value from memory into floating-point register rd
 * preserve NaN payload
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FLW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FLW...\n");
    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);
    err |= fe_enc64(&current, FE_SSE_MOVSSrm,regDest, FE_MEM(regSrc1, 0, 0, instr->op_field.op.imm));
    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FSW instruction.
 * Description: store a single-precision floating-point value from floating-point register rs2 into memory
 * preserve NaN payload
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FSW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FSW...\n");
    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);
    err |= fe_enc64(&current, FE_SSE_MOVSSmr,FE_MEM(regSrc1, 0, 0, instr->op_field.op.imm), regSrc2);
}

/**
 * Translate the FMADDS instruction.
 * Description: rs1 x rs2 + rs3
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMADDS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMADDS...\n");

#ifdef AVX_SUPPORT
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //move src1 to dest if src1 was mapped
    if (regSrc1 != regDest) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
    }

    //depending if src3 was mapped or not use memory or register operand
    FeReg regSrc3 = getFpRegNoLoad(instr->op_field.f_op.reg_src_3, r_info, FIRST_FP_REG);
    if (regSrc3 == FIRST_FP_REG) {
        //regSrc3 is in memory
        err |= fe_enc64(&current, FE_VFMADD213SSrrm, regDest, regSrc2,
                        FE_MEM_ADDR(r_info->fp_base + 8 * instr->op_field.f_op.reg_src_3));
    } else {
        err |= fe_enc64(&current, FE_VFMADD213SSrrr, regDest, regSrc2, regSrc3);
    }
#else
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, SECOND_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //TODO could be optimized by reducing moves
    //move src1 to SECOND_FP_REG if src1 was mapped
    if (regSrc1 != SECOND_FP_REG) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc1); //TODO check encoding
    }

    //multiply rs2
    err |= fe_enc64(&current, FE_SSE_MULSSrr, SECOND_FP_REG, regSrc2);

    //move rs3 into dest
    FeReg regSrc3 = getFpReg(instr->op_field.f_op.reg_src_3, r_info, regDest);
    if (regSrc3 != regDest) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc3); //TODO check encoding
    }

    //add multiply result
    err |= fe_enc64(&current, FE_SSE_ADDSSrr, regDest, SECOND_FP_REG);
#endif

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FMSUBS instruction.
 * Description:  rs1 x rs2 - rs3
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMSUBS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMSUBS...\n");

#ifdef AVX_SUPPORT
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //move src1 to dest if src1 was mapped
    if (regSrc1 != regDest) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
    }

    //depending if src3 was mapped or not use memory or register operand
    FeReg regSrc3 = getFpRegNoLoad(instr->op_field.f_op.reg_src_3, r_info, FIRST_FP_REG);
    if (regSrc3 == FIRST_FP_REG) {
        //regSrc3 is in memory
        err |= fe_enc64(&current, FE_VFMSUB213SSrrm, regDest, regSrc2,
                        FE_MEM_ADDR(r_info->fp_base + 8 * instr->op_field.f_op.reg_src_3));
    } else {
        err |= fe_enc64(&current, FE_VFMSUB213SSrrr, regDest, regSrc2, regSrc3);
    }
#else
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, SECOND_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);

    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, SECOND_FP_REG);

    //multiply rs1 and rs2
    //move src1 to SECOND_FP_REG if src1 was mapped
    if (regSrc1 != SECOND_FP_REG) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc1); //TODO check encoding
    }

    err |= fe_enc64(&current, FE_SSE_MULSSrr, SECOND_FP_REG, regSrc2);

    FeReg regSrc3 = getFpReg(instr->op_field.f_op.reg_src_3, r_info, FIRST_FP_REG);

    //if src3 is already in dest save it
    if (regDest == regSrc3) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, FIRST_FP_REG, regSrc3); //TODO check encoding
        regSrc3 = FIRST_FP_REG;
    }


    //move multiply result in SECOND_FP_REG to regDest
    if (regSrc1 != FIRST_FP_REG) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, SECOND_FP_REG); //TODO check encoding
    }

    //subtract rs3
    err |= fe_enc64(&current, FE_SSE_SUBSSrr, regDest, regSrc3);
#endif

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FNMSUBS instruction.
 * Description: -(rs1 x rs2) + rs3
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FNMSUBS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FNMSUBS...\n");

#ifdef AVX_SUPPORT
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //move src1 to dest if src1 was mapped
    if (regSrc1 != regDest) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
    }

    //depending if src3 was mapped or not use memory or register operand
    FeReg regSrc3 = getFpRegNoLoad(instr->op_field.f_op.reg_src_3, r_info, FIRST_FP_REG);
    if (regSrc3 == FIRST_FP_REG) {
        //regSrc3 is in memory
        err |= fe_enc64(&current, FE_VFNMSUB213SSrrr, regDest, regSrc2,
                        FE_MEM_ADDR(r_info->fp_base + 8 * instr->op_field.f_op.reg_src_3));
    } else {
        err |= fe_enc64(&current, FE_VFNMSUB213SSrrr, regDest, regSrc2, regSrc3);
    }

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
#else
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, SECOND_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);

    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //move src1 to SECOND_FP_REG if src1 was mapped
    if (regSrc1 != SECOND_FP_REG) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc1); //TODO check encoding
    }

    //multiply rs1 and rs2
    err |= fe_enc64(&current, FE_SSE_MULSSrr, SECOND_FP_REG, regSrc2);

    //FIRST_FP_REG is now free again
    FeReg regSrc3 = getFpReg(instr->op_field.f_op.reg_src_3, r_info, FIRST_FP_REG);

    //if src3 is not already in dest move it
    if (regDest == regSrc3) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc3); //TODO check encoding
        regSrc3 = FIRST_FP_REG;
    }

    //subtract multiply result
    err |= fe_enc64(&current, FE_SSE_SUBSSrr, regDest, SECOND_FP_REG);
#endif
    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FNMADDS instruction.
 * Description -(rs1 x rs2) - rs3
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FNMADDS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FNMADDS...\n");

#ifdef AVX_SUPPORT
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //move src1 to dest if src1 was mapped
    if (regSrc1 != regDest) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
    }

    //depending if src3 was mapped or not use memory or register operand
    FeReg regSrc3 = getFpRegNoLoad(instr->op_field.f_op.reg_src_3, r_info, FIRST_FP_REG);
    if (regSrc3 == FIRST_FP_REG) {
        //regSrc3 is in memory
        err |= fe_enc64(&current, FE_VFNMADD213SSrrr, regDest, regSrc2,
                        FE_MEM_ADDR(r_info->fp_base + 8 * instr->op_field.f_op.reg_src_3));
    } else {
        err |= fe_enc64(&current, FE_VFNMADD213SSrrr, regDest, regSrc2, regSrc3);
    }
#else
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, SECOND_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);

    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    //multiply rs1 and rs2
    //move src1 to SECOND_FP_REG if src1 was mapped
    if (regSrc1 != SECOND_FP_REG) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc1); //TODO check encoding
    }

    err |= fe_enc64(&current, FE_SSE_MULSSrr, SECOND_FP_REG, regSrc2);

    //negate by subtracting
    err |= fe_enc64(&current, FE_SSE_XORPSrr, FIRST_FP_REG, FIRST_FP_REG);
    err |= fe_enc64(&current, FE_SSE_SUBSSrr, FIRST_FP_REG, SECOND_FP_REG);

    FeReg regSrc3 = getFpReg(instr->op_field.f_op.reg_src_3, r_info, SECOND_FP_REG);
    //if src3 is already in dest save it
    if (regDest == regSrc3) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc3); //TODO check encoding
        regSrc3 = SECOND_FP_REG;
    }


    //move negated multiply result in FIRST_FP_REG to regDest
    if (regDest != FIRST_FP_REG) {
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, FIRST_FP_REG); //TODO check encoding
    }

    //subtract rs3
    err |= fe_enc64(&current, FE_SSE_SUBSSrr, regDest, regSrc3);
#endif
    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FADDS instruction.
 * Description: rs1 + rs2
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FADDS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FADDS...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    doFpArithmCommutative(regSrc1, regSrc2, regDest, FE_SSE_ADDSSrr);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FSUBS instruction.
 * Description: rs1 - rs2
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FSUBS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FSUBS...\n");
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    if (instr->op_field.op.reg_dest == instr->op_field.op.reg_src_2 &&
            instr->op_field.op.reg_dest == instr->op_field.op.reg_src_1) {

        err |= fe_enc64(&current, FE_SSE_XORPSrr, regDest,
                        regDest); // SUB same, same, same is equivalent to zeroing
    } else {
        //save rs2
        FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
        if (regSrc2 == regDest) { //save src2 if necessary
            err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc2); //TODO check encoding
            regSrc2 = SECOND_FP_REG;
        }
        //load first operand into regDest
        FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, regDest);
        if (regSrc1 != regDest) { // move if src1 was mapped
            err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
        }
        err |= fe_enc64(&current, FE_SSE_SUBSSrr, regDest, regSrc2);
    }

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FMULS instruction.
 * Description: rs1 x rs2
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMULS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMULS...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    doFpArithmCommutative(regSrc1, regSrc2, regDest, FE_SSE_MULSSrr);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FDIVS instruction.
 * Description: rs1 / rs2
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FDIVS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FDIVS...\n");
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    if (regDest == regSrc2) {
        //happens only if both are mapped and the same
        //save rs2
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, SECOND_FP_REG, regSrc2); //TODO check encoding
        regSrc2 = SECOND_FP_REG;
    }

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, regDest); //load src1 into destination
    if (regSrc1 != regDest) {
        //src1 was mapped => move into regDest
        err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest,
                        regSrc1); //TODO check encoding needs to be F3 0F 11 and not 10
    }
    err |= fe_enc64(&current, FE_SSE_DIVSSrr, regDest, regSrc2);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FSQRTS instruction.
 * Description: compute the square root of rs1
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FSQRTS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FSQRTS...\n");
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    err |= fe_enc64(&current, FE_SSE_SQRTSSrr, regDest, regSrc1);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FSGNJS instruction.
 * Description: takes all bits but sign from rs1, sign bit is that from rs2
 * naive approach by first moving into normal register, doing bit artihmetic and then putting it back
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FSGNJS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FSGNJS...\n");
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    if (instr->op_field.op.reg_src_1 == instr->op_field.op.reg_src_2) {
        //simple move
        if (regSrc1 != regDest) {
            //move to regDest
            err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1);
        }
    } else {
        FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);


        err |= fe_enc64(&current, FE_SSE_MOVDrr, FIRST_REG, regSrc1);
        err |= fe_enc64(&current, FE_SSE_MOVDrr, SECOND_REG, regSrc2);

        //remove sign from first
        err |= fe_enc64(&current, FE_AND32ri, FIRST_REG, ~SIGN_BIT_MASK);

        //mask sign from second
        err |= fe_enc64(&current, FE_AND32ri, SECOND_REG, SIGN_BIT_MASK);
        //merge
        err |= fe_enc64(&current, FE_OR32rr, FIRST_REG, SECOND_REG);
        //move back to fp_reg
        err |= fe_enc64(&current, FE_SSE_MOVDrr, regDest, FIRST_REG);
    }

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);

}

/**
 * Translate the FSGNJNS instruction.
 * Description: takes all bits but sign from rs1, sign bit is the opposite of rs2's
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FSGNJNS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FSGNJNS...\n");
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);


    err |= fe_enc64(&current, FE_SSE_MOVDrr, FIRST_REG, regSrc1);
    err |= fe_enc64(&current, FE_SSE_MOVDrr, SECOND_REG, regSrc2);

    //remove sign from first
    err |= fe_enc64(&current, FE_AND32ri, FIRST_REG, ~SIGN_BIT_MASK);

    //flip second (negate sign)
    err |= fe_enc64(&current, FE_NOT32r, SECOND_REG);
    //mask sign from second
    err |= fe_enc64(&current, FE_AND32ri, SECOND_REG, SIGN_BIT_MASK);
    //merge
    err |= fe_enc64(&current, FE_OR32rr, FIRST_REG, SECOND_REG);
    //move back to fp_reg
    err |= fe_enc64(&current, FE_SSE_MOVDrr, regDest, FIRST_REG);


    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FSGNJXS instruction.
 * Description: takes all bits but sign from rs1, sign bit is the xor of rs1's and rs2's
 * may be used for e.g. FABS, to compute the absolute of a value FSGNJXS ry, rx, rx
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FSGNJXS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FSGNJXS...\n");
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);


    err |= fe_enc64(&current, FE_SSE_MOVDrr, FIRST_REG, regSrc1);
    err |= fe_enc64(&current, FE_SSE_MOVDrr, SECOND_REG, regSrc2);

    //xor first and second sign
    err |= fe_enc64(&current, FE_XOR32rr, SECOND_REG, FIRST_REG);

    //remove sign from first
    err |= fe_enc64(&current, FE_AND32ri, FIRST_REG, ~SIGN_BIT_MASK);

    //mask sign from second
    err |= fe_enc64(&current, FE_AND32ri, SECOND_REG, SIGN_BIT_MASK);
    //merge
    err |= fe_enc64(&current, FE_OR32rr, FIRST_REG, SECOND_REG);
    //move back to fp_reg
    err |= fe_enc64(&current, FE_SSE_MOVDrr, regDest, FIRST_REG);


    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FMINS instruction.
 * Description: compute min of rs1, rs2 store in rd, if only one is NaN, the result is not NaN
 * if one of the inputs is NaN NV Exception (Invalid Operation is set)
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMINS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMINS...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    if (regSrc1 == regSrc2) { // we don't need any comparison in this case, because they are the same
        if (regDest != regSrc1) {
            //move src1 in dest
            err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
        }
    } else {
        if (regSrc2 == regDest) {//rs2 needs to be first operand, thus, check rs1
            //check if rs1 is nan
            err |= fe_enc64(&current, FE_SSE_UCOMISSrr, regSrc1, regSrc1);

            uint8_t *jmpBufNaN = current;
            err |= fe_enc64(&current, FE_JP, (intptr_t) current); //dummy

            err |= fe_enc64(&current, FE_SSE_MINSSrr, regDest, regSrc1); //if regSrc2 is NaN regSrc1 will be returned

            err |= fe_enc64(&jmpBufNaN, FE_JP, (intptr_t) current);
            //branch if rs1 is nan, nothing to do as src2 is already in dest
        } else {
            //check if rs2 is nan
            err |= fe_enc64(&current, FE_SSE_UCOMISSrr, regSrc2, regSrc2);

            uint8_t *jmpBufNaN = current;
            err |= fe_enc64(&current, FE_JP, (intptr_t) current); //dummy

            //move src1 in dest
            if (regSrc1 != regDest) {
                err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
            }
            err |= fe_enc64(&current, FE_SSE_MINSSrr, regDest, regSrc2); //if regSrc1 is NaN regSrc2 will be returned

            //check if rs1 is already in dest, in this case we don't need the jump
            if (regSrc1 == regDest) {
                err |= fe_enc64(&jmpBufNaN, FE_JP, (intptr_t) current);
            } else {

                uint8_t *jmpBufEnd = current;
                err |= fe_enc64(&current, FE_JMP, (intptr_t) current); //dummy
                err |= fe_enc64(&jmpBufNaN, FE_JP, (intptr_t) current);

                err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
                err |= fe_enc64(&jmpBufEnd, FE_JMP, (intptr_t) current);
            }
        }
    }


    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FMAXS instruction.
 * Description: compute max of rs1, rs2 store in rd, if only one is NaN, the result is not NaN
 * if one of the inputs is NaN NV Exception (Invalid Operation is set)
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMAXS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMAXS...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    if (regSrc1 == regSrc2) { // we don't need any comparison in this case, because they are the same
        if (regDest != regSrc1) {
            //move src1 in dest
            err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
        }
    } else {
        if (regSrc2 == regDest) {//rs2 needs to be first operand, thus, check rs1
            //check if rs1 is nan
            err |= fe_enc64(&current, FE_SSE_UCOMISSrr, regSrc1, regSrc1);

            uint8_t *jmpBufNaN = current;
            err |= fe_enc64(&current, FE_JP, (intptr_t) current); //dummy

            err |= fe_enc64(&current, FE_SSE_MAXSSrr, regDest, regSrc1); //if regSrc2 is NaN regSrc1 will be returned

            err |= fe_enc64(&jmpBufNaN, FE_JP, (intptr_t) current);
            //branch if rs1 is nan, nothing to do as src2 is already in dest
        } else {
            //check if rs2 is nan
            err |= fe_enc64(&current, FE_SSE_UCOMISSrr, regSrc2, regSrc2);

            uint8_t *jmpBufNaN = current;
            err |= fe_enc64(&current, FE_JP, (intptr_t) current); //dummy

            //move src1 in dest
            if (regSrc1 != regDest) {
                err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
            }
            err |= fe_enc64(&current, FE_SSE_MAXSSrr, regDest, regSrc2); //if regSrc1 is NaN regSrc2 will be returned

            //check if rs1 is already in dest, in this case we don't need the jump
            if (regSrc1 == regDest) {
                err |= fe_enc64(&jmpBufNaN, FE_JP, (intptr_t) current);
            } else {

                uint8_t *jmpBufEnd = current;
                err |= fe_enc64(&current, FE_JMP, (intptr_t) current); //dummy
                err |= fe_enc64(&jmpBufNaN, FE_JP, (intptr_t) current);

                err |= fe_enc64(&current, FE_SSE_MOVSSrr, regDest, regSrc1); //TODO check encoding
                err |= fe_enc64(&jmpBufEnd, FE_JMP, (intptr_t) current);
            }
        }
    }


    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FCVTWS instruction.
 * Description: convert floating-point number in floating-point register rs2 to signed 32-bit int into int register rd
 * if the value is not representable it is clipped an the invalid flag is set
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTWS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTWS...\n");

    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_SSE_CVTSS2SI32rr, regDest, regSrc2);
    err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, regDest);//sign extend

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FCVTWUS instruction.
 * Description: convert floating-point number in floating-point register rs2 to unsigned 32-bit int in int register rd
 * WARNING this implementation will still clamp the value but won't set the exceptions every time
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTWUS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTWUS...\n");

    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_SSE_CVTSS2SI64rr, regDest, regSrc2);
    err |= fe_enc64(&current, FE_MOV32rr, regDest, regDest);//TODO necessary? does cvtss2si clear them already

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FMVXW instruction.
 * Description: move ths fp value in fp register rs1 unchanged to lower 32bits in rd
 * upper bits are filled with rs1 sign, the bits don't get changed
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMVXW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMVXW...\n");
    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_SSE_MOVDrr, regDest, regSrc1);
    err |= fe_enc64(&current, FE_MOVSXr64r32, regDest, regDest);//sign extend

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FEQS instruction.
 * Description: check if rs1 = rs2, write 1 to the integer register rd if the condition holds, 0 otherwise
 * the result is zero if either operand is NaN and Invalid operation exception is set
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FEQS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FEQS...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_XOR64rr, regDest, regDest);
    err |= fe_enc64(&current, FE_SSE_COMISSrr, regSrc1, regSrc2);
    err |= fe_enc64(&current, FE_MOV64ri, SECOND_REG, 0);
    err |= fe_enc64(&current, FE_SETNP8r, regDest);
    err |= fe_enc64(&current, FE_CMOVNZ64rr, regDest, SECOND_REG);

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FLTS instruction.
 * Description: check if rs1 < rs2, write 1 to the integer register rd if the condition holds, 0 otherwise
 * the result is zero if either operand is NaN and Invalid operation exception is set
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FLTS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FLTS...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_XOR64rr, regDest, regDest);
    err |= fe_enc64(&current, FE_SSE_COMISSrr, regSrc2, regSrc1);
    err |= fe_enc64(&current, FE_SETA8r, regDest);

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FLES instruction.
 * Description: check if rs1 <= rs2, write 1 to the integer register rd if the condition holds, 0 otherwise
 * the result is zero if either operand is NaN and Invalid operation exception is set
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FLES(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FLES...\n");

    FeReg regSrc1 = getFpReg(instr->op_field.f_op.reg_src_1, r_info, FIRST_FP_REG);
    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, SECOND_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_XOR64rr, regDest, regDest);
    err |= fe_enc64(&current, FE_SSE_COMISSrr, regSrc2, regSrc1);
    err |= fe_enc64(&current, FE_SETNC8r, regDest);

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FCLASSS instruction.
 * Description: examines fp value in fp register rs1
 * and writes to integer register rd a 10-bit mask that indicates the class of the fp number, format Table 11.5 p.72
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCLASSS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCLASSS...\n");
    /* Definiton bit is set if:
     * 0: rs is negative infinity
     * 1: rs is a negative normal number
     * 2: rs is a negative subnormal number
     * 3: rs is minus zero
     * 4: rs is positive zero
     * 5: rs is a positive subnormal number
     * 6: rs is a positive normal number
     * 7: rs is positive infinity
     * 8: rs is a signaling NaN
     * 9: rs is a quiet NaN
     * Assembler code for the following instructions
     */
    critical_not_yet_implemented("FCLASSS not yet implemented.\n");
}

/**
 * Translate the FCVTSW instruction.
 * Description: convert signed 32-bit int from rs1 into flp in fp rd
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTSW(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTSW...\n");

    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    err |= fe_enc64(&current, FE_SSE_XORPSrr, regDest, regDest); //needs to be done because of cvtsi2ss design
    err |= fe_enc64(&current, FE_SSE_CVTSI2SS32rr, regDest, regSrc1);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FCVTSWU instruction.
 * Description: convert unsigned 32-bit int from rs1 into flp in fp rd
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTSWU(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTSWU...\n");

    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);
    //zero upper bits by moving into FIRST_REG
    err |= fe_enc64(&current, FE_MOV32rr, FIRST_REG, regSrc1);

    err |= fe_enc64(&current, FE_SSE_XORPSrr, regDest, regDest); //needs to be done because of cvtsi2ss design
    err |= fe_enc64(&current, FE_SSE_CVTSI2SS64rr, regDest,
                    FIRST_REG); //use 64 bit convert to get a unsigned conversion

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FMVWX instruction.
 * Description: move the lower 32-bits from int register rs1 to the fp register rd
 * the bits don't get changed
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FMVWX(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FMVWX...\n");
    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    err |= fe_enc64(&current, FE_SSE_MOVDrr, regDest, regSrc1);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FCVTLS instruction.
 * Description: convert a fp number in fp register rs2 to a signed 64-bit int in rd
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTLS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTLS...\n");

    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    err |= fe_enc64(&current, FE_SSE_CVTSS2SI64rr, regDest, regSrc2);

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FCVTLUS instruction.
 * Description: convert a fp number in fp register rs2 to a unsigned 64-bit int in rd
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTLUS(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTLUS...\n");

    FeReg regSrc2 = getFpReg(instr->op_field.f_op.reg_src_2, r_info, FIRST_FP_REG);
    FeReg regDest = getRd(instr, r_info, FIRST_REG);

    //constant saved here
    const unsigned long C0 = 1593835520;
    //load const into register
    err |= fe_enc64(&current, FE_MOV32ri, SECOND_REG, C0);
    err |= fe_enc64(&current, FE_SSE_MOVDrr, SECOND_FP_REG, SECOND_REG);

    err |= fe_enc64(&current, FE_SSE_COMISSrr, regSrc2, SECOND_FP_REG);
    uint8_t *jmpBufCOM = current;
    err |= fe_enc64(&current, FE_JNC, (intptr_t) current); //dummy

    err |= fe_enc64(&current, FE_SSE_CVTSS2SI64rr, regDest, regSrc2);

    uint8_t *jmpBufEnd = current;
    err |= fe_enc64(&current, FE_JMP, (intptr_t) current); //dummy


    err |= fe_enc64(&jmpBufCOM, FE_JNC, (intptr_t) current);
    err |= fe_enc64(&current, FE_SSE_SUBSSrr, regSrc2, SECOND_FP_REG);
    err |= fe_enc64(&current, FE_SSE_CVTSS2SI64rr, regDest, regSrc2);
    err |= fe_enc64(&current, FE_BTC64ri, regDest, 63);
    err |= fe_enc64(&jmpBufEnd, FE_JMP, (intptr_t) current);

    storeRd(instr, r_info, regDest);
}

/**
 * Translate the FCVTSL instruction.
 * Description: convert a signed 64-bit int in rs1 to a fp number in fp register rd
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTSL(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTSL...\n");

    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    err |= fe_enc64(&current, FE_SSE_XORPSrr, regDest, regDest); //needs to be done because of cvtsi2ss design
    err |= fe_enc64(&current, FE_SSE_CVTSI2SS64rr, regDest, regSrc1);

    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

/**
 * Translate the FCVTSLU instruction.
 * Description: convert a unsigned 64-bit int in rs1 to a fp number in fp register rd
 * Conversion taken from godbolt
 * @param instr the RISC-V instruction to translate
 * @param r_info the runtime register mapping (RISC-V -> x86)
 */
void translate_FCVTSLU(const t_risc_instr *instr, const register_info *r_info) {
    log_asm_out("Translate FCVTSLU...\n");

    FeReg regSrc1 = getRs1(instr, r_info, FIRST_REG);
    FeReg regDest = getFpRegNoLoad(instr->op_field.f_op.reg_dest, r_info, FIRST_FP_REG);

    err |= fe_enc64(&current, FE_SSE_XORPSrr, regDest, regDest); //needs to be done because of cvtsi2ss design
    err |= fe_enc64(&current, FE_TEST64rr, regSrc1, regSrc1);

    uint8_t *jmpBufSign = current;
    err |= fe_enc64(&current, FE_JS, (intptr_t) current); //dummy
    err |= fe_enc64(&current, FE_SSE_CVTSI2SS64rr, regDest, regSrc1);

    uint8_t *jmpBufEnd = current;
    err |= fe_enc64(&current, FE_JMP, (intptr_t) current); //dummy

    //set jump address
    err |= fe_enc64(&jmpBufSign, FE_JS, (intptr_t) current);

    if (regSrc1 != FIRST_REG) {
        err |= fe_enc64(&current, FE_MOV64rr, FIRST_REG, regSrc1);
    }
    err |= fe_enc64(&current, FE_MOV64rr, SECOND_REG, regSrc1);
    err |= fe_enc64(&current, FE_AND32ri, FIRST_REG, 0x1);
    err |= fe_enc64(&current, FE_SSE_XORPSrr, FIRST_FP_REG, FIRST_FP_REG);
    err |= fe_enc64(&current, FE_SHR64ri, SECOND_REG, 0x1);
    err |= fe_enc64(&current, FE_OR64rr, SECOND_REG, FIRST_REG);
    err |= fe_enc64(&current, FE_SSE_CVTSI2SS64rr, regDest, SECOND_REG);
    err |= fe_enc64(&current, FE_SSE_ADDSSrr, regDest, regDest);


    //set jump address
    err |= fe_enc64(&jmpBufEnd, FE_JMP, (intptr_t) current);


    setFpReg(instr->op_field.f_op.reg_dest, r_info, regDest);
}

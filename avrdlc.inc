;******************************************************************************
;* Title        : AVRDLC (AVR Delay Loop Calculator)
;* Version      : 1.0
;* Last updated : Mar 31 2023
;* File         : avrdlc.inc
;* Author       : Imre Horvath <imi.horvath [at] gmail [dot] com>
;* License      : MIT
;* Description  : A set of assembler macros for the AVRASM2 assembler -based on
;*                Bret Mulvey's original with T. Morland's enhancements- to
;*                generate busy-wait loops, that wastes the required amount of
;*                clock cycles for a given miro- or milliseconds on an AVR with
;*                CPU clock frequency F_CPU.
;******************************************************************************

#ifndef _AVRDLC_INC_
#define _AVRDLC_INC_

;******************************************************************************
;*
;* MACRO
;*      avrdlc_delay_us N
;*
;* DESCRIPTION
;*      Generates code, that takes N microseconds to execute given F_CPU, by
;*      wasting CPU cycles.
;*
;******************************************************************************

.macro avrdlc_delay_us
    .set AVRDLC_CYCLES = @0*F_CPU/1000000
    .if (@0*F_CPU/100000)-(@0*F_CPU/1000000)*10 >= 5
        .set AVRDLC_CYCLES = AVRDLC_CYCLES+1
    .endif
    .if !AVRDLC_CYCLES
        .error "Too less delay (for this F_CPU value)."
    .endif
    avrdlc_waste_cycles AVRDLC_CYCLES
.endm

;******************************************************************************
;*
;* MACRO
;*      avrdlc_delay_ms N
;*
;* DESCRIPTION
;*      Generates code, that wastes CPU cycles by calling
;*      avrdlc_delay_us N*1000.
;*
;******************************************************************************

.macro avrdlc_delay_ms
    avrdlc_delay_us @0*1000
.endm

.equ AVRDLC_STARTING_REG = 16   ; The first high register number to use

;******************************************************************************
;*
;* MACRO
;*      avrdlc_waste_cycles N
;*
;* DESCRIPTION
;*      Generates code, that wastes N clock cycles. Contents of registers
;*      starting at AVRDLC_STARTING_REG gets modified. Register allocations
;*      like AVRDLC_STARTING_REG, AVRDLC_STARTING_REG+1, etc. gets dynamically
;*      calculated, based on the value of N.
;*
;******************************************************************************

.macro avrdlc_waste_cycles
    .if @0 < 12
        avrdlc_small_waste @0       ; Below 12 cycles do simple wasting
    .else
        .set AVRDLC_CYCLES = @0     ; Otherwise do complex wasting
        .set AVRDLC_LENGTH = 3
        .set AVRDLC_DEPTH = 1
        avrdlc_calc_length_depth    ; Calculate the needed length and depth
        .set AVRDLC_REG_I = 0
        avrdlc_ldi_part             ; Generate the LDI instructions as needed
        .set AVRDLC_REG_I = AVRDLC_REG_I-1
        .set AVRDLC_PC_OFFSET = -1
        avrdlc_loops_part           ; Generate the loops as needed
        avrdlc_small_waste AVRDLC_CYCLES    ; Waste the remaining cycles
    .endif
.endm

;******************************************************************************
;*
;* MACRO
;*      avrdlc_small_waste N
;*
;* DESCRIPTION
;*      Generates code, that wastes N clock cycles, when N < 12.
;*
;******************************************************************************

.macro avrdlc_small_waste
    .if @0 == 1
        nop
    .endif
    .if @0 == 2
        rjmp    PC+1
    .endif
    .if @0 == 3
        rjmp    PC+1
        nop
    .endif
    .if @0 > 3
        rjmp    PC+1
        rjmp    PC+1
        .set AVRDLC_REST_CYCLES = @0-4
        avrdlc_small_waste AVRDLC_REST_CYCLES
    .endif
.endm

;******************************************************************************
;*
;* MACRO
;*      avrdlc_calc_length_depth
;*
;* DESCRIPTION
;*      Calculates the length and depth values based on the cycles needed.
;*
;******************************************************************************

.macro avrdlc_calc_length_depth
    .if AVRDLC_CYCLES > AVRDLC_LENGTH*256+AVRDLC_DEPTH+8
        .set AVRDLC_LENGTH = AVRDLC_LENGTH*256+2
        .set AVRDLC_DEPTH = AVRDLC_DEPTH+1
        avrdlc_calc_length_depth
    .endif
.endm

;******************************************************************************
;*
;* MACRO
;*      avrdlc_ldi_part
;*
;* DESCRIPTION
;*      Generates the LDI instructions as needed. The register numbers and
;*      values gets calculated for each.
;*
;******************************************************************************

.macro avrdlc_ldi_part
    .set AVRDLC_C = (AVRDLC_CYCLES-3*(AVRDLC_DEPTH-AVRDLC_REG_I))/AVRDLC_LENGTH+1
    .if AVRDLC_C > 256
        .set AVRDLC_C = 256
    .endif
    .if AVRDLC_C < 1
        .set AVRDLC_C = 1
    .endif
    .set AVRDLC_CYCLES = AVRDLC_CYCLES-((AVRDLC_C-1)*AVRDLC_LENGTH+3)
    .set AVRDLC_LENGTH = (AVRDLC_LENGTH-2)/256
    .if AVRDLC_C == 256
        .set AVRDLC_C = 0
    .endif
    ldir AVRDLC_STARTING_REG+AVRDLC_REG_I, AVRDLC_C
    .set AVRDLC_REG_I = AVRDLC_REG_I+1
    .if AVRDLC_REG_I < AVRDLC_DEPTH
        avrdlc_ldi_part
    .endif
.endm

;******************************************************************************
;*
;* MACRO
;*      avrdlc_loops_part
;*
;* DESCRIPTION
;*      Generates the looping constructs as needed. The register numbers and
;*      branch target addresses gets calculated for each.
;*
;******************************************************************************

.macro avrdlc_loops_part
    .if AVRDLC_REG_I >= 0
        decr AVRDLC_STARTING_REG+AVRDLC_REG_I
        brne    PC+AVRDLC_PC_OFFSET
        .set AVRDLC_REG_I = AVRDLC_REG_I-1
        .set AVRDLC_PC_OFFSET = AVRDLC_PC_OFFSET-2
        avrdlc_loops_part
    .endif
.endm

;******************************************************************************
;*
;* MACRO
;*      ldir reg, byte
;*
;* DESCRIPTION
;*      Extension of the ldi instruction, that accepts an expression, that
;*      yields an integer value between 16 and 31 for reg, to allow calculated
;*      register numbers.
;*
;******************************************************************************

.macro ldir
    .if (@0 < 16) || (@0 > 31)
        .error "LDIR expects a high reg number as first arg. Eg. LDIR 16, 0x00"
    .endif
    .if @1 > 255
        .error "LDIR expects a byte as second argument. Eg. LDIR 16, 0x00"
    .endif
    .dw 0xe000 | ((@0 - 16) << 4) | ((@1 & 0xf0) << 4) | (@1 & 0x0f)
.endm

;******************************************************************************
;*
;* MACRO
;*      decr reg
;*
;* DESCRIPTION
;*      Extension of the dec instruction, that accepts an expression, that
;*      yields an integer value between 0 and 31 for reg, to allow calculated
;*      register numbers.
;*
;******************************************************************************

.macro decr
    .if (@0 < 0) || (@0 > 31)
        .error "DECR expects a register number between 0 and 31. Eg. DECR 16"
    .endif
    .dw 0x940a | (@0 << 4)
.endm

#endif  /* _AVRDLC_INC_ */

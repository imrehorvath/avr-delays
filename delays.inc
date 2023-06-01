;******************************************************************************
;* Title        : DELAYS
;* Version      : 1.0
;* Last updated : Mar 31 2023
;* File         : delays.inc
;* Author       : Imre Horvath <imi.horvath [at] gmail [dot] com>
;* License      : MIT
;* Description  : A set of assembler macros for the AVRASM2 assembler -based on
;*                Jan Huygh's original- to generate busy-wait loops, that
;*                wastes the required amount of clock cycles for a given miro-
;*                or milliseconds on an AVR with CPU clock frequency F_CPU.
;******************************************************************************

#ifndef _DELAYS_INC_
#define _DELAYS_INC_

;******************************************************************************
;*
;* MACRO
;*      delays_delay_us N
;*
;* DESCRIPTION
;*      Generates code, that takes N microseconds to execute given F_CPU, by
;*      wasting CPU cycles.
;*
;******************************************************************************

.macro delays_delay_us
    .set DELAYS_CYCLES = @0*F_CPU/1000000
    .if (@0*F_CPU/100000)-(@0*F_CPU/1000000)*10 >= 5
        .set DELAYS_CYCLES = DELAYS_CYCLES+1
    .endif
    .if !DELAYS_CYCLES
        .error "Too less delay (for this F_CPU value)."
    .endif
    .if DELAYS_CYCLES >= 25769803780
        .error "Too much delay."
    .endif
    delays_waste_cycles DELAYS_CYCLES
.endm

;******************************************************************************
;*
;* MACRO
;*      delays_delay_ms N
;*
;* DESCRIPTION
;*      Generates code, that wastes CPU cycles by calling
;*      delays_delay_us N*1000.
;*
;******************************************************************************

.macro delays_delay_ms
    delays_delay_us @0*1000
.endm

;******************************************************************************
;*
;* MACRO
;*      delays_waste_cycles N
;*
;* DESCRIPTION
;*      Generates code, that wastes N clock cycles, by successively breaking-
;*      down the number of cycles to 32, 24, 16 and 8 bit counters and
;*      instructions wasting small amounts, until all cycles gets consumed.
;*      Based on the value of N, the generated code can modify the registers
;*      r16, r17, r18 and r19.
;*
;******************************************************************************

.macro delays_waste_cycles
    .if @0 == 1
    ; used_cycles = 1
        nop
    .endif
    .if @0 == 2
    ; used_cycles = 2
        rjmp    PC+1
    .endif
    .if @0 > 2 && @0 < 769
    ; used_cycles = 3*loop_count
    ; maximum_cycles = 3*2^8 = 768
        .set DELAYS_LOOP_COUNT = @0/3
        ldi     r16, low(DELAYS_LOOP_COUNT)
    loop:
        dec     r16
        brne    loop
        .set DELAYS_REST_CYCLES = @0-3*DELAYS_LOOP_COUNT
        delays_waste_cycles DELAYS_REST_CYCLES
    .endif
    .if @0 > 768 && @0 < 262146
    ; used_cycles = 4*loop_count+1
    ; maximum_cycles = 4*2^16+1 = 262145
        .set DELAYS_LOOP_COUNT = (@0-1)/4
        ldi     r16, low(DELAYS_LOOP_COUNT)
        ldi     r17, high(DELAYS_LOOP_COUNT)
    loop:
        subi    r16, 1
        sbci    r17, 0
        brne    loop
        .set DELAYS_REST_CYCLES = @0-(4*DELAYS_LOOP_COUNT+1)
        delays_waste_cycles DELAYS_REST_CYCLES
    .endif
    .if @0 > 262145 && @0 < 83886083
    ; used_cycles = 5*loop_count+2
    ; maximum_cycles = 5*2^24+2 = 83886082
        .set DELAYS_LOOP_COUNT = (@0-2)/5
        ldi     r16, low(DELAYS_LOOP_COUNT)
        ldi     r17, byte2(DELAYS_LOOP_COUNT)
        ldi     r18, byte3(DELAYS_LOOP_COUNT)
    loop:
        subi    r16, 1
        sbci    r17, 0
        sbci    r18, 0
        brne    loop
        .set DELAYS_REST_CYCLES = @0-(5*DELAYS_LOOP_COUNT+2)
        delays_waste_cycles DELAYS_REST_CYCLES
    .endif
    .if @0 > 83886082 && @0 < 25769803780
    ; used_cycles = 6*loop_count+3
    ; maximum_cycles = 6*2^32+3 = 25769803779
        .set DELAYS_LOOP_COUNT = (@0-3)/6
        ldi     r16, low(DELAYS_LOOP_COUNT)
        ldi     r17, byte2(DELAYS_LOOP_COUNT)
        ldi     r18, byte3(DELAYS_LOOP_COUNT)
        ldi     r19, byte4(DELAYS_LOOP_COUNT)
    loop:
        subi    r16, 1
        sbci    r17, 0
        sbci    r18, 0
        sbci    r19, 0
        brne    loop
        .set DELAYS_REST_CYCLES = @0-(6*DELAYS_LOOP_COUNT+3)
        delays_waste_cycles DELAYS_REST_CYCLES
    .endif
.endm

#endif  /* _DELAYS_INC_ */

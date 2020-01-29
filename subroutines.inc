;
; File: subroutines.inc (part of Feron Led Advanced Control project)
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)

; Hue rotation function

rotate_hue:
    
    sbrc  STATUS_REG, INV_BIT           ; [2][1]
    rjmp  PC+6                          ; [0][2] ->
    
    subi  HB_VALUE_REG, -1              ; [1][1]
    brhs  PC+3                          ; [1][2]
    sbr   STATUS_REG, _BV(INV_BIT)      ; [1][0]
    subi  HB_VALUE_REG, 2               ; [1][0]
    ret                                 ; [4][4]
    
    subi  HB_VALUE_REG, 1               ; [1][1] <-
    brhc  PC+3                          ; [1][2]
    cbr   STATUS_REG, _BV(INV_BIT)      ; [1][0]
    subi  HB_VALUE_REG, -2              ; [1][0]
    ret                                 ; [4][4]

; Update both channels target registers from HB value (0xBH: high 4-bits is Brightness, low - Hue)
; [11]: power is OFF, [16]: power is ON

update_target_registers:
    
    sbrs  STATUS_REG, PWR_BIT           ; [2][1]
    rjmp  reset_target_registers        ; [0][2] ->

update_from_table:
    
    ; POWER IS ON. LOAD PRECALCULATED VALUE FROM TABLE FOR BOTH CHANNELS
    mov   ZL, HB_VALUE_REG              ; [1]
    lpm   CH1_TARGET_REG, Z             ; [3]
    ldi   TMP_REG, 0b1111               ; [1]
    eor   ZL, TMP_REG                   ; [1]
    lpm   CH2_TARGET_REG, Z             ; [3]
    ret                                 ; [4][=16] =>
    
reset_target_registers:
    
    ; POWER IS OFF
    ldi   TMP_REG, PWM_OFF              ; [1] <-
    mov   CH1_TARGET_REG, TMP_REG       ; [1]
    mov   CH2_TARGET_REG, TMP_REG       ; [1]
    ret                                 ; [4][=11] =>


; eeprom_write function

; Optimized function to store byte to EEPROM
; Inputs: TIMLO_REG: address, TIMHI_REG: byte to store

; 1. Waiting for previous write is complete;
; 2. Read old byte from specified address, compare it with new byte;
; 3. If bytes are equals - do nothing.
; 4. Else analyse bit changes:
;    a. If bits needs only to write (set to '0') - start 'write only' op.
;    b. Else if new byte equals 255 - start 'erase only' op.
;    c. Else - start 'atomic: erase + write' op.

eeprom_write:

    sbic  EECR,  EEPE                       ; [2][1] <-
    rjmp  PC-1                              ; [0][2] ->
    out   EEARL, TIMLO_REG                  ; [1]
    sbi   EECR, EERE                        ; [2+4]
    in    TMP_REG, EEDR                     ; [1]
    eor   TMP_REG, TIMHI_REG                ; [1]
    breq  PC+11                             ; [1][2] -> update not needed. return.
    and   TMP_REG, TIMHI_REG                ; [1]
    ldi   TMP_REG, _BV(EEPM1) | _BV(EEMPE)  ; [1]
    breq  PC+5                              ; [1][2] -> write only op start.
    ldi   TMP_REG, _BV(EEPM0) | _BV(EEMPE)  ; [1]
    cpi   TIMHI_REG, 0xFF                   ; [1]
    breq  PC+3                              ; [1][2] -> erase only op start.
    ldi   TMP_REG, _BV(EEMPE)               ; [1] atomic: erase + write op start
    out   EEDR,  TIMHI_REG                  ; [1]
    out   EECR,  TMP_REG                    ; [1]
    sbi   EECR,  EEPE                       ; [2+2]
    ret                                     ; [4]

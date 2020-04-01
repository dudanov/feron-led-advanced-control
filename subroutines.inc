;
; File: subroutines.inc (part of Feron Light Advanced Control project)
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; Git    : https://github.com/dudanov/feron-light-advanced-control
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)

; rotate_hue: low nibble rotation function

rotate_hue:

    sbrc  STATUS_REG, INV_BIT           ; [2][1]
    rjmp  PC+6                          ; [0][2] -> 1:
    
    subi  BH_VALUE_REG, -1              ; [1]
    brhs  PC+3                          ; [1][2] -> 2:
    sbr   STATUS_REG, _BV(INV_BIT)      ; [1]
    subi  BH_VALUE_REG, 2               ; [1]
    ret                                 ; [4] <- 2:
    
    subi  BH_VALUE_REG, 1               ; [1] <- 1:
    brhc  PC+3                          ; [1][2] -> 3:
    cbr   STATUS_REG, _BV(INV_BIT)      ; [1]
    subi  BH_VALUE_REG, -2              ; [1]
    ret                                 ; [4] <- 3:

; Update both channels target registers from HB value (0xBH: high 4-bits is Brightness, low - Hue)
; [10]: power is OFF, [15]: power is ON

update_target_registers:
    
    sbrs  STATUS_REG, PWR_BIT           ; [2][1]
    rjmp  reset_target_registers        ; [0][2] -> 1:

update_from_table:
    
    ; POWER IS ON. LOAD PRECALCULATED VALUE FROM TABLE FOR BOTH CHANNELS
    mov   ZL, BH_VALUE_REG              ; [1]
    lpm   CH1_TARGET_REG, Z             ; [3]
    ldi   TMP_REG, 0x0F                 ; [1]
    eor   ZL, TMP_REG                   ; [1]
    lpm   CH2_TARGET_REG, Z             ; [3]
    ret                                 ; [4]
    
reset_target_registers:
    
    ; POWER IS OFF
    ldi   TMP_REG, PWM_OFF              ; [1] <- 1:
    mov   CH1_TARGET_REG, TMP_REG       ; [1]
    mov   CH2_TARGET_REG, TMP_REG       ; [1]
    ret                                 ; [4]


; eeprom_read function
; Input:  TMP_REG: EEPROM address
; Output: BH_VALUE_REG: byte value

eeprom_read:
    
    sbic  EECR,  EEPE                   ; [2][1] <- 1:
    rjmp  PC-1                          ; [0][2] -> 1:

eeprom_read_unsafe:
    
    out   EEARL, TMP_REG                ; [1]
    sbi   EECR, EERE                    ; [2+4]
    in    BH_VALUE_REG, EEDR            ; [1]
    ret                                 ; [4]

; eeprom_write_bh function
; Optimized function to store byte to EEPROM
; Inputs: TMP_REG (corrupted): EEPROM address, BH_VALUE_REG (not corrupted): byte to store

; 1. Waiting for previous write is complete;
; 2. Read old byte from specified address, compare it with new byte;
; 3. If bytes are equals - do nothing.
; 4. Else analyse bit changes:
;    a. If bits needs only to write (set to '0') - start 'write only' op.
;    b. Else if new byte equals 0xFF - start 'erase only' op.
;    c. Else - start 'atomic: erase + write' op.

eeprom_write_bh:
    
    sbic  EECR,  EEPE                       ; [2][1] <- 1:
    rjmp  PC-1                              ; [0][2] -> 1:
    out   EEARL, TMP_REG                    ; [1]
    sbi   EECR, EERE                        ; [2+4]
    in    TMP_REG, EEDR                     ; [1]
    eor   TMP_REG, BH_VALUE_REG             ; [1]
    breq  PC+11                             ; [1][2] -> 2: update not needed
    and   TMP_REG, BH_VALUE_REG             ; [1]
    ldi   TMP_REG, _BV(EEPM1) | _BV(EEMPE)  ; [1] write op
    breq  PC+5                              ; [1][2] -> 3: write op start
    ldi   TMP_REG, _BV(EEPM0) | _BV(EEMPE)  ; [1] erase op
    cpi   BH_VALUE_REG, 0xFF                ; [1]
    breq  PC+3                              ; [1][2] -> 4: erase op start
    ldi   TMP_REG, _BV(EEMPE)               ; [1] atomic: erase + write op
    out   EEDR,  BH_VALUE_REG               ; [1] <- 3:
    out   EECR,  TMP_REG                    ; [1] <- 4:
    sbi   EECR,  EEPE                       ; [2+2]
    ret                                     ; [4] <- 2:

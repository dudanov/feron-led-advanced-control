;
; File   : main.inc (part of "Feron Light Advanced Control" project)
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)

_main_loop:
    
    ; NEXT RANDOM VALUE
    lsr   RAND_REG                      ; [1]
    brcc  PC+3                          ; [1][2] ->
    ldi   TMP_REG, RAND_MASK            ; [1]
    eor   RAND_REG, TMP_REG             ; [1]

    ; CHECK FOR TIMER EVENT (~0.75 NEC_T)
    in    TMP_REG, TIM_FLAGS_REG        ; [1]
    sbrs  TMP_REG, TIM_FLAG_BIT         ; [2][1]
    rjmp  pcint_event                   ; [0][2] -> NO OVERFLOW. CHECK FOR PIN CHANGE EVENT.
    
    ; TIMER IS OVERFLOW
    out   TIM_FLAGS_REG, TMP_REG        ; [1]
    inc   NECCNT_REG                    ; [1]
    brne  PC+2                          ; [1][2]
    dec   NECCNT_REG                    ; [1][0]
    dec   PWRTMR_REG                    ; [1]
    brne  pcint_event                   ; [1][2] -> POWER IS OK. CHECK FOR PIN CHANGE EVENT.
    
    ; AC POWER LOSS. STORE RETAIN DATA TO EEPROM.
    
.if PD_MASK
    out   DDRB,  ZERO_REG                   ; [1]
.endif
    
    ; *** STORE HB TO EEPROM ***
    ldi   TIMLO_REG, eeprom_hb              ; [1]
    mov   TIMHI_REG, HB_VALUE_REG           ; [1]
    rcall eeprom_write                      ; [3]

    ; *** STORE STATUS TO EEPROM ***
    ldi   TIMLO_REG, eeprom_status          ; [1]
    mov   TIMHI_REG, STATUS_REG             ; [1]
    ori   TIMHI_REG, ~STATUS_EEPROM_MASK    ; [1]
    andi  STATUS_REG, STATUS_EEPROM_MASK    ; [1]
    rcall eeprom_write                      ; [3]

.if PD_MASK
    ; Waiting for write is completed
    sbic  EECR,  EEPE                       ; [2][1] <-
    rjmp  PC-1                              ; [0][2] ->
    ; FAST DISCHARGE POWER CAPACITORS
    ldi   TMP_REG, PD_MASK                  ; [1]
    out   DDRB, TMP_REG                     ; [1]
.endif
    
    ; WAIT UNTIL AC POWER HAS BEEN RESTORED OR RESET BY BOD
    sbis  PINB, AC_PIN                      ; [2][1] <-
    rjmp  PC-1                              ; [0][2] ->
    
pcint_event:
    
    ; CHECK FOR PIN CHANGE EVENT
    mov   PINCHG_REG, PIN_REG           ; [1] <-
    in    PIN_REG, PINB                 ; [1]
    eor   PINCHG_REG, PIN_REG           ; [1]
    
    ; CHECK FOR IR SIGNAL CHANGE
    sbrs  PINCHG_REG, TSSOP_PIN         ; [2][1]
    rjmp  ac_event                      ; [0][2] -> NO IR SIGNAL CHANGE. CHECK FOR AC PIN CHANGE
    
.include "..\nec.inc"
    
ac_event:
    
    ; CHECK FOR AC EVENT (POSITIVE EDGE)
    sbrc  PINCHG_REG, AC_PIN            ; [2][1]
    sbrs  PIN_REG, AC_PIN               ; [2][1]
    rjmp  _main_loop                    ; [0][2] -> NO AC EVENT. BACK TO START LOOP
    
    ; **** START AC EVENT ****
    
    ldi   PWRTMR_REG, MS_T(AC_LOSS_MS)  ; [1] reset power timer
    
    dec   TICK_REG                      ; [1]
    brne  ac_skip_1s                    ; [1][2] ->
    
    ; **** START OF 1S BLOCK ****
    
    ldi   TICK_REG, AC_FREQ             ; [1]
    sbrs  STATUS_REG, TMR_BIT           ; [2][1]
    rjmp  ac_skip_1s                    ; [0][2] -> timer not active
    ; Timer tick
    sbiw  TIMLO_REG, 1                  ; [1]
    brne  ac_skip_1s                    ; [1][2]
    andi  STATUS_REG, _BV(INV_BIT)      ; [1]
    rcall reset_target_registers        ; [10]
    
    ; **** END OF 1S BLOCK ****
    
ac_skip_1s:
    
    sbrs  TICK_REG, 0                   ; [2][1]
    rjmp  _main_loop                    ; [0][2] -> main loop
    
.include "ctrl.inc"
    
    ; **** END OF AC EVENT ****
    
    rjmp _main_loop                     ; [2] -> BACK TO THE START
    
.include "..\subroutines.inc"
    
; In last 128 words (256 bytes) of Flash memory allocate Brightness-Hue to OCR value conversion table.
    
.org FLASHEND - 127
    
pm_table:
    
; AVR Assembler doesn't have repeat macro extensinons. So, enter all this terrible table by hands... 8)
    
    .db BH2OCR(0x00),BH2OCR(0x01),BH2OCR(0x02),BH2OCR(0x03),BH2OCR(0x04),BH2OCR(0x05),BH2OCR(0x06),BH2OCR(0x07)
    .db BH2OCR(0x08),BH2OCR(0x09),BH2OCR(0x0A),BH2OCR(0x0B),BH2OCR(0x0C),BH2OCR(0x0D),BH2OCR(0x0E),BH2OCR(0x0F)
    .db BH2OCR(0x10),BH2OCR(0x11),BH2OCR(0x12),BH2OCR(0x13),BH2OCR(0x14),BH2OCR(0x15),BH2OCR(0x16),BH2OCR(0x17)
    .db BH2OCR(0x18),BH2OCR(0x19),BH2OCR(0x1A),BH2OCR(0x1B),BH2OCR(0x1C),BH2OCR(0x1D),BH2OCR(0x1E),BH2OCR(0x1F)
    .db BH2OCR(0x20),BH2OCR(0x21),BH2OCR(0x22),BH2OCR(0x23),BH2OCR(0x24),BH2OCR(0x25),BH2OCR(0x26),BH2OCR(0x27)
    .db BH2OCR(0x28),BH2OCR(0x29),BH2OCR(0x2A),BH2OCR(0x2B),BH2OCR(0x2C),BH2OCR(0x2D),BH2OCR(0x2E),BH2OCR(0x2F)
    .db BH2OCR(0x30),BH2OCR(0x31),BH2OCR(0x32),BH2OCR(0x33),BH2OCR(0x34),BH2OCR(0x35),BH2OCR(0x36),BH2OCR(0x37)
    .db BH2OCR(0x38),BH2OCR(0x39),BH2OCR(0x3A),BH2OCR(0x3B),BH2OCR(0x3C),BH2OCR(0x3D),BH2OCR(0x3E),BH2OCR(0x3F)
    .db BH2OCR(0x40),BH2OCR(0x41),BH2OCR(0x42),BH2OCR(0x43),BH2OCR(0x44),BH2OCR(0x45),BH2OCR(0x46),BH2OCR(0x47)
    .db BH2OCR(0x48),BH2OCR(0x49),BH2OCR(0x4A),BH2OCR(0x4B),BH2OCR(0x4C),BH2OCR(0x4D),BH2OCR(0x4E),BH2OCR(0x4F)
    .db BH2OCR(0x50),BH2OCR(0x51),BH2OCR(0x52),BH2OCR(0x53),BH2OCR(0x54),BH2OCR(0x55),BH2OCR(0x56),BH2OCR(0x57)
    .db BH2OCR(0x58),BH2OCR(0x59),BH2OCR(0x5A),BH2OCR(0x5B),BH2OCR(0x5C),BH2OCR(0x5D),BH2OCR(0x5E),BH2OCR(0x5F)
    .db BH2OCR(0x60),BH2OCR(0x61),BH2OCR(0x62),BH2OCR(0x63),BH2OCR(0x64),BH2OCR(0x65),BH2OCR(0x66),BH2OCR(0x67)
    .db BH2OCR(0x68),BH2OCR(0x69),BH2OCR(0x6A),BH2OCR(0x6B),BH2OCR(0x6C),BH2OCR(0x6D),BH2OCR(0x6E),BH2OCR(0x6F)
    .db BH2OCR(0x70),BH2OCR(0x71),BH2OCR(0x72),BH2OCR(0x73),BH2OCR(0x74),BH2OCR(0x75),BH2OCR(0x76),BH2OCR(0x77)
    .db BH2OCR(0x78),BH2OCR(0x79),BH2OCR(0x7A),BH2OCR(0x7B),BH2OCR(0x7C),BH2OCR(0x7D),BH2OCR(0x7E),BH2OCR(0x7F)
    .db BH2OCR(0x80),BH2OCR(0x81),BH2OCR(0x82),BH2OCR(0x83),BH2OCR(0x84),BH2OCR(0x85),BH2OCR(0x86),BH2OCR(0x87)
    .db BH2OCR(0x88),BH2OCR(0x89),BH2OCR(0x8A),BH2OCR(0x8B),BH2OCR(0x8C),BH2OCR(0x8D),BH2OCR(0x8E),BH2OCR(0x8F)
    .db BH2OCR(0x90),BH2OCR(0x91),BH2OCR(0x92),BH2OCR(0x93),BH2OCR(0x94),BH2OCR(0x95),BH2OCR(0x96),BH2OCR(0x97)
    .db BH2OCR(0x98),BH2OCR(0x99),BH2OCR(0x9A),BH2OCR(0x9B),BH2OCR(0x9C),BH2OCR(0x9D),BH2OCR(0x9E),BH2OCR(0x9F)
    .db BH2OCR(0xA0),BH2OCR(0xA1),BH2OCR(0xA2),BH2OCR(0xA3),BH2OCR(0xA4),BH2OCR(0xA5),BH2OCR(0xA6),BH2OCR(0xA7)
    .db BH2OCR(0xA8),BH2OCR(0xA9),BH2OCR(0xAA),BH2OCR(0xAB),BH2OCR(0xAC),BH2OCR(0xAD),BH2OCR(0xAE),BH2OCR(0xAF)
    .db BH2OCR(0xB0),BH2OCR(0xB1),BH2OCR(0xB2),BH2OCR(0xB3),BH2OCR(0xB4),BH2OCR(0xB5),BH2OCR(0xB6),BH2OCR(0xB7)
    .db BH2OCR(0xB8),BH2OCR(0xB9),BH2OCR(0xBA),BH2OCR(0xBB),BH2OCR(0xBC),BH2OCR(0xBD),BH2OCR(0xBE),BH2OCR(0xBF)
    .db BH2OCR(0xC0),BH2OCR(0xC1),BH2OCR(0xC2),BH2OCR(0xC3),BH2OCR(0xC4),BH2OCR(0xC5),BH2OCR(0xC6),BH2OCR(0xC7)
    .db BH2OCR(0xC8),BH2OCR(0xC9),BH2OCR(0xCA),BH2OCR(0xCB),BH2OCR(0xCC),BH2OCR(0xCD),BH2OCR(0xCE),BH2OCR(0xCF)
    .db BH2OCR(0xD0),BH2OCR(0xD1),BH2OCR(0xD2),BH2OCR(0xD3),BH2OCR(0xD4),BH2OCR(0xD5),BH2OCR(0xD6),BH2OCR(0xD7)
    .db BH2OCR(0xD8),BH2OCR(0xD9),BH2OCR(0xDA),BH2OCR(0xDB),BH2OCR(0xDC),BH2OCR(0xDD),BH2OCR(0xDE),BH2OCR(0xDF)
    .db BH2OCR(0xE0),BH2OCR(0xE1),BH2OCR(0xE2),BH2OCR(0xE3),BH2OCR(0xE4),BH2OCR(0xE5),BH2OCR(0xE6),BH2OCR(0xE7)
    .db BH2OCR(0xE8),BH2OCR(0xE9),BH2OCR(0xEA),BH2OCR(0xEB),BH2OCR(0xEC),BH2OCR(0xED),BH2OCR(0xEE),BH2OCR(0xEF)
    .db BH2OCR(0xF0),BH2OCR(0xF1),BH2OCR(0xF2),BH2OCR(0xF3),BH2OCR(0xF4),BH2OCR(0xF5),BH2OCR(0xF6),BH2OCR(0xF7)
    .db BH2OCR(0xF8),BH2OCR(0xF9),BH2OCR(0xFA),BH2OCR(0xFB),BH2OCR(0xFC),BH2OCR(0xFD),BH2OCR(0xFE),BH2OCR(0xFF)
    
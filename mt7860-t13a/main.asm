;
; Feron Light Advanced Control
;
; This program control Maxic MT7860 chip (PWM method) based luminaries.
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; Git    : https://github.com/dudanov/feron-light-advanced-control
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)
;
; MCU:   ATTINY13A @ 600KHz
; LFUSE: 0x21
; HFUSE: 0xF9

.equ MCU_CLK_NS         = 1667  ; MCU clock period in nanoseconds.
.equ TIM_DIV            = 256   ; In this project main tick based on TIM0 overflow.

;.equ OSCCAL_VALUE       = 74    ; RC internal 4.8MHz oscillator calibration value (0..127). Comment if not necessary. Manufacturer value may be reading by programmer tool at address 0x01.

.equ PWM1_PIN           = PB0   ; OC0A pin number
.equ PWM2_PIN           = PB1   ; OC0B pin number
.equ TSSOP_PIN          = PB2   ; TSSOP IR receiver pin number
.equ AC_PIN             = PB3   ; AC pin number

; MT7860 PWM DUTY LEVELS CONTROL DEFINITIONS (DO NOT EDIT!)
.equ PWM_MIN            = ( 20 * 256 - 1 ) / 100    ; 20% - MIN
.equ PWM_MAX            = ( 80 * 256 - 1 ) / 100    ; 80% - MAX

.include "../defines.inc"   ; must be here (define some variables if it not defined & macro _BV() used in code below)

; PWM timer defines
.equ OCR1_REG           = OCR0A
.equ OCR2_REG           = OCR0B
.equ TIM_FLAGS_REG      = TIFR0 ; Timer flags register
.equ TIM_FLAG_BIT       = TOV0  ; Timer flag
.equ PWM_ON_CTRLA       = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00)
.equ PWM_OFF_CTRLA      = _BV(COM0A1) | _BV(COM0B1)
.equ PWM_ON_CTRLB       = _BV(CS00)

.cseg

; **** START OF SETUP CODE ****

    ; Set PB0(OC0A) and PB1(OC0B) to Output mode.
    ldi   TMP_REG, PWM_MASK             ; [1]
    out   DDRB, TMP_REG                 ; [1]

    ; Disable ADC (power reduction)
    ldi  TMP_REG, _BV(PRADC)            ; [1]
    out  PRR, TMP_REG                   ; [1]

    ; Pull-up floating pins after getting random bits for seed.
    ldi   TMP_REG, PU_MASK              ; [1]
    out   PORTB, TMP_REG                ; [1]

.ifdef OSCCAL_VALUE
    ; Load oscillator calibration byte
    ldi   TMP_REG, OSCCAL_VALUE         ; [1]
    out   OSCCAL, TMP_REG               ; [1]
.endif

    ; INIT STACK POINTER
    ldi  TMP_REG, RAMEND                ; [1]
    out  SPL, TMP_REG                   ; [1]

    ; INIT GLOBAL REGISTERS
    clr   ZERO_REG                      ; [1]
    ldi   TICK_REG, AC_FREQ             ; [1]
    ldi   PWRTMR_REG, MS_T(AC_LOSS_MS)  ; [1]
    ldi   XH, high(sram_nec_data)       ; [1]
    ldi   YL, low(sram_nec_data)        ; [1]
    ldi   YH, high(sram_nec_data)       ; [1]
    ldi   ZH, high(pm_table*2)          ; [1]

    ; LOAD DATA FROM EEPROM (Address: 0x00 - HB-value, 0x01 - power status)
    ldi   TMP_REG, eeprom_status        ; [1]
    
    ; Waiting for write is completed
    sbic  EECR, EEPE                    ; [2][1] <-
    rjmp  PC-1                          ; [0][2] ->
    
    out   EEARL, TMP_REG                ; [1]
    sbi   EECR, EERE                    ; [2+4]
    in    STATUS_REG, EEDR              ; [1]
    andi  STATUS_REG, STATUS_EEPROM_MASK; [1]
    
    out   EEARL, ZERO_REG               ; [1]
    sbi   EECR, EERE                    ; [2+4]
    in    HB_VALUE_REG, EEDR            ; [1]

    rcall update_target_registers       ; [14|19]

    ; SET INIT PWM DUTY TO <20%
    ldi   TMP_REG, PWM_OFF              ; [1]
    out   OCR1_REG, TMP_REG             ; [1]
    out   OCR2_REG, TMP_REG             ; [1]
    ; START TIMER WITHOUT PRESCALER @600KHz
    ldi   TMP_REG, PWM_ON_CTRLB         ; [1]
    out   TCCR0B, TMP_REG               ; [1]

    ; INIT PSEUDO-RANDOM REGISTER
    ldi   TMP_REG, RAND_INIT            ; [1]
    mov   RAND_REG, TMP_REG             ; [1]

    ; STORE INIT INPUT PINS STATE
    in    PIN_REG, PINB                 ; [1]

; **** END OF SETUP CODE ****

.include "..\main.inc"

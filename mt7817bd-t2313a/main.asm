;
; Feron Light Advanced Control
;
; This program control Maxic MT7817BD chip (PWM RC analog (700..1600 mV) method, more precise control than PWM) based luminaries.
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)
;
; MCU:   ATTINY2313A @500KHz
; LFUSE: 0x62
; HFUSE: 0x99
; EFUSE: 0xFF

.equ MCU_CLK_NS         = 2000  ; MCU clock period in nanoseconds.

.equ INIT_PWR_STATE     = 2     ; Init power state. 0: OFF, 1: ON, 2: LAST
.equ WORK_MODE          = 0     ; Program work mode: 0 - normal mode, 1 - PWM_TOP value select, 2 - PWM_MIN value select.

;.equ OSCCAL_VALUE       = 74    ; RC internal 4.0MHz oscillator calibration value (0..127). Comment if not necessary. Manufacturer value may be reading by programmer tool at address 0x01.

; TIMING VALUES. SELECTED EXPERIMENTALLY IN THE APPROPRIATE WORK MODE.
; 1. PWM TOP VALUE: selected in WORK_MODE equals 1. The output voltage after the RC circuit should be about 1600 mV. Inverse dependence of the output value.
.equ PWM_TOP            = 948
; 2. PWM MIN VALUE: selected in WORK_MODE equals 2. The output voltage after the RC circuit should be about 700 mV. Direct dependence of the output value.
.equ PWM_MIN            = 98

; Pin defines
.equ PWM1_PIN           = PB3   ; OC1A pin number
.equ PWM2_PIN           = PB4   ; OC1B pin number
.equ TSSOP_PIN          = PB5   ; TSSOP IR receiver pin number
.equ AC_PIN             = PB6   ; AC pin number
.equ PWM1_PD_PIN        = PB1   ; OC1A pull-down pin number
.equ PWM2_PD_PIN        = PB0   ; OC1B pull-down pin number

.include "..\defines.inc"   ; must be here (define some variables if it not defined & macro _BV() used in code below)

; PWM timer defines
.equ OCR1_REG           = OCR1AL
.equ OCR2_REG           = OCR1BL
.equ TIM_FLAGS_REG      = TIFR          ; Timer flags register
.equ TIM_FLAG_BIT       = OCF0A         ; Timer flag
.equ PWM_ON_CTRLA       = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11)
.equ PWM_OFF_CTRLA      = _BV(COM1A1) | _BV(COM1B1)
.equ PWM_ON_CTRLB       = _BV(WGM13) | _BV(WGM12) | _BV(CS10)

.cseg

.if WORK_MODE == 0

; **** START OF SETUP CODE ****

    ldi   TMP_REG, PD_MASK                  ; [1]
    out   DDRB, TMP_REG                     ; [1]
    
    ; Disable USI and USART
    ldi  TMP_REG, _BV(PRUSI) | _BV(PRUSART) ; [1]
    out  PRR, TMP_REG                       ; [1]

    ; Pull-up floating pins
    ldi   TMP_REG, PU_MASK                  ; [1]
    out   PORTB, TMP_REG                    ; [1]
    ser   TMP_REG                           ; [1]
    out   PORTA, TMP_REG                    ; [1]
    out   PORTD, TMP_REG                    ; [1]

 .ifdef OSCCAL_VALUE
    ; Load oscillator calibration byte
    ldi   TMP_REG, OSCCAL_VALUE             ; [1]
    out   OSCCAL, TMP_REG                   ; [1]
 .endif
    
    ; INIT STACK POINTER
    ldi  TMP_REG, RAMEND                    ; [1]
    out  SPL, TMP_REG                       ; [1]

    ; INIT GLOBAL REGISTERS
    clr   ZERO_REG                          ; [1]
    ldi   TICK_REG, AC_FREQ                 ; [1]
    ldi   PWRTMR_REG, MS_T(AC_LOSS_MS)      ; [1]
    ldi   XH, high(sram_nec_data)           ; [1]
    ldi   YL, low(sram_nec_data)            ; [1]
    ldi   YH, high(sram_nec_data)           ; [1]
    ldi   ZH, high(pm_table*2)              ; [1]

.if INIT_PWR_STATE > 1
    ; load last state from EEPROM
    ldi   TMP_REG, ee_status                ; [1]
    rcall eeprom_read                       ; [17+]
    mov   STATUS_REG, BH_VALUE_REG          ; [1]
    andi  STATUS_REG, _BV(PWR_BIT)          ; [1]
    ; load BH from EEPROM
    ldi   TMP_REG, ee_bh_current            ; [1]
    rcall eeprom_read_unsafe                ; [15]
    rcall update_target_registers           ; [13+]
.else
    ; load BH from EEPROM
    ldi   TMP_REG, ee_bh_current            ; [1]
    rcall eeprom_read                       ; [17+]
 .if INIT_PWR_STATE == 1
    ldi   STATUS_REG, _BV(PWR_BIT)          ; [1]
    rcall update_from_table                 ; [18]
 .else
    clr   STATUS_REG                        ; [1]
    rcall reset_target_registers            ; [13]
 .endif
.endif
    
    ; START TIM1 IN FAST PWM MODE (TOP IN ICR1) WITHOUT PRESCALER @500KHz. OC1A and OC1B pins not yet enabled as outputs.
    ldi   TMP_REG, high(PWM_TOP)            ; [1]
    out   ICR1H, TMP_REG                    ; [1]
    ldi   TMP_REG, low(PWM_TOP)             ; [1]
    out   ICR1L, TMP_REG                    ; [1]
    out   OCR1AH, ZERO_REG                  ; [1] clear TIM1 temporary buffer
    ldi   TMP_REG, PWM_OFF                  ; [1]
    out   OCR1_REG, TMP_REG                 ; [1]
    out   OCR2_REG, TMP_REG                 ; [1]
    ldi   TMP_REG, PWM_ON_CTRLA             ; [1]
    out   TCCR1A, TMP_REG                   ; [1]
    ldi   TMP_REG, PWM_ON_CTRLB             ; [1]
    out   TCCR1B, TMP_REG                   ; [1]

    ; START TIM0 IN CTC MODE (TOP IN OCR1_REG, ~0.75 NEC_T) WITHOUT PRESCALER @500KHz
    ldi   TMP_REG, TIM_DIV - 1              ; [1]
    out   OCR0A, TMP_REG                    ; [1]
    ldi   TMP_REG, _BV(WGM01)               ; [1]
    out   TCCR0A, TMP_REG                   ; [1]
    ldi   TMP_REG, _BV(CS00)                ; [1]
    out   TCCR0B, TMP_REG                   ; [1]

    ; INIT PSEUDO-RANDOM REGISTER
    ldi   TMP_REG, RAND_INIT                ; [1]
    mov   RAND_REG, TMP_REG                 ; [1]

    ; STORE INIT INPUT PINS STATE
    in    PIN_REG, PINB                     ; [1]

; **** END OF SETUP CODE ****

.include "..\main.inc"

.else

    ldi   TMP_REG, PWM_MASK                 ; [1]
    out   DDRB, TMP_REG                     ; [1]

 .ifdef OSCCAL_VALUE
    ; Load oscillator calibration byte
    ldi   TMP_REG, OSCCAL_VALUE             ; [1]
    out   OSCCAL, TMP_REG                   ; [1]
 .endif

    ldi   TMP_REG, high(PWM_TOP)            ; [1]
    out   ICR1H, TMP_REG                    ; [1]
    ldi   TMP_REG, low(PWM_TOP)             ; [1]
    out   ICR1L, TMP_REG                    ; [1]
    ldi   TMP_REG, 0                        ; [1]
    out   OCR1AH, TMP_REG                   ; [1] clear TIM1 temporary buffer

 .if WORK_MODE < 2
    ser   TMP_REG                           ; [1]
 .else
    ldi   TMP_REG, PWM_MIN                  ; [1]
 .endif
    
    out   OCR1_REG, TMP_REG                 ; [1]
    out   OCR2_REG, TMP_REG                 ; [1]

    ldi   TMP_REG, PWM_ON_CTRLA             ; [1]
    out   TCCR1A, TMP_REG                   ; [1]
    ldi   TMP_REG, PWM_ON_CTRLB             ; [1]
    out   TCCR1B, TMP_REG                   ; [1]

    rjmp  PC                                ; [1] infinite loop

.endif

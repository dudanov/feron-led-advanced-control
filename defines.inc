;
; File   : defines.inc (part of "Feron Light Advanced Control" project)
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; Git    : https://github.com/dudanov/feron-light-advanced-control
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)

; Checking criticals variables
.ifndef MCU_CLK_NS
 .error "MCU_CLK_NS must be defined (main clock period in nanoseconds)."
.endif
.ifndef PWM_MIN
 .error "PWM_MIN must be defined."
.elseif PWM_MIN < 1 || PWM_MIN > 255
 .error "PWM_MIN must be from 1 to 255."
.endif

.ifndef AC_FREQ
 .equ AC_FREQ           = 50            ; AC frequency (default: 50Hz)
.endif
.ifndef RAND_INIT
 .equ RAND_INIT         = 0b10110101    ; default: 181
.endif
.ifndef RAND_MASK
 .equ RAND_MASK         = 0b11010011    ; default: 211
.endif

; REGISTERS DEFINES (r31:r26 are reserved for indirect RAM access. Beware using it!)

; GLOBAL REGISTERS
; LOW REGISTERS
.def ZERO_REG           = r0    ; ALWAYS ZERO
.def CH1_TARGET_REG     = r1    ; 
.def CH2_TARGET_REG     = r2    ; 
.def RAND_REG           = r3    ; 
.def BYTE_REG           = r4    ; NEC DECODER: CURRENT BYTE
.def PIN_REG            = r5    ; PIN STATES
.def PINCHG_REG         = r6    ; PIN CHANGED
; HIGH REGISTERS
.def STATUS_REG         = r17   ; STATUS
.def BH_VALUE_REG       = r18   ; HB VALUE
.def TICK_REG           = r19   ; SECONDS TICK COUNTER
.def PWRTMR_REG         = r20   ; AC TIMER:     USED FOR POWER LOSS DETECTION
.def BITCNT_REG         = r21   ; NEC:          BITS COUNTER
.def NEC_FRAME_REG      = r22   ; NEC:          FRAME TYPE
.def NECCNT_REG         = r23   ; NEC:          MAIN COUNTER
.def TIMLO_REG          = r24   ; POWER TIMER:  LOW BYTE
.def TIMHI_REG          = r25   ; POWER TIMER:  HIGH BYTE

; SOME LOCAL AND TEMPORARY REGISTERS
.def INCA_REG           = r7    ; 
.def INCB_REG           = r8    ; 
.def TMPA_REG           = r9    ; 
.def TMPB_REG           = r10   ; 
.def TMPLO_REG          = r15   ; TEMPORARY LOW REGISTER
.def TMP_REG            = r16   ; TEMPORARY HIGH REGISTER

#define _BV(b)          ((1 << (b)))

; GPIO defines
.equ IN_MASK            = _BV(TSSOP_PIN) | _BV(AC_PIN)
.equ PWM_MASK           = _BV(PWM1_PIN) | _BV(PWM2_PIN)
.ifdef PWM1_PD_PIN
 .ifdef PWM2_PD_PIN
  .equ PD_MASK          = _BV(PWM1_PD_PIN) | _BV(PWM2_PD_PIN)
 .endif
.endif
.ifndef PD_MASK
 .equ PD_MASK           = 0
.endif
.equ PU_MASK            = ~(IN_MASK | PD_MASK | PWM_MASK)

; PWM defines
.ifndef PWM_MAX
 .equ PWM_MAX           = 255
.endif
.equ PWM_OFF            = PWM_MIN - 1
#define BH2OCR(v)       ((((PWM_MAX) - (PWM_OFF)) * ((v) & 15) * (((v) >> 4) + 1) + (PWM_OFF) * 240 + 120) / 240)

; Time defines
.ifndef TIM_DIV
 .equ TIM_DIV           = (421875 + (MCU_CLK_NS) / 2) / (MCU_CLK_NS)
.endif

#define AC_LOSS_MS      (1200 / (AC_FREQ))
#define TICK_NS         ((MCU_CLK_NS) * (TIM_DIV))
#define NEC_T(t)        ((562500 * (t) + (TICK_NS) / 2) / (TICK_NS))
#define MS_T(t)         ((1000000 * (t) + (TICK_NS) / 2) / (TICK_NS))

; Main status register defines
.equ PWR_BIT            = 0
.equ INV_BIT            = 1
.equ CH1_BIT            = 2
.equ TARGET_BIT         = 3
.equ FRC_BIT            = 4
.equ TMR_BIT            = 5
.equ NEC_LEADER_BIT     = 6
.equ NEC_FIN_BIT        = 7
.equ NEC_STATUS_MASK    = _BV(NEC_LEADER_BIT) | _BV(NEC_FIN_BIT)
.equ NECCNT_MAX         = 255

.ifndef WORK_MODE
 .equ WORK_MODE         = 0
.endif

.if WORK_MODE == 0
 .message "Firmware is assembled in NORMAL mode."
 ; Defines variables in SRAM & EEPROM
 .dseg
    sram_nec_data:  .byte 3
 .eseg
    ee_bh_storage:  .byte 16
    ee_bh_current:  .byte 1
 .if INIT_PWR_STATE > 1
    ee_status:      .byte 1
 .endif
.endif

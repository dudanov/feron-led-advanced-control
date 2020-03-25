;
; File   : ctrl.inc (part of "Feron Light Advanced Control" project)
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)


    ; **** START OF DIMMING CONTROL BLOCK ****

    sbrc  STATUS_REG, FRC_BIT           ; [2][1]
    rjmp  ac_force_off                  ; [0][2] -> Turn OFF immediatly.
    
    clr   INCA_REG                      ; [1]
    in    TMPA_REG, OCR1_REG            ; [1]
    sub   TMPA_REG, CH1_TARGET_REG      ; [1]
    breq  PC+5                          ; [1][2] -> no changing. INCA_REG = 0.
    dec   INCA_REG                      ; [1]
    brcc  PC+3                          ; [1][2] -> decrement. INCA_REG = -1.
    neg   INCA_REG                      ; [1]
    neg   TMPA_REG                      ; [1]
    
    clr   INCB_REG                      ; [1]
    in    TMPB_REG, OCR2_REG            ; [1]
    sub   TMPB_REG, CH2_TARGET_REG      ; [1]
    breq  PC+5                          ; [1][2] -> no changing. INCB_REG = 0.
    dec   INCB_REG                      ; [1]
    brcc  PC+3                          ; [1][2] -> decrement. INCB_REG = -1.
    neg   INCB_REG                      ; [1]
    neg   TMPB_REG                      ; [1]

    sbr   STATUS_REG, _BV(TARGET_BIT)   ; [1]

    cp    INCA_REG, ZERO_REG            ; [1]
    cpc   INCB_REG, ZERO_REG            ; [1]
    breq  ac_switch_pwm_mode            ; [1][2] -> NOT NEED UPDATE
    
    cbr   STATUS_REG, _BV(TARGET_BIT)   ; [1]

    tst   INCB_REG                      ; [1]
    breq  ac_update_a                   ; [1][2] -> UPDATE A
    tst   INCA_REG                      ; [1]
    breq  ac_update_b                   ; [1][2] -> UPDATE B

    ; COMPARE DELTA OF CHANNELS
    cp    TMPA_REG, TMPB_REG            ; [1]
    breq  ac_update_ab                  ; [1][2] -> dA == dB [UPDATE A OR B]
    brlo  PC+5                          ; [1][2] -> dA < dB
    
    ; dA > dB
    lsr   TMPA_REG                      ; [1]
    cp    TMPA_REG, TMPB_REG            ; [1]
    brsh  ac_update_a                   ; [1][2] -> dA >= 2*dB [UPDATE A]
    rjmp  ac_update_ab                  ; [2]    -> dA < 2*dB  [UPDATE A OR B]

    ; dB > dA
    lsr   TMPB_REG                      ; [1] <- 
    cp    TMPB_REG, TMPA_REG            ; [1]
    brsh  ac_update_b                   ; [1][2] -> dB >= 2*dA [UPDATE B]
    ; dB < 2*dA [UPDATE A OR B]

ac_update_ab:
    sbrc  STATUS_REG, CH1_BIT           ; [2][1]
    rjmp  ac_update_b                   ; [0][2] -> LAST TIME [A] HAS BEEN UPDATED, NOW IT'S TIME TO UPDATE [B]

    ; UPDATE CHANNEL A
ac_update_a:
    in    TMPA_REG, OCR1_REG            ; [1]
    add   TMPA_REG, INCA_REG            ; [1]
    out   OCR1_REG, TMPA_REG            ; [1]
    sbr   STATUS_REG, _BV(CH1_BIT)      ; [1]
    rjmp  ac_switch_pwm_mode            ; [2]

    ; UPDATE CHANNEL B
ac_update_b:
    in    TMPB_REG, OCR2_REG            ; [1] <- UPDATE OCR2_REG
    add   TMPB_REG, INCB_REG            ; [1]
    out   OCR2_REG, TMPB_REG            ; [1]
    cbr   STATUS_REG, _BV(CH1_BIT)      ; [1]

ac_switch_pwm_mode:
    in    TMP_REG, OCR1_REG             ; [1]
    cpi   TMP_REG, PWM_OFF              ; [1]
    in    TMPLO_REG, OCR2_REG           ; [1]
    cpc   TMP_REG, TMPLO_REG            ; [1]
    ldi   TMP_REG, PWM_ON_CTRLA         ; [1] set TIM0 to Fast PWM mode. Clear OC0x pins on Compare Match, set at 255.
    brne  PC+3                          ; [1][2] -> Fast PWM mode needed.

ac_force_off:
    cbr   STATUS_REG, _BV(FRC_BIT)      ; [2][1]
    ldi   TMP_REG, PWM_OFF_CTRLA        ; [1][0] set TIM0 to Normal mode. Clear OC0x pins on Compare Match.
    out   TCCR0A, TMP_REG               ; [1][1] <-

    ; **** END OF DIMMING CONTROL BLOCK ****


;
; File   : nec.inc (part of "Feron Light Advanced Control" project)
;
; Author : Sergey V. DUDANOV
; E-Mail : sergey.dudanov@gmail.com
; Git    : https://github.com/dudanov/feron-light-advanced-control
; License: GPL v3 (https://www.gnu.org/licenses/gpl.txt)

; CUSTOM ADDRESS FOR SET HB-VALUE (AS COMMAND) WITH DIMMING
.equ NEC_HBDIM_ADDRESS  = 0xB721

; CUSTOM ADDRESS FOR SET HB-VALUE (AS COMMAND) WITHOUT DIMMING
.equ NEC_HBSET_ADDRESS  = 0xB781

; NATIVE ADDRESS [ USED HB: A, B, D, E, F ]
.equ NEC_FERON_ADDRESS  = 0xB7A0

; NATIVE COMMANDS
.equ NEC_FERON_TIM1H    = 0xA8
.equ NEC_FERON_BRUP     = 0xAB
.equ NEC_FERON_MODE     = 0xAC
.equ NEC_FERON_BRDN     = 0xBC
.equ NEC_FERON_SELECT   = 0xDB
.equ NEC_FERON_POWER    = 0xE9
.equ NEC_FERON_HUEDN    = 0xEB
.equ NEC_FERON_HUEUP    = 0xEF
.equ NEC_FERON_NIGHT    = 0xF5
.equ NEC_FERON_TIM30M   = 0xFA

; ### CUSTOM COMMANDS ###
.equ NEC_PWRON          = 0xEA  ; power ON
.equ NEC_PWROFF         = 0xEC  ; power OFF
.equ NEC_ROTATE_HUE     = 0xF0  ; rotate HUE
.equ NEC_ROTATE_BR      = 0xA2  ; rotate BRIGHTNESS
; 0x0X: HUE  = X
; 0x1X: BR   = X
; 0x2X: HUE += X
; 0x3X: HUE -= X
; 0x4X: BR  += X
; 0x5X: BR  -= X

.equ NEC_RX_NONE        = 0
.equ NEC_RX_MAIN        = 1
.equ NEC_RX_REPEAT      = 2

    ; **** START OF IR EVENT BLOCK ****

    sbrs  PIN_REG, TSSOP_PIN                        ; [1]
    rjmp  _space                                    ; [1][2] -> space occured

    ; [MARK] ANALYSE

    cpi   NECCNT_REG, NEC_T(21)                     ; [1]
    brsh  _abort                                    ; [1][2] -> PROTOCOL ERROR: TOO BIG MARK

    sbrc  STATUS_REG, NEC_LEADER_BIT                ; [2][1]
    rjmp  _data_mark                                ; [0][2] -> DATA MARK 1T CHECKING

    ; LEADER [MARK] DETECTION

    cpi   NECCNT_REG, NEC_T(12)                     ; [1]
    brlo  _abort                                    ; [1][2] -> PROTOCOL ERROR: TOO LOW LEADER MARK
    sbr   STATUS_REG, _BV(NEC_LEADER_BIT)           ; [1]
    rjmp  _nec_tim_reset                            ; [2] ----> GOOD: LEADER MARK DETECTED
    
    ; [SPACE] ANALYSE
_space:
    cpi   NECCNT_REG, 255                           ; [1]
    breq  _abort                                    ; [1][2] -> SPACE TOO MUCH. NEW COMMAND SESSION DETECTED.
    cpi   NECCNT_REG, NEC_T(10)                     ; [1]
    brsh  _frame_end                                ; [1][2] -> FRAME END: SPACE BEETWEEN FRAMES DETECTED
    tst   BITCNT_REG                                ; [1]
    brne  PC+11                                     ; [1][2] -> NEED READ DATA BIT
    cpi   NECCNT_REG, NEC_T(6)                      ; [1]
    brsh  PC+5                                      ; [1][2] -> MAIN FRAME DETECTED
    tst   NEC_FRAME_REG                             ; [1]
    breq  _frame_end                                ; [1][2] -> PROTOCOL ERROR: NO MAIN FRAME IN BUFFER
    ldi   NEC_FRAME_REG, NEC_RX_REPEAT              ; [1]
    rjmp  _set_fin                                  ; [2]
    ; prepare for main frame reading
    ldi   NEC_FRAME_REG, NEC_RX_MAIN                ; [1]
    ldi   BITCNT_REG, 32                            ; [1]
    ldi   XL, sram_nec_data                         ; [1]
    rjmp  _nec_tim_reset                            ; [2] ----> GOOD: MAIN FRAME INIT
    
    ; [SPACE] READING DATA

    cpi   NECCNT_REG, NEC_T(2)                      ; [1]
    ror   BYTE_REG                                  ; [1]
    dec   BITCNT_REG                                ; [1]
    breq  PC+7                                      ; [1][2] -> OK: READING COMPLETE. CHECK INVERSION OF LAST DATA BYTE
    mov   TMP_REG, BITCNT_REG                       ; [1]
    andi  TMP_REG, 0b111                            ; [1]
    brne  PC+3                                      ; [1][2] -> OK: CONTINUE READ
    com   BYTE_REG                                  ; [1]
    st    X+, BYTE_REG                              ; [2]
    rjmp  _nec_tim_reset                            ; [2] ----> OK: ONE MORE BYTE READED
    ld    TMP_REG, -X                               ; [2]
    cpse  TMP_REG, BYTE_REG                         ; [2][1]
_abort:
    rjmp  _nec_abort                                ; [0][2] -> PROTOCOL ERROR: LAST DATA BYTE NOT INVERTED

_set_fin:
    sbr   STATUS_REG, _BV(NEC_FIN_BIT)              ; [1]
    rjmp  _nec_tim_reset                            ; [2]

_frame_end:
    rjmp  _nec_frame_end                            ; [2]

    ; 1T DATA [MARKS] CHECKING
_data_mark:
    cpi   NECCNT_REG, NEC_T(2)                      ; [1]
    brsh  _abort                                    ; [1][2] -> PROTOCOL ERROR: TOO LONG DATA MARK
    sbrs  STATUS_REG, NEC_FIN_BIT                   ; [2][1]
    rjmp  _nec_tim_reset                            ; [2] ----> GOOD: DATA MARK TIMING IS OK

    ; #### START OF PACKET PROCESSING ####

    ldd   TMP_REG, Y+1                              ; [2]
    cpi   TMP_REG, high(NEC_FERON_ADDRESS)          ; [1]
    brne  PC+5                                      ; [1][2] -> ERROR: WRONG HIGH BYTE OF ADDRESS

    ldd   TMP_REG, Y+0                              ; [2]
    cpi   TMP_REG, low(NEC_FERON_ADDRESS)           ; [1]
    breq  _native_address                           ; [1][2] -> GOOD: FERON NATIVE ADDRESS RECEIVED

    sbrc  NEC_FRAME_REG, 1                          ; [2][1]
    rjmp  _nec_abort                                ; [0][2] -> IGNORE REPEAT PACKETS

    cpi   TMP_REG, low(NEC_HBDIM_ADDRESS)           ; [1]
    brne  PC+4                                      ; [1][2] -> ERROR: WRONG LOW BYTE OF ADDRESS
    
    ; CUSTOM "HB-DIM" COMMAND
    ldd   HB_VALUE_REG, Y+2                         ; [2]
    sbr   STATUS_REG, _BV(PWR_BIT)                  ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL

    cpi   TMP_REG, low(NEC_HBSET_ADDRESS)           ; [1]
    brne  PC+6                                      ; [1][2] -> ERROR: WRONG LOW BYTE OF ADDRESS
    
    ; CUSTOM "HB-SET" COMMAND    
    ldd   HB_VALUE_REG, Y+2                         ; [2]
    sbr   STATUS_REG, _BV(PWR_BIT)                  ; [1]
    rcall update_from_table                         ; [16]
    out   OCR1_REG, CH1_TARGET_REG                  ; [1]
    out   OCR2_REG, CH2_TARGET_REG                  ; [1]
    rjmp  _nec_abort                                ; [0][2] ->

_native_address:
    
    ; FERON NATIVE ADDRESS COMMANDS

    ldd   TMP_REG, Y+2                              ; [2]

    sbrc  NEC_FRAME_REG, 1                          ; [2][1]
    rjmp  _accept_repeat_frames                     ; [0][2] -> IGNORE REPEAT PACKETS

    ; #### ONLY MAIN PACKETS ####

    cpi   TMP_REG, NEC_FERON_POWER                  ; [1]
    brne  PC+6                                      ; [1][2]
    ; [0xE9]: NATIVE "POWER" COMMAND
    ldi   TMP_REG, _BV(PWR_BIT)                     ; [1]
    eor   STATUS_REG, TMP_REG                       ; [1]
    andi  STATUS_REG, STATUS_EEPROM_MASK            ; [1]
    rcall update_target_registers                   ; [19|14]
    rjmp  _nec_abort                                ; [2] ->

    cpi   TMP_REG, NEC_PWRON                        ; [1]
    brne  PC+4                                      ; [1][2]
    ; [0xEA]: CUSTOM "POWER ON" COMMAND. MAY BE USED TO TURN OFF TIMER.
    sbr   STATUS_REG, _BV(PWR_BIT)                  ; [1]
    andi  STATUS_REG, STATUS_EEPROM_MASK            ; [1]
    rjmp  _cmd_final                                ; [2]
    
    sbrs  STATUS_REG, PWR_BIT                       ; [2][1]
    rjmp  _nec_abort                                ; [0][2] -> IGNORE OTHER COMMANDS IN "POWER OFF" STATE

    ; #### ONLY MAIN PACKETS IN "POWER ON" STATE ####

    cpi   TMP_REG, 0x60                             ; [1]
    brsh  _above_5f                                 ; [1][2] -> NOT DIRECT COMMANDS
    
    ; ### CUSTOM GROUP MODIFY COMMANDS [0x00-0x5F] ###
    bst   TMP_REG, 4                                ; [1]
    cpi   TMP_REG, 0x20                             ; [1]
    brcc  PC+9                                      ; [1][2] -> SOME OF CUSTOM MODIFY COMMANDS
    andi  TMP_REG, 0x0F                             ; [1]
    brts  PC+3                                      ; [1][2] -> DIRECT BRIGHTNESS COMMANDS
    ; [0x0X]: CUSTOM "HUE = X" COMMAND
    andi  HB_VALUE_REG, 0xF0                        ; [1]
    rjmp  PC+3                                      ; [2] ->
    ; [0x1X]: CUSTOM "BR = X" COMMAND
    swap  TMP_REG                                   ; [1]
    andi  HB_VALUE_REG, 0x0F                        ; [1]
    or    HB_VALUE_REG, TMP_REG                     ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL
    cpi   TMP_REG, 0x40                             ; [1]
    andi  TMP_REG, 0x0F                             ; [1] CF not affected
    brcc  PC+3                                      ; [1][2] -> NOT INCREMENT COMMANDS
    ; [0x2X]: CUSTOM "HUE += X" COMMAND
    brtc  _hue_add_command                          ; [1][2] -> CUSTOM "HUE+X" COMMAND [0x2X]
    ; [0x3X]: CUSTOM "HUE -= X" COMMAND
    rjmp  _hue_sub_command                          ; [2]    -> CUSTOM "HUE-X" COMMAND [0x3X]
    swap  TMP_REG                                   ; [1]
    ; [0x4X]: CUSTOM "BR += X" COMMAND
    brtc  _br_add_command                           ; [2] ->
    ; [0x5X]: CUSTOM "BR -= X" COMMAND
    rjmp  _br_sub_command                           ; [2] ->

_above_5f:
    
    cpi   TMP_REG, NEC_PWROFF                       ; [1]
    brne  PC+4                                      ; [1][2]
    ; [0xEC]: CUSTOM "POWER OFF" COMMAND
    andi  STATUS_REG, _BV(INV_BIT)                  ; [1]
    rcall reset_target_registers                    ; [10]
    rjmp  _nec_abort                                ; [2] ---->
    
    cpi   TMP_REG, NEC_FERON_TIM30M                 ; [1]
    brne  PC+4                                      ; [1][2]
    ; [0xFA]: NATIVE "30MIN TIMER" COMMAND
    ldi   TIMLO_REG, low(1800)                      ; [1]
    ldi   TIMHI_REG, high(1800)                     ; [1]
    rjmp  PC+5                                      ; [2] ---->
    
    cpi   TMP_REG, NEC_FERON_TIM1H                  ; [1]
    brne  PC+5                                      ; [1][2]
    ; [0xA8]: NATIVE "1H TIMER" COMMAND
    ldi   TIMLO_REG, low(3600)                      ; [1]
    ldi   TIMHI_REG, high(3600)                     ; [1]
    sbr   STATUS_REG, _BV(TMR_BIT) | _BV(FRC_BIT)   ; [1]
    rjmp  _nec_abort                                ; [2] ---->

_accept_repeat_frames:

    ; #### MAIN AND REPEAT PACKETS ####
    
    sbrc  STATUS_REG, PWR_BIT                       ; [2][1]
    sbrs  STATUS_REG, TARGET_BIT                    ; [2][1]
    rjmp  _nec_frame_end                            ; [0][2] -> IGNORE OTHER COMMANDS IN POWEROFF OR DIMMING STATE

    ; ### MAIN AND REPEAT PACKETS IN "POWER ON" AND "NOT DIMMING" STATE ###

    cpi   TMP_REG, NEC_FERON_HUEUP                  ; [1]
    brne  PC+7                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xEF]: NATIVE "HUE++" COMMAND
    ldi   TMP_REG, 0x01                             ; [1]
_hue_add_command:
    add   HB_VALUE_REG, TMP_REG                     ; [1]
    brhc  _cmd_final                                ; [1][2] -> HUE is not overflowed. Final command.
    ; HUE is overflowed. Limit value to 15 and fix corrupted BR value.
    sbr   HB_VALUE_REG, 0x0F                        ; [1]
    subi  HB_VALUE_REG, 0x10                        ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL

    cpi   TMP_REG, NEC_FERON_BRUP                   ; [1]
    brne  PC+6                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xAB]: NATIVE "BR++" COMMAND
    ldi   TMP_REG, 0x10                             ; [1]
_br_add_command:
    add   HB_VALUE_REG, TMP_REG                     ; [1]
    brcc  _cmd_final                                ; [1][2] -> BR is not overflowed
    ; BR is overflowed. Limit value to 15.
    sbr   HB_VALUE_REG, 0xF0                        ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL

    cpi   TMP_REG, NEC_FERON_HUEDN                  ; [1]
    brne  PC+7                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xEB]: NATIVE "HUE--" COMMAND
    ldi   TMP_REG, 0x01                             ; [1]
_hue_sub_command:
    sub   HB_VALUE_REG, TMP_REG                     ; [1]
    brhc  _cmd_final                                ; [1][2] -> HUE is not overflowed.
    ; HUE is overflowed. Limit value to 0 and fix corrupted BR value.
    cbr   HB_VALUE_REG, 0x0F                        ; [1]
    subi  HB_VALUE_REG, 0xF0                        ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL

    cpi   TMP_REG, NEC_FERON_BRDN                   ; [1]
    brne  PC+6                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xBC]: NATIVE "BR--" COMMAND
    ldi   TMP_REG, 0x10                             ; [1]
_br_sub_command:
    sub   HB_VALUE_REG, TMP_REG                     ; [1]
    brcc  _cmd_final                                ; [1][2] -> BR is not overflowed.
    ; BR is overflowed. Limit value to 0.
    cbr   HB_VALUE_REG, 0xF0                        ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL

    ; ROTATE COMMANDS BLOCK

    cpi   TMP_REG, NEC_ROTATE_BR                    ; [1]
    brne  PC+5                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xA2]: CUSTOM "ROTATE BRIGHTNESS" COMMAND
    swap  HB_VALUE_REG                              ; [1]
    rcall rotate_hue                                ; [18]
    swap  HB_VALUE_REG                              ; [1]
    rjmp  _cmd_final                                ; [2] -> COMMAND FINAL

    cpi   TMP_REG, NEC_ROTATE_HUE                   ; [1]
    ; [0xF0]: CUSTOM "ROTATE HUE" COMMAND
    breq  PC+12                                     ; [1][2] -> ROTATE HUE COMMAND

    cpi   TMP_REG, NEC_FERON_NIGHT                  ; [1]
    brne  PC+5                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xF5]: NATIVE "NIGHT" COMMAND
    cpi   HB_VALUE_REG, 0x10                        ; [1]
    cbr   HB_VALUE_REG, 0xF0                        ; [1] SET BRIGHTNESS TO MIN
    brlo  PC+7                                      ; [1][2] -> BRIGHTNESS ALREADY ZERO. ROTATE HUE
    rjmp  _cmd_final                                ; [2]    -> COMMAND FINAL

    cpi   TMP_REG, NEC_FERON_MODE                   ; [1]
    brne  PC+6                                      ; [1][2] -> IS NEXT COMMAND?
    ; [0xAC]: NATIVE "MAX" COMMAND
    cpi   HB_VALUE_REG, 0xF0                        ; [1]
    sbr   HB_VALUE_REG, 0xF0                        ; [1] SET BRIGHTNESS TO MAX
    brlo  _cmd_final                                ; [1][2] -> COMMAND FINAL
    rcall rotate_hue                                ; [18]
    rjmp  _cmd_final                                ; [2]    -> COMMAND FINAL

    ; END OF ROTATE COMMANDS BLOCK

    cpi   TMP_REG, NEC_FERON_SELECT                 ; [1]
    brne  _nec_abort                                ; [1][2] -> LAST COMMAND
    ; [0xDB]: NATIVE "SELECT" COMMAND
    mov   HB_VALUE_REG, RAND_REG                    ; [1] <-

_cmd_final:
    rcall update_from_table                         ; [16]
    rjmp  _nec_frame_end                            ; [2] ---->

    ; END OF MESSAGE PROCESSING

_nec_abort:
    ldi   NEC_FRAME_REG, NEC_RX_NONE                ; [1]
_nec_frame_end:
    cbr   STATUS_REG, NEC_STATUS_MASK               ; [1]
    clr   BITCNT_REG                                ; [1]
_nec_tim_reset:
    clr   NECCNT_REG                                ; [1]

    ; **** END OF IR EVENT BLOCK ****

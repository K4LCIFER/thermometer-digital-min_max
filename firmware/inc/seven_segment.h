////////////////////////////////////////////////////////////////////////////////
// To use this header file, make sure that you define the name/series of the 
// 7 segment display that you will be using in your program code where this 
// header file is included.
////////////////////////////////////////////////////////////////////////////////

#ifndef _SEVEN_SEGMENT_H_
#define _SEVEN_SEGMENT_H_

#ifdef INND_TS56
//      SEVEN_SEGMENT_<X>   0b<DP><G><F><E><D><C><B><A>
#define _SS_A   0b00000001
#define _SS_B   0b00000010
#define _SS_C   0b00000100
#define _SS_D   0b00001000
#define _SS_E   0b00010000
#define _SS_F   0b00100000
#define _SS_G   0b01000000
#define _SS_DP  0b10000000

#define SS_0    _SS_A | _SS_B | _SS_C | _SS_D | _SS_E | _SS_F
#define SS_1    _SS_B | _SS_C
#define SS_2    _SS_A | _SS_B | _SS_G | _SS_E | _SS_D
#define SS_3    _SS_A | _SS_B | _SS_G | _SS_C | _SS_D
#define SS_4    _SS_F | _SS_G | _SS_B | _SS_C
#define SS_5    _SS_A | _SS_F | _SS_G | _SS_C | _SS_D
#define SS_6    _SS_A | _SS_F | _SS_G | _SS_C | _SS_D | _SS_E
#define SS_7    _SS_A | _SS_B | _SS_C
#define SS_8    _SS_A | _SS_B | _SS_C | _SS_D | _SS_E | _SS_F \
    | _SS_G
#define SS_9    _SS_A | _SS_F | _SS_B | _SS_G | _SS_C
#define SS_DEC  _SS_DP
#define SS_NEG  _SS_G
#define SS_CLEAR    0
#endif

#endif

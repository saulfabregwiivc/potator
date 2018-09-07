/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                          Tables.h                       **/
/**                                                         **/
/** This file contains tables of used by 6502 emulation to  **/
/** compute NEGATIVE and ZERO flags. There are also timing  **/
/** tables for 6502 opcodes. This file is included from     **/
/** 6502.c.                                                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

static byte Cycles[256] =
{
    //7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
    //2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
    //6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
    //2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
    //6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
    //2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
    //6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
    //2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
    //2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    //2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
    //2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    //2,5,2,5,4,4,4,4,2,4,2,5,4,4,4,4,
    //2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    //2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
    //2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    //2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7

 //https://github.com/mamedev/historic-mess/blob/master/src/emu/cpu/m6502/t6502.c
 //https://github.com/mamedev/historic-mess/blob/master/src/emu/cpu/m6502/t65c02.c
  //0 1 2 3 4 5 6 7 8 9 a b c d e f
    7,6,2,8,3,3,5,5,3,2,2,2,2,4,6,5, // 0
    2,5,3,8,3,4,6,5,2,4,2,7,4,4,7,5, // 1
    6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,5, // 2
    2,5,3,8,4,4,6,5,2,4,2,7,4,4,7,5, // 3
    6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,5, // 4
    2,5,3,8,4,4,6,5,2,4,3,7,5,4,7,5, // 5
    6,6,2,8,2,3,5,5,4,2,2,2,5,4,6,5, // 6
    2,5,3,8,4,4,6,5,2,4,4,7,2,4,7,5, // 7
    2,6,2,6,3,3,3,5,2,2,2,2,4,4,4,5, // 8
    2,6,4,6,4,4,4,5,2,5,2,5,4,5,5,5, // 9
    2,6,2,6,3,3,3,5,2,2,2,2,4,4,4,5, // a
    2,5,3,5,4,4,4,5,2,4,2,5,4,4,4,5, // b
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,5, // c
    2,5,3,8,4,4,6,5,2,4,3,7,5,4,7,5, // d
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,5, // e
    2,5,3,8,4,4,6,5,2,4,4,7,5,4,7,5  // f
  //0 1 2 3 4 5 6 7 8 9 a b c d e f
};

byte ZNTable[256] =
{
    Z_FLAG,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
    N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
};

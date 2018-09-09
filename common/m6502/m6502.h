/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                         M6502.h                         **/
/**                                                         **/
/** This file contains declarations relevant to emulation   **/
/** of 6502 CPU.                                            **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**               Alex Krasivsky  1996                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef M6502_H
#define M6502_H

#ifdef __cplusplus
extern "C" {
#endif



/** INLINE ***************************************************/
/** Different compilers inline C functions differently.     **/
/*************************************************************/
#define INLINE inline

/** System-Dependent Stuff ***********************************/
/** This is system-dependent code put here to speed things  **/
/** up. It has to stay inlined to be fast.                  **/
/*************************************************************/
#ifdef INES
#define FAST_RDOP
    extern byte *Page[];
    INLINE byte Op6502(register word A) { return(Page[A >> 13][A & 0x1FFF]); }
#endif

/** FAST_RDOP ************************************************/
/** With this #define not present, Rd6502() should perform  **/
/** the functions of Rd6502().                              **/
/*************************************************************/
#ifndef FAST_RDOP
#define Op6502(A) Rd6502(A)
#endif

/* "Izp" added by uso. */

/** Addressing Methods ***************************************/
/** These macros calculate and return effective addresses.  **/
/*************************************************************/
/* These are old versions
#define MC_Ab(Rg)       M_LDWORD(Rg)
#define MC_Zp(Rg)       Rg.B.l=Op6502(R->PC.W++);Rg.B.h=0
#define MC_Zx(Rg)       Rg.B.l=Op6502(R->PC.W++)+R->X;Rg.B.h=0
#define MC_Zy(Rg)       Rg.B.l=Op6502(R->PC.W++)+R->Y;Rg.B.h=0
#define MC_Ax(Rg)       M_LDWORD(Rg);Rg.W+=R->X
the current code is */
#define MC_Ab(Rg)       M_LDWORD(Rg)
#define MC_Zp(Rg)       Rg.W=Op6502(R->PC.W++)
#define MC_Zx(Rg)       Rg.W=(byte)(Op6502(R->PC.W++)+R->X)
#define MC_Zy(Rg)       Rg.W=(byte)(Op6502(R->PC.W++)+R->Y)
#define MC_Ax(Rg)       M_LDWORD(Rg);Rg.W+=R->X

#define MC_Ay(Rg)       M_LDWORD(Rg);Rg.W+=R->Y
/*#define MC_Ix(Rg)       K.B.l=Op6502(R->PC.W++)+R->X;K.B.h=0; \
                        Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W)
#define MC_Iy(Rg)       K.B.l=Op6502(R->PC.W++);K.B.h=0; \
                        Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W); \
                        Rg.W+=R->Y
#define MC_Izp(Rg)      K.B.l=Op6502(R->PC.W++);K.B.h=0; \
                        Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W);*/
#define MC_Ix(Rg)       K.W=(byte)(Op6502(R->PC.W++)+R->X); \
			Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W)
#define MC_Iy(Rg)       K.W=Op6502(R->PC.W++); \
			Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W); \
			Rg.W+=R->Y
#define MC_Izp(Rg)      K.W=Op6502(R->PC.W++); \
			Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W);

/** Reading From Memory **************************************/
/** These macros calculate address and read from it.        **/
/*************************************************************/
#define MR_Ab(Rg)       MC_Ab(J);Rg=Rd6502(J.W)
#define MR_Im(Rg)       Rg=Op6502(R->PC.W++)
#define MR_Zp(Rg)       MC_Zp(J);Rg=Rd6502(J.W)
#define MR_Zx(Rg)       MC_Zx(J);Rg=Rd6502(J.W)
#define MR_Zy(Rg)       MC_Zy(J);Rg=Rd6502(J.W)
#define MR_Ax(Rg)       MC_Ax(J);Rg=Rd6502(J.W)
#define MR_Ay(Rg)       MC_Ay(J);Rg=Rd6502(J.W)
#define MR_Ix(Rg)       MC_Ix(J);Rg=Rd6502(J.W)
#define MR_Iy(Rg)       MC_Iy(J);Rg=Rd6502(J.W)
#define MR_Izp(Rg)      MC_Izp(J);Rg=Rd6502(J.W)

/** Writing To Memory ****************************************/
/** These macros calculate address and write to it.         **/
/*************************************************************/
#define MW_Ab(Rg)       MC_Ab(J);Wr6502(J.W,Rg)
#define MW_Zp(Rg)       MC_Zp(J);Wr6502(J.W,Rg)
#define MW_Zx(Rg)       MC_Zx(J);Wr6502(J.W,Rg)
#define MW_Zy(Rg)       MC_Zy(J);Wr6502(J.W,Rg)
#define MW_Ax(Rg)       MC_Ax(J);Wr6502(J.W,Rg)
#define MW_Ay(Rg)       MC_Ay(J);Wr6502(J.W,Rg)
#define MW_Ix(Rg)       MC_Ix(J);Wr6502(J.W,Rg)
#define MW_Iy(Rg)       MC_Iy(J);Wr6502(J.W,Rg)
#define MW_Izp(Rg)      MC_Izp(J);Wr6502(J.W,Rg)

/** Modifying Memory *****************************************/
/** These macros calculate address and modify it.           **/
/*************************************************************/
#define MM_Ab(Cmd)      MC_Ab(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Zp(Cmd)      MC_Zp(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Zx(Cmd)      MC_Zx(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Ax(Cmd)      MC_Ax(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)

/** Other Macros *********************************************/
/** Calculating flags, stack, jumps, arithmetics, etc.      **/
/*************************************************************/
#define M_FL(Rg)        R->P=(R->P&~(Z_FLAG|N_FLAG))|ZNTable[Rg]
#define M_LDWORD(Rg)    Rg.B.l=Op6502(R->PC.W++);Rg.B.h=Op6502(R->PC.W++)

#define M_PUSH(Rg)      Wr6502(0x0100|R->S,Rg);R->S--
#define M_POP(Rg)       R->S++;Rg=Op6502(0x0100|R->S)
#define M_JR            R->PC.W+=(offset)Op6502(R->PC.W)+1;R->ICount--

/* Added by uso, fixed by h.p. */
#define M_TSB(Data) R->P = (R->P & ~Z_FLAG) | ((Data & R->A) == 0 ? Z_FLAG : 0);        \
                    Data |=  R->A;
#define M_TRB(Data) R->P = (R->P & ~Z_FLAG) | ((Data & R->A) == 0 ? Z_FLAG : 0);        \
                    Data &= ~R->A;

//#define NO_DECIMAL
#ifdef NO_DECIMAL

#define M_ADC(Rg) \
  K.W=R->A+Rg+(R->P&C_FLAG); \
  R->P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
  R->P|=(~(R->A^Rg)&(R->A^K.B.l)&0x80? V_FLAG:0)| \
        (K.B.h? C_FLAG:0)|ZNTable[K.B.l]; \
  R->A=K.B.l

/* Warning! C_FLAG is inverted before SBC and after it */
#define M_SBC(Rg) \
  K.W=R->A-Rg-(~R->P&C_FLAG); \
  R->P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
  R->P|=((R->A^Rg)&(R->A^K.B.l)&0x80? V_FLAG:0)| \
        (K.B.h? 0:C_FLAG)|ZNTable[K.B.l]; \
  R->A=K.B.l

#else /* NO_DECIMAL */

#define M_ADC(Rg) \
  if(R->P&D_FLAG) \
  { \
    K.B.l=(R->A&0x0F)+(Rg&0x0F)+(R->P&C_FLAG); \
    if(K.B.l>9) K.B.l+=6; \
    K.B.h=(R->A>>4)+(Rg>>4)+(K.B.l>15? 1:0); \
    R->A=(K.B.l&0x0F)|(K.B.h<<4); \
    R->P=(R->P&~C_FLAG)|(K.B.h>15? C_FLAG:0); \
  } \
  else \
  { \
    K.W=R->A+Rg+(R->P&C_FLAG); \
    R->P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
    R->P|=(~(R->A^Rg)&(R->A^K.B.l)&0x80? V_FLAG:0)| \
          (K.B.h? C_FLAG:0)|ZNTable[K.B.l]; \
    R->A=K.B.l; \
  }

/* Warning! C_FLAG is inverted before SBC and after it */
#define M_SBC(Rg) \
  if(R->P&D_FLAG) \
  { \
    K.B.l=(R->A&0x0F)-(Rg&0x0F)-(~R->P&C_FLAG); \
    if(K.B.l&0x10) K.B.l-=6; \
    K.B.h=(R->A>>4)-(Rg>>4)-((K.B.l&0x10)>>4); \
    if(K.B.h&0x10) K.B.h-=6; \
    R->A=(K.B.l&0x0F)|(K.B.h<<4); \
    R->P=(R->P&~C_FLAG)|(K.B.h>15? 0:C_FLAG); \
  } \
  else \
  { \
    K.W=R->A-Rg-(~R->P&C_FLAG); \
    R->P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
    R->P|=((R->A^Rg)&(R->A^K.B.l)&0x80? V_FLAG:0)| \
          (K.B.h? 0:C_FLAG)|ZNTable[K.B.l]; \
    R->A=K.B.l; \
  }

#endif /* NO_DECIMAL */


#define M_CMP(Rg1,Rg2) \
  K.W=Rg1-Rg2; \
  R->P&=~(N_FLAG|Z_FLAG|C_FLAG); \
  R->P|=ZNTable[K.B.l]|(K.B.h? 0:C_FLAG)
#define M_BIT(Rg) \
  R->P&=~(N_FLAG|V_FLAG|Z_FLAG); \
  R->P|=(Rg&(N_FLAG|V_FLAG))|(Rg&R->A? 0:Z_FLAG)

#define M_AND(Rg)       R->A&=Rg;M_FL(R->A)
#define M_ORA(Rg)       R->A|=Rg;M_FL(R->A)
#define M_EOR(Rg)       R->A^=Rg;M_FL(R->A)
#define M_INC(Rg)       Rg++;M_FL(Rg)
#define M_DEC(Rg)       Rg--;M_FL(Rg)

#define M_ASL(Rg)       R->P&=~C_FLAG;R->P|=Rg>>7;Rg<<=1;M_FL(Rg)
#define M_LSR(Rg)       R->P&=~C_FLAG;R->P|=Rg&C_FLAG;Rg>>=1;M_FL(Rg)
#define M_ROL(Rg)       K.B.l=(Rg<<1)|(R->P&C_FLAG); \
                        R->P&=~C_FLAG;R->P|=Rg>>7;Rg=K.B.l; \
                        M_FL(Rg)
#define M_ROR(Rg)       K.B.l=(Rg>>1)|(R->P<<7); \
                        R->P&=~C_FLAG;R->P|=Rg&C_FLAG;Rg=K.B.l; \
                        M_FL(Rg)


/* Compilation options:       */
//#define FAST_RDOP	           /* Separate Op6502()/Rd6502() */
/* #define DEBUG */            /* Compile debugging version  */
#define LSB_FIRST              /* Compile for low-endian CPU */

                               /* Loop6502() returns:        */
#define INT_NONE  0            /* No interrupt required      */
#define INT_IRQ	  1            /* Standard IRQ interrupt     */
#define INT_NMI	  2            /* Non-maskable interrupt     */
#define INT_QUIT  3            /* Exit the emulation         */

                               /* 6502 status flags:         */
#define	C_FLAG	  0x01         /* 1: Carry occured           */
#define	Z_FLAG	  0x02         /* 1: Result is zero          */
#define	I_FLAG	  0x04         /* 1: Interrupts disabled     */
#define	D_FLAG	  0x08         /* 1: Decimal mode            */
#define	B_FLAG	  0x10         /* Break [0 on stk after int] */
#define	R_FLAG	  0x20         /* Always 1                   */
#define	V_FLAG	  0x40         /* 1: Overflow occured        */
#define	N_FLAG	  0x80         /* 1: Result is negative      */

/** Simple Datatypes *****************************************/
/** NOTICE: sizeof(byte)=1 and sizeof(word)=2               **/
/*************************************************************/
typedef unsigned char byte;
typedef unsigned short word;
typedef signed char offset;

/** Structured Datatypes *************************************/
/** NOTICE: #define LSB_FIRST for machines where least      **/
/**         signifcant byte goes first.                     **/
/*************************************************************/
typedef union
{
#ifdef LSB_FIRST
    struct { byte l, h; } B;
#else
    struct { byte h, l; } B;
#endif
    word W;
} pair;

typedef struct
{
    byte A, P, X, Y, S;  /* CPU registers and program counter   */
    pair PC;

    int IPeriod, ICount; /* Set IPeriod to number of CPU cycles */
                         /* between calls to Loop6502()         */
    byte IRequest;       /* Set to the INT_IRQ when pending IRQ */
    byte AfterCLI;       /* Private, don't touch                */
    int IBackup;         /* Private, don't touch                */
    void *User;          /* Arbitrary user data (ID,RAM*,etc.)  */
    byte TrapBadOps;     /* Set to 1 to warn of illegal opcodes */
    word Trap;           /* Set Trap to address to trace from   */
    byte Trace;          /* Set Trace=1 to start tracing        */
} M6502;

/** Reset6502() **********************************************/
/** This function can be used to reset the registers before **/
/** starting execution with Run6502(). It sets registers to **/
/** their initial values.                                   **/
/*************************************************************/
void Reset6502(register M6502 *R);

/** Int6502() ************************************************/
/** This function will generate interrupt of a given type.  **/
/** INT_NMI will cause a non-maskable interrupt. INT_IRQ    **/
/** will cause a normal interrupt, unless I_FLAG set in R.  **/
/*************************************************************/
void Int6502(register M6502 *R, register byte Type);

/** Run6502() ************************************************/
/** This function will run 6502 code until Loop6502() call  **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word Run6502(register M6502 *R);

/** Rd6502()/Wr6502/Op6502() *********************************/
/** These functions are called when access to RAM occurs.   **/
/** They allow to control memory access. Op6502 is the same **/
/** as Rd6502, but used to read *opcodes* only, when many   **/
/** checks can be skipped to make it fast. It is only       **/
/** required if there is a #define FAST_RDOP.               **/
/************************************ TO BE WRITTEN BY USER **/
void Wr6502(register word Addr, register byte Value);
byte Rd6502(register word Addr);
byte Op6502(register word Addr);


/** Loop6502() ***********************************************/
/** 6502 emulation calls this function periodically to      **/
/** check if the system hardware requires any interrupts.   **/
/** This function must return one of following values:      **/
/** INT_NONE, INT_IRQ, INT_NMI, or INT_QUIT to exit the     **/
/** emulation loop.                                         **/
/************************************ TO BE WRITTEN BY USER **/
byte Loop6502(register M6502 *R);

#ifdef __cplusplus
}
#endif

#endif /* M6502_H */

/*** m6809: Portable 6809 emulator ******************************************/

#ifndef _M6809_H
#define _M6809_H

#include "memory.h"
#include "osd_cpu.h"

/* 6809 Registers */
typedef struct
{
	UINT16	pc; 	/* Program counter */
	UINT16	u, s;	/* Stack pointers */
	UINT16	x, y;	/* Index registers */
	UINT8	dp; 	/* Direct Page register */
	UINT8	a, b;	/* Accumulator */
	UINT8	cc;
	INT8	int_state;	/* SYNC and CWAI flags */
    INT8    nmi_state;
    INT8    irq_state[2];
	int (*irq_callback)(int irqline);
} m6809_Regs;

#ifndef INLINE
#define INLINE static inline
#endif

/* flag bits in the cc register */
#define CC_C	0x01		/* Carry */
#define CC_V	0x02		/* Overflow */
#define CC_Z	0x04		/* Zero */
#define CC_N	0x08		/* Negative */
#define CC_II	0x10		/* Inhibit IRQ */
#define CC_H	0x20		/* Half (auxiliary) carry */
#define CC_IF	0x40		/* Inhibit FIRQ */
#define CC_E	0x80		/* entire state pushed */


#define M6809_INT_NONE	0	/* No interrupt required */
#define M6809_INT_IRQ	1	/* Standard IRQ interrupt */
#define M6809_INT_FIRQ	2	/* Fast IRQ */
#define M6809_INT_NMI	4	/* NMI */	/* NS 970909 */

/* PUBLIC FUNCTIONS */
extern void m6809_SetRegs(m6809_Regs *Regs);
extern void m6809_GetRegs(m6809_Regs *Regs);
extern unsigned m6809_GetPC(void);
extern void m6809_reset(void);
extern int m6809_execute(int cycles);  /* NS 970908 */

#define M6809_IRQ_LINE	0	/* IRQ line number */
#define M6809_FIRQ_LINE 1   /* FIRQ line number */

extern void m6809_set_nmi_line(int state);
extern void m6809_set_irq_line(int irqline, int state);
extern void m6809_set_irq_callback(int (*callback)(int irqline));

/* PUBLIC GLOBALS */
extern int	m6809_ICount;


/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6809_RDMEM(A) ((unsigned)cpu_readmem16(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6809_WRMEM(A,V) (cpu_writemem16(A,V))

/****************************************************************************/
/* Z80_RDOP() is identical to Z80_RDMEM() except it is used for reading     */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6809_RDOP(A) ((unsigned)cpu_readop(A))

/****************************************************************************/
/* Z80_RDOP_ARG() is identical to Z80_RDOP() except it is used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6809_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6809_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6809_FAST_OP is set, opcodes are fetched     */
/* directly from the ROM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6809_Flags;
#define M6809_FAST_NONE	0x00	/* no memory optimizations */
#define M6809_FAST_S	0x02	/* stack */
#define M6809_FAST_U	0x04	/* user stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#endif /* _M6809_H */
/*
 * Copyright (C) 1997-2001, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* MMX routines for tinting XImages written by Willem Monsuwe <willem@stack.nl> */

/* Function calling conventions:
 *   shade_ximage_xx(void *data, int bpl, int w, int h, int rm, int gm, int bm);
 */

#define data	8(%ebp)
#define bpl	12(%ebp)
#define w	16(%ebp)
#define h	20(%ebp)
#define rm	24(%ebp)
#define gm	28(%ebp)
#define bm	32(%ebp)

#ifdef UNDERSCORE_SYMBOLS /* need this to link with msvc */
#define SHADE_XIMAGE_15 _shade_ximage_15_mmx
#define SHADE_XIMAGE_16 _shade_ximage_16_mmx
#define SHADE_XIMAGE_32 _shade_ximage_32_mmx
#define HAVE_MMX _have_mmx
#else
#define SHADE_XIMAGE_15 shade_ximage_15_mmx
#define SHADE_XIMAGE_16 shade_ximage_16_mmx
#define SHADE_XIMAGE_32 shade_ximage_32_mmx
#define HAVE_MMX have_mmx
#endif

.globl SHADE_XIMAGE_15
.globl SHADE_XIMAGE_16
.globl SHADE_XIMAGE_32
.globl HAVE_MMX

.bss
.text
.align 8

#define ENTER                   \
        pushl %ebp              ;\
        movl %esp, %ebp         ;\
        pushl %ebx              ;\
        pushl %ecx              ;\
        pushl %edx              ;\
        pushl %edi              ;\
        pushl %esi              ;\
        movl data, %esi         ;\
        movl w, %ebx            ;\
        movl h, %edx

#define LEAVE                   \
4:                              ;\
        emms                    ;\
        popl %esi               ;\
        popl %edi               ;\
        popl %edx               ;\
        popl %ecx               ;\
        popl %ebx               ;\
        movl %ebp, %esp         ;\
        popl %ebp               ;\
        ret


SHADE_XIMAGE_15:
        ENTER

        leal -6(%esi, %ebx, 2), %esi
        negl %ebx
        jz 5f

        /* Setup multipliers */
        movd rm, %mm5
        movd gm, %mm6
        movd bm, %mm7
        punpcklwd %mm5, %mm5    /* 00 00 00 00 rm rm rm rm */
        punpcklwd %mm6, %mm6    /* 00 00 00 00 gm gm gm gm */
        punpcklwd %mm7, %mm7    /* 00 00 00 00 bm bm bm bm */
        punpckldq %mm5, %mm5    /* rm rm rm rm rm rm rm rm */
        punpckldq %mm6, %mm6    /* gm gm gm gm gm gm gm gm */
        punpckldq %mm7, %mm7    /* bm bm bm bm bm bm bm bm */

        cmpl $256, rm
        jg shade_ximage_15_mmx_saturate
        cmpl $256, gm
        jg shade_ximage_15_mmx_saturate
        cmpl $256, bm
        jg shade_ximage_15_mmx_saturate

1:      movl %ebx, %ecx
        addl $3, %ecx
        jns 3f
2:
        movq (%esi, %ecx, 2), %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $10, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $11, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $3, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* 00 0r */
        pmulhw %mm6, %mm1       /* 00 0g */
        pmulhw %mm7, %mm2       /* 00 0b */

        psllw $10, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */
        
        movq %mm0, (%esi, %ecx, 2)

        addl $4, %ecx
        js 2b
        jmp 4f
3:
        movw (%esi, %ecx, 2), %ax
        movd %eax, %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $10, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $11, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $3, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* 00 0r */
        pmulhw %mm6, %mm1       /* 00 0g */
        pmulhw %mm7, %mm2       /* 00 0b */

        psllw $10, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */

        movd %mm0, %eax
        movw %ax, (%esi, %ecx, 2)

        incl %ecx
4:
        cmpl $2, %ecx
        jng 3b

        addl bpl, %esi
        decl %edx
        jnz 1b
5:
        LEAVE


shade_ximage_15_mmx_saturate:

        pcmpeqw %mm3, %mm3
        psllw $5, %mm3          /* ff e0 ff e0 ff e0 ff e0 */

1:      movl %ebx, %ecx
        addl $3, %ecx
        jns 3f
2:
        movq (%esi, %ecx, 2), %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $10, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $11, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $3, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* xx xr */
        pmulhw %mm6, %mm1       /* xx xg */
        pmulhw %mm7, %mm2       /* xx xb */

        /* Saturate upper */
        paddusw %mm3, %mm0      /* ff er */
        paddusw %mm3, %mm1      /* ff eg */
        paddusw %mm3, %mm2      /* ff eb */

        psubw %mm3, %mm1        /* 00 0g */
        psubw %mm3, %mm2        /* 00 0b */
        
        psllw $10, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */

        movq %mm0, (%esi, %ecx, 2)

        addl $4, %ecx
        js 2b
        jmp 4f
3:
        movw (%esi, %ecx, 2), %ax
        movd %eax, %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $10, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $11, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $3, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* xx xr */
        pmulhw %mm6, %mm1       /* xx xg */
        pmulhw %mm7, %mm2       /* xx xb */

        /* Saturate upper */
        paddusw %mm3, %mm0      /* ff er */
        paddusw %mm3, %mm1      /* ff eg */
        paddusw %mm3, %mm2      /* ff eb */

        psubw %mm3, %mm1        /* 00 0g */
        psubw %mm3, %mm2        /* 00 0b */
        
        psllw $10, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */

        movd %mm0, %eax
        movw %ax, (%esi, %ecx, 2)

        incl %ecx
4:
        cmpl $2, %ecx
        jng 3b

        addl bpl, %esi
        decl %edx
        jnz 1b
5:
        LEAVE


SHADE_XIMAGE_16:
        ENTER

        leal -6(%esi, %ebx, 2), %esi
        negl %ebx
        jz 5f

        /* Setup multipliers */
        movd rm, %mm5
        movd gm, %mm6
        movd bm, %mm7
        punpcklwd %mm5, %mm5    /* 00 00 00 00 rm rm rm rm */
        punpcklwd %mm6, %mm6    /* 00 00 00 00 gm gm gm gm */
        punpcklwd %mm7, %mm7    /* 00 00 00 00 bm bm bm bm */
        punpckldq %mm5, %mm5    /* rm rm rm rm rm rm rm rm */
        punpckldq %mm6, %mm6    /* gm gm gm gm gm gm gm gm */
        punpckldq %mm7, %mm7    /* bm bm bm bm bm bm bm bm */

        cmpl $256, rm
        jg shade_ximage_16_mmx_saturate
        cmpl $256, gm
        jg shade_ximage_16_mmx_saturate
        cmpl $256, bm
        jg shade_ximage_16_mmx_saturate

1:      movl %ebx, %ecx
        addl $3, %ecx
        jns 3f
2:
        movq (%esi, %ecx, 2), %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $11, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $10, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $2, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* 00 0r */
        pmulhw %mm6, %mm1       /* 00 0g */
        pmulhw %mm7, %mm2       /* 00 0b */

        psllw $11, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */
        
        movq %mm0, (%esi, %ecx, 2)

        addl $4, %ecx
        js 2b
	jmp 4f
3:
        movw (%esi, %ecx, 2), %ax
        movd %eax, %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $11, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $10, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $2, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* 00 0r */
        pmulhw %mm6, %mm1       /* 00 0g */
        pmulhw %mm7, %mm2       /* 00 0b */

        psllw $11, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */

        movd %mm0, %eax
        movw %ax, (%esi, %ecx, 2)

        incl %ecx
4:
        cmpl $2, %ecx
        jng 3b

        addl bpl, %esi
        decl %edx
        jnz 1b
5:
        LEAVE


shade_ximage_16_mmx_saturate:

        pcmpeqw %mm3, %mm3
        movq %mm3, %mm4
        psllw $5, %mm3          /* ff e0 ff e0 ff e0 ff e0 */
        psllw $6, %mm4          /* ff c0 ff c0 ff c0 ff c0 */

1:      movl %ebx, %ecx
        addl $3, %ecx
        jns 3f
2:
        movq (%esi, %ecx, 2), %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $11, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $10, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $2, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* xx xr */
        pmulhw %mm6, %mm1       /* xx xg */
        pmulhw %mm7, %mm2       /* xx xb */

        /* Saturate upper */
        paddusw %mm3, %mm0      /* ff er */
        paddusw %mm4, %mm1      /* ff cg */
        paddusw %mm3, %mm2      /* ff eb */

        psubw %mm4, %mm1        /* 00 0g */
        psubw %mm3, %mm2        /* 00 0b */
        
        psllw $11, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */

        movq %mm0, (%esi, %ecx, 2)

        addl $4, %ecx
        js 2b
        jmp 4f
3:
        movw (%esi, %ecx, 2), %ax
        movd %eax, %mm0

        movq %mm0, %mm1         /* rg gb */
        movq %mm0, %mm2         /* rg gb */
        psrlw $5, %mm1          /* 0r rg */
        psrlw $11, %mm0         /* 00 0r */
        psllw $11, %mm2         /* b0 00 */
        psllw $10, %mm1         /* g0 00 */
        psllw $8, %mm0          /* 0r 00 */
        psrlw $2, %mm1          /* 0g 00 */
        psrlw $3, %mm2          /* 0b 00 */

        pmulhw %mm5, %mm0       /* xx xr */
        pmulhw %mm6, %mm1       /* xx xg */
        pmulhw %mm7, %mm2       /* xx xb */

        /* Saturate upper */
        paddusw %mm3, %mm0      /* ff er */
        paddusw %mm4, %mm1      /* ff cg */
        paddusw %mm3, %mm2      /* ff eb */

        psubw %mm4, %mm1        /* 00 0g */
        psubw %mm3, %mm2        /* 00 0b */
        
        psllw $11, %mm0         /* r0 00 */
        psllw $5, %mm1          /* 0g g0 */
        por %mm2, %mm0          /* r0 0b */
        por %mm1, %mm0          /* rg gb */

        movd %mm0, %eax
        movw %ax, (%esi, %ecx, 2)

        incl %ecx
4:
        cmpl $2, %ecx
        jng 3b

        addl bpl, %esi
        decl %edx
        jnz 1b
5:
        LEAVE


SHADE_XIMAGE_32:
        ENTER

        leal (%esi, %ebx, 4), %esi
        negl %ebx
        jz 3f

        movd rm, %mm4
        movd gm, %mm5
        movd bm, %mm6
        psllq $32, %mm4
        psllq $16, %mm5
        por %mm6, %mm4
        por %mm5, %mm4

        pcmpeqw %mm6, %mm6
        psllw $15, %mm6                 /* 80 00 80 00 80 00 80 00 */
        movq %mm6, %mm5
        pmulhw %mm4, %mm5               /* Get correction factor */
1:
        movl %ebx, %ecx
2:
        movd (%esi, %ecx, 4), %mm1      /* 00 rr gg bb */
        pxor %mm0, %mm0
        punpcklbw %mm1, %mm0            /* 00 00 rr 00 gg 00 bb 00 */
        pxor %mm6, %mm0                 /* Flip sign */

        pmulhw %mm4, %mm0               /* 00 00 xx rr xx gg xx bb */
        psubw %mm5, %mm0                /* Correct range */
        packuswb %mm0, %mm0             /* 00 rr gg bb 00 rr gg bb */

        movd %mm0, (%esi, %ecx, 4)

        incl %ecx
        jnz 2b

        addl bpl, %esi
        decl %edx
        jnz 1b
3:
        LEAVE


HAVE_MMX:
	push	%ebx
/* Check if bit 21 in flags word is writeable */
	pushfl	
	popl	%eax
	movl	%eax,%ebx
	xorl	$0x00200000, %eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax

	cmpl	%eax, %ebx
	je	8f

/* OK, we have CPUID */

	movl	$1, %eax
	cpuid
	
	test	$0x00800000, %edx
	jz	8f

	movl	$1, %eax	/* success, have mmx */
	popl	%ebx
	ret

8:
	xorl	%eax,%eax	/* failed, no mmx */
	popl	%ebx
	ret

#if defined(__GNUC__) && !defined(_WIN32)
.section .note.GNU-stack, "", @progbits
.previous
#endif

////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

/* This is an assembly optimized version of the following WavPack function:
 *
 * void decorr_stereo_pass_cont (struct decorr_pass *dpp,
 *                               long *buffer, long sample_count);
 *
 * It performs a single pass of stereo decorrelation on the provided buffer.
 * Note that this version of the function requires that the 8 previous stereo
 * samples are visible and correct. In other words, it ignores the "samples_*"
 * fields in the decorr_pass structure and gets the history data directly
 * from the buffer. It does, however, return the appropriate history samples
 * to the decorr_pass structure before returning.
 *
 * This is written to work on a MCF5249 processor, or any processor based on
 * the ColdFire V2 core with an EMAC unit. The EMAC is perfectly suited for
 * the "apply_weight" function of WavPack decorrelation because it provides
 * the requires 40-bit product. The fractional rounding mode of the EMAC is not
 * configurable and uses "round to even" while WavPack uses "round to larger",
 * so the rounding has to be done manually.
 */

        .text
        .align  2
        .global decorr_stereo_pass_cont_mcf5249

decorr_stereo_pass_cont_mcf5249:

        lea     (-44, %sp), %sp
        movem.l %d2-%d7/%a2-%a6, (%sp)
        move.l  44+4(%sp), %a2          | a2 = dpp->
        move.l  44+8(%sp), %a1          | a1 = bptr
        move.w  2(%a2), %a3             | a3 = dpp->delta
        move.w  4(%a2), %d3             | d3 = dpp->weight_A (sign extended)
        ext.l   %d3
        move.w  6(%a2), %d4             | d4 = dpp->weight_B (sign extended)
        ext.l   %d4
        move.l 44+12(%sp), %d0          | d0 = sample_count
        jbeq    return_only             | if zero, nothing to do

        lsl.l   #3, %d0                 | d5 = bptr + (sample_count * 8)
        move.l  %d0, %d5
        add.l   %a1, %d5

        moveq.l #17, %d0                | left shift weights & delta 17 places
        asl.l   %d0, %d3
        asl.l   %d0, %d4
        move.l  %a3, %d1
        asl.l   %d0, %d1
        move.l  %d1, %a3

        moveq.l #0x20, %d6
        move.l  %d6, %macsr             | set fractional mode for MAC
        move.l  #0, %acc1               | acc1 = 0x00 0000 80 (for rounding)
        move.l  #0x800000, %accext01
        
        move.l  #1024<<17, %d6          | d6 & d7 are weight clipping limits
        move.l  #-1024<<17, %d7         | (only used by negative terms)

        move.w  (%a2), %d0              | d0 = term
        ext.l   %d0
        cmp.l   #17, %d0
        jbeq    term_17                 | term = 17
        cmp.l   #18, %d0
        jbeq    term_18                 | term = 18
        addq.l  #1, %d0
        jbeq    term_minus_1            | term = -1
        addq.l  #1, %d0
        jbeq    term_minus_2            | term = -2
        addq.l  #1, %d0
        jbeq    term_minus_3            | term = -3
        jbra    term_default            | default term = 1 - 8

|------------------------------------------------------------------------------
| Loop to handle term = 17 condition
|
| a0 =                          d0 = (2 * bptr [-1]) - bptr [-2]
| a1 = bptr                     d1 = initial bptr [0]
| a2 = dpp->                    d2 = updated bptr [0]
| a3 = dpp->delta << 17         d3 = dpp->weight_A << 17
| a4 =                          d4 = dpp->weight_B << 17
| a5 =                          d5 = eptr
| macsr = 0x20                  acc1 = 0x00 0000 80
|------------------------------------------------------------------------------

term_17:
        move.l  -8(%a1), %d0            | d0 = 2 * bptr [-1] - bptr [-2]
        add.l   %d0, %d0
        sub.l   -16(%a1), %d0
        beq     .L251                   | if zero, skip calculation
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + (d0 << 4) * weight_A
        mac.l   %d0, %d3, %acc0
        move.l  (%a1), %d1
        beq     .L255
        eor.l   %d1, %d0                | else compare signs
        bge     .L256                   | if same, add delta to weight
        sub.l   %a3, %d3                | else subtract delta from weight
        sub.l   %a3, %d3                | subtract again instead of branch
.L256:  add.l   %a3, %d3                | add delta to weight

.L255:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | update bptr [0] and store
        move.l  %d2, (%a1)+

.L253:  move.l  -8(%a1), %d0            | d0 = 2 * bptr [-1] - bptr [-2]
        add.l   %d0, %d0
        sub.l   -16(%a1), %d0
        beq     .L257                   | if zero, skip calculations
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + (d0 << 4) * weight_B
        mac.l   %d0, %d4, %acc0
        move.l  (%a1), %d1
        beq     .L254
        eor.l   %d1, %d0                | else compare signs
        bge     .L259                   | if same, add delta to weight
        sub.l   %a3, %d4                | else subtract delta from weight
        sub.l   %a3, %d4                | subtract again instead of branch
.L259:  add.l   %a3, %d4                | add delta to weight

.L254:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | update bptr [0] and store
        move.l  %d2, (%a1)+

.L252:  cmp.l   %a1, %d5                | loop if bptr < eptr
        jbhi    term_17
        bra     term_17_18_finish       | exit through common path

.L251:  addq.l  #4, %a1                 | update point and jump back into loop
        bra     .L253

.L257:  addq.l  #4, %a1                 | update point and jump back into loop
        bra     .L252

|------------------------------------------------------------------------------
| Loop to handle term = 18 condition
|
| a0 =                          d0 = ((3 * bptr [-1]) - bptr [-2]) >> 1
| a1 = bptr                     d1 = initial bptr [0]
| a2 = dpp->                    d2 = updated bptr [0]
| a3 = dpp->delta << 17         d3 = dpp->weight_A << 17
| a4 =                          d4 = dpp->weight_B << 17
| a5 =                          d5 = eptr
| macsr = 0x20                  acc1 = 0x00 0000 80
|------------------------------------------------------------------------------

term_18:
        move.l  -8(%a1), %a0            | d0 = (3 * bptr [-1] - bptr [-2]) >> 1
        lea     (%a0,%a0.l*2), %a0
        move.l  %a0, %d0
        sub.l   -16(%a1), %d0
        asr.l   #1, %d0
        beq     .L260
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + (d0 << 4) * weight_A
        mac.l   %d0, %d3, %acc0
        move.l  (%a1), %d1
        beq     .L266
        eor.l   %d1, %d0                | else compare signs
        bge     .L267                   | if same, add delta to weight
        sub.l   %a3, %d3                | else subtract delta from weight
        sub.l   %a3, %d3                | subtract again instead of branch
.L267:  add.l   %a3, %d3                | add delta to weight

.L266:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [0], store
        move.l  %d2, (%a1)+

.L268:  move.l  -8(%a1), %a0            | d0 = (3 * bptr [-1] - bptr [-2]) >> 1
        lea     (%a0,%a0.l*2), %a0
        move.l  %a0, %d0
        sub.l   -16(%a1), %d0
        asr.l   #1, %d0
        beq     .L261
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + (d0 << 4) * weight_B
        mac.l   %d0, %d4, %acc0
        move.l  (%a1), %d1
        beq     .L265
        eor.l   %d1, %d0                | else compare signs
        bge     .L270                   | if same, add delta to weight
        sub.l   %a3, %d4                | else subtract delta from weight
        sub.l   %a3, %d4                | subtract again instead of branch
.L270:  add.l   %a3, %d4                | add delta to weight

.L265:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [0], store
        move.l  %d2, (%a1)+

.L269:  cmp.l   %a1, %d5                | loop if bptr < eptr
        jbhi    term_18
        bra     term_17_18_finish       | exit through common path

.L260:  addq.l  #4, %a1                 | bump pointer and jump back into loop
        bra     .L268

.L261:  addq.l  #4, %a1                 | bump pointer and jump back into loop
        bra     .L269

term_17_18_finish:
        move.l  -4(%a1), 40(%a2)        | restore dpp->samples_A [0-1], B [0-1]
        move.l  -8(%a1), 8(%a2)
        move.l  -12(%a1), 44(%a2)
        move.l  -16(%a1), 12(%a2)
        jbra    finish_up

|------------------------------------------------------------------------------
| Loop to handle default terms (i.e. 1 - 8)
|
| a0 = tptr                     d0 = tptr [0]
| a1 = bptr                     d1 = initial bptr [0]
| a2 = dpp->                    d2 = updated bptr [0]
| a3 = dpp->delta << 17         d3 = dpp->weight_A << 17
| a4 =                          d4 = dpp->weight_B << 17
| a5 =                          d5 = eptr
| macsr = 0x20                  acc1 = 0x00 0000 80
|------------------------------------------------------------------------------

term_default:
        move.w  (%a2), %d0              | a0 = a1 - (dpp->term * 8)
        ext.l   %d0
        lsl.l   #3, %d0
        move.l  %a1, %a0
        sub.l   %d0, %a0

term_default_loop:
        move.l  (%a0)+, %d0             | d0 = tptr [0], skip ahead if zero
        beq     .L271
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + (d0 << 4) * weight_A
        mac.l   %d0, %d3, %acc0
        move.l  (%a1), %d1
        beq     .L277
        eor.l   %d1, %d0                | else compare signs
        bge     .L278                   | if same, add delta to weight
        sub.l   %a3, %d3                | else subtract delta from weight
        sub.l   %a3, %d3                | subtract again instead of branch
.L278:  add.l   %a3, %d3                | add delta to weight

.L277:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [0], store
        move.l  %d2, (%a1)+

.L275:  move.l  (%a0)+, %d0             | d0 = tptr [0], skip ahead if zero
        beq     .L272
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + (d0 << 4) * weight_B
        mac.l   %d0, %d4, %acc0
        move.l  (%a1), %d1
        beq     .L276
        eor.l   %d1, %d0                | else compare signs
        bge     .L281                   | if same, add delta to weight
        sub.l   %a3, %d4                | else subtract delta from weight
        sub.l   %a3, %d4                | subtract again instead of branch
.L281:  add.l   %a3, %d4                | add delta to weight

.L276:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [0], store
        move.l  %d2, (%a1)+

.L274:  cmp.l   %a1, %d5                | loop back if bptr < eptr
        jbhi    term_default_loop
        move.w  (%a2), %d0              | d0 = term - 1
        moveq.l #8, %d1                 | d1 = loop counter

.L323:  subq.l  #1, %d0                 | back up & mask index
        and.l   #7, %d0
        move.l  -(%a1), 40(%a2,%d0.l*4) | store dpp->samples_B [d0]
        move.l  -(%a1), 8(%a2,%d0.l*4)  | store dpp->samples_A [d0]
        subq.l  #1, %d1                 | loop on count
        jbne    .L323
        jbra    finish_up

.L271:  addq.l  #4, %a1                 | bump pointer and jump back into loop
        bra     .L275

.L272:  addq.l  #4, %a1                 | bump pointer and jump back into loop
        bra     .L274


|------------------------------------------------------------------------------
| Loop to handle term = -1 condition
|
| a0 =                          d0 = decorrelation sample
| a1 = bptr                     d1 = initial bptr [0]
| a2 = dpp->                    d2 = updated bptr [0]
| a3 = dpp->delta << 17         d3 = dpp->weight_A << 17
| a4 =                          d4 = dpp->weight_B << 17
| a5 =                          d5 = eptr
| a6 =                          d6 = 1024 << 17
| a7 =                          d7 = -1024 << 17
| macsr = 0x20                  acc1 = 0x00 0000 80
|------------------------------------------------------------------------------

term_minus_1:
        move.l  -4(%a1), %d0            | d0 = bptr [-1]
        beq     .L402
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + ((d0 << 4) * weight_A)
        mac.l   %d0, %d3, %acc0
        move.l  (%a1), %d1
        beq     .L405
        eor.l   %d1, %d0                | else compare signs
        bge     .L404                   | if same, add delta to weight
        sub.l   %a3, %d3                | else subtract delta from weight
        cmp.l   %d7, %d3                | check for negative clip limit
        bge     .L405
        move.l  %d7, %d3
        bra     .L405

.L404:  add.l   %a3, %d3                | add delta to weight
        cmp.l   %d6, %d3                | check for positive clip limit
        ble     .L405
        move.l  %d6, %d3

.L405:  move.l  %acc0, %d0              | d2 = rounded product
        add.l   %d1, %d0                | add applied weight to bptr [0], store
        move.l  %d0, (%a1)+
        beq     .L401

.L410:  move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + ((d0 << 4) * weight_B)
        mac.l   %d0, %d4, %acc0
        move.l  (%a1), %d1
        beq     .L403
        eor.l   %d1, %d0                | else compare signs
        bge     .L407                   | if same, add delta to weight
        sub.l   %a3, %d4                | else subtract delta from weight
        cmp.l   %d7, %d4                | check for negative clip limit
        bge     .L403
        move.l  %d7, %d4
        bra     .L403

.L407:  add.l   %a3, %d4                | add delta to weight
        cmp.l   %d6, %d4                | check for positive clip limit
        ble     .L403
        move.l  %d6, %d4

.L403:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [1], store
        move.l  %d2, (%a1)+

.L411:  cmp.l   %a1, %d5                | loop back if bptr < eptr
        jbhi    term_minus_1
        move.l  -4(%a1), 8(%a2)         | dpp->samples_A [0] = bptr [-1]
        jbra    finish_up

.L402:  move.l  (%a1)+, %d0
        bne     .L410

.L401:  addq.l  #4, %a1
        bra     .L411


|------------------------------------------------------------------------------
| Loop to handle term = -2 condition
|
| a0 =                          d0 = decorrelation sample
| a1 = bptr                     d1 = initial bptr [0]
| a2 = dpp->                    d2 = updated bptr [0]
| a3 = dpp->delta << 17         d3 = dpp->weight_A << 17
| a4 =                          d4 = dpp->weight_B << 17
| a5 =                          d5 = eptr
| a6 =                          d6 = 1024 << 17
| a7 =                          d7 = -1024 << 17
| macsr = 0x20                  acc1 = 0x00 0000 80
|------------------------------------------------------------------------------

term_minus_2:
        move.l  -8(%a1), %d0            | d0 = bptr [-2]
        beq     .L511
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + ((d0 << 4) * weight_B)
        mac.l   %d0, %d4, %acc0
        move.l  4(%a1), %d1
        beq     .L505
        eor.l   %d1, %d0                | else compare signs
        bge     .L504                   | if same, add delta to weight
        sub.l   %a3, %d4                | else subtract delta from weight
        cmp.l   %d7, %d4                | ckeck for negative clip limit
        bge     .L505
        move.l  %d7, %d4
        bra     .L505

.L504:  add.l   %a3, %d4                | add delta to weight
        cmp.l   %d6, %d4                | check for positive clip limit
        ble     .L505
        move.l  %d6, %d4

.L505:  move.l  %acc0, %d0              | d2 = rounded product
        add.l   %d1, %d0                | add applied weight to bptr [0], store
        move.l  %d0, 4(%a1)
        beq     .L512

.L510:  move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + ((d0 << 4) * weight_A)
        mac.l   %d0, %d3, %acc0
        move.l  (%a1), %d1
        beq     .L503
        eor.l   %d1, %d0                | else compare signs
        bge     .L507                   | if same, add delta to weight
        sub.l   %a3, %d3                | else subtract delta from weight
        cmp.l   %d7, %d3                | check for negative clip limit
        bge     .L503
        move.l  %d7, %d3
        bra     .L503

.L507:  add.l   %a3, %d3                | add delta to weight
        cmp.l   %d6, %d3                | check for negative clip limit
        ble     .L503
        move.l  %d6, %d3

.L503:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [1], store
        move.l  %d2, (%a1)

.L512:  addq.l  #8, %a1
        cmp.l   %a1, %d5                | loop if bptr < eptr
        jbhi    term_minus_2
        move.l  -8(%a1), 40(%a2)        | dpp->samples_B [0] = bptr [-4]
        jbra    finish_up

.L511:  move.l  4(%a1), %d0
        beq     .L512
        bra     .L510


|------------------------------------------------------------------------------
| Loop to handle term = -3 condition
|
| a0 =                          d0 = decorrelation sample
| a1 = bptr                     d1 = initial bptr [0]
| a2 = dpp->                    d2 = updated bptr [0]
| a3 = dpp->delta << 17         d3 = dpp->weight_A << 17
| a4 =                          d4 = dpp->weight_B << 17
| a5 =                          d5 = eptr
| a6 =                          d6 = 1024 << 17
| a7 =                          d7 = -1024 << 17
| macsr = 0x20                  acc1 = 0x00 0000 80
|------------------------------------------------------------------------------

term_minus_3:
        move.l  -4(%a1), %d0            | d0 = bptr [-1]
        beq     .L301
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + ((d0 << 4) * weight_A)
        mac.l   %d0, %d3, %acc0
        move.l  (%a1), %d1
        beq     .L320
        eor.l   %d1, %d0                | else compare signs
        bge     .L319                   | if same, add delta to weight
        sub.l   %a3, %d3                | else subtract delta from weight
        cmp.l   %d7, %d3                | check for negative clip limit
        bge     .L320
        move.l  %d7, %d3
        bra     .L320

.L319:  add.l   %a3, %d3                | add delta to weight
        cmp.l   %d6, %d3                | check for positive clip limit
        ble     .L320
        move.l  %d6, %d3

.L320:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [0], store
        move.l  %d2, (%a1)+

.L330:  move.l  -12(%a1), %d0           | d0 = bptr [-2]
        beq     .L302
        move.l  %acc1, %acc0
        asl.l   #4, %d0                 | acc0 = acc1 + ((d0 << 4) * weight_B)
        mac.l   %d0, %d4, %acc0
        move.l  (%a1), %d1
        beq     .L318
        eor.l   %d1, %d0                | else compare signs
        bge     .L322                   | if same, add delta to weight
        sub.l   %a3, %d4                | else subtract delta from weight
        cmp.l   %d7, %d4                | check for negative clip limit
        bge     .L318
        move.l  %d7, %d4
        bra     .L318

.L322:  add.l   %a3, %d4                | add delta to weight
        cmp.l   %d6, %d4                | check for positive clip limit
        ble     .L318
        move.l  %d6, %d4

.L318:  move.l  %acc0, %d2              | d2 = rounded product
        add.l   %d1, %d2                | add applied weight to bptr [1], store
        move.l  %d2, (%a1)+

.L331:  cmp.l   %a1, %d5                | bptr, eptr
        jbhi    term_minus_3
        move.l  -4(%a1), 8(%a2)         | dpp->samples_A [0] = bptr [-1]
        move.l  -8(%a1), 40(%a2)        | dpp->samples_B [0] = bptr [-2]
        jbra    finish_up

.L301:  addq.l  #4, %a1
        bra     .L330

.L302:  addq.l  #4, %a1
        bra     .L331

| finish and return

finish_up:
        moveq.l #17, %d0
        asr.l   %d0, %d3
        asr.l   %d0, %d4
        move.w  %d3, 4(%a2)     | weight_A, dpp->weight_A
        move.w  %d4, 6(%a2)     | weight_B, dpp->weight_B

        clr.l   %d0             | clear up EMAC
        move.l  %d0, %acc0
        move.l  %d0, %acc1

return_only:
        movem.l (%sp), %d2-%d7/%a2-%a6
        lea     (44,%sp), %sp
        rts

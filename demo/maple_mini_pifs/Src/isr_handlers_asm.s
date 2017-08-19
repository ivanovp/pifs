.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.weak HardFault_Handler
.global HardFault_Handler
.extern hard_fault_handler_c

HardFault_Handler:
  TST LR, #4
  ITE EQ            /* if-then-else */
  MRSEQ R0, MSP     /* if TST LR, #4 is true*/
  MRSNE R0, PSP     /* otherwise */

  B hard_fault_handler_c
  /*LDR PC, =hard_fault_handler_c*/

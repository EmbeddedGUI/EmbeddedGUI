/**
 * Minimal startup for QEMU ARM Cortex-M emulation.
 * Provides vector table, Reset_Handler, and default fault handlers.
 * Compatible with Cortex-M0/M0+/M3/M4/M7 (Thumb mode).
 */

    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

.global g_pfnVectors
.global Default_Handler
.global Reset_Handler

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Set stack pointer */
    ldr r0, =_estack
    mov sp, r0

    /* Copy .data from FLASH to RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    movs r3, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4

LoopCopyDataInit:
    adds r4, r0, r3
    cmp r4, r1
    bcc CopyDataInit

    /* Zero fill .bss */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    /* Call main */
    bl main

    /* If main returns, loop forever */
LoopForever:
    b LoopForever

    .size Reset_Handler, .-Reset_Handler

/**
 * Default handler for all interrupts/exceptions.
 */
    .section .text.Default_Handler, "ax", %progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, .-Default_Handler

/**
 * Minimal vector table (only reset + basic faults).
 */
    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word _estack               /* Top of Stack */
    .word Reset_Handler         /* Reset Handler */
    .word NMI_Handler           /* NMI Handler */
    .word HardFault_Handler     /* Hard Fault Handler */
    .word MemManage_Handler     /* MPU Fault Handler */
    .word BusFault_Handler      /* Bus Fault Handler */
    .word UsageFault_Handler    /* Usage Fault Handler */
    .word 0                     /* Reserved */
    .word 0                     /* Reserved */
    .word 0                     /* Reserved */
    .word 0                     /* Reserved */
    .word SVC_Handler           /* SVCall Handler */
    .word DebugMon_Handler      /* Debug Monitor Handler */
    .word 0                     /* Reserved */
    .word PendSV_Handler        /* PendSV Handler */
    .word SysTick_Handler       /* SysTick Handler */
    /* External interrupts (IRQ 0-7) */
    .word Default_Handler       /* IRQ 0: UART0 */
    .word Default_Handler       /* IRQ 1: UART1 */
    .word Default_Handler       /* IRQ 2: UART2 */
    .word Default_Handler       /* IRQ 3 */
    .word Default_Handler       /* IRQ 4 */
    .word Default_Handler       /* IRQ 5 */
    .word Default_Handler       /* IRQ 6 */
    .word Default_Handler       /* IRQ 7 */
    .word CMSDK_TIMER0_Handler  /* IRQ 8: CMSDK Timer 0 */

    /* Weak aliases for fault handlers */
    .weak NMI_Handler
    .thumb_set NMI_Handler, Default_Handler
    .weak HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler
    .weak MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler
    .weak BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler
    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler
    .weak SVC_Handler
    .thumb_set SVC_Handler, Default_Handler
    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler
    .weak PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler
    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler
    .weak CMSDK_TIMER0_Handler
    .thumb_set CMSDK_TIMER0_Handler, Default_Handler

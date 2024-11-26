/* Vector Table
 * This file defines the interrupt vector table and handlers for the RP2040
 * The vector table is placed at a specific memory location and contains
 * pointers to interrupt service routines */

#include <stdint.h>
#include <stdbool.h>

/* Function pointer type for vector table entries
 * Each entry must be a void function that takes no arguments
 * This matches ARM Cortex-M0+ exception handler signature */
typedef void (*vectFunc) (void);

/* External stack pointer symbol
 * Defined by the linker script, marks the top of stack memory
 * Used as the initial stack pointer value */
extern uint32_t _sstack;

/* Core handler function declarations
 * These functions handle primary system exceptions
 * defaultHandler and resetHandler are defined in this file */
__attribute__((noreturn)) void defaultHandler();
__attribute__((noreturn)) void resetHandler();

/* Core exception handler declarations
 * These are marked as weak symbols that alias to defaultHandler
 * Can be overridden by strong symbols in other source files
 * Handles system-level exceptions like NMI, HardFault, etc. */
void nmiHandler         () __attribute__((weak, alias("defaultHandler")));
void hardFaultHandler   () __attribute__((weak, alias("defaultHandler")));
void svCallHandler      () __attribute__((weak, alias("defaultHandler")));
void pendSvHandler      () __attribute__((weak, alias("defaultHandler")));
void sysTickHandler     () __attribute__((weak, alias("defaultHandler")));

/* Peripheral interrupt handler declarations
 * All peripheral interrupts are initially weak aliases to defaultHandler
 * Each can be overridden by defining a strong symbol in another source file
 * Handles device-specific interrupts like timers, GPIO, etc. */
void timerIrq0          () __attribute__((weak, alias("defaultHandler")));
void timerIrq1          () __attribute__((weak, alias("defaultHandler")));
void timerIrq2          () __attribute__((weak, alias("defaultHandler")));
void timerIrq3          () __attribute__((weak, alias("defaultHandler")));
void pwmIrqWrap         () __attribute__((weak, alias("defaultHandler")));
void usbctrlIrq         () __attribute__((weak, alias("defaultHandler")));
void xipIrq             () __attribute__((weak, alias("defaultHandler")));
void pio0Irq0           () __attribute__((weak, alias("defaultHandler")));
void pio0Irq1           () __attribute__((weak, alias("defaultHandler")));
void pio1Irq0           () __attribute__((weak, alias("defaultHandler")));
void pio1Irq1           () __attribute__((weak, alias("defaultHandler")));
void dmaIrq0            () __attribute__((weak, alias("defaultHandler")));
void dmaIrq1            () __attribute__((weak, alias("defaultHandler")));
void ioIrqBank0         () __attribute__((weak, alias("defaultHandler"))); 
void ioIrqQspi          () __attribute__((weak, alias("defaultHandler")));
void sioIrqProc0        () __attribute__((weak, alias("defaultHandler")));
void sioIrqProc1        () __attribute__((weak, alias("defaultHandler")));
void clocksIrq          () __attribute__((weak, alias("defaultHandler")));
void spi0Irq            () __attribute__((weak, alias("defaultHandler")));
void spi1Irq            () __attribute__((weak, alias("defaultHandler")));
void uart0Irq           () __attribute__((weak, alias("defaultHandler")));
void uart1Irq           () __attribute__((weak, alias("defaultHandler")));
void adcIrqFifo         () __attribute__((weak, alias("defaultHandler")));
void i2c0Irq            () __attribute__((weak, alias("defaultHandler")));
void i2c1Irq            () __attribute__((weak, alias("defaultHandler")));
void rtcIrq             () __attribute__((weak, alias("defaultHandler")));

/* Main program entry point declaration
 * Defined in a separate source file */
extern int main(void);

/* Vector table definition
 * Placed in .vector section by linker script
 * Contains pointers to exception and interrupt handlers
 * Order matches ARM Cortex-M0+ vector table specification */
const vectFunc vector[] __attribute__((section(".vector"))) = 
{
    /* Core System Handler Vectors
     * First 16 entries are system exception handlers */
    (vectFunc)(&_sstack),   /* Initial Stack Pointer value */
    resetHandler,           /* Reset Handler - called on reset */
    nmiHandler,             /* Non-Maskable Interrupt Handler */
    hardFaultHandler,       /* Hard Fault Handler */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    svCallHandler,          /* SVCall Handler */
    0,                      /* Reserved */
    0,                      /* Reserved */
    pendSvHandler,          /* PendSV Handler */
    sysTickHandler,         /* SysTick Handler */

    /* Device-Specific Interrupt Vectors
     * Entries 16 and above are peripheral interrupt handlers */
    timerIrq0,              /* Timer 0 */
    timerIrq1,              /* Timer 1 */
    timerIrq2,              /* Timer 2 */
    timerIrq3,              /* Timer 3 */
    pwmIrqWrap,             /* PWM Wrap */
    usbctrlIrq,             /* USB Controller */
    xipIrq,                 /* XIP Controller */
    pio0Irq0,               /* PIO0 IRQ 0 */
    pio0Irq1,               /* PIO0 IRQ 1 */
    pio1Irq0,               /* PIO1 IRQ 0 */
    pio1Irq1,               /* PIO1 IRQ 1 */
    dmaIrq0,                /* DMA IRQ 0 */
    dmaIrq1,                /* DMA IRQ 1 */
    ioIrqBank0,             /* IO Bank 0 */
    ioIrqQspi,              /* IO QSPI */
    sioIrqProc0,            /* SIO Processor 0 */
    sioIrqProc1,            /* SIO Processor 1 */
    clocksIrq,              /* Clocks */
    spi0Irq,                /* SPI 0 */
    spi1Irq,                /* SPI 1 */
    uart0Irq,               /* UART 0 */
    uart1Irq,               /* UART 1 */
    adcIrqFifo,             /* ADC FIFO */
    i2c0Irq,                /* I2C 0 */
    i2c1Irq,                /* I2C 1 */
    rtcIrq,                 /* RTC */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
};

/* Reset handler implementation
 * Called on system reset, responsible for initializing the system
 * Jumps to main() and enters infinite loop if main returns */
void resetHandler()
{
    main(); 
    while(true); /* Infinite loop if main returns */
}

/* Hardware register definitions
 * Direct register access for basic GPIO control */
#define RESETS_RESET                  *(volatile uint32_t *) (0x4000c000)
#define IO_BANK0_GPIO25_CTRL          *(volatile uint32_t *) (0x400140cc)
#define SIO_GPIO_OE_SET               *(volatile uint32_t *) (0xd0000024)
#define SIO_GPIO_OUT_XOR              *(volatile uint32_t *) (0xd000001c)

/* Default interrupt handler
 * Called for any unimplemented interrupt
 * Enters low-power wait state until next interrupt */
void defaultHandler()
{
    while (true) {
        __asm volatile("wfi");  /* Wait for interrupt - low power state */
    }
}
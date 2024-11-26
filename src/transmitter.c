#include <stdint.h>
#include <stdbool.h>

/* Hardware Register Structures */
/* SIO (Single-cycle IO) registers for fast GPIO access */
struct sio_hw {
    uint32_t cpuid;          /* Processor core identifier */
    uint32_t gpio_in;        /* Input values for GPIO 0-29 */
    uint32_t gpio_hi_in;     /* Input values for GPIO 30-35 */
    uint32_t unused;         /* Reserved */
    uint32_t gpio_out;       /* GPIO output values */
    uint32_t gpio_out_set;   /* Set GPIO output bits */
    uint32_t gpio_out_clr;   /* Clear GPIO output bits */
    uint32_t gpio_out_xor;   /* XOR GPIO output bits */
    uint32_t gpio_oe;        /* GPIO output enable */
    uint32_t gpio_oe_set;    /* Set GPIO output enable bits */
    uint32_t gpio_oe_clr;    /* Clear GPIO output enable bits */
    uint32_t gpio_oe_xor;    /* XOR GPIO output enable bits */
};

/* IO Bank 0 registers for GPIO configuration and interrupts */
/* Each structure has two 32-bit values: status and ctrl */
struct io_bank0_hw {
    struct {
        uint32_t status;     /* GPIO status */
        uint32_t ctrl;       /* GPIO control including function selection */
    } gpio[30];             /* We repeated this 30 times for each GPIO */
    uint32_t intr[4];       /* Raw interrupts */
    uint32_t proc0_inte[4]; /* Interrupt enable for processor 0 */
    uint32_t proc0_intf[4]; /* Interrupt force for processor 0 */
    uint32_t proc0_ints[4]; /* Interrupt status for processor 0 */
};

/* Pad control registers for GPIO electrical properties */
struct pads_bank0_hw {
    uint32_t voltage_select; /* Voltage select */
    uint32_t gpio[30];      /* Pad control register for each GPIO */
    uint32_t swclk;         /* Pad control register for SWCLK */
    uint32_t swd;           /* Pad control register for SWD */
};

/* Base addresses for hardware registers */
#define SIO_BASE        0xd0000000
#define IO_BANK0_BASE   0x40014000
#define PADS_BANK0_BASE 0x4001c000
#define RESETS_BASE     0x4000c000

/* Register access pointers */
#define sio  ((volatile struct sio_hw*)SIO_BASE)
/* This line means:
   1. Take SIO_BASE address (0xd0000000)
   2. Cast it to a pointer to struct sio_hw
   3. Make it volatile (tells compiler value can change unexpectedly)
   4. Store in variable 'sio' */
#define io   ((volatile struct io_bank0_hw*)IO_BANK0_BASE)
#define pads ((volatile struct pads_bank0_hw*)PADS_BANK0_BASE)

/* Pin definitions */
#define BUTTON_PIN 16    /* Push button input */
#define SPEAKER_PIN 21   /* Speaker output */
#define LED_PIN 25      /* Onboard LED */
#define GPIO_FUNC_SIO 5 /* SIO function for GPIO */

/* Interrupt configuration */
#define GPIO_INT_EDGE_HIGH  0x8
#define IO_BANK0_IRQ 13    /* IO Bank 0 interrupt number */
#define NVIC_BASE 0xe000e000
#define NVIC_ISER (*(volatile uint32_t*)(NVIC_BASE + 0x100))

/* Simple delay function using NOP instructions */
static void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm("nop");
    }
}

/* Interrupt handler for IO Bank 0 */
void ioIrqBank0(void) {
    /* This checks if button caused interrupt:
       1. proc0_ints[BUTTON_PIN / 8] gets the right interrupt status register
       2. BUTTON_PIN / 8 divides pin number by 8 to get right register (pins 0-7 use 0, 8-15 use 1, etc)
       3. BUTTON_PIN % 8 gets remainder to find position within register
       4. << (4 * (BUTTON_PIN % 8)) shifts bits to right position (each pin uses 4 bits) */
    if (io->proc0_ints[BUTTON_PIN / 8] & (GPIO_INT_EDGE_HIGH << (4 * (BUTTON_PIN % 8)))) {

         /* Set bits using OR operation:
           1. 1U creates unsigned integer 1
           2. << LED_PIN shifts 1 left by LED_PIN positions
           3. | combines both shifted values with OR
           4. Writing to gpio_out_set sets those pins high */
        sio->gpio_out_set = (1U << LED_PIN) | (1U << SPEAKER_PIN);
        delay(100000);
        /* Deactivate LED and speaker */
        sio->gpio_out_clr = (1U << LED_PIN) | (1U << SPEAKER_PIN);
        
        /* Clear bits - same as set but writes to clear register */
        io->intr[BUTTON_PIN / 8] = 0xF << (4 * (BUTTON_PIN % 8));
    }
}

int main(void) {
    /* Reset IO Bank 0 peripheral:
       1. Create pointer to reset register
       2. Clear bit 5 using AND with inverted bit pattern
       3. Wait while bit is still set */
    volatile uint32_t* RESETS_RESET = (volatile uint32_t*)(RESETS_BASE + 0x0);
    *RESETS_RESET &= ~(1U << 5);
    while ((*RESETS_RESET & (1U << 5)) != 0) {}

    /* Configure button (GPIO16) 
       1. Set GPIO function using direct register write
       2. Clear output enable bit for input mode
       3. Set pad control (pull-up and input enable) */
    io->gpio[BUTTON_PIN].ctrl = GPIO_FUNC_SIO;    /* Set to SIO function */
    sio->gpio_oe_clr = 1U << BUTTON_PIN;         /* Set as input */
    pads->gpio[BUTTON_PIN] = (1U << 3) | (1U << 6); /* Enable pull-up and input */
    
    /* Configure LED (GPIO25) */
    io->gpio[LED_PIN].ctrl = GPIO_FUNC_SIO;      /* Set to SIO function */
    sio->gpio_oe_set = 1U << LED_PIN;           /* Set as output */
    
    /* Configure speaker (GPIO21) */
    io->gpio[SPEAKER_PIN].ctrl = GPIO_FUNC_SIO;  /* Set to SIO function */
    sio->gpio_oe_set = 1U << SPEAKER_PIN;       /* Set as output */
    
    /* Setup button interrupt 
       1. Clear existing interrupts
       2. Enable rising edge interrupt for button
       3. Enable IO Bank 0 interrupt in NVIC (Nested Vectored Interrupt Controller) */
    io->intr[BUTTON_PIN / 8] = 0xF << (4 * (BUTTON_PIN % 8));  /* Clear pending interrupts */
    io->proc0_inte[BUTTON_PIN / 8] |= GPIO_INT_EDGE_HIGH << (4 * (BUTTON_PIN % 8));  /* Enable rising edge interrupt */
    NVIC_ISER = 1U << IO_BANK0_IRQ;

    /* Startup test pattern */
    for(int i = 0; i < 3; i++) {
        /* Turn on LED and speaker */
        sio->gpio_out_set = (1U << LED_PIN) | (1U << SPEAKER_PIN);
        delay(100000);
        /* Turn off LED and speaker */
        sio->gpio_out_clr = (1U << LED_PIN) | (1U << SPEAKER_PIN);
        delay(100000);
    }
    
    /* Debug LED flash */
    sio->gpio_out_set = 1U << LED_PIN;
    delay(500000);
    sio->gpio_out_clr = 1U << LED_PIN;
    
    /* 1. CPU sleeps until interrupt occurs
       2. wfi = Wait For Interrupt instruction
       3. volatile prevents compiler optimization */
    while (1) {
        __asm volatile("wfi");  
    }
}
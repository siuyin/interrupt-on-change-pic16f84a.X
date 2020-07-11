/*
 * File:   main.c
 * Author: siuyin
 *
 * Created on 10 July, 2020, 10:01 AM
 */

// README: The pull-up resistor on MCLR is absolutely required.

// CONFIG
#pragma config FOSC = EXTRC     // Oscillator Selection bits (RC oscillator)
#pragma config WDTE = OFF       // Watchdog Timer (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (Power-up Timer is disabled)
#pragma config CP = OFF         // Code Protection bit (Code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define _XTAL_FREQ 740000   // 6.8k and 100pF resistor is 10% and capacitor is 20% tolerance
#define BUTTON RB4
#define UPPER_LED RB2
#define LOWER_LED RB3

// Global variables used by main and interrupt service routine.
volatile unsigned char button_pushed_task_ctr;
volatile unsigned char tick; // system timer tick is approx 11 ms.

// Local function declarations.
void setup_TMR0_for_interrupts(void);
void setup_portb_for_interrupt_on_change(void);
void setup_eeprom_write_interrupt(void);
void check_button_pushed_and_toggle_LEDs(void);

void main(void) {
    setup_TMR0_for_interrupts();
    setup_portb_for_interrupt_on_change();
    setup_eeprom_write_interrupt();
    ei();

    while (1) {
        check_button_pushed_and_toggle_LEDs();
    }
    return;
}



const unsigned char tmr0_reload_val = 248;

void setup_TMR0_for_interrupts(void) {
    tick = 0;

    // example:
    // Fosc = 1/(2RC)
    // Tcyc = 4/Fosc
    // TMR0_t = prescaler * Tcyc
    // If R=6.8k and C=100pF, and prescaler = 256, each TMR0_t = 1.39 ms
    PSA = 0; // 0: Assign prescaler to TMR0
    PS2 = 1;
    PS1 = 1;
    PS0 = 1;

    TMR0 = tmr0_reload_val; // setup for 256 - x counts before triggering interrupt.
    T0CS = 0; // 0: TMR0 counter source is internal clock

    T0IE = 1; // enable TMR0 interrupt
    LOWER_LED = 1;
}



void setup_portb_for_interrupt_on_change(void) {
    // Set port b to output, except for RB4 which is a button input.
    PORTB = 0;
    UPPER_LED = 1; // turn on upper LED
    TRISB = 0;
    TRISB4 = 1; // RB4 is an input

    state = released;
    RBIE = 1;
}



unsigned char button_pushed_times;

void setup_eeprom_write_interrupt() {
    button_pushed_times = eeprom_read(0);
    EEIE = 1; // eeprom interrupt enable
}

enum button_state_t {
    pushed, maybeReleased, released
};
volatile enum button_state_t state;

void check_button_pushed_and_toggle_LEDs(void) {
    static unsigned char lda; // lda: last done at
    const unsigned char t = 2;

    if (button_pushed_task_ctr != 0 || lda == tick) {
        return;
    }

    lda = tick;
    button_pushed_task_ctr = t;

    switch (state) {
        case released:
            if (BUTTON == 0) {
                state = pushed;
                UPPER_LED = ~UPPER_LED;
                eeprom_write(0, ++button_pushed_times); // data can be read by the pickit2 programmer with: ./pk2cmd -ppic16f84a -ge0-4 -r -t
                break;
            }
            state = released;
            break;
        case pushed:
            if (BUTTON == 0) {
                state = pushed;
                break;
            }
            state = maybeReleased;
            break;
        case maybeReleased:
            if (BUTTON == 0) {
                state = pushed;
                break;
            }
            state = released;
            RBIE = 1; // enable port b change interrupt
            break;
    }
}



// interrupt service routine

void __interrupt() interrupt_service_routine(void) {
    if (T0IE && T0IF) { // timer 0 interrupt enable and interrupt flag
        if (button_pushed_task_ctr > 0)button_pushed_task_ctr--;
        tick++;

        TMR0 = tmr0_reload_val; // reload TMR0 for next tick
        T0IF = 0; // clear TMR0 interrupt flag
    }

    if (RBIE && RBIF) { // port b interrupt enable and interrupt request flag.
        if (BUTTON == 0) {
            LOWER_LED = ~LOWER_LED;
            RBIE = 0; // disable port b change interrupt, will be re-enabled in main line code.
        }
        RBIF = 0; // The interrupt flag bit(s) must be cleared in software before re-enabling interrupts to avoid infinite interrupt requests.
    }

    if (EEIE && EEIF) { // eeprom write was completed
        EEIF = 0;
    }
}
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

#define BUTTON RB4
#define UPPER_LED RB2
#define LOWER_LED RB3
const unsigned char button_pushed_task_period_ticks = 2;
volatile unsigned char button_pushed_task_ctr;
volatile unsigned char tick; // system timer tick
const unsigned char tmr0_reload_val = 248;

enum button_state_t {
    pushed, maybeReleased, released
};
volatile enum button_state_t button_state;

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
        }
        RBIF = 0;
    }

}

#define _XTAL_FREQ 740000   // 6.8k and 100pF resitor is 10% and capacitor is 20% tolerance
void setup_TMR0_for_interrupts(void);
void setup_portb_for_interrupt_on_change(void);
void check_button_pushed_and_toggle_LEDs(void);
void wait_for_next_tick(unsigned char *);

void main(void) {
    setup_TMR0_for_interrupts();
    setup_portb_for_interrupt_on_change();
    ei();

    unsigned char current_tick = 0;
    while (1) {
        check_button_pushed_and_toggle_LEDs();
        wait_for_next_tick(&current_tick);
    }
    return;
}

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

    button_state = released;
    RBIE = 1;
}

void check_button_pushed_and_toggle_LEDs(void) {
    if (button_pushed_task_ctr != 0) {
        return;
    }

    switch (button_state) {
        case released:
            if (BUTTON == 0) {
                button_state = pushed;
                UPPER_LED = ~UPPER_LED;
                break;
            }
            button_state = released;
            break;
        case pushed:
            if (BUTTON == 0) {
                button_state = pushed;
                break;
            }
            button_state = maybeReleased;
            break;
        case maybeReleased:
            if (BUTTON == 0) {
                button_state = pushed;
                break;
            }
            button_state = released;
            break;
    }

    button_pushed_task_ctr = button_pushed_task_period_ticks;
}

void wait_for_next_tick(unsigned char * current_tick) {
    while (*current_tick == tick) {
    }
    *current_tick = tick;
}

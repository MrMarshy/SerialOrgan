#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_uart.h"

#include "newscale16.h"

#define SPEAKER_PIN     D5_PIN
#define NOTE_DURATION   0xF2FEU

static const uint8_t KEYS[] = {
    'a', 'w', 's', 'e', 'd', 'f', 't',
    'g', 'y', 'h', 'u', 'j', 'k', 'o',
    'l', 'p', ';', '\''
};

static const uint16_t NOTES[] = {
    C6, Cx6, D6, Dx6, E6, F6, Fx6,
    G6, Gx6, A6, Ax6, B6, C7, Cx7,
    D7, Dx7, E7, F7
};

static bool isNote = false;
static uint16_t currentNoteLength = NOTE_DURATION;

static void playNote(const uint16_t period, const uint16_t duration);

static void rest(uint16_t duration);

/**
 * Connect RX and TX pins of NRF Device to FTDI Breakout and use Serial Program to view
 * output and input commands to use
 */

#define UART_TX_BUFFER_SIZE      128
#define UART_RX_BUFFER_SIZE      128

#define UART_HWFC                APP_UART_FLOW_CONTROL_DISABLED

/** @brief Function for handling incoming characters from PC Serial Program.
 * 
 *  @param ch The received character
 */
static void uart_command_handler(uint8_t ch){

    for(size_t i = 0; i < sizeof(KEYS); ++i){

        if(ch == KEYS[i]){
            playNote(NOTES[i], currentNoteLength);
            isNote = true;
            break;
        }
    }

    if(!isNote){
        if(ch == '['){// Short Note
            // Switching to short note duration.
            currentNoteLength = NOTE_DURATION >> 1;
        }
        else if(ch == ']'){// Long Note
            // Switching to long note duration.
            currentNoteLength = NOTE_DURATION;
        }
        else{
            rest(currentNoteLength);
        }
    }
}

/** @brief Function for handling events on the uart.
 */
void uart_evt_handler(app_uart_evt_t * p_app_uart_event){

    (void)p_app_uart_event;
    uint8_t ch;
    
    switch(p_app_uart_event->evt_type){

        case APP_UART_COMMUNICATION_ERROR:
            bsp_board_leds_on();
            while(1);
            break;

        case APP_UART_DATA_READY:
            app_uart_get(&ch);
            printf("N - %c\r\n", ch); // Alert PC we are ready for next note.
            isNote = false;
            uart_command_handler(ch);
            break;

        default:;
    }
    
}



/**
 * @brief Function for main application entry.
 */
int main(void){

    uint32_t err_code;
    
    nrf_gpio_cfg_output(SPEAKER_PIN);
    nrf_gpio_pin_clear(SPEAKER_PIN);

    bsp_board_init(BSP_INIT_LEDS);

    const app_uart_comm_params_t com_params = {
        RX_PIN_NUMBER, // P0.24 on feather-express (see adafruit_nrf52840.h)
        TX_PIN_NUMBER, // P0.25 on feather-express (see adafruit_nrf52840.h)
        -1, // RTS_PIN_NUMBER, not used in this example
        -1, // CTS_PIN_NUMBER, not used in this example
        UART_HWFC,
        false, // no parity
        NRF_UART_BAUDRATE_115200
    };

    APP_UART_FIFO_INIT(&com_params, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 
                            uart_evt_handler, APP_IRQ_PRIORITY_LOWEST, err_code); 

    APP_ERROR_CHECK(err_code);

    printf("----- Serial Organ -----\r\n");
    printf("Read to play. Hit some keys...\r\n");

    for(;;){
        
        __WFI(); // enter low power mode

        
    }
}

static void playNote(const uint16_t period, const uint16_t duration){

    for(uint16_t elapsed = 0; elapsed < duration; elapsed += period){

        for(int i = 0; i < period; ++i){
            nrf_delay_us(1);
        }

        nrf_gpio_pin_toggle(SPEAKER_PIN);
    }
}

static void rest(uint16_t duration){

    do{
        nrf_delay_us(1);

    }while(--duration);
}

/** @} */

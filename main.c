#include <stdint.h>
#include <string.h>
#include <avr/io.h>			// for the input/output register
#include <avr/interrupt.h> 	
#include <util/delay.h>  	// for the _delay_ms
#include "RGB_setup.h"
#include "main.h"

// Variables globales

Task liste_attente[NB_TASK]; // [Task tache1, Task tache2]


uint8_t compteur;

void init_task_rgb(void){

    liste_attente[TASK_RGB_ID].adress = (uint16_t)rgb_main;
    liste_attente[TASK_RGB_ID].sp_save = 0x0600;
    liste_attente[TASK_RGB_ID].running = 0x00;

}

void send_serial(unsigned char c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}


void envoie_message(void){
    
    char message[] = "message\r\n";
    while(1){
        uint8_t i = 0;
        for(i=0; i<strlen(message); i++){
//      while(message[i]!='\0'){
            send_serial(message[i]);
  //        i++;
   //         if(i==255){
     //           break;
       //     }
        }
        _delay_ms(100);
    }
}

void init_serial(void)
{
	/* ACHTUNG : we suppose UBRR value < 0xff */
	/* Not true in all case */
	uint8_t baudrate = BAUDRATE;
	/* Set baud rate */
	UBRR0H = 0;
	UBRR0L = baudrate;

	/* Enable transmitter */
	UCSR0B = (1<<TXEN0);

	/* Set frame format */
	UCSR0C = 0x06;

    liste_attente[TASK_SERIAL_ID].adress = (uint16_t)envoie_message;
    liste_attente[TASK_SERIAL_ID].sp_save = 0x0700;
    liste_attente[TASK_SERIAL_ID].running = 0x00;
}


void task_led_red(void)
{
    while(1){
        PORTB ^= 0x01;
        _delay_ms(300);
    }
}


void init_task_led_red(void)
{
    DDRB |= 0x03;
    liste_attente[TASK_LED_ID].adress = (uint16_t)task_led_red;
    liste_attente[TASK_LED_ID].sp_save = 0x0500;
    liste_attente[TASK_LED_ID].running = 0x00;
}



void init_timer(void)
{
	TCCR1B |= _BV(WGM12); // CTC mode with value in OCR1A 
	TCCR1B |= _BV(CS12);  // CS12 = 1; CS11 = 0; CS10 =1 => CLK/1024 prescaler
	TCCR1B |= _BV(CS10);
	OCR1A   = NB_TICK;
	TIMSK1 |= _BV(OCIE1A);
}

void scheduler(void)
{
    compteur++;
    if(compteur > (NB_TASK-1))
    {
        compteur=0;
    }
}


ISR(TIMER1_COMPA_vect){

    SAVE_CONTEXT();
    
    if(liste_attente[compteur].running == 0x01)
    {
        liste_attente[compteur].sp_save = SP;
    }

    scheduler();
    
    
    if(liste_attente[compteur].running == 0x00){
        SP = liste_attente[compteur].sp_save;
        liste_attente[compteur].running = 0x01;
        asm volatile("push %0" : : "r" (liste_attente[compteur].adress & 0x00ff) );
        asm volatile("push %0" : : "r" ((liste_attente[compteur].adress & 0xff00)>>8) );
        asm("reti");
    }
    
    SP = liste_attente[compteur].sp_save;
    RESTORE_CONTEXT();
}

int main(void)
{
    init_timer();
    compteur = 0;
    init_task_led_red();
    PORTB ^= 0x02;
    _delay_ms(500);
    //PORTB ^= 0x02;
    init_serial();
    init_task_rgb();
    sei();
    while(1)
    {}
    return 0;
}


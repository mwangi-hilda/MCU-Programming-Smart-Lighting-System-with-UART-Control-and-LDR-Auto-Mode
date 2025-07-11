#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define LED_PORT PORTF
#define LED_DDR DDRF
#define LED1_TERMINAL PF1
#define LED2_TERMINAL PF2
#define LED3_TERMINAL PF3

#define LIGHT_SWITCHES_DDR DDRJ
#define LIGHT_SWITCHES_PORT PORTJ
#define LIGHT_SWITCHES_PIN PINJ
#define LIGHT1_SWITCH PJ0
#define LIGHT2_SWITCH PJ1
#define LIGHT3_SWITCH PJ2

#define LDR_TERMINAL 0

#define RX_BUFFER_SIZE 32

#define DEBOUNCE_TIME_MS 20

//light switches' global variables
volatile uint8_t light1_switch_counter = 0;
volatile uint8_t light2_switch_counter = 0;
volatile uint8_t light3_switch_counter = 0;

//ADC Global Variables
volatile uint16_t adc_results[16];
volatile uint8_t channel = 0;

//UART Global Variables
volatile char rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t new_message_flag = 0;

//Function Prototypes
void UART0_Initialization(uint32_t baud_rate);
void UART0_Transmit(char data);
void UART0_Send_String(const char *str);

//LED States Structure
typedef struct  
{
	uint8_t current_state;
	uint8_t mannual_override_active;
} led_control_t;

led_control_t led_states[3];

ISR(ADC_vect){
	adc_results[channel] = ADCL | (ADCH<<8);
	channel++;
	if (channel>=16) channel = 0;
	ADMUX = (1<<REFS0) | (channel & 0x0F);
	ADCSRA |= (1<<ADSC);
}

ISR(USART0_RX_vect){
	char received_char = UDR0;
	
	if (received_char == '\r' || received_char == '\n')
	{
		if (rx_index > 0)
		{
			rx_buffer[rx_index] = '\0';
			new_message_flag = 1;
			rx_index = 0;
		}
	} else if (rx_index < (RX_BUFFER_SIZE - 1))
	{
		rx_buffer[rx_index++] = received_char;
	}
}

int main(void)
{
	LED_DDR |= (1<<LED1_TERMINAL) | (1<<LED2_TERMINAL) | (1<<LED3_TERMINAL);
	LED_PORT &= ~((1<<LED1_TERMINAL) | (1<<LED2_TERMINAL) | (1<<LED3_TERMINAL));
	
	ADMUX = (1<<REFS0);
	ADCSRA = (1<<ADEN) | (1<<ADIE) | (7<<ADPS0);
	
	UART0_Initialization(9600);
	
	sei();
	
	ADCSRA |= (1<<ADSC);
	
	for (int i = 0; i < 3; i++)
	{
		led_states[i].current_state = 0;
		led_states[i].mannual_override_active = 0;
	}
	
	UART0_Send_String("MCU Ready!\r\n");

    while (1) 
    {
		float light_value = (adc_results[LDR_TERMINAL] / 1024.0) * 100.0;
		
		uint8_t low_light_intensity = (light_value < 30);
				
		if (new_message_flag)
		{
			new_message_flag = 0;
			
			//Converting the message to lower_case for easier comparison
			for (int i = 0; rx_buffer[i]; i++)
			{
				if (rx_buffer[i] >= 'A' && rx_buffer[i] <= 'Z') {
					rx_buffer[i] +=32;
				}
			}
			
			if (strstr((const char*)rx_buffer, "all lights on") != NULL)
			{
				for (int i = 0; i < 3; i++)
				{
					led_states[i].current_state = 1;
					led_states[i].mannual_override_active = 1;
				}
			} else if (strstr((const char*)rx_buffer, "all lights off") != NULL)
			{
				for (int i = 0; i < 3; i++)
				{
					led_states[i].current_state = 0;
					led_states[i].mannual_override_active = 1;
				}
			} else if (strstr((const char*)rx_buffer, "light 1 on") != NULL)
			{
				led_states[0].current_state = 1;
				led_states[0].mannual_override_active = 1;
			} else if (strstr((const char*)rx_buffer, "light 1 off") != NULL)
			{
				led_states[0].current_state = 0;
				led_states[0].mannual_override_active = 1;
			} else if (strstr((const char*)rx_buffer, "light 2 on") != NULL)
			{
				led_states[1].current_state = 1;
				led_states[1].mannual_override_active = 1;
			} else if (strstr((const char*)rx_buffer, "light 2 off") != NULL)
			{
				led_states[1].current_state = 0;
				led_states[1].mannual_override_active = 1;
			} else if (strstr((const char*)rx_buffer, "light 3 on") != NULL)
			{
				led_states[2].current_state = 1;
				led_states[2].mannual_override_active = 1;
			} else if (strstr((const char*)rx_buffer, "light 3 off") != NULL)
			{
				led_states[2].current_state = 0;
				led_states[2].mannual_override_active = 1;
			} else if (strstr((const char*)rx_buffer, "ldr on") != NULL)
			{
				for (int i = 0; i < 3; i++)
				{
					led_states[i].mannual_override_active = 0;
				}
			} else{
				UART0_Send_String("Unknown Command.\r\n");
			}
			
			memset((void*)rx_buffer, 0, RX_BUFFER_SIZE);

		}
		
		/*
		if (!(LIGHT_SWITCHES_PIN & (1<<LIGHT1_SWITCH)))
		{
			if (light1_switch_counter < DEBOUNCE_TIME_MS)
			{
				light1_switch_counter++;
			} else
			{
				led_states[0].current_state = 1;
				led_states[0].mannual_override_active = 1;
			}
		} else
		{
			led_states[0].current_state = 0;
			light1_switch_counter = 0;
		}
		
		if (!(LIGHT_SWITCHES_PIN & (1<<LIGHT2_SWITCH)))
		{
			if (light2_switch_counter < DEBOUNCE_TIME_MS)
			{
				light2_switch_counter++;
			} else
			{
				led_states[1].current_state = 1;
				led_states[1].mannual_override_active = 1;
			}
		} else
		{
			led_states[1].current_state = 0;
		}
		
		if (!(LIGHT_SWITCHES_PIN & (1<<LIGHT3_SWITCH)))
		{
			if (light3_switch_counter < 20)
			{
				light3_switch_counter++;
			} else
			{
				led_states[2].current_state = 1;
				led_states[2].mannual_override_active = 1;
			}
		} else
		{
			light3_switch_counter = 0;
			led_states[2].current_state = 0;
		}
		*/
		
		for (int i = 0; i < 3; i++)
		{
			if (!led_states[i].mannual_override_active)
			{
				if (low_light_intensity)
				{
					led_states[i].current_state = 1;
				} else
				{
					led_states[i].current_state = 0;
				}
			}
		}
		
		
		if (led_states[0].current_state)
		{
			LED_PORT |= (1<<LED1_TERMINAL);
		} else
		{
			LED_PORT &= ~(1<<LED1_TERMINAL);
		}
		
		if (led_states[1].current_state)
		{
			LED_PORT |= (1<<LED2_TERMINAL);
		} 
		else
		{
			LED_PORT &= ~(1<<LED2_TERMINAL);
		}
	
		if (led_states[2].current_state)
		{
			LED_PORT |= (1<<LED3_TERMINAL);
		}
		else
		{
			LED_PORT &= ~(1<<LED3_TERMINAL);
		}
    }
}

void UART0_Initialization(uint32_t baud_rate){
	uint16_t ubrr_value = (F_CPU / (8 * baud_rate)) - 1;
	UBRR0H = (ubrr_value>>8);
	UBRR0L = ubrr_value;
	
	UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	UCSR0B &= ~(1<<UCSZ02);
	
	UCSR0C &= ~((1<<UMSEL01) | (1<<UMSEL00) | (1<<UPM01) | (1<<UPM00) | (1<<USBS0));
	UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);
	
	UCSR0A |= (1<<U2X0);

}

void UART0_Transmit(char data){
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void UART0_Send_String(const char *str){
	while (*str){
		UART0_Transmit(*str++);
	}
}

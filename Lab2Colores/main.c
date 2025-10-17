#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

/*================ UART =================*/
#define BAUD 9600
#define UBRR_VALUE ((F_CPU/16/BAUD)-1)
static int uart_putchar(char c, FILE *s){ if(c=='\n') uart_putchar('\r',s); while(!(UCSR0A&(1<<UDRE0))); UDR0=c; return 0; }
FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar,NULL,_FDEV_SETUP_WRITE);
static inline void uart_init(void){
	UBRR0H=(uint8_t)(UBRR_VALUE>>8); UBRR0L=(uint8_t)UBRR_VALUE;
	UCSR0B=(1<<TXEN0); UCSR0C=(1<<UCSZ01)|(1<<UCSZ00); stdout=&uart_stdout;
}

/*================ ADC ==================*/
#define LDR_ADC_CH 0
static void adc_init(void){
	ADMUX  = (1<<REFS0) | (LDR_ADC_CH & 0x0F);               // AVcc
	ADCSRA = (1<<ADEN) | (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);   // presc 128
	_delay_ms(2);
}
static uint16_t adc_read(uint8_t ch){
	ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
	ADCSRA |= (1<<ADSC); while(ADCSRA & (1<<ADSC)); return ADC;
}
static uint16_t adc_avg(uint8_t ch, uint8_t n){
	uint32_t s=0; for(uint8_t i=0;i<n;i++) s+=adc_read(ch); return (uint16_t)(s/n);
}

/*================ SERVO (Timer1) =======*/
#define SERVO_OC1A_PIN PB1
#define SERVO_TIMER_TOP     40000   // 20 ms @ presc 8
#define SERVO_MIN_PULSE_TCK 1000    // 0.5 ms - mínimo del rango
#define SERVO_MAX_PULSE_TCK 5000    // 2.5 ms - máximo del rango

static void servo_init(void){
	DDRB |= (1<<SERVO_OC1A_PIN);
	TCCR1A=(1<<COM1A1)|(1<<WGM11);
	TCCR1B=(1<<WGM13)|(1<<WGM12)|(1<<CS11); // presc 8
	ICR1=SERVO_TIMER_TOP;
	OCR1A=SERVO_MIN_PULSE_TCK;
}

static void servo_angle(uint8_t a){
	if(a>180) a=180;
	uint16_t p = SERVO_MIN_PULSE_TCK +
	(uint32_t)(SERVO_MAX_PULSE_TCK-SERVO_MIN_PULSE_TCK)*a/180UL;
	OCR1A = p;
}

/*============ LÓGICA POR RANGOS =========*/
typedef enum { C_NONE=0, C_ROSA, C_ROJO, C_AMARILLO, C_VERDE } color_t;

typedef struct {
	const char *nombre;
	uint16_t low, high;   // rango [LOW..HIGH] ADC
	uint8_t  ang;         // ángulo del servo para ese color
	uint8_t  rgb[3];      // Color RGB para los LEDs
} rango_t;

// RANGOS MODIFICADOS - sin solapamientos
static rango_t R[] = {
	[C_ROSA]     = {"ROSA",     500, 540,   0, {255, 0, 80}},
	[C_ROJO]     = {"ROJO",     560, 590,  60, {255, 0, 0}},
	[C_AMARILLO] = {"AMARILLO", 600, 630, 120, {255, 255, 0}},
	[C_VERDE]    = {"VERDE",    640, 680, 180, {0, 255, 0}},
};

static inline color_t detectar(uint16_t v){
	for(color_t c=C_ROSA; c<=C_VERDE; c++)
	if(v>=R[c].low && v<=R[c].high) return c;
	return C_NONE;
}

static inline uint16_t sp_mid(color_t c){
	return (uint16_t)((R[c].low + R[c].high)/2);
}

// LEDS
#define LED 3
#define WIDTH 8
#define HEIGHT 8
#define NUM_LEDS (WIDTH * HEIGHT)

uint8_t leds[NUM_LEDS][3];

void sendBit(uint8_t bitVal);
void sendByte(uint8_t byte);
void show(uint8_t (*colors)[3]);
void setLedRGB(uint8_t (*leds)[3], int ledIndex, uint8_t r, uint8_t g, uint8_t b);
void fillAllLedsRGB(uint8_t r, uint8_t g, uint8_t b);

void sendBit(uint8_t bitVal){
	if(bitVal){
		PORTD |= (1 << LED);
		asm volatile (
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
		PORTD &= ~ (1 << LED);
		asm volatile(
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
		} else {
		PORTD |= (1 << LED);
		asm volatile(
		"nop\n\t""nop\n\t""nop\n\t"
		);
		PORTD &= ~(1 << LED);
		asm volatile(
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
	}
}

void sendByte(uint8_t byte){
	for(uint8_t i = 0; i < 8; i++){
		sendBit(byte & (1 << (7-i)));
	}
}

void show(uint8_t (*colors)[3]){
	cli();
	for (int i = 0; i < NUM_LEDS; i++){
		sendByte(colors[i][1]); // G
		sendByte(colors[i][0]); // R
		sendByte(colors[i][2]); // B
	}
	sei();
	_delay_us(60);
}

void setLedRGB(uint8_t (*leds)[3], int ledIndex, uint8_t r, uint8_t g, uint8_t b){
	if (ledIndex < 0 || ledIndex >= NUM_LEDS) return;
	leds[ledIndex][0]=r;
	leds[ledIndex][1]=g;
	leds[ledIndex][2]=b;
}

void fillAllLedsRGB(uint8_t r, uint8_t g, uint8_t b){
	for(int i = 0; i < NUM_LEDS; i++){
		setLedRGB(leds, i, r, g, b);
	}
}

// Función para aplicar el color actual a los LEDs
void aplicar_color_actual(color_t color_actual){
	switch(color_actual){
		case C_ROSA:
		fillAllLedsRGB(R[C_ROSA].rgb[0], R[C_ROSA].rgb[1], R[C_ROSA].rgb[2]);
		break;
		case C_ROJO:
		fillAllLedsRGB(R[C_ROJO].rgb[0], R[C_ROJO].rgb[1], R[C_ROJO].rgb[2]);
		break;
		case C_AMARILLO:
		fillAllLedsRGB(R[C_AMARILLO].rgb[0], R[C_AMARILLO].rgb[1], R[C_AMARILLO].rgb[2]);
		break;
		case C_VERDE:
		fillAllLedsRGB(R[C_VERDE].rgb[0], R[C_VERDE].rgb[1], R[C_VERDE].rgb[2]);
		break;
		default:
		// C_NONE - no hacer nada, mantener el color anterior
		break;
	}
	show(leds);
}

/*================ MAIN =================*/
int main(void){
	cli();
	uart_init();
	adc_init();
	servo_init();
	
	// INICIALIZACIÓN DEL LED
	DDRD |= (1 << LED);  // Configurar PD3 como salida
	PORTD &= ~(1 << LED); // Inicialmente apagado
	
	// Inicializar matriz de LEDs apagada
	for(int i = 0; i < NUM_LEDS; i++){
		setLedRGB(leds, i, 0, 0, 0);
	}
	show(leds);
	
	sei();

	color_t actual = C_NONE;
	uint8_t estab = 0, NEST = 3;   // 3 lecturas consecutivas para confirmar

	printf("Sistema iniciado - Esperando detección de colores...\n");

	while(1){
		uint16_t v = adc_avg(LDR_ADC_CH, 32);   // suavizado
		color_t c = detectar(v);

		if(c == actual){
			estab = 0;
		}
		else if(c != C_NONE){
			if(++estab >= NEST){
				actual = c;
				estab = 0;
				
				// SWITCH para controlar el servomotor según el color
				switch(actual){
					case C_ROSA:
					servo_angle(R[C_ROSA].ang);
					printf("Color ROSA detectado - Servo en 0°\n");
					break;
					case C_ROJO:
					servo_angle(R[C_ROJO].ang);
					printf("Color ROJO detectado - Servo en 60°\n");
					break;
					case C_AMARILLO:
					servo_angle(R[C_AMARILLO].ang);
					printf("Color AMARILLO detectado - Servo en 120°\n");
					break;
					case C_VERDE:
					servo_angle(R[C_VERDE].ang);
					printf("Color VERDE detectado - Servo en 180°\n");
					break;
					default:
					// No debería llegar aquí
					break;
				}
				
				// Aplicar el color a los LEDs
				aplicar_color_actual(actual);
				
				// Log adicional
				uint16_t sp = sp_mid(c);
				int16_t dif = (int16_t)sp - (int16_t)v;
				printf("LDR=%u | Color=%s | SP=%u | Dif=%d\n", v, R[c].nombre, sp, dif);
			}
		}
		else {
			// Cuando no se detecta color, mantener el color actual (último detectado)
			// No cambiamos 'actual', así que mantiene su último valor válido
			estab = 0;
			printf("Ningún color detectado - ADC: %u | Manteniendo color anterior\n", v);
		}

		_delay_ms(10);
	}
}
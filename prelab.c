/*
 * Archivo: prelab.c
 * Dispositivo: PIC16F887
 * Compilador:  XC8, MPLABX v5.40
 * Autor: José Fernando de León González
 * Programa: Comunicación SPI donde el maestro envía un valor producido por un pot al esclavo y este lo despliega en PORTD
 * Hardware: potenciómetro en PORTA (maestro), leds y resistencias en PORTD (esclavo)
 * 
 * Creado: 9/05/22
 * Última modificación: 9/05/22
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>



/*------------------------------------------------------------------------------
 * Constantes
------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000


/*------------------------------------------------------------------------------
 * Variables
------------------------------------------------------------------------------*/
uint8_t pot = 0;

/*------------------------------------------------------------------------------
 * Prototipos de funciones
------------------------------------------------------------------------------*/
void setup (void);


/*------------------------------------------------------------------------------
 * Interrupciones
------------------------------------------------------------------------------*/

void __interrupt() isr (void){     
    if (PIR1bits.ADIF){          // Interrupción del ADC   
         if (ADCON0bits.CHS == 0b0000){
             pot = ADRESH;
             PORTD = pot;
             PIR1bits.ADIF = 0;
         }    
    }
    if(PORTBbits.RB0 == 0)
    if (PIR1bits.SSPIF){
        PORTD = SSPBUF;
        PIR1bits.SSPIF = 0;
    }
    return;
}
/*------------------------------------------------------------------------------
 * Ciclo principal
------------------------------------------------------------------------------*/
void main(void) {
    setup();
    ADCON0bits.GO = 1;      // primera conversión del ADC
    while(1){
    
        if (PORTBbits.RB0){
            if (ADCON0bits.GO == 0) {
            __delay_us(1000);
            ADCON0bits.GO = 1;    
        }

            __delay_us(1000);
            if (SSPSTATbits.BF){
                SSPBUF = pot;
                 
            }
        }    
    }      
    return;
}

/*------------------------------------------------------------------------------
 * Configuración
------------------------------------------------------------------------------*/
void setup (void){
    
    // Configuración de los puertos
    ANSEL = 0b00000001;          //AN0 & como entrada analógica
    ANSELH = 0;
    
    TRISB = 0b1;                // RB0 como entrada (selector de master/esclavo)
    PORTB = 0;
    
    TRISD = 0;                  // PORTD como salida 
    PORTD = 0;                  // Limpiamos PORTD
    
    // Configuración del oscilador
    OSCCONbits.IRCF = 0b0100;    // IRCF <2:0> -> 100 1 MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    
    // CONFIGURACIONES DEL MASTER
    if (PORTBbits.RB0) {
        
        // Configuración de comunicación SPI
        TRISA = 0b00000001;         // PORTA como salida, RA0 como entrada
        PORTA = 0;                  // Limpiamos PORTA 
        
        TRISC = 0b00010000;          // SDI como entrada, SCK y SDO como salida
        PORTC = 0;

        SSPCONbits.SSPM = 0b0000;   //SPI Maestro, Reloj -> Fosc/4 (250 kbits/s)
        SSPCONbits.CKP = 0;         // Reloj inactivo en 0 (polaridad)
        SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI

        SSPSTATbits.CKE = 1;         // Envío de datos con cada flanco de subida
        SSPSTATbits.SMP = 1;         // Dato al final del pulso del reloj
        SSPBUF = pot; 

        //COnfiguración del ADC
        ADCON1bits.ADFM = 0;        // Justificado a la izquierda
        ADCON1bits.VCFG0 = 0;       // Referencia en VDD
        ADCON1bits.VCFG1 = 0;       // Referencia en VSS

        ADCON0bits.ADCS = 0b01;     // ADCS <1:0> -> 01 FOSC/8
        ADCON0bits.CHS = 0b0000;    // CHS  <3:0> -> 0000 AN0

        ADCON0bits.ADON = 1;        // Encender ADC
        __delay_us(50);

        //Configuración de las interrupciones
        INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
        INTCONbits.PEIE = 1;        // Habilitamos interrupciones de los puertos

        PIE1bits.ADIE = 1;          // Habilitamos interrupciones del ADC
        PIR1bits.ADIF = 0;          // Flag del ADC en 0
        
        
    }
    
    // CONFIGURACIONES DEL SLAVE
    else {
        // Configuración de comunicación SPI
        TRISA = 0b00100000;
        TRISC = 0b00011000;          // SDI y SCK como entradas, SDO como salida
        PORTC = 0;

        SSPCONbits.SSPM = 0b0100;   //SPI Esclavo, SS habilitado
        SSPCONbits.CKP = 0;         // Reloj inactivo en 0 (polaridad)
        SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI

        SSPSTATbits.CKE = 1;         // Envío de datos con cada flanco de subida
        SSPSTATbits.SMP = 0;         // Dato al final del pulso del reloj
        
        //Configuración de las interrupciones
        INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
        INTCONbits.PEIE = 1;        // Habilitamos interrupciones de los puertos

        PIE1bits.SSPIE = 1;          // Habilitamos interrupciones del SPI
        PIR1bits.SSPIF = 0;          // Limpiamos bandera del SPI    
    }
        
}
/*------------------------------------------------------------------------------
 * Funciones
------------------------------------------------------------------------------*/

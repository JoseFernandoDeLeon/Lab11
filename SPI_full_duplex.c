/* 
 * File:   SPI_full_duplex.c
 * Author: Christopher Chiroy
 * 
 * Comunicación SPI, desde maestro a esclavo y desde esclavo a maestro.
 * Maestro: Contador ascendente que se muestra en el PORTB e incrementa cada segundo
 * Esclavo: Contador descendente que se muestra en el PORTB ey decrementa cada segundo
 * 
 * Ambos dispositivos muestran el valor recibido en el PORTD
 * 
 * Conectar SS (RA5) del esclavo a RA7 del maestro
 *
 * Created on 7 mei 2022, 14:00
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
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000
#define FLAG_SPI 0xFF

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
char cont_master = 0;
char cont_slave = 0xFF;
char val_temporal = 0;
/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if (PIR1bits.SSPIF){
        /* Para que el esclavo pueda enviar datos es necesario generar los 
         * pulsos de reloj en el maestro y este los genera cuando hace un envío
         * de datos, entonces se activa la interrupción aunque los datos recibidos
         * sean solo de la solicitud del maestro para obtener los datos.
         * 
         * En este caso para solicitar los datos al esclavo se envía el valor 
         * 0xFF (FLAG_SPI), si ese es el valor recibido, lo ignoramos.
         */
        val_temporal = SSPBUF;
        if (val_temporal != FLAG_SPI){  // Es envío solo para generar los pulsos de reloj?
            PORTD = val_temporal;       // Mostramos valor recibido en PORTD
       
            SSPBUF = cont_slave;        // Cargamos contador del esclavo al buffer
            PORTB = cont_slave;         // Mostramos contador en PORTB
            cont_slave--;               // Decrementamos contador para próximo envío
        }
        
        PIR1bits.SSPIF = 0;             // Limpiamos bandera de interrupción
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){        
        // Envío y recepción de datos en maestro
        if (PORTAbits.RA0){         // ¿Es maestro?
            // Preparamos envío del contador del maestro al esclavo
            SSPBUF = cont_master;   // Cargamos valor del contador al buffer
            while(!SSPSTATbits.BF){}// Esperamos a que termine el envio
            PORTB = cont_master;    // Mostramos valor del contador del maestro en PORTB
            cont_master++;          // Incrementamos contador del maestro

            /* Ciertos dispositivos que usan comunicación SPI en modo esclavo
             * necesitan un cambio en la señal en el selector de esclavo (ss)
             * para generar una acción, el PIC es uno de ellos. Entonces para que
             * el uC pueda enviar datos al maestro, es necesario generar ese cambio
             * en el pin RA5 del esclavo
             */
            PORTAbits.RA7 = 1;      // Deshabilitamos el ss del esclavo
            __delay_ms(10);         // Esperamos un tiempo para que el PIC pueda detectar el cambio en el pin
            PORTAbits.RA7 = 0;      // habilitamos nuevamente el escalvo
            
            SSPBUF = FLAG_SPI;      // Se envía cualquier cosa, Esto es para que el maestro  
                                    //  genere los pulsos del reloj que necesita el esclavo
                                    //  para transmitir datos.
            
            while(!SSPSTATbits.BF){}// Esperamos a que se reciba un dato
            PORTD = SSPBUF;         // Mostramos dato recibido en PORTD
            
            __delay_ms(1000);       // Enviamos y pedimos datos cada 1 segundo
                   
        }
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    ANSEL = 0;
    ANSELH = 0;
    
    TRISB = 0;
    PORTB = 0;
    
    TRISD = 0;
    PORTD = 0;
    
    TRISA = 0b00100001;
    PORTA = 0;
    
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno
    
    // Configuración de SPI
    // Configs de Maestro
    if(PORTAbits.RA0){
        TRISC = 0b00010000;         // -> SDI entrada, SCK y SD0 como salida
        PORTC = 0;
    
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0000;   // -> SPI Maestro, Reloj -> Fosc/4 (250kbits/s)
        SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 1;        // -> Dato al final del pulso de reloj
        SSPBUF = cont_master;              // Enviamos un dato inicial
    }
    // Configs del esclavo
    else{
        TRISC = 0b00011000; // -> SDI y SCK entradas, SD0 como salida
        PORTC = 0;
        
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0100;   // -> SPI Esclavo, SS hablitado
        SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 0;        // -> Dato al final del pulso de reloj
        
        PIR1bits.SSPIF = 0;         // Limpiamos bandera de SPI
        PIE1bits.SSPIE = 1;         // Habilitamos int. de SPI
        INTCONbits.PEIE = 1;
        INTCONbits.GIE = 1;
    }
}

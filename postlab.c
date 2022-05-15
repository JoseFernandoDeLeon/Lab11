/*
 * Archivo: postlab.c
 * Dispositivo: PIC16F887
 * Compilador:  XC8, MPLABX v5.40
 * Autor: José Fernando de León González
 * Programa: Comunicación SPI donde el maestro y dos esclavos intercambian datos
 * Hardware: potenciómetro en PORTA & leds y resistencias en PORTD (maestro), Servo en PORTC (esclavo 1) botones en PORTB (esclavo 3) 
 * 
 * Creado: 14/05/22
 * Última modificación: 14/05/22
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
#define _XTAL_FREQ 8000000
#define FLAG_SPI 0xFF

#define AUMENTAR PORTBbits.RB0     // Asignamos un alias a RB0
#define DISMINUIR PORTBbits.RB1     // Asignamos un alias a RB1

/*------------------------------------------------------------------------------
 * Variables
------------------------------------------------------------------------------*/
char pot = 0;
char cont_slave = 0xFF;
char temp_val = 0;

char ADRESH_MS = 0;


/*------------------------------------------------------------------------------
 * Prototipos de funciones
------------------------------------------------------------------------------*/
void setup (void);


/*------------------------------------------------------------------------------
 * Interrupciones
------------------------------------------------------------------------------*/

void __interrupt() isr (void){     
    
    
    if (PIR1bits.ADIF){          // Interrupción del ADC   
        if (ADCON0bits.CHS == 0b0001){
            ADRESH_MS = ADRESH;
            PIR1bits.ADIF = 0;
        }    
    }
    
    if (PORTAbits.RA0 == 0){
        if (INTCONbits.RBIF)  
    {
        
        if (!AUMENTAR)
        {
            cont_slave = ++cont_slave;    
        }    
        if (!DISMINUIR)
        {
            cont_slave = --cont_slave;       
        }
        INTCONbits.RBIF = 0;    // Limpiamos bandera de interrupción RBIF
    }
        
        if (PIR1bits.SSPIF){
            /* Para que el esclavo pueda enviar datos es necesario generar los 
         * pulsos de reloj en el maestro y este los genera cuando hace un envío
         * de datos, entonces se activa la interrupción aunque los datos recibidos
         * sean solo de la solicitud del maestro para obtener los datos.
         * 
         * En este caso para solicitar los datos al esclavo se envía el valor 
         * 0xFF (FLAG_SPI), si ese es el valor recibido, lo ignoramos.
         */
        temp_val = SSPBUF;
        if (temp_val != FLAG_SPI){       // Es envío solo para generar los pulsos de reloj?
            CCPR1L = (temp_val>>1)+124;       
            CCP1CONbits.DC1B1 = temp_val & 0b01;
            
            SSPBUF = cont_slave;        // Cargamos contador del esclavo al buffer
            PORTD = cont_slave;         // Mostramos contador en PORTB
            //cont_slave--;               // Decrementamos contador para próximo envío
        }
        
        PIR1bits.SSPIF = 0;             // Limpiamos bandera de interrupción
        }
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
        
        // Envío y recepción de datos en maestro
        if (PORTAbits.RA0){
            
            if (ADCON0bits.GO == 0){
                __delay_us(1000);
                ADCON0bits.GO = 1;    
            }

            // Preparamos envío del contador del maestro al esclavo
            SSPBUF = ADRESH_MS;
            while(!SSPSTATbits.BF){}        // Esperamos a que termine el envio

            
            /* Ciertos dispositivos que usan comunicación SPI en modo esclavo
             * necesitan un cambio en la señal en el selector de esclavo (ss)
             * para generar una acción, el PIC es uno de ellos. Entonces para que
             * el uC pueda enviar datos al maestro, es necesario generar ese cambio
             * en el pin RA5 del esclavo
             */
            
            PORTAbits.RA6 = 1;      // Deshabilitamos el ss del esclavo
            __delay_ms(10);         // Esperamos un tiempo para que el PIC pueda detectar el cambio en el pin
            PORTAbits.RA6 = 0;      // habilitamos nuevamente el escalvo
            //SSPBUF = FLAG_SPI;      // Se envía cualquier cosa, Esto es para que el maestro  
                                    //  genere los pulsos del reloj que necesita el esclavo
                                    //  para transmitir datos.
            
            while(!SSPSTATbits.BF){}// Esperamos a que se reciba un dato
            //PORTD = SSPBUF;         // Mostramos dato recibido en PORTD
            //__delay_ms(1000);       // Enviamos y pedimos datos cada 1 segundo
            
            PORTAbits.RA7 = 1;      // Deshabilitamos el ss del esclavo
            __delay_ms(10);         // Esperamos un tiempo para que el PIC pueda detectar el cambio en el pin
            PORTAbits.RA7 = 0;      // habilitamos nuevamente el escalvo
            SSPBUF = FLAG_SPI;      // Se envía cualquier cosa, Esto es para que el maestro  
                                    //  genere los pulsos del reloj que necesita el esclavo
                                    //  para transmitir datos.
            
            while(!SSPSTATbits.BF){}// Esperamos a que se reciba un dato
            PORTD = SSPBUF;         // Mostramos dato recibido en PORTD
        }          
    }          
    return;
}

/*------------------------------------------------------------------------------
 * Configuración
------------------------------------------------------------------------------*/
void setup (void){
    
    // Configuración de los puertos
    ANSEL = 0b00000010;          //AN0 como entrada analógica
    ANSELH = 0;
    
    TRISB = 0b00000011;         // RB0 & RB1 de PORTB como entradas
    PORTB = 0;
    
    OPTION_REGbits.nRBPU = 0;   // Habilitamos resistencias de pull-up del PORTB
    WPUBbits.WPUB0 = 1;         // Habilitamos resistencia de pull-up de RB0 & RB1
    WPUBbits.WPUB1 = 1;
    
    INTCONbits.RBIE = 1;        // Habilitamos interrupciones del PORTB
    IOCBbits.IOCB0 = 1;         // Habilitamos IOCB en RB0
    IOCBbits.IOCB1 = 1;         // Habilitamos IOCB en RB1
    INTCONbits.RBIF = 0;        // Limpiamos bandera de interrupción de PORTB
    
    TRISD = 0;                  // PORTD como salida 
    PORTD = 0;                  // Limpiamos PORTD
    
    TRISA = 0b00100011;         // PORTA como salida, RA0 como selector master/slave; RA1 entrada potenciómetro, RA5 como receptor SS
    PORTA = 0;                  // Limpiamos PORTA 
    
    // Configuración del oscilador
    OSCCONbits.IRCF = 0b0111;    // IRCF <2:0> -> 111 8 MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    
    // CONFIGURACIONES DEL MASTER
    if (PORTAbits.RA0) {
        
        // Configuración de comunicación SPI
    
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

        ADCON0bits.ADCS = 0b10;     // ADCS <1:0> -> 10 FOSC/32
        ADCON0bits.CHS = 0b0001;    // CHS  <3:0> -> 0001 AN1

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
        TRISC = 0b00011000;          // SDI y SCK como entradas, SDO como salida
        PORTC = 0;

        SSPCONbits.SSPM = 0b0100;   //SPI Esclavo, SS habilitado
        SSPCONbits.CKP = 0;         // Reloj inactivo en 0 (polaridad)
        SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI

        SSPSTATbits.CKE = 1;         // Envío de datos con cada flanco de subida
        SSPSTATbits.SMP = 0;         // Dato al final del pulso del reloj
        
        // Configuración del PWM
        TRISCbits.TRISC2 = 1;       // RC2 -> CCP1 como entrada
        CCP1CONbits.P1M = 0;        // Salida simple
        CCP1CONbits.CCP1M = 0b1100; // asignación del modo a PWM


        CCPR1L = 0x0F;              // Valor inicial del duty cycle
        CCP1CONbits.DC1B = 0;       // CONFIG bits menos significativos


        // Configuración del TIMER2
        PR2 = 255;                  // Periodo del TIMER2
        T2CONbits.T2CKPS = 0b11;    // Prescaler 1:16
        T2CONbits.TMR2ON = 1;       // Encendemos TIMER2
        PIR1bits.TMR2IF = 0;        // Flag del TIMER2 en 0

        while (PIR1bits.TMR2IF == 0); // Esperamos una interrupción del TIMER2
        PIR1bits.TMR2IF = 0;

        TRISCbits.TRISC2 = 0;           // RC2 -> CCP2 como salida del PWM
        
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
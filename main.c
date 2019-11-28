//***************************************************************************************
//  Greenhouse_demo
//  Rev1
//
//  This code is designed for the greenhouse project.
//
//  Author: Andrea Martinez Chung and Priyanka Naik
//  November 22, 2019
//***************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>
#include "main.h"
#include "hal_LCD.h"
#include "Board.h"
#include "adc.h"
#include "gpio.h"
#include "eusci_a_uart.h"

int adc_display[4];
int degree;
char user_input = 0;

int motor_enable = 0;       //0 is on, 1 is off
int vent_threshold = 50;    //default value
int irr_threshold = 50;     //default value
int interrupt_tracker = 0;  //setting ventilation first, then irrigation
int current_temp;
int current_moisture;

void motor_driver_ventilation_setup(){
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);  //STBY
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN6);  //AI2
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0);  //AI1
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN7);  //PWMA
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);  //PWMB

    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);    //STBY
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);    //AI1
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN7);    //PWMA
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);    //AI2
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);    //PWMB
}

void motor_driver_irrigation_setup(){
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN3);  //STBY
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4);  //AI2
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5);  //AI1
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);  //PWMA
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN2);  //PWMB

    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN3);    //STBY
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);    //AI1
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);    //PWMA
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);    //AI2
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);    //PWMB
}

void button_setup(){
    P1REN = BIT2; // Enable internal pull-up/down resistors for P1.2
    P1OUT |= BIT2; //Select pull-up mode for P1.2

    P2REN = BIT6; // Enable internal pull-up/down resistors for P2.6
    P2OUT |= BIT6; //Select pull-up mode for P2.6
}

void multiplexer_setup(){
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN7);  //ctrl_S0
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0);  //ctrl_S1
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);  //ctrl_S2
}

void display(int sensor){                       //sensor == 0 is moisture sensor, sensor == 1 is temperature sensor
    unsigned int adc_value = ADCMEM0;           //base address of ADC registers is 0700h. ADCMEM0 = 12h
    int digit;                                  //read from ADC

    if (sensor == 0){
        current_moisture = adc_value;
        adc_value = (adc_value*100)/1023;      //(value/max)x100 = % moisture
    }
    else{
        current_temp = adc_value;
        adc_value = (adc_value*(3300/1023)-500)/10; //Temp in °C = [(Vout in mV) - 500] / 10
    }

    for (degree = 0; adc_value > 0; degree++) { //split value
        digit = adc_value % 10;
        adc_display[degree] = digit;
        adc_value /= 10;
    }

    showChar(adc_display[0] + '0', 10);         //display value
    if (degree >= 2)
        showChar(adc_display[1] + '0', 8);
    if (degree >= 3)
        showChar(adc_display[2] + '0', 6);
    if (degree == 4)
        showChar(adc_display[3] + '0', 4);

    if (sensor == 0)                          //display units depending on which sensor we're reading
       showChar('P', 2);                      //ascii of % is 37
    else
       showChar('C', 2);                      //ascii of C (for degrees Celsius) is 67
}

void ventilation_zone1(){
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);    //AI2
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);   //AI1
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN7);   //PWMA
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);   //STBY
        _delay_cycles(500000);                              //run the motor for a certain amount of time

        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);    //STBY
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);    //AI1
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN7);    //PWMA
}

void ventilation_zone2(){
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);    //AI2
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);   //AI1
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);   //PWMB
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);   //STBY
        _delay_cycles(500000);                              //run the motor for a certain amount of time

        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);    //STBY
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);    //AI1
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);    //PWMB
}

void irrigation_zone1(){
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);    //AI2
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);   //AI1
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);   //PWMA
        GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN3);   //STBY
        _delay_cycles(50000000);                            //run the motor for a certain amount of time

        GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN3);    //STBY
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);    //AI1
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);    //PWMA
}

void irrigation_zone2(){
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);    //AI2
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);   //AI1
        GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2);   //PWMB
        GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN3);   //STBY
        _delay_cycles(50000000);                            //run the motor for a certain amount of time

        GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN3);    //STBY
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);    //AI1
        GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);    //PWMB
}

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    Init_LCD();                             //setup functions
    Init_UART();                            //UART is a serial communication. Made up of TX-> Transmitter and RX-> Receiver. The MCU has a TX and RX and so does the computer
                                            //Computer TX goes to MCU RX and computer RX goes to MCU TX. To communicate they must be connected to the same serial port ("hallway")
                                            //In this case, it's COM6 (all ports will always be COM#). Serial ports must be opened. The computer one is opened when you run putty
                                            //and connect to COM#. MCU one is opened in this function.
    motor_driver_ventilation_setup();
    motor_driver_irrigation_setup();
    button_setup();
    multiplexer_setup();
    _enable_interrupt();                    //interrupt is triggered by an "event", an event is when something happens ex: button click
                                            //the event for this interrupt is when there is a char sent through UART (when something is received at RX of the computer -> see interrupt)

    int j;
    int zone_toggle = 0; //0 = zone1, 1 = zone2
    int sensor_toggle = 0; //0 = moisture sensor, 1 = temperature
    unsigned int ambient_light;

    for (j = 0; j < 4; j++){                //initialize to zero
        adc_display[j] = 0;
    }

    for(;;) {
        if ((P1IN & BIT2)!=BIT2) {
            zone_toggle = zone_toggle^1;     //toggling zones
        }

        if ((P2IN & BIT6)!=BIT6){
            sensor_toggle = sensor_toggle^1; //toggling sensors
        }

        if(((P2IN & BIT6)!=BIT6) && ((P1IN & BIT2)!=BIT2)){     //if button 2.6 and 1.2 are pushed
                    if (motor_enable == 0){                     //if motor_enable is activated - set by UART
                       if(zone_toggle == 0){//zone 1            //if zone one chosen
                        Init_ADC();                             //initialize ADC
                        SYSCFG2 |= ADCPCTL9;

                        ADCCTL0 = ADCCTL0 | ADCENC;                     //sampling and conversion start
                        ADCCTL0 = ADCCTL0 | ADCSC;

                        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);  //S0
                        GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);  //S1
                        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5); //S2

                        ambient_light = ADCMEM0;                //read value at ADC register and store it

                        if(ambient_light > 775) //current_temp > vent_threshold)
                            ventilation_zone1();
                        else if (ambient_light <= 775) //current_temp < irr_threshold)
                            ventilation_zone2();
                        }
                       else{
                          /* GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);  //S0
                           GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0);  //S1
                           GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);   //S2

                           unsigned int ambient_light = ADCMEM0;              //base address of ADC registers is 0700h. ADCMEM0 = 12h

                           if(ambient_light > 512 && current_temp > vent_threshold)
                               ventilation_zone2();
                           else if (ambient_light <=512 && current_moisture < irr_threshold)
                               irrigation_zone2();*/
                       }
                    }
        }
        else{   //if buttons 2.6 and 1.2 not pushed
            if (zone_toggle == 0) { //zone1
                if (sensor_toggle == 0){//moisture
                    Init_ADC();                                     //initialize ADC
                    SYSCFG2 |= ADCPCTL9;

                    ADCCTL0 = ADCCTL0 | ADCENC;                     //sampling and conversion start
                    ADCCTL0 = ADCCTL0 | ADCSC;

                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);  //S0
                    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);   //S1
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);  //S2

                    display(sensor_toggle);                         //display
                    _delay_cycles(500000);
                }
                else{//temperature sensor
                    Init_ADC();                                     //initialize ADC
                    SYSCFG2 |= ADCPCTL9;

                    ADCCTL0 = ADCCTL0 | ADCENC;                     //sampling and conversion start
                    ADCCTL0 = ADCCTL0 | ADCSC;

                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);  //S0
                    GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0);  //S1
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);  //S2

                    display(sensor_toggle);                         //display
                    _delay_cycles(500000);
                }
            }
            else{ //zone2
                if (sensor_toggle == 0){//moisture
                    Init_ADC();
                    SYSCFG2 |= ADCPCTL9;

                    ADCCTL0 = ADCCTL0 | ADCENC;                     //sampling and conversion start
                    ADCCTL0 = ADCCTL0 | ADCSC;

                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);  //S0
                    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);   //S1
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);   //S2

                    display(sensor_toggle);
                    _delay_cycles(500000);
                }
                else{ //temperature sensor
                    Init_ADC();
                    SYSCFG2 |= ADCPCTL9;

                    ADCCTL0 = ADCCTL0 | ADCENC;                     //sampling and conversion start
                    ADCCTL0 = ADCCTL0 | ADCSC;

                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);   //S0
                    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);   //S1
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);   //S2

                    display(sensor_toggle);
                    _delay_cycles(500000);
                }
            }
        }

        for (j = 0; j < degree; j++){
            adc_display[j] = 0;
        }

        clearLCD();
    }
}

#pragma vector=USCI_A0_VECTOR       //interrupt setup
__interrupt
void EUSCI_A0_ISR(void){
   uint8_t RxStatus = EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG); //setup for receiver, EUSCI_A0_BASE -> UART base register
   EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, RxStatus); //clear interrupt (must be clear every time the interrupt is called

   if(RxStatus) //RX -> Receiver, TX -> Transmitter (if char received)
       user_input = EUSCI_A_UART_receiveData(EUSCI_A0_BASE);    //read what's in EUSCI_A0_BASE and store

   if (user_input == '0')                                       //if value read is 0, toggle motors
       motor_enable = motor_enable^1;
   else if (interrupt_tracker == 0){
       vent_threshold = user_input*10;     //ventilation first (in degrees celsius)
       interrupt_tracker++;
   }
   else if (interrupt_tracker == 1){
       irr_threshold = user_input*10;      //irrigation second (in percent)
       interrupt_tracker = 0;
   }
}

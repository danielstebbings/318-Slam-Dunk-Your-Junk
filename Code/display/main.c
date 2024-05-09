#include <msp430.h>
#include <intrinsics.h>
#include <stdio.h>
#include <EA_DOG128.h>
#include <stdlib.h>



//motor livs
#include "sg90.h"
#include "mp6500_driver.h"
#include "driverlib/driverlib.h"

#define XTAL   4
#define delay_us(x) __delay_cycles ((unsigned long)(((unsigned long)x) * XTAL))
#define delay_ms(x) __delay_cycles ((unsigned long)(((unsigned long)x) * XTAL * 1000))
#define delay_s(x)  __delay_cycles ((unsigned long)(((unsigned long)x) * XTAL * 1000000))

#define LIGHT_THRES     600

unsigned char random_timer = 0;
unsigned long game_running_timer = 0;
unsigned int round_timer = 0;
unsigned char sys_state = 0;//0 is the game end status, showing the last score, 1 is the game in progress
unsigned char next_target = 0;//0 is metal, 1 is non-metal, 2 is non-recyclable
unsigned int current_score = 0;
unsigned char disp_buff[17] = " Score:0000T    ";
unsigned int ultrasonic_timer = 0;
unsigned char current_target = 0xFF;//0xFF means no object, 0 means metal, 1 means plastic, and 2 means non-plastic.
unsigned char target_flag = 0;//BIT0 indicates metal objects, BIT2 indicates whether it is transparent or not
unsigned int time_cnt = 0;
unsigned long detected_timer = 0;
unsigned int ad_val = 0;
unsigned int lowest_ad_val = 0xFFFF;


volatile char sg90_interrupt_flag = 0;
volatile char stepper_interrupt_flag = 0;

// TYPES
typedef enum test_state {
    open,
    wait_open,
    close,
    wait_close
} test_state;


//functions
void Disp_Refresh(void)
{
    unsigned long temp = 0;

    disp_buff[7] = current_score/1000+'0';
    disp_buff[8] = current_score%1000/100+'0';
    disp_buff[9] = current_score%100/10+'0';
    disp_buff[10] = current_score%10+'0';
    temp = (100000-game_running_timer)/1000;
    disp_buff[12] = temp/100+'0';
    disp_buff[13] = temp%100/10+'0';
    disp_buff[14] = temp%10+'0';
    EA_DOG128_Disp_String(0, 2, disp_buff);
    if(next_target == 0){
        EA_DOG128_Disp_String(63, 3, "Metal    ");
    }else if(next_target == 1){
        EA_DOG128_Disp_String(63, 3, "CPlastic");
    }else{
        EA_DOG128_Disp_String(63, 3, "OPlastic");
    }
}

//Timer Interrupt service routines

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer_A (void) {

        Timer_A_stop(TIMER_A1_BASE); // Stop counter
        Timer_A_stop(TIMER_A0_BASE); // Stop PWM

        int j = 0;
        for (j = 0; j< 10; j++) {
             __delay_cycles(65000);
             P4OUT ^= BIT0;
             __delay_cycles(65000);
             P1OUT ^= BIT0;
        };


        P4OUT &= ~BIT0; //turn off
        P1OUT &= ~BIT0; //turn off
        sg90_interrupt_flag = 1;

        Timer_A_clearTimerInterrupt(TIMER_A0_BASE);

        return;
    };

/* Initialize non-used ISR vectors with a trap function */

#pragma vector=PORT2_VECTOR
#pragma vector=PORT1_VECTOR
//#pragma vector=TIMER1_A1_VECTOR
//#pragma vector=TIMER1_A0_VECTOR
//#pragma vector=TIMER0_A1_VECTOR
//#pragma vector=TIMER0_A0_VECTOR
//#pragma vector=ADC10_VECTOR
//#pragma vector=USCIAB0TX_VECTOR
#pragma vector=WDT_VECTOR
//#pragma vector=USCIAB0RX_VECTOR
//#pragma vector=NMI_VECTOR
//#pragma vector=COMPARATORA_VECTOR

__interrupt void ISR_trap(void)
{
    /*
    int j = 0;
            for (j = 0; j< 10; j++) {
                 __delay_cycles(65000);
                 P4OUT ^= BIT0;
                 __delay_cycles(65000);
                 P4OUT ^= BIT0;
            };
            */
    return;
}

int _system_pre_init(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    return 1;
};

void main(void)
{

    //WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    P4SEL0 |= BIT1 | BIT2;                  // set XT1 pin as second function

    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);              // Test oscillator fault flag

    __bis_SR_register(SCG0);                // disable FLL
    CSCTL3 |= SELREF__XT1CLK;               // Set XT1CLK as FLL reference source
    CSCTL0 = 0;                             // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                 // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_2;                    // Set DCO = 4MHz
    CSCTL2 = FLLD_0 + 121;                  // DCODIV = 4MHz

    __delay_cycles(10);
    __bic_SR_register(SCG0);                // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked

    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;  // Set ACLK = XT1CLK = 32768Hz
                                            // DCOCLK = MCLK and SMCLK source
    CSCTL5 |= DIVM_0 | DIVS_0;              // MCLK = DCOCLK = 4MHZ,
                                            // SMCLK = MCLK/1 = 4MHz

    P4OUT &= ~BIT0;
    P4DIR |= BIT0;
    P1DIR &= ~BIT2;//按键S1，开始键
    P1REN |= BIT2;
    P1OUT |= BIT2;
    P2DIR &= ~BIT6;//按键S2，
    P2REN |= BIT6;
    P2OUT |= BIT6;
    P1OUT &= ~BIT7;
    P1DIR |= BIT7;//Trig
    P5DIR &= ~BIT0;//Echo
    P8DIR &= ~BIT0; //Inductive sensor input

    //DEBUG LED SETUP
    P1DIR |= 0x01;                          // Set P1.0 to output direction
    //P1OUT  = 0x00;                          // P1.0 off
    P4DIR |= 0x01;                          // Set P4.0 to output direction
    //P4OUT  = 0x00;                          // P4.0 off

    //interrupts
    __enable_interrupt();
    sg90_init_pins();

    // Configure ADC A1 pin
    SYSCFG2 |= ADCPCTL1;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    // Configure ADC10
    ADCCTL0 |= ADCSHT_2 | ADCON;                              // ADCON, S&H=16 ADC clks
    ADCCTL1 |= ADCSHP;                                        // ADCCLK = MODOSC; sampling timer
    ADCCTL2 |= ADCRES;                                        // 10-bit conversion results
    ADCIE |= ADCIE0;                                          // Enable ADC conv complete interrupt
    ADCMCTL0 |= ADCINCH_9 | ADCSREF_1;                        // A1 ADC input select; Vref=1.5V
    // Configure reference
    PMMCTL0_H = PMMPW_H;                                      // Unlock the PMM registers
    PMMCTL2 |= INTREFEN;                                      // Enable internal reference
    __delay_cycles(400);                                      // Delay for reference settling


    // motor test loop
    test_state state = open;
    test_state next_state;
    int finish = 0;
    int j = 0;
    // sg90_init_pins();
      while (1) {
          switch (state) {
                      case open:
                          //printf("open \n");
                          sg90_interrupt_flag = 0;
                          sg90_move(OPEN);
                          next_state = wait_open;
                          break;
                      case wait_open: //wait for interrupt

                          if (sg90_interrupt_flag ==1) {
                              //printf("interrupt fired \n");

                                          for (j = 0; j< 10; j++) {
                                               __delay_cycles(65000);
                                               P1OUT ^= BIT0;
                                               __delay_cycles(65000);
                                               P4OUT ^= BIT0;
                                          };
                              next_state = close;
                          } else {
                              //pass
                          };

                          break;
                      case close:
                          sg90_interrupt_flag = 0;
                          sg90_move(CLOSED);
                          next_state = wait_close;

                          break;
                      case wait_close:
                          if (sg90_interrupt_flag ==1) {
                                          next_state = open;
                                          finish++;
                                          for (j = 0; j< 10; j++) {
                                                                                         __delay_cycles(65000);
                                                                                         P1OUT ^= BIT0;
                                                                                         __delay_cycles(65000);
                                                                                         P4OUT ^= BIT0;
                                                                                    };
                                      } else {
                                          //pass
                                      };
                          break;
                      }; //end swiitch
                  state = next_state;



          if (finish == 2) { //button two to break
              break;
          }
      };

      EA_DOG128_Init();
      //EA_DOG128_Disp_String(0, 0, " Welcome to use ");
      //EA_DOG128_Disp_String(0, 1, "  Shoot Games   ");
        EA_DOG128_Disp_String(0, 0, "   SLAM DUNK!   ");
        EA_DOG128_Disp_String(0, 1, "   YOUR JUNK!   ");
        EA_DOG128_Disp_String(0, 2, " Score:0000     ");
        EA_DOG128_Disp_String(0, 3, "  S1->>Playing  ");
    while(1){
        delay_ms(1);
        random_timer++;

        if(sys_state){//The game is in progress
            game_running_timer++;
            round_timer++;
            if(game_running_timer > 100000){//The game ends in 100 seconds
                sys_state = 0;
                EA_DOG128_Disp_String(0, 3, "Congratulations!");
                delay_ms(3000);
                EA_DOG128_Disp_String(0, 3, "  S1->>Playing  ");
            }else{//The game is not over
                if(round_timer > 10000){//Randomly rotate targets every 10 seconds
                    round_timer = 0;
                    next_target = rand() % 3;
                    Disp_Refresh();
                }
                if((round_timer % 1000) == 0){
                    Disp_Refresh();
                }
            }
            //Ultrasonic ranging, measured once every 32ms, after detection, other sensors will start to detect
            if(game_running_timer > detected_timer){
                if((random_timer & 31) == 0){
                    ultrasonic_timer = 0;

                    //set trig high for 12 us
                    P1OUT |= BIT7;
                    delay_us(12);
                    P1OUT &= ~BIT7;

                    //small delay before the echo pin goes high
                    while((P5IN & BIT0) == 0); //wait until rising edge
                    while((P5IN & BIT0) != 0){ //rising edge, time until it goes low again
                        ultrasonic_timer++;
                        if(ultrasonic_timer > 60000)break; //stop forever loop
                    }
                    if(ultrasonic_timer < 70){//Adapt trigger point based on tube diameter.
                        //lowest_ad_val = 0xFFFF;

                        target_flag = 0;
                        do{
                            time_cnt++;
                            if((P8IN & BIT0) == 0){ //inductive sensor is active low -> metal
                                target_flag |= BIT0;
                            }
                            ADCCTL0 |= ADCENC | ADCSC;
                            __bis_SR_register(LPM0_bits | GIE);
                            if(ad_val < lowest_ad_val){ //update lowest ADC value?
                                lowest_ad_val = ad_val;
                            }
                        }while(time_cnt < 500);


                        if(target_flag & BIT0){
                            sg90_move(OPEN);
                            //Wdelay_ms(3000);
                            int j = 0;
                                    for (j = 0; j< 5; j++) {
                                         __delay_cycles(65000);
                                         P4OUT ^= BIT0;
                                         __delay_cycles(65000);
                                         P4OUT ^= BIT0;
                                    };

                                    sg90_move(CLOSED);
                                    //delay_ms(3000);
                            current_target = 0; //metal
                        }else{

                            if(lowest_ad_val > LIGHT_THRES){
                                sg90_move(OPEN);
                                //delay_ms(3000);
                                int j = 0;
                                                                    for (j = 0; j< 5; j++) {
                                                                         __delay_cycles(65000);
                                                                         P1OUT ^= BIT0;
                                                                         __delay_cycles(65000);
                                                                         P1OUT ^= BIT0;
                                                                    };
                                                                    sg90_move(CLOSED);
                                current_target = 1;//clear plastic
                            }else{
                                sg90_move(OPEN);
                                //delay_ms(3000);
                                int j = 0;
                                for (j = 0; j< 5; j++) {
                                                                         __delay_cycles(65000);
                                                                         P1OUT ^= BIT0;
                                                                         __delay_cycles(65000);
                                                                         P4OUT ^= BIT0;
                                                                    };
                                sg90_move(CLOSED);
                                //delay_ms(3000);
                                current_target = 2;//opaque plastic
                            }
                        }
                        if(current_target == next_target){
                            current_score += 3;
                            next_target = rand() % 3;
                            round_timer = 0;
                            Disp_Refresh();
                            //sg90_move(OPEN);
                            //delay_s(3);
                            //sg90_move(CLOSED);
                        }else{
                            current_score += current_target;
                        }
                        Disp_Refresh();
                        detected_timer = game_running_timer+200;
                        __no_operation();
                    }
                }
            }
        }

        if(((P1IN & BIT2) == 0)&&(sys_state == 0)){//Press the start button when the game is over
            delay_ms(10);
            if((P1IN & BIT2) == 0){//Start game
                current_target = 0xFF;
                detected_timer = 0;
                target_flag = 0;
                sys_state = 1;
                srand(random_timer);
                next_target = rand() & 1;
                game_running_timer = 0;
                round_timer = 0;
                current_score = 0;
                EA_DOG128_Disp_String(0, 2, " Score:     100 ");
                EA_DOG128_Disp_String(0, 3, " Target:         ");
                Disp_Refresh();
                while(1){
                    if((P1IN & BIT2) != 0){
                        delay_ms(10);
                        if((P1IN & BIT2) != 0)break;
                    }
                }
            }
        }//The start button is useless at other times

    }
}

// ADC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:
            ad_val = ADCMEM0;
            __bic_SR_register_on_exit(LPM0_bits);              // Clear CPUOFF bit from LPM0
            break;
        default:
            break;
    }
}



/*

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer_A1 (void) {

        Timer_A_stop(TIMER_A1_BASE); // Stop counter
        Timer_A_stop(TIMER_A0_BASE); // Stop PWM

        int j = 0;
        for (j = 0; j< 10; j++) {
             __delay_cycles(65000);
             P4OUT ^= BIT0;
             __delay_cycles(65000);
             P1OUT ^= BIT0;
        };

        P4OUT &= !BIT0; //turn off
        P1OUT &= !BIT0; //turn off
        stepper_interrupt_flag = 1;

        Timer_A_clearTimerInterrupt(TIMER_A1_BASE);

        return;
    };
*/

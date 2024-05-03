#include <msp430.h>
#include <intrinsics.h>
#include <stdio.h>
#include <EA_DOG128.h>
#include <stdlib.h>

#define XTAL   4
#define delay_us(x) __delay_cycles ((unsigned long)(((unsigned long)x) * XTAL))
#define delay_ms(x) __delay_cycles ((unsigned long)(((unsigned long)x) * XTAL * 1000))
#define delay_s(x)  __delay_cycles ((unsigned long)(((unsigned long)x) * XTAL * 1000000))

#define LIGHT_THRES     500

unsigned char random_timer = 0;
unsigned long game_running_timer = 0;
unsigned int round_timer = 0;
unsigned char sys_state = 0;//0为游戏结束状态，显示上一次的分数，1为游戏进行时
unsigned char next_target = 0;//0为金属，1为非金属，2为不可回收
unsigned int current_score = 0;
unsigned char disp_buff[17] = "Scores:0000     ";
unsigned int ultrasonic_timer = 0;
unsigned char current_target = 0xFF;//0xFF为无物体，0为金属，1为塑料，2为非塑料
unsigned char target_flag = 0;//BIT0表示金属物体，BIT2表示是否透光
unsigned int time_cnt = 0;
unsigned long detected_timer = 0;
unsigned int ad_val = 0;
unsigned int lowest_ad_val = 0xFFFF;

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
        EA_DOG128_Disp_String(56, 3, "Metal    ");
    }else if(next_target == 1){
        EA_DOG128_Disp_String(56, 3, "Plastic  ");
    }else{
        EA_DOG128_Disp_String(56, 3, "N-Plastic");
    }
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

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
    P1DIR &= ~BIT6;//Echo
    P8DIR &= ~BIT0;

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

    EA_DOG128_Init();
    EA_DOG128_Disp_String(0, 0, " Welcome to use ");
    EA_DOG128_Disp_String(0, 1, "  Shoot Games！  ");
    EA_DOG128_Disp_String(0, 2, "Scores:0000     ");
    EA_DOG128_Disp_String(0, 3, "  S1->>Playing  ");

    while(1){
        delay_ms(1);
        random_timer++;

        if(sys_state){//游戏进行中
            game_running_timer++;
            round_timer++;
            if(game_running_timer > 100000){//100秒游戏结束
                sys_state = 0;
                EA_DOG128_Disp_String(0, 3, "Congratulations!");
                delay_ms(3000);
                EA_DOG128_Disp_String(0, 3, "  S1->>Playing  ");
            }else{//游戏未结束
                if(round_timer > 10000){//10秒随机轮换一次目标
                    round_timer = 0;
                    next_target = rand() % 3;
                    Disp_Refresh();
                }
                if((round_timer % 1000) == 0){
                    Disp_Refresh();
                }
            }
            //超声测距，32ms测一次，检测到以后开始其它传感器的检测
            if(game_running_timer > detected_timer){
                if((random_timer & 31) == 0){
                    ultrasonic_timer = 0;
                    P1OUT |= BIT7;
                    delay_us(12);
                    P1OUT &= ~BIT7;
                    while((P1IN & BIT6) == 0);
                    while((P1IN & BIT6) != 0){
                        ultrasonic_timer++;
                        if(ultrasonic_timer > 60000)break;
                    }
                    if(ultrasonic_timer < 140){//根据实际情况改动这个值
                        lowest_ad_val = 0xFFFF;
                        target_flag = 0;
                        do{
                            time_cnt++;
                            if((P8IN & BIT0) == 0){
                                target_flag |= BIT0;
                            }
                            ADCCTL0 |= ADCENC | ADCSC;
                            __bis_SR_register(LPM0_bits | GIE);
                            if(ad_val < lowest_ad_val){
                                lowest_ad_val = ad_val;
                            }
                        }while(time_cnt < 500);
                        if(target_flag & BIT0){
                            current_target = 0;
                        }else{
                            if(lowest_ad_val > LIGHT_THRES){
                                current_target = 1;
                            }else{
                                current_target = 2;
                            }
                        }
                        if(current_target == next_target){
                            current_score += 3;
                        }else{
                            current_score += 1;
                        }
                        Disp_Refresh();
                        detected_timer = game_running_timer+200;
                        __no_operation();
                    }
                }
            }
        }

        if(((P1IN & BIT2) == 0)&&(sys_state == 0)){//结束游戏状态下开始键按下
            delay_ms(10);
            if((P1IN & BIT2) == 0){//开始游戏
                current_target = 0xFF;
                detected_timer = 0;
                target_flag = 0;
                sys_state = 1;
                srand(random_timer);
                next_target = rand() & 1;
                game_running_timer = 0;
                round_timer = 0;
                current_score = 0;
                EA_DOG128_Disp_String(0, 2, "Scores:     100 ");
                EA_DOG128_Disp_String(0, 3, "Target:         ");
                Disp_Refresh();
                while(1){
                    if((P1IN & BIT2) != 0){
                        delay_ms(10);
                        if((P1IN & BIT2) != 0)break;
                    }
                }
            }
        }//其它时候开始键无用

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

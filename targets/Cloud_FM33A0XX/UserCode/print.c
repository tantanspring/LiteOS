#include "define_all.h"  
#include "print.h"



void Init_PrintUartx(void)
{	
#if( __DEBUG_PRINT__ != 0 )
	Uartx_Init(PRINT_UART);//��ʼ��uart����
	
	UARTx_RXSTA_RXEN_Setable(PRINT_UART, ENABLE);		//�򿪽���ʹ��
	UARTx_TXSTA_TXEN_Setable(PRINT_UART, ENABLE);		//�򿪷���ʹ��	
#endif
}


void Init_lptimer_for_runtime_stats(void)
{
	LPTIM_InitTypeDef init_para;

	//ʹ��LPTIMER������ʱ��
	RCC_PERCLK_SetableEx( LPTFCLK, 		ENABLE );		//LPTIM����ʱ��ʹ��
	RCC_PERCLK_SetableEx( LPTRCLK, 		ENABLE );		//LPTIM����ʱ��ʹ��

	
	init_para.LPTIM_TMODE = LPTIM_LPTCFG_TMODE_PWMIM;//���ù���ģʽΪ�������������ͨ��ʱ��ģʽ
	init_para.LPTIM_MODE = LPTIM_LPTCFG_MODE_CONTINUE;//���ü���ģʽΪ��������ģʽ

	init_para.LPTIM_PWM = LPTIM_LPTCFG_PWM_PWM;//ѡ�����������Ʒ�ʽ
    init_para.LPTIM_POLAR = LPTIM_LPTCFG_POLARITY_POS;//��һ�μ���ֵ=�Ƚ�ֵ�ǲ���������

	init_para.LPTIM_TRIG_CFG = LPTIM_LPTCFG_TRIGCFG_POS;//�ⲿ�����ź�������trigger
	init_para.LPTIM_FLTEN = ENABLE;//ʹ�������˲�
	
	init_para.LPTIM_LPTIN_EDGE = LPTIM_LPTCFG_EDGESEL_POS;
	
	init_para.LPTIM_CLK_SEL = LPTIM_LPTCFG_CLKSEL_LSCLK;//ѡ��LPTIMER��ʱ��Դ
	init_para.LPTIM_CLK_DIV = LPTIM_LPTCFG_DIVSEL_64;//���÷�Ƶֵ,512Hz,ֻ����ȷ����120��ͳ��

	init_para.LPTIM_compare_value = 0;//���ñȽ�ֵ
    init_para.LPTIM_target_value = 0xffff;//����Ŀ��ֵ

	LPTIM_Init(&init_para);//��ʼ��LPTIMER

    //ʹ��LPTIMER�������ж�
	NVIC_DisableIRQ(LPTIM_IRQn);
//	NVIC_SetPriority(LPTIM_IRQn, 2);
//	NVIC_EnableIRQ(LPTIM_IRQn);		

	LPTIM_LPTCTRL_LPTEN_Setable(ENABLE);//LPTIMERģ��ʹ��
}

#if 1 //suzhen
#pragma import(__use_no_semihosting)                        
struct __FILE 
{ 
	int handle; 
}; 
FILE __stdout;     
void _sys_exit(int x) 
{ 
	x = x; 
} 
#endif 


// �ض���c�⺯��printf
int fputc(int ch, FILE *f)
{
	UARTx_TXREG_Write(PRINT_UART, ch);		//����������д�뷢�ͼĴ���

	while(SET == UARTx_TXBUFSTA_TXFF_Chk(PRINT_UART));	//�ȴ��������		

	return (ch);
}

// �ض���c�⺯��scanf
int fgetc(FILE *f)
{
	while (RESET == UARTx_RXBUFSTA_RXFF_Chk(PRINT_UART));

	return (int)UARTx_RXREG_Read(PRINT_UART);
}



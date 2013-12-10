#include<stm32f4xx_gpio.h>
#include<stm32f4xx_rcc.h>
#include<misc.h>
#include<stm32f4xx_spi.h>
#include<stm32f4xx_it.h>
#include<nrf24l01.h>
#include<stdio.h>
//Definicje
#define SET_CSN GPIO_SetBits(GPIOB, GPIO_Pin_11)
#define CLEAR_CSN GPIO_ResetBits(GPIOB, GPIO_Pin_11)
#define SET_CE GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define CLEAR_CE GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define W 1
#define R 0
char h=0;

//Zmienne
uint8_t Tx_buff[5];
uint8_t X_acc;
uint8_t Y_acc;
uint8_t Z_acc;
static __IO uint32_t TimingDelay;

//Funkcje

//Funkcje odpowiadajace za opoznienie
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  {
	TimingDelay--;
  }
}
void Delay_ms(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);

}
//End

//Obsluga diody led

void LED_init(void)
	{
	  GPIO_InitTypeDef  GPIO_InitStructure;
	  /* GPIOD Periph clock enable */
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	  /* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(GPIOD, &GPIO_InitStructure);

	}

//End

//Funkcje odpowiadajace za obsluge SPI2 i Transrecivera
void init_SPI2(void)
	{

	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStructure;

	// enable clock for used IO pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* configure pins used by SPI1
	 * PB11 = CSN
	 * PB12 = CE
	 * PB13 = SCK
	 * PB14 = MISO
	 * PB15 = MOSI
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// connect SPI2 pins to SPI alternate function
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);


	/* Configure the chip select pin
	   in this case we will use PB11 and chip enable we will use PB12 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	SET_CSN;// Set CSN
	CLEAR_CE;//CLear CE


	// enable peripheral clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	/* configure SPI1 in Mode 0
	 * CPOL = 0 --> clock is low when idle
	 * CPHA = 0 --> data is sampled at the first edge
	 */
    SPI_I2S_DeInit(SPI2);
      SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
      SPI_InitStructure.SPI_Mode = SPI_Mode_Master ;
      SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b ;
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low    ;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      SPI_InitStructure.SPI_NSS = SPI_NSS_Soft ;
      SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
      SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
      SPI_InitStructure.SPI_CRCPolynomial = 0;
      SPI_Init(SPI2, &SPI_InitStructure);
      SPI_Cmd(SPI2, ENABLE);


	}

uint8_t WriteByteSPI(uint8_t cData)
	{

	while(!SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE));  //transmit buffer empty?
	SPI_I2S_SendData(SPI2, cData); //Send data
	while(!SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE)); //data received?
	return SPI_I2S_ReceiveData(SPI2);//return received data

	}
uint8_t GetReg(uint8_t reg)
	{
	Delay_ms(1);//delay
	CLEAR_CSN;//CSN goes to low state
	Delay_ms(1);//delay
	WriteByteSPI(R_REGISTER+reg);//send address of register in read mode
	Delay_ms(1);//delay
	reg=WriteByteSPI(NOP);//send dummy byte to generate clock
	SET_CSN;//CSN goes high
	return reg;//return the value of register
	}
/*void WriteToNrf(uint8_t reg,uint8_t Package)
	{
	Delay_ms(1);
	CLEAR_CSN;
	Delay_ms(1);
	WriteByteSPI(W_REGISTER+reg);
	Delay_ms(1);
	WriteByteSPI(Package);
	Delay_ms(1);
	SET_CSN;
	}*/
uint8_t *WriteToNrf(uint8_t ReadWrite ,uint8_t reg, uint8_t *val, uint8_t antVal)
	{
	if(ReadWrite == W)
	{
		reg = W_REGISTER + reg; // if we want to write data we need to add W_REGISTER

	}
	static uint8_t ret[32];
	Delay_ms(1);
	CLEAR_CSN;
	Delay_ms(1);
	WriteByteSPI(reg);
	Delay_ms(1);

	int i;
	for(i = 0 ; i < antVal ; i++ )
		{
		if(ReadWrite == R && reg != W_TX_PAYLOAD)
		{
		ret[i]=WriteByteSPI(NOP);
		Delay_ms(1);
		}
		else
		{
			WriteByteSPI(val[i]);
			Delay_ms(1);
		}
		}
	SET_CSN;
	return ret;
	}

void nrf24L01_init(void)
	{
	Delay_ms(100);
	uint8_t val[5];
	//EN_AA enable auto acknowlegments ON
	val[0]=0x01;
	WriteToNrf(W,EN_AA,val,1);
	//
	val[0]=0x2f;
	WriteToNrf(W,SETUP_RETR,val,1);
	//Number of pipes to enable
	val[0]=0x01;
	WriteToNrf(W,EN_RXADDR,val,1);//only data pipe 0 is enable
	//RF_address width setup(of how many bytes address consists
	val[0]=0x03;
	WriteToNrf(W,SETUP_AW,val,1);// 5 bytes address
	//RF channel
	val[0]=0x01;
	WriteToNrf(W,RF_CH,val,1);//2.401 GHz frequency
	//RF_Setup
	val[0]=0x27;
	WriteToNrf(W,RF_SETUP,val,1);
	//Setting Receiver Address
	int i;
	for(i = 0 ; i < 5 ; i++ )
	{
		val[i]=0x12;
	}
	WriteToNrf(W,RX_ADDR_P0,val,5);
	//Setting Transmiter Address
	for(i = 0 ; i < 5 ; i++ )
	{
		val[i]=0x12;
	}
	WriteToNrf(W,TX_ADDR,val,5);
	//Payload width setup
	val[0]=5;
	WriteToNrf(W,RX_PW_P0,val,1);
	//Config reg setup
	val[0]=0x1E;
	WriteToNrf(W,CONFIG,val,1);
	Delay_ms(100);
	}
void transmit_payload(uint8_t * W_buff)
	{
	WriteToNrf(R,FLUSH_TX,W_buff,0);
	WriteToNrf(R,W_TX_PAYLOAD,W_buff,5);
	Delay_ms(10);
	SET_CE;
	Delay_ms(1);
	CLEAR_CE;
	Delay_ms(10);
	}
void receive_payload(void)
	{
	SET_CE;
	Delay_ms(1000);
	CLEAR_CE;
	}
void reset(void)
	{
	Delay_ms(1);
	CLEAR_CSN;
	Delay_ms(1);
	WriteByteSPI(W_REGISTER + STATUS);
	Delay_ms(1);
	WriteByteSPI(0x70);
	Delay_ms(1);
	SET_CSN;


	}

//End

//Funckje odpowiedzialne za obs³uge akcelerometru

void init_SPI1(void){

	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStructure;

	// enable clock for used IO pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* configure pins used by SPI1
	 * PA5 = SCK
	 * PA6 = MISO
	 * PA7 = MOSI
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// connect SPI1 pins to SPI alternate function
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

	// enable clock for used IO pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	/* Configure the chip select pin
	   in this case we will use PE7 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStruct);
	GPIO_SetBits(GPIOE,GPIO_Pin_3);


	// enable peripheral clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	/* configure SPI1 in Mode 0
	 * CPOL = 0 --> clock is low when idle
	 * CPHA = 0 --> data is sampled at the first edge
	 */
    SPI_I2S_DeInit(SPI1);
      SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
      SPI_InitStructure.SPI_Mode = SPI_Mode_Master ;
      SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b ;
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_High    ;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
      SPI_InitStructure.SPI_NSS = SPI_NSS_Soft ;
      SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
      SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
      SPI_InitStructure.SPI_CRCPolynomial = 0;
      SPI_Init(SPI1, &SPI_InitStructure);
      SPI_Cmd(SPI1, ENABLE);


}

/* This funtion is used to transmit and receive data
 * with SPI1
 * 			data --> data to be transmitted
 * 			returns received value
 */
uint8_t mySPI_GetData(uint8_t adress){

	GPIO_ResetBits(GPIOE, GPIO_Pin_3);

	adress = 0x80 | adress;

	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));  //transmit buffer empty?
	SPI_I2S_SendData(SPI1, adress);
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE)); //data received?
	SPI_I2S_ReceiveData(SPI1);	//Clear RXNE bit

	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));  //transmit buffer empty?
	SPI_I2S_SendData(SPI1, 0x00);	//Dummy byte to generate clock
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE)); //data received?
	GPIO_SetBits(GPIOE, GPIO_Pin_3);

	return SPI_I2S_ReceiveData(SPI1); //return reveiced data
}

void mySPI_SendData(uint8_t adress, uint8_t data){

	GPIO_ResetBits(GPIOE, GPIO_Pin_3);

	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));  //transmit buffer empty?
	SPI_I2S_SendData(SPI1, adress);
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE)); //data received?
	SPI_I2S_ReceiveData(SPI1);

	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));  //transmit buffer empty?
	SPI_I2S_SendData(SPI1, data);
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE)); //data received?
	SPI_I2S_ReceiveData(SPI1);

	GPIO_SetBits(GPIOE, GPIO_Pin_3);
}

//End

//END

int main(void)
{
	SysTick_Config(24000);
	LED_init();
	init_SPI1();
	mySPI_SendData(0x20,0xC7);
	init_SPI2();
	GPIO_SetBits(GPIOD,GPIO_Pin_12);
	Delay_ms(2000);
	GPIO_ResetBits(GPIOD,GPIO_Pin_12);
	if(GetReg(STATUS) == 0x0E)
	{
		GPIO_ToggleBits(GPIOD,GPIO_Pin_13);
		Delay_ms(1000);
		GPIO_ToggleBits(GPIOD,GPIO_Pin_13);
		Delay_ms(1000);
	}
	nrf24L01_init();



    while(1)
    {
    	X_acc=mySPI_GetData(0x29);
    	Y_acc=mySPI_GetData(0x2b);
    	Z_acc=mySPI_GetData(0x2d);
    	Tx_buff[0]=X_acc;
    	Tx_buff[1]=Y_acc;
    	Tx_buff[2]=Z_acc;
    	Tx_buff[3]=1;
    	Tx_buff[4]=1;

    	//Delay_ms(100);
    	//while(( GetReg(STATUS) & (1<<TX_DS)) != 0 )
    	//{
    	//GPIO_SetBits(GPIOD,GPIO_Pin_12);
    	transmit_payload(Tx_buff);
    	//GPIO_ResetBits(GPIOD,GPIO_Pin_12);
    	//}
    	Delay_ms(50);
    	reset();

    }
}

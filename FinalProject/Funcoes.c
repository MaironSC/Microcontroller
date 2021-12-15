#include "PID.h"
/*
float pid(int error_ctrl, int prev_error_ctrl, int kp, int ki, int kd)
{
 double p = 0, i = 0, d = 0;
 float sum = 0;
 p = kp * error_ctrl;
 i = ki * (error_ctrl + prev_error_ctrl); 
 d = kd * (error_ctrl - prev_error_ctrl);
 sum = p + i +d; // PID
return sum;
} 
*/
void lcd_init()
{ // Inicia LCD
	unsigned char config[] = {0x38,0x0c,0x01,0x80};
	unsigned int i;
	for (i=0; i<4;i++){lcd_comm(config[i]);}
	lcd_data("Sistema iniciado");
}

void lcd_comm (char dado) 
{ // envia flags de controle para o LCD
	lcd_bus = dado;
	delay_ms(10);
	RS = 0;
	delay_ms(10);
	EN = 1;
	delay_ms(10);
	EN = 0;
	delay_ms(10);
	return;
}

void lcd_data (char dado[])
{ // envia o dado para o barramento do lcd
	int i;
	int len;
	len = strlen(dado);
	for (i=0;i<len;i++){
		lcd_bus = dado[i];
		delay_ms(5);
		RS = 1;
		delay_ms(5);
		EN = 1;
		delay_ms(5);
		EN = 0;
	}
}

void delay_ms (unsigned ms)
{ 

	unsigned int i,j;
	for (i=0;i<ms;i++){	   
		for (j=0;j<65;j++){_nop_();}	
		}
  }
void lcd_clear()
{
	lcd_comm(0x01);
}

void exibe (int number) // Recebe número e decresce de 155
{
  char aux[3];
	//lcd_comm(0xc7);
	sprintf(aux,"%d",(number-155));
		lcd_data(aux);
		delay_ms(5);
	return;
}
// Interrupção externa 0
void INT_EX0 (void) interrupt 0{ // Inicia ou reseta
	if (start == 0){//Inicia contagem
		start = 1;
	}else if (start == 1){  // permite resetar
		start = 0;
	}
}

//Sensor Functions
char s_write_byte (unsigned char value)
{ 
	// Manda um byte para o sensor e verifica se foi reconhecido 
	unsigned char i, erro=0;
	for (i=0x80;i>0;i/=2){         // Shift bit para mascarar value
		if (i & value) {DATA=1;}     // utilizando i para efetuar uma mascara em value 
		else DATA = 0;
		_nop_();
		SCK = 1;                     //coloca clock em nivel alto
		_nop_();                     //pulswith approx. 5 us
		_nop_();
		_nop_();
		SCK =0;                      //baixa nivel clock
		_nop_();
	}
	DATA =1;                       //libera linha de dados
	_nop_();
	SCK = 1;                       //9º clock para ack
	erro = DATA;                   //checa ack (dados serão retirados pelo sensor)
	SCK = 0;
	return erro;                   //erro=1 em caso de não reconhecimento
}

char s_read_byte (unsigned char ack)
{
	unsigned char i,val=0;
	DATA = 1;									//Libera a linha de dados puxando para 1. 
	for (i=0x80;i>0;i/=2){		// Bit de mudança para efetuar mascara
		SCK =1;									// clock do sensor
		if (DATA){val =(val | i);} //le o bit
		SCK = 0;
	}
	DATA=!ack;              // caso ack == 1 puxa para nivel baixo a linha de dados.
	_nop_();
	SCK = 1;
	_nop_();
	_nop_();
	_nop_();
	SCK = 0;
	_nop_();
	DATA=1;
	return val;             // retorna bit que foi lido
}

void s_transstart(void)
{
// Gera o início da transmissão
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______ 
	DATA=1;
	SCK=0;
	_nop_();
	SCK=1;
	_nop_();
	DATA=0;
	_nop_();
	SCK=0;
	_nop_();
	_nop_();
	_nop_();
	SCK=1;
	_nop_();
	DATA=1;
	_nop_();
	SCK=0;
}
void s_connectionreset(void)
{
//----------------------------------------------------------------------------------
// reset comunicação com sensor: DATA-line=1 e pelo menos 9 ciclos em SCK seguido pelo inicio da transmissão
//       _______________________________________________________________________         ________
// DATA:                                                                        |_______|
//          ___    ___    ___    ___    ___    ___    ___    ___    ___        ___     ___      
// SCK : __|   |__|   |__|   |__|   |__|   |__|   |__|   |__|   |__|   |______|   |___|   |______

	unsigned char i;
	DATA=1; SCK=0;      //Estado inicial para reset
	for(i=0;i<9;i++){  //Efetua os 9 ciclos em SCK
		SCK=1;
		SCK=0;
  }
  s_transstart(); //Inicia Transmissão
}
char s_measure(unsigned char *p_value,unsigned char *p_checksum, unsigned char mode)
{
//----------------------------------------------------------------------------------
// faz a medição da umidade e temperatura usando checksum 

	unsigned char error=0;
	unsigned int i;
	s_transstart(); 													//Start da comunicaçã
	switch(mode){ 														//seleciona qual medição vai fazer e envia para sensor sensor
		case TEMP : error+=s_write_byte(med_temp); break;
		case HUMI : error+=s_write_byte(med_umi); break;
		default : break;
	}
	for (i=0;i<65535;i++){
		if(DATA==0) break; //Aguarda Sensor terminar medição, quando sensor finaliza a medição puxa data para zero.
	}
	if(DATA) {error+=1;}             //Ou caso o tempo limite (proximo de 2 seg) é atingido
	*(p_value)  = s_read_byte(ACK);    //le o primeiro byte(MSB)
	*(p_value+1)= s_read_byte(ACK);   //le o segundo byte(LSB)
	*p_checksum = s_read_byte(noACK); //le checksum para indicar final de dados.
	return error; 
}
void calc_sth11(float *p_humidity, float *p_temperature )
{ 
//----------------------------------------------------------------------------------
// Calculos de temperatura [°C] e umidade [%RH]
// entrada : umidade [Ticks] (12 bit)
// temp [Ticks] (14 bit)
// retorna: umi [%RH]
// retorna: temp [°C]
	const float C1=  -2.0468;        						// for 12 Bit RH
	const float C2=  0.0367;         						// for 12 Bit RH
	const float C3=  -0.0000015955;          		// for 12 Bit RH
	const float T1= 0.1; 			          				// for 12 Bit RH
	const float T2= 0.00008; 					        	// for 12 Bit RH
	float rh = *p_humidity; 								    // rh: umidade 12 Bit
	float t = *p_temperature; 							    // t: temperatura 14 Bit
	float rh_lin; 												      // rh_lin: umidade linear
	float rh_true; 												      // rh_true: umidade compensada pela temperatura
	float t_C; 														      // t_C : temperatura [°C]
	t_C=t*0.01 - 40.1; 										      //calc. temperatura [°C]from 14 bit temp.ticks @5V
	rh_lin=C3*rh*rh + C2*rh + C1; 				      //calc. umidade para interação para [%RH]
	rh_true=(t_C-25)*(T1+T2*rh)+rh_lin; 	      //calc. umidade compensada pela temperatura [%RH]

	if(rh_true>100){rh_true=100;} // Trata dos limites aceitaveis 
	if(rh_true<0.1){rh_true=0.1;} //
	*p_temperature=t_C; //Retorna o valor da temperatura [%RH]
	*p_humidity=rh_true; //Retorna o valor da umidade [%RH]
}
float medidor()
{
//----------------------------------------------------------------------------------
// Utilizando SHT75 para medir umidade e temperatura, temperatura usada para calculo de precisão da umidade
// 1. Reseta a conexão com o sensor
// 2. mede a umidade (12 bit) e temperatura(14 bit)
// 3. Calcula a umidade[%RH] e a temperatura [°C]
// 4. Mostra no LCD o valor medido em %
  	value umi_val,temp_val;
		unsigned char error,checksum;
		//unsigned int i;
	  char dado[5];
		//SCON = 0x52;
		//TMOD = 0x35;
		//TCON = 0x69;
		//TH1 =  0xfd; 
		s_connectionreset();
	  _nop_();
	  _nop_();
	  _nop_();
	  _nop_();
		error=0;
		error+=s_measure((unsigned char*) &temp_val.i,&checksum,TEMP);// medição de temp; 
		error+=s_measure((unsigned char*) &umi_val.i,&checksum,HUMI); //medição de umidade
		if(error!=0){
			s_connectionreset(); 
			umi_val.f = 404;
		}                           //Caso tenha algum erro reseta a coneção
		else{
			umi_val.f=(float)umi_val.i;                                //converte inteiro em float
			temp_val.f=(float)temp_val.i;                              //converte inteiro em float
			calc_sth11(&umi_val.f,&temp_val.f);                        //calcula umidade
			sprintf(dado,"%0.1f",umi_val.f);
			lcd_comm(0x80);
			delay_ms(5);
			lcd_data("Umidade:");
			_nop_();
		 	lcd_comm(0x88);                                            // Colocar posição que vai ser mostrado no LCD
		 	_nop_();
		  lcd_data(strcat(dado,"%"));
		}
			
			
/*----------wait approx. 0.8s to avoid heating up SHTxx-----------------------------
		 for (i=0;i<40000;i++){i=i;} //(be sure that the compiler doesn't eliminate this line!)
----------------------------------------------------------------------------------*/
    return(umi_val.f);
}

void ISR_Timer1(void) interrupt 3
{
	flics =TL0 * 20;
	TF1 = 0;
	
	TH0 = 0x00;
	TL0 = 0x00;
	
	TH1 = 0x4B;
	TL1 = 0xFF;
	TR1 = 1;
	TR0 = 1;
}


void contaRPS(void)
{
	IE = 0x89; // 0x88 = 1000 1000 mudar para 0x89 que é 1000 1001
	
	TMOD = 0x15;  //0011 0101
	TH0 = 0x00;
	TL0 = 0x00;
	TH1 = 0x4B;
	TL1 = 0xff;
	TR1 = 1;
	TR0 = 1;
	
}


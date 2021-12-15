#include<at89x52.h>
#include<string.h>
#include<stdio.h>
#include<intrins.h> //Biblioteca para usar nop()
#include<math.h>   
#include "PID.h"


#define DATA P1_1 // Comunicação de dados com sensor
#define SCK  P1_2 // Porta de clock do sensor

#define noACK 0 // bit de controle de reconhecimento ou não
#define ACK   1 //
/*Utilizando como Base a tabela de comando do sensor, considerando adress= 0000*/
										  	//comando     Read(1)/Write(0)
#define med_umi   0x05 //  0010           1
#define med_temp  0x03 //  0001           1
#define reset     0x1e //  1111           0
#define sts_reg_w 0x06 //  0011						0
#define sts_reg_r 0x07 //  0011						1
typedef union { // Define estrutura com váriaveis utilizadas para encontrar/calcular umidade e temperatura
	unsigned int i;
	float f;
}value;
enum{TEMP,HUMI};
char s_write_byte (unsigned char value){ 
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

char s_read_byte (unsigned char ack){
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

void s_transstart(void){
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
//----------------------------------------------------------------------------------
void s_connectionreset(void){
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

//----------------------------------------------------------------------------------
char s_softreset(void){
//----------------------------------------------------------------------------------
// resets the sensor by a softreset

  unsigned char error=0;
  s_connectionreset();          //reset communication
	error+=s_write_byte(reset);  //send RESET-command to sensor
	return error;                //error=1 in case of no response form the sensor
} 



//----------------------------------------------------------------------------------
char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum){
//----------------------------------------------------------------------------------
// le o registrador de status  com  checksum (8-bit)

	unsigned char error=0;
	s_transstart(); 										//Inicio da transmissão
	error=s_write_byte(sts_reg_r); 	    //Envia comando para sensor
	*p_value=s_read_byte(ACK);				  //Le registrador de status(8-bit)
	*p_checksum=s_read_byte(noACK); 	  //Le checksum (8-bit)
  return error; 											//error=1 caso não tenha resposta do sensor
} 

//----------------------------------------------------------------------------------
char s_write_statusreg(unsigned char *p_value){
//----------------------------------------------------------------------------------
// Grava no registro de status o checksum (contendo 8 bits)

	unsigned char error=0;
	s_transstart(); 									//Inicia Transmissão
	error+=s_write_byte(sts_reg_w);   //envia comando para sensor
	error+=s_write_byte(*p_value); 		//Envia o valor do registro de status
	return error; 										//error>=1 caso o sensor não responda
} 

//----------------------------------------------------------------------------------
char s_measure(unsigned char *p_value,unsigned char *p_checksum, unsigned char mode){
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
//----------------------------------------------------------------------------------
void calc_sth11(float *p_humidity, float *p_temperature ){ 
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

//----------------------------------------------------------------------------------
void medidor(){
//----------------------------------------------------------------------------------
// Utilizando SHT75 para medir umidade e temperatura, temperatura usada para calculo de precisão da umidade
// 1. Reseta a conexão com o sensor
// 2. mede a umidade (12 bit) e temperatura(14 bit)
// 3. Calcula a umidade[%RH] e a temperatura [°C]
// 4. Mostra no LCD o valor medido em %
  	value umi_val,temp_val;
		unsigned char error,checksum;
		unsigned int i;
	  char dado[3];
		SCON = 0x52;
		TMOD = 0x20;
		TCON = 0x69;
		TH1 =  0xfd; 
		s_connectionreset();
	  _nop_();
	  _nop_();
	  _nop_();
	  _nop_();
		error=0;
		error+=s_measure((unsigned char*) &temp_val.i,&checksum,TEMP);// medição de temp; 
		error+=s_measure((unsigned char*) &umi_val.i,&checksum,HUMI); //medição de umidade
		if(error!=0){ s_connectionreset();}                           //Caso tenha algum erro reseta a coneção
		else{
			umi_val.f=(float)umi_val.i;                                //converte inteiro em float
			temp_val.f=(float)temp_val.i;                              //converte inteiro em float
			calc_sth11(&umi_val.f,&temp_val.f);                        //calcula umidade
			sprintf(dado,"%f",umi_val.f);
		 	lcd_comm(0x80);                                            // Colocar posição que vai ser mostrado no LCD
		 	_nop_();
		  lcd_data(strcat(dado,"%"));
		}
			
			
//----------wait approx. 0.8s to avoid heating up SHTxx-----------------------------
		 for (i=0;i<40000;i++){i=i;} //(be sure that the compiler doesn't eliminate this line!)
//----------------------------------------------------------------------------------
	}
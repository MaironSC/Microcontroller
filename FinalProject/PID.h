#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H
#include<at89x52.h>
#include<string.h>
#include<stdio.h>
#include "PID.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <intrins.h>
#define lcd_bus	P2 
#define RS P3_7    
#define EN P3_6

//Parâmetros do PID
#define ON P1_0 // Liga o lcd
#define aceito P1_7 // Contador de eventos

// Saída do uC
#define out P0

//Parâmetro do sensor
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

extern unsigned int temp;
extern volatile int start;
extern volatile float flics;

//float pid(int error_ctrl, int prev_error_ctrl, int kp, int ki, int kd);
void delay_ms (unsigned ms);
//Interrupções
void INT_EX0 (void);  //inicia contagem


//LCD Functions
void lcd_init();
void lcd_comm (char dado);
void lcd_data (char dado[]);
void lcd_clear();
void exibe (int number);
long int decimal_to_binary(int n);


//Sensor
char s_write_byte (unsigned char value);
char s_read_byte (unsigned char ack);
void s_transstart(void);
void s_connectionreset(void);
char s_softreset(void);
char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum);
char s_write_statusreg(unsigned char *p_value);
char s_measure(unsigned char *p_value,unsigned char *p_checksum, unsigned char mode);
void calc_sth11(float *p_humidity, float *p_temperature );
float medidor();

//Função Motor
void contaRPS(void);
#endif
#include "PID.h"
unsigned int temp = 0;
volatile float flics = 0;
volatile int start =0;

void main()
{
	float umi;
	float w;
	int error_ctrl=2, prev_error_ctrl=1;
	char x_out[3];
	int cont;
	//char mensagemInicial[] = "Sistema iniciado";
	EA  = 1; // Habilita interrupções
	IE0 = 1; // Habilita 
	EX0 = 1; // Habilita interrupção externa 0
	out = 0;
	
	while(1)
	{   		
		if(start == 1){
			lcd_init();
			delay_ms(1000);
			lcd_clear();
			cont = 1;
			break;
		}
	}
	contaRPS();
	while (1){
			umi=medidor(); // reaproveita variavel para poupar memoria.
			w =(flics)/60;// Divide por 60 pois temos 60 pulsos por revolução
			if (w > 0.3)
			{
				w=w-0.3;
			} // trata do erro inicial
			else 
			{
				w=0;
			}
			if (umi < (float)20 )
			{
				out = 0xff;
			}
			else if (umi > 20  && umi < 70 )
			{
				out = 0x0F;
			}
			else 
			if(umi > 70 )
			{
				out = 0x00;
			}
			lcd_comm(0xC0);
			lcd_data("W (RPS):");
			lcd_comm(0xc9);
			sprintf(x_out,"%0.1f",w);
			lcd_data(x_out);
	}	
}
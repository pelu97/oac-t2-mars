//
//  main.c
//  oact2
//
//  Created by Riheldo Melo Santos on 07/05/17.
//  Copyright © 2017 riheldo. All rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>


#ifdef _WIN32
	#define CLEAR "cls"
#else
	#define CLEAR "clear"
#endif


// ENUMS
enum OPCODES{
    EXT=0x00,	LW=0x23,	LB=0x20,	LBU=0x24,
    LH=0x21, 	LHU=0x25,	LUI=0x0F,	SW=0x2B,
    SB=0x28, 	SH=0x29,	BEQ=0x04,	BNE=0x05,
    BLEZ=0x06,	BGTZ=0x07,	ADDI=0x08,	SLTI=0x0A,
    SLTIU=0x0B,	ANDI=0x0C,	ORI=0x0D,	XORI=0x0E,
    J=0x02,		JAL=0x03,	ADDIU=0x09
} opcode;
enum FUNCT{
    ADD=0x20,   SUB=0x22,		MULT=0x18,	DIV=0x1A,
    AND=0x24,   OR=0x25, 		XOR=0x26,	NOR=0x27,
    SLT=0x2A,	JR=0x08,		SLL=0x00,	SRL=0x02,
    SRA=0x03,	SYSCALL=0x0c,	MFHI=0x10,	MFLO=0x12
} funct;

// Definicao de memoria e registradores
#define MEM_SIZE 4096
int32_t mem[MEM_SIZE];
int32_t reg[32], lo, hi;
uint32_t pc = 0, ri;

int op, rs, rt, rd, sa, fct, k16, k26;

typedef enum {
    false, true
} bool;

bool syc = false;


//
//
// ESCOPO
//

// Le um inteiro alinhado. endereco mult 4
int32_t lw(uint32_t address, int16_t kte);
// Le meia palavra - 16 bits, retorna inteiro com sinal. endereco mult 2
int32_t lh(uint32_t address, int16_t kte);
// Le meia palavra - 16 bits. endereco mult 4
int32_t lhu(uint32_t address, int16_t kte);
// Le um byte, retorno int com sinal
int32_t lb(uint32_t address, int16_t kte);
// Le byte
int32_t lbu(uint32_t address, int16_t kte);
// Escreve uma palavra na memoria. endereco mult 4
void sw(uint32_t address, int16_t kte, int32_t dado);
// Escreve meia palavra - 16 bits. endereco mult 2
void sh(uint32_t address, int16_t kte, int16_t dado);
// Escreve um byte na memoria
void sb(uint32_t address, int16_t kte, int8_t dado);
// Executa syscall
void fsyscall();
// Funcao para inicializar os vetores de memoria e registradores
void init();
// Funcao que le o arquivo passado como parametro e coloca na memoria a partir da posição indicada
void leitura(FILE *p, int pos, int impr);
// Aponta a instrucao atual e guarda qual sera a proxima
void fetch();
//  Associa o binário com a função correspondente
void decode();
// Execução de cada instrucao
void execute();
// fetch, decode, execute
void step();
// Executa todas as instrucoes fornecidas pelo binario - fetch, decode, execute ate o termino
void run();
// Imprime na tela e gera um arquivo binario com o resultado da memoria de dados
void dump_mem(int start, int end, char format);
// Imprime na tela e gera um arquivo binario com o resultado dos registradores
void dump_reg(char format);


//
//
// FUNCOES
//

int32_t lw(uint32_t address, int16_t kte){
    int32_t temp;
    temp = (int32_t) mem[(address + kte)/4];
    return temp;
}

int32_t lh(uint32_t address, int16_t kte){
    int32_t temp;
    if( (address+kte)%4 == 0){
        temp = (int32_t) (mem[(address + kte)/4] & 0x0000ffff);
        if(temp >= 0x00008000) temp = (int32_t) temp | 0xffff0000;
    } else {
        temp = (int32_t) (mem[(address + kte-2)/4] & 0xffff0000);
        if(temp >= 0x80000000) temp = (int32_t) temp | 0x0000ffff;
        temp = temp >> 16;
    }
    return temp;
}

int32_t lhu(uint32_t address, int16_t kte){
    uint32_t temp;
    if((address+kte)%4 == 0) {
        temp = (int32_t) (mem[(address + kte)/4] & 0x0000ffff);
    } else {
        temp = (int32_t) (mem[(address + kte-2)/4] & 0xffff0000);
        temp = temp >> 16;
    }
    return temp;
}

int32_t lb(uint32_t address, int16_t kte){
    int32_t temp;
    
    if((address+kte)%4 == 0){
        temp = (int32_t) (mem[(address + kte)/4] & 0x000000ff);
        if(temp >= 0x00000080) temp = (int32_t) temp | 0xffffff00;
    } else if ( (address+kte)%4 == 3){
        temp = (int32_t) (mem[(address + kte-3)/4] & 0xff000000);
        if(temp >= 0x80000000) temp = (int32_t) temp | 0x00ffffff;
        temp = temp >> 24;
    } else if ( (address+kte)%2 == 0){
        temp = (int32_t) (mem[(address + kte-2)/4] & 0x00ff0000);
        if(temp >= 0x00800000) temp = (int32_t) temp | 0xff00ffff;
        temp = temp >> 16;
    } else {
        temp = (int32_t) (mem[(address + kte-1)/4] & 0x0000ff00);
        if(temp >= 0x00008000) temp = (int32_t) temp | 0xffff00ff;
        temp = temp >> 8;
    }
    
    return temp;
}

int32_t lbu(uint32_t address, int16_t kte){
    uint32_t temp;
    
    if((address+kte)%4 == 0)
        temp = (int32_t) (mem[(address + kte)/4] & 0x000000ff);
    else if((address+kte)%4 == 3){
        temp = (int32_t) (mem[(address + kte-3)/4] & 0xff000000);
        temp = temp >> 24;
    } else if((address+kte)%2 == 0){
        temp = (int32_t) (mem[(address + kte-2)/4] & 0x00ff0000);
        temp = temp >> 16;
    } else{
        temp = (int32_t) (mem[(address + kte-1)/4] & 0x0000ff00);
        temp = temp >> 8;
    }
    
    return temp;
}

void sw(uint32_t address, int16_t kte, int32_t dado){
    mem[(address + kte)/4] = (int32_t) dado;
}

void sh(uint32_t address, int16_t kte, int16_t dado){
    int32_t t1, t2;

    if((address+kte)%4 == 0){
        t1 = (int32_t) mem[(address + kte)/4] & 0xffff0000;
        t2 = (int32_t) dado & 0x0000ffff;
        mem[(address + kte)/4] = (int32_t) t1 + t2;
    } else {
        t1 = (int32_t) mem[(address + kte-2)/4] & 0x0000ffff;
        t2 = (int32_t) dado & 0x0000ffff;
        t2 = t2 << 16;
        mem[(address + kte-2)/4] = (int32_t) t1 + t2;
    }
}

void sb(uint32_t address, int16_t kte, int8_t dado){
    int32_t t1, t2;
    
    if( (address+kte)%4 == 0){
        t1 = (int32_t) mem[(address + kte)/4] & 0xffffff00;
        t2 = (int32_t) dado & 0x000000ff;
        mem[(address + kte)/4] = (int32_t) t1 + t2;
    } else if((address+kte)%4 == 3) {
        t1 = (int32_t) mem[(address + kte-3)/4] & 0x00ffffff;
        t2 = (int32_t) dado & 0x000000ff;
        t2 = t2 << 24;
        mem[(address + kte-3)/4] = (int32_t) t1 + t2;
    } else if((address+kte)%2 == 0) {
        t1 = (int32_t) mem[(address + kte-2)/4] & 0xff00ffff;
        t2 = (int32_t) dado & 0x000000ff;
        t2 = t2 << 16;
        mem[(address + kte-2)/4] = (int32_t) t1 + t2;
    } else {
        t1 = (int32_t) mem[(address + kte-1)/4] & 0xffff00ff;
        t2 = (int32_t) dado & 0x000000ff;
        t2 = t2 << 8;
        mem[(address + kte-1)/4] = (int32_t) t1 + t2;
    }
}

void fsyscall(){
    int temp, c, i;
    
    if (reg[2] == 1) {
        printf("%d\n", reg[4]);
    } else if(reg[2] == 4) {
        temp = reg[4];
        c = lb(temp,0);
        for(i = 1; c != '\0'; i++) {
            printf("%c",c);
            c = lb(temp,i);
            if(i == 3){
                i = -1;
                temp += 4;
            }
        }
        printf("\n");
    } else if(reg[2] == 10){
        syc = true;
    }
}

void init(){
    int i;
    for(i = 0; i < 32; i++) {
        reg[i] = 0;
    }
    for(i = 0; i < MEM_SIZE; i++) {
        mem[i] = 0;
    }
    hi = 0;
    lo = 0;
    
}


void fetch(){
    ri = mem[pc/4];
    pc += 4;
}

void decode(){
    op  = ri >> 26 & 0x3f;
    rs  = ri >> 21 & 0x1f;
    rt  = ri >> 16 & 0x1f;
    rd  = ri >> 11 & 0x1f;
    sa  = ri >> 6 & 0x1f;
    fct = ri & 0x3f;
    k16 = ri & 0xffff;
    k26 = ri & 0x3ffffff;
}

void execute(){
    int ender;
    
    opcode = op;
    if(opcode == BEQ || opcode == BNE || opcode == BLEZ ||
       opcode == BGTZ || opcode == ADDI || opcode == ADDIU ||
       opcode == SLTI || opcode == SLTIU ||
       opcode == ANDI || opcode == ORI || opcode == XORI) {
        
        if(k16 >= 0x8000) {
            k16 = k16 | 0xffff0000;
        } else {
            k16 = k16 & 0x0000ffff;
        }
    }
    
    switch(opcode) {
        case LW:
            reg[rt] = lw(reg[rs],k16);
            break;
            
        case LH:
            reg[rt] = lh(reg[rs],k16);
            break;
            
        case LHU:
            reg[rt] = lhu(reg[rs],k16);
            break;
            
        case LB:
            reg[rt] = lb(reg[rs],k16);
            break;
            
        case LBU:
            reg[rt] = lbu(reg[rs],k16);
            break;
            
        case LUI:
            reg[rt] = k16 << 16;
            break;
            
        case SW:
            sw(reg[rs],k16,reg[rt]);
            ender = (reg[rs] + k16)/4;
            break;
            
        case SH:
            sh(reg[rs],k16,reg[rt]);
            ender = (reg[rs] + k16)/4;
            break;
            
        case SB:
			sb(reg[rs],k16,reg[rt]);
            ender = (reg[rs] + k16)/4;
            break;
            
        case BEQ:
            if(reg[rs] == reg[rt]) pc += (k16 << 2);
            break;
            
        case BNE:
            if(reg[rs] != reg[rt]) pc += (k16 << 2);
            break;
            
        case BLEZ:
            if(reg[rs] <= 0) pc += (k16 << 2);
            break;
            
        case BGTZ:
            if(reg[rs] > 0) pc += (k16 << 2);
            break;
            
        case ADDI:
            reg[rt] = reg[rs] + k16;
            break;
            
        case ADDIU:
            reg[rt] = ((uint32_t) reg[rs] + k16);
            break;
            
        case SLTI:
            reg[rt] = reg[rs] < k16;
            break;
            
        case SLTIU:
            reg[rt] = ((uint32_t) reg[rs]) < ((uint32_t) k16);
            break;
            
        case ANDI:
            reg[rt] = reg[rs] & k16;
            break;
            
        case ORI:
            reg[rt] = reg[rs] | k16;
            break;
            
        case XORI:
            reg[rt] = reg[rs] ^ k16;
            break;
            
        case J:
            pc = (pc & 0xf0000000) | (k26 << 2);
            break;
            
        case JAL:
            reg[31] = pc;
            pc = (pc & 0xf0000000) | (k26 << 2);
            break;
            
        default:
            break;
    }
    
    if(op == EXT){
        switch(fct){
            case ADD:
                reg[rd] = reg[rs] + reg[rt];
                break;
                
            case SUB:
                reg[rd] = reg[rs] - reg[rt];
                break;
                
            case MULT:
                lo = reg[rs] * reg[rt];
                break;
                
            case DIV:
                lo = reg[rs] / reg[rt];
                hi = reg[rs] % reg[rt];
                break;
                
            case AND:
                reg[rd] = reg[rs] & reg[rt];
                break;
                
            case OR:
                reg[rd] = reg[rs] | reg[rt];
                break;
                
            case XOR:
                reg[rd] = reg[rs] ^ reg[rt];
                break;
                
            case NOR:
                reg[rd] = ~(reg[rs] | reg[rt]);
                break;
                
            case SLT:
                reg[rd] = reg[rs] < reg[rt];
                break;
                
            case JR:
                pc = reg[rs];
                break;
                
            case SLL:
                reg[rd] = (uint32_t) reg[rt] << sa;
                break;
                
            case SRL:
                reg[rd] = (uint32_t) reg[rt] >> sa;
                break;
                
            case SRA:
                reg[rd] = reg[rt] >> sa;
                break;
                
            case MFHI:
                reg[rd] = hi;
                break;
                
            case MFLO:
                reg[rd] = lo;
                break;
                
            case SYSCALL:
                fsyscall();
                break;
                
            default:
                break;
        }
    }
}

void step(){
    if(syc == false && pc < MEM_SIZE * 2) {
        fetch();
        decode();
        execute();
    }
}

void run(){
    while(syc == false && pc < MEM_SIZE * 2) {
        fetch();
        decode();
        execute();
    }
}

void dump_mem(int start, int end, char format){
    int i, cont;
    FILE *dataS;
    
    dataS = fopen("mem.txt", "w");
    
	fprintf(dataS, "MEMORY:\n");
	
	if((format=='h')||(format=='H')){
		for (i=start;i<=end;i++){
			fprintf(dataS, "0x%08x = 0x%08x\n", i*4, mem[i]);
			
		}
	}
	else{
		for (i=start;i<=end;i++){
			fprintf(dataS, "0x%08x = %d\n", i*4, mem[i]);
		}
	}
    
    fclose(dataS);
}

void dump_reg(char format){
    int i;
    FILE *regS;
    
    regS = fopen("reg.txt", "w");
    
    fprintf(regS, "REGISTERS:\n");
    if(format == 'h' || format == 'H') {
        for(i = 0; i < 32; i++){
            fprintf(regS,"$%d = 0x%08x\n", i, reg[i]);
        }
        fprintf(regS,"pc = 0x%08x\nhi = 0x%08x\nlo = 0x%08x\n",pc,hi,lo);
    } else{
        for(i = 0; i < 32; i++){
            fprintf(regS,"$%d = %d\n", i, reg[i]);
        }
        fprintf(regS,"pc = %d\nhi = %d\nlo = %d\n",pc,hi,lo);
    }
    fclose(regS);
}

void delay(int mseconds){
	clock_t limit = mseconds + clock();
	while (limit>clock());
}
void menu(){
	int choice, quit=0, dump_mem_inicio=-1, dump_mem_end=-1, erro_flag, i;
	char formato='s', anim;
	
	while(quit==0){
		system(CLEAR);
		printf("Insira o numero da funcao desejada:\n\n");
		printf("(1) - Step\n	+Essa funcao executa uma instrucao do MIPS. Nao eh necessario inserir parametros na mesma.\n\n");
		printf("(2) - Run\n	+Essa funcao executa o programa completo, ou seja, ateh encontrar uma chamada de sistema para encerramento\n	 ou ateh atingir o limite do segmento de codigo. Nao eh necessario inserir parametros na mesma.\n\n");
		printf("(3) - Dump_mem\n	+Essa funcao imprime e exporta o conteudo de uma secao da memoria, em hexadecimal(h) ou decimal(d),\n	 delimitada por enderecos de inicio e fim. Eh necessario inserir os parametros:\n		-Inicio(0-4095)\n		-Fim(0-4095)\n		-Formato(d/h)\n\n");
		printf("(4) - Dump_reg\n	+Essa funcao imprime e exporta o conteudo dos registradores, em hexadecimal(h) ou decimal(d). \n	 Eh necessario inserir os parametros:\n		-Formato(d/h)\n\n");
		printf("(5) - Exit\n	+Termina a execucao do programa.\n\n");
		scanf("%d", &choice);
		getchar();
		system(CLEAR);
		
		erro_flag=0;
		switch(choice){
			case 1:
				printf("Funcao escolhida - Step\n\n");
				step();
				printf("\nExecucao terminada. Pressione qualquer tecla para continuar\n");
				getchar();
				break;
			case 2:
				printf("Funcao escolhida - Run\n\n");
				run();
				printf("\nExecucao terminada. Pressione qualquer tecla para continuar\n");
				getchar();
				break;
			case 3:
				printf("Funcao escolhida - Dump_mem\n");
				while((dump_mem_inicio<0)||(dump_mem_inicio>4095)){
					if(erro_flag==1){
						printf("O parametro inserido esta errado\n");
					}
					printf("Insira os parametros:\n	-Endereco de inicio (0-4095): ");
					scanf("%d", &dump_mem_inicio);
					erro_flag=1;
				}
				erro_flag=0;
				while((dump_mem_end<0)||(dump_mem_end>4095)){
					if(erro_flag==1){
						printf("O parametro inserido esta errado\n");
					}
					printf("\n	-Endereco de fim (0-4095): ");
					scanf("%d", &dump_mem_end);
					erro_flag=1;
				}
				erro_flag=0;
				while((formato!='d')&&(formato!='D')&&(formato!='h')&&(formato!='H')){
					if(erro_flag==1){
						printf("O parametro inserido esta errado\n");
					}
					getchar();
					printf("\n	-Formato (d/h):");
					scanf("%c", &formato);
					erro_flag=1;
				}
				system(CLEAR);
				printf("Funcao escolhida - Dump_mem\n");
				dump_mem(dump_mem_inicio, dump_mem_end, formato);
				dump_mem_inicio=-1;
				dump_mem_end=-1;
				formato='s';
				printf("	-Arquivo mem.txt criado.\n");
				delay(2000);
				break;
			case 4:
				printf("Funcao escolhida - Dump_reg\n");
				printf("Insira os parametros:");
				erro_flag=0;
				while((formato!='d')&&(formato!='D')&&(formato!='h')&&(formato!='H')){
					if(erro_flag==1){
						printf("O parametro inserido esta errado\n");
					}
					printf("\n	-Formato (d/h):");
					scanf("%c", &formato);
					erro_flag=1;
				}
				system(CLEAR);
				printf("Funcao escolhida - Dump_reg\n");
				dump_reg(formato);
				formato='s';
				printf("	-Arquivo reg.txt criado.\n");
				delay(2000);
				break;
			case 5:
				quit=1;
				printf("Funcao escolhida - Exit\n	-Encerrando Simulador\n");
				for (i=0;i<16;i++){
					switch(i%4){
						case 0:
							anim='|';
							break;
						case 1:
							anim='/';
							break;
						case 2:
							anim='-';
							break;
						case 3:
							anim='\\';
							break;
					}
					
					delay(250);
					printf("\r	%c", anim);
				}
				system(CLEAR);
				break;
			default:
				printf("Opcao invalida\n");
				delay(2000);
				break;
		}
	}
}

int main(int argc, const char * argv[]) {
    
    FILE *prog_text, *prog_data;
    
    init();
    pc = 0;
    
	if (argc==4){
		prog_text = fopen(argv[1], "rb");
		prog_data = fopen(argv[2], "rb");
		fread(mem, 4, 2048, prog_text); /* Le os arquivos gerados pelo MARS */
		fread(mem + 2048, 4, 2048, prog_data);
		fclose(prog_text);
		fclose(prog_data);
		if(argv[3][0]=='f'){
				run();
				dump_reg('h');
				dump_mem(0, 4095, 'h');
		}
		else{
			if(argv[3][0]=='p'){
				menu();
			}
			else{
				printf("Terceiro parametro incorreto ('p' ou 'f'\n)\n");
			}
		}				
	}
	else{
		printf("Erro no numero de parametros inseridos\n");
		delay(3000);
	}
    
    return 0;
}

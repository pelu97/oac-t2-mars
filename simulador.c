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
        printf("\033[31msyscall\n");
        printf(">> %d\033[0m\n", reg[4]);
    } else if(reg[2] == 4) {
        printf("\033[31msyscall\n>> ");
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
        printf("\033[0m\n");
    } else if(reg[2] == 10){
        printf("\033[31msyscall\nTerminate program.\033[0m\n");
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

void leitura(FILE *p, int pos, int impr){
    int32_t n;
    int i = pos, cont;
    
    fread(&n,sizeof(int32_t),1,p);
    
    while(!feof(p)){
        mem[i] = n;
        if(n == 0) {
            cont = 1;
            fread(&n, sizeof(int32_t), 1, p);
            while(n == 0 && !feof(p)){
                cont++;
                i++;
                mem[i] = n;
                fread(&n, sizeof(int32_t), 1, p);
            }
            if (n != 0) {
                i++;
                mem[i] = n;
            }
            
            if (impr == 0) {
                if(cont == 1) {
                    printf("0x%08x 0x%08x ", mem[i-1], mem[i]);
                } else {
                    if(n == 0) {
                        printf("{%d repeticoes de 0x%08x} ", cont, mem[i]);
                    } else {
                        printf("{%d repeticoes de 0x%08x} 0x%08x ", cont, mem[i-1], mem[i]);
                    }
                }
            }
        } else {
            if(impr == 0) {
                printf("0x%08x ", mem[i]);
            }
        }
        
        i++;
        fread(&n, sizeof(int32_t), 1, p);
    }
    printf("\n");
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
            printf("lw $%d,%d($%d)\n",rt,k16,rs);
            reg[rt] = lw(reg[rs],k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case LH:
            printf("lh $%d,%d($%d)\n",rt,k16,rs);
            reg[rt] = lh(reg[rs],k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case LHU:
            printf("lhu $%d,%d($%d)\n",rt,k16,rs);
            reg[rt] = lhu(reg[rs],k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case LB:
            printf("lb $%d,%d($%d)\n",rt,k16,rs);
            reg[rt] = lb(reg[rs],k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case LBU:
            printf("lbu $%d,%d($%d)\n",rt,k16,rs);
            reg[rt] = lbu(reg[rs],k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case LUI:
            printf("lui $%d,%d\n",rt,k16);
            reg[rt] = k16 << 16;
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case SW:
            printf("sw $%d,%d($%d)\n",rt,k16,rs);
            sw(reg[rs],k16,reg[rt]);
            ender = (reg[rs] + k16)/4;
            printf(">> mem[$%d+%d] = 0x%08x\n",rs,k16,mem[ender]);
            break;
            
        case SH:
            printf("sh $%d,%d($%d)\n",rt,k16,rs);
            sh(reg[rs],k16,reg[rt]);
            ender = (reg[rs] + k16)/4;
            printf(">> mem[$%d+%d] = 0x%08x\n",rs,k16,mem[ender]);
            break;
            
        case SB:
            printf("sb $%d,%d($%d)\n",rt,k16,rs);
            sb(reg[rs],k16,reg[rt]);
            ender = (reg[rs] + k16)/4;
            printf(">> mem[$%d+%d] = 0x%08x\n",rs,k16,mem[ender]);
            break;
            
        case BEQ:
            printf("beq $%d,$%d,%d\n",rs,rt,k16);
            if(reg[rs] == reg[rt]) pc += (k16 << 2);
            printf(">> pc = 0x%08x\n",pc);
            break;
            
        case BNE:
            printf("bne $%d,$%d,%d\n",rs,rt,k16);
            if(reg[rs] != reg[rt]) pc += (k16 << 2);
            printf(">> pc = 0x%08x\n",pc);
            break;
            
        case BLEZ:
            printf("blez $%d,%d\n",rs,k16);
            if(reg[rs] <= 0) pc += (k16 << 2);
            printf(">> pc = 0x%08x\n",pc);
            break;
            
        case BGTZ:
            printf("bgtz $%d,%d\n",rs,k16);
            if(reg[rs] > 0) pc += (k16 << 2);
            printf(">> pc = 0x%08x\n",pc);
            break;
            
        case ADDI:
            printf("addi $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = reg[rs] + k16;
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case ADDIU:
            printf("addiu $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = ((uint32_t) reg[rs] + k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case SLTI:
            printf("slti $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = reg[rs] < k16;
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case SLTIU:
            printf("sltiu $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = ((uint32_t) reg[rs]) < ((uint32_t) k16);
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case ANDI:
            printf("andi $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = reg[rs] & k16;
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case ORI:
            printf("ori $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = reg[rs] | k16;
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case XORI:
            printf("xori $%d,$%d,0x%04x\n",rt,rs,k16);
            reg[rt] = reg[rs] ^ k16;
            printf(">> $%d = 0x%08x\n",rt,reg[rt]);
            break;
            
        case J:
            printf("j %d\n",k26);
            pc = (pc & 0xf0000000) | (k26 << 2);
            printf(">> pc = 0x%08x\n",pc);
            break;
            
        case JAL:
            printf("jal %d\n",k26);
            reg[31] = pc;
            pc = (pc & 0xf0000000) | (k26 << 2);
            printf(">> $ra = 0x%08x, pc = 0x%08x\n",reg[31],pc);
            break;
            
        default:
            break;
    }
    
    if(op == EXT){
        switch(fct){
            case ADD:
                printf("add $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = reg[rs] + reg[rt];
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case SUB:
                printf("sub $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = reg[rs] - reg[rt];
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case MULT:
                printf("mult $%d,$%d\n",rs,rt);
                lo = reg[rs] * reg[rt];
                printf(">> $lo = 0x%08x\n",lo);
                break;
                
            case DIV:
                printf("div $%d,$%d\n",rs,rt);
                lo = reg[rs] / reg[rt];
                hi = reg[rs] % reg[rt];
                printf(">> $lo = 0x%08x, $hi = 0x%08x\n",lo,hi);
                break;
                
            case AND:
                printf("and $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = reg[rs] & reg[rt];
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case OR:
                printf("or $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = reg[rs] | reg[rt];
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case XOR:
                printf("add $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = reg[rs] ^ reg[rt];
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case NOR:
                printf("nor $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = ~(reg[rs] | reg[rt]);
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case SLT:
                printf("slt $%d,$%d,$%d\n",rd,rs,rt);
                reg[rd] = reg[rs] < reg[rt];
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case JR:
                printf("jr $%d\n",rs);
                pc = reg[rs];
                printf(">> pc = 0x%08x\n",reg[rs]);
                break;
                
            case SLL:
                printf("sll $%d,$%d,%d\n",rd,rt,sa);
                reg[rd] = (uint32_t) reg[rt] << sa;
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case SRL:
                printf("srl $%d,$%d,%d\n",rd,rt,sa);
                reg[rd] = (uint32_t) reg[rt] >> sa;
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case SRA:
                printf("sra $%d,$%d,%d\n",rd,rt,sa);
                reg[rd] = reg[rt] >> sa;
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case MFHI:
                printf("mfhi $%d\n",rd);
                reg[rd] = hi;
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
                break;
                
            case MFLO:
                printf("mflo $%d\n",rd);
                reg[rd] = lo;
                printf(">> $%d = 0x%08x\n",rd,reg[rd]);
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
    
    dataS = fopen("dataSim.bin", "wb");
    
    printf("\nData obtida pela simulacao:\n");
    if(format == 'h' || format == 'H'){
        for(i = start/4; i < end/4; i++){
            if(mem[i] == 0) {
                cont = 0;
                while(mem[i] == 0 && i < end/4){
                    fprintf(dataS,"0x%08x ",mem[i]);
                    cont++; i++;
                }
                
                if(cont == 1) {
                    printf("0x%08x 0x%08x ", mem[i-1],mem[i]);
                } else {
                    if(mem[i] == 0) {
                        printf("{%d repeticoes de 0x%08x} ", cont,mem[i]);
                    } else {
                        printf("{%d repeticoes de 0x%08x} 0x%08x ", cont,mem[i-1],mem[i]);
                    }
                }
            } else {
                printf("0x%08x ",mem[i]);
            }
            fprintf(dataS,"0x%08x ",mem[i]);
        }
    } else {
        for(i = start/4; i < end/4; i++) {
            if(mem[i] == 0) {
                cont = 0;
                while(mem[i] == 0 && i < end/4) {
                    fprintf(dataS,"%d ",mem[i]);
                    i++;
                    cont++;
                }
                
                if(cont == 1) {
                    printf("%d %d ", mem[i-1],mem[i]);
                }
                else{
                    if(mem[i] == 0) {
                        printf("{%d repeticoes de %d} ", cont,mem[i]);
                    } else {
                        printf("{%d repeticoes de %d} %d ", cont,mem[i-1],mem[i]);
                    }
                }
            }
            else {
                printf("%d ",mem[i]);
            }
            
            fprintf(dataS,"%d ",mem[i]);
        }
    }
    printf("\n");
    
    fclose(dataS);
}

void dump_reg(char format){
    int i;
    FILE *regS;
    
    regS = fopen("regSim.bin", "wb");
    
    printf("\nRegistradores apos a simulacao:\n");
    if(format == 'h' || format == 'H') {
        for(i = 0; i < 32; i++){
            fprintf(regS,"0x%08x ",reg[i]);
            printf("$%d = 0x%08x   ", i, reg[i]);
        }
        fprintf(regS,"0x%08x 0x%08x 0x%08x",pc,hi,lo);
        printf("pc = 0x%08x\thi = 0x%08x\tlo = 0x%08x\n",pc,hi,lo);
    } else{
        for(i = 0; i < 32; i++){
            fprintf(regS,"%d ",reg[i]);
            printf("$%d = %d   ", i, reg[i]);
        }
        fprintf(regS,"%d %d %d",pc,hi,lo);
        printf("pc = %d   hi = %d   lo = %d\n",pc,hi,lo);
    }
    fclose(regS);
}

int main(int argc, const char * argv[]) {
    
    FILE *prog_text, *prog_data;
    
    init();
    pc = 0;
    
    prog_text = fopen("/Users/riheldomelosantos/repo/oac-t2-mars/oact2/oact2/primos_text.bin", "rb");
    prog_data = fopen("/Users/riheldomelosantos/repo/oac-t2-mars/oact2/oact2/primos_data.bin", "rb");
    fread(mem, 4, 2048, prog_text); /* Le os arquivos gerados pelo MARS */
    fread(mem + 2048, 4, 2048, prog_data);
    fclose(prog_text);
    fclose(prog_data);
    
    run();
    
    return 0;
}

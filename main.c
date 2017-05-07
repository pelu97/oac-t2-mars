//
//  main.c
//  oact2
//
//  Created by Riheldo Melo Santos on 07/05/17.
//  Copyright © 2017 riheldo. All rights reserved.
//

#include <stdio.h>


#include <stdint.h>
#include <stdio.h>

#define MEM_SIZE 4096 //Tamanho da mem�ria
#define MASK 0xFFFF0000 //O melhor filme
#define MASK_1 0x00FFFFFF
#define MASK_2 0xFF00FFFF
#define MASK_3 0xFFFF00FF
#define MASK_4 0xFFFFFF00

int8_t opcode;
int8_t rs;
int8_t rt;
int8_t rd;
int8_t shamnt;
int8_t funct;
int16_t k16;
int32_t k26;
int64_t auxiliar;
int saidaaux;

uint32_t ri;
uint32_t pc;

int32_t hi;
int32_t lo;

int32_t mem[MEM_SIZE];
int32_t breg[MEM_SIZE];

//int syscall () {}

enum OPCODES{
    EXT=0x00,   LW=0x23,    LB=0x20,    LBU=0x24,
    LH=0x21,    LHU=0x25,   LUI=0x0F,   SW=0x2B,
    SB=0x28,    SH=0x29,    BEQ=0x04,   BNE=0x05,
    BLEZ=0x06,  BGTZ=0x07,  ADDI=0x08,  SLTI=0x0A,
    SLTIU=0x0B, ANDI=0x0C,  ORI=0x0D,   XORI=0x0E,
    J=0x02,     JAL=0x03
};

enum FUNCT{
    ADD=0x20,   SUB=0x22,   MULT=0x18,  DIV=0x1A,   AND=0x24,
    OR=0x25,    XOR=0x26,   NOR=0x27,   SLT=0x2A,   JR=0x08,
    SLL=0x00,   SRL=0x02,   SRA=0x03,   SYSCALL=0x0c,   MFHI=0x10,  MFLO=0x12
};

enum REGISTERS {
    ZERO=0, AT=1,   V0=2,
    V1=3,   A0=4,   A1=5,
    A2=6,   A3=7,   T0=8,
    T1=9,   T2=10,  T3=11,
    T4=12,  T5=13,  T6=14,
    T7=15,  T8=24,  T9=25,
    S0=16,  S1=17,  S2=18,
    S3=19,  S4=20,  S5=21,
    S6=22,  S7=23,  K0=26,
    K1=27,  GP=28,  SP=29,
    FP=30,  RA=31
};

int32_t lw(uint32_t address, int16_t kte){
    int pos = (address + kte)/4;
    
    return mem[pos];
}

int32_t lh(uint32_t address, int16_t kte){
    int pos = (address + kte)/4;
    if(kte%4 == 0){
        int32_t res = mem[pos];
        res = res >> 16;
        return res;
    }else{
        int32_t dado = mem[pos];
        return ((dado << 16) >> 16);
    }
}

uint32_t lhu(uint32_t address, int16_t kte){
    int pos = (address + kte)/4;
    if(kte%4 == 0){
        int32_t res = mem[pos];
        res = res >> 16;
        res = res & ~MASK;
        return res;
    }else{
        int32_t dado = mem[pos];
        return (((dado << 16) >> 16) & ~MASK);
    }
}

int32_t lb(uint32_t address, int16_t kte){
    int pos = (address + kte)/4;
    if(kte%4 == 0)
        return (mem[pos] >> 24);
    else if(kte%4 == 1)
        return ((mem[pos] << 8) >> 24);
    else if(kte%4 == 2)
        return ((mem[pos] << 16) >> 24);
    else
        return ((mem[pos] << 24) >> 24);
}

uint32_t lbu(uint32_t address, int16_t kte){
    int pos = (address + kte)/4;
    if(kte%4 == 0){
        return ((mem[pos] >> 24) & ~MASK_4);
    }
    else if(kte%4 == 1){
        return (((mem[pos] << 8) >> 24) & ~MASK_4);
    }
    else if(kte%4 == 2){
        return (((mem[pos] << 16) >> 24) & ~MASK_4);
    }
    else{
        return (((mem[pos] << 24) >> 24) & ~MASK_4);
    }
}

void sw(uint32_t address, int16_t kte, int32_t dado){
    int pos = (address + kte)/4;
    
    mem[pos] = dado;
}

void sh(uint32_t address, int16_t kte, int16_t dado){
    int pos = (address + kte)/4;
    if(kte%4 == 0){
        mem[pos] = MASK | mem[pos];
        int32_t res = mem[pos] & ((dado << 16) | ~MASK);
        mem[pos] = res;
    }else{
        mem[pos] = ~MASK | mem[pos];
        int32_t res = mem[pos] & (dado | MASK);
        mem[pos] = res;
    }
}

void sb(uint32_t address, int16_t kte, int8_t dado){
    int pos = (address + kte)/4;
    if(kte%4 == 0){
        int32_t aux = ~MASK_1 | mem[pos];
        int32_t res = aux & ((dado << 24) | MASK_1);
        mem[pos] = res;
    } else if(kte%4 == 1){
        int32_t aux = ~MASK_2 | mem[pos];
        int32_t res = aux & ((dado << 24) >> 8 | MASK_2);
        mem[pos] = res;
    } else if(kte%4 == 2){
        int32_t aux = ~MASK_3 | mem[pos];
        int32_t res = aux & ((dado << 24) >> 16 | MASK_3);
        mem[pos] = res;
    } else {
        int32_t aux = ~MASK_4 | mem[pos];
        int32_t res = aux & ((dado << 24) >> 24 | MASK_4);
        mem[pos] = res;
        
    }
}

void fetch(){
    ri = lw(pc, 0);
    pc += 4;
}

void decode(){
    opcode = (int8_t) ((ri >> 26) & 0x0000003f);
    rs = (int8_t) (((ri << 5) >> 27) & 0x0000001f);
    rt = (int8_t) (((ri << 10) >> 27) & 0x0000001f);
    rd = (int8_t) (((ri << 15) >> 27) & 0x0000001f);
    shamnt = (int8_t) (((ri << 20) >> 27) & 0x0000001f);
    funct = (int8_t) (((ri << 26) >> 26) & 0x0000003f);
    k16 = (int16_t) (ri & 0x0000ffff);
    k26 = (ri & 0x03ffffff);
}

void execute(){
    switch(opcode){
        case EXT:
            printf("Executando EXT...\n");
            switch(funct){
                case ADD:
                    printf("Executando ADD...\n");
                    breg[rd] = breg[rs] + breg[rt];
                    break;
                case SUB:
                    printf("Executando SUB...\n");
                    breg[rd] = breg[rs] - breg[rt];
                    break;
                case MULT:
                    break;
                case DIV:
                    break;
                case AND:
                    breg[rd] = breg[rs] & breg[rt];
                    break;
                case OR:
                    breg[rd] = breg[rs] | breg[rt];
                    break;
                case XOR:
                    breg[rd] = breg[rs] ^ breg[rt];
                    break;
                case NOR:
                    breg[rd] = ~(breg[rs] | breg[rt]);
                    break;
                case SLT:
                    if(breg[rs] < breg[rt])
                        breg[rd] = 1;
                    else
                        breg[rd] = 0;
                    break;
                case JR:
                    pc = breg[rs];
                    break;
                case SLL:
                    breg[rd] = ((uint32_t) breg[rt]) << shamnt;
                    break;
                case SRL:
                    breg[rd] = ((uint32_t) breg[rt]) >> shamnt;
                    break;
                case SRA:
                    breg[rd] = breg[rt] >> shamnt;
                    break;
                case SYSCALL:
                    syscall(mem[REGISTERS.V0]);
                    break;
                case MFHI:
                    break;
                case MFLO:
                    break;
                default:
                    break;
            }
            break;
        case LW:
            printf("Executando LW...\n");
            printf("%x\n%x\n%x", breg[rt], breg[rs], k16);
            breg[rs] = lw(breg[rt], k16);
            break;
        case LB:
            printf("Executando LB...\n");
            breg[rt] = lb(breg[rs], k16);
            break;
        case LBU:
            printf("Executando LBU...\n");
            breg[rt] = lbu(breg[rs], k16);
            break;
        case LH:
            printf("Executando LH...\n");
            breg[rt] = lh(breg[rs], k16);
            break;
        case LHU:
            printf("Executando LHU...\n");
            breg[rt] = lhu(breg[rs], k16);
            break;
        case LUI:
            printf("Executando LUI...\n");
            breg[rt] = ((int32_t) k16) << 16;
            break;
        case SW:
            printf("Executando SW...\n");
            sw(breg[rs], k16, breg[rt]);
            break;
        case SB:
            printf("Executando SB...\n");
            sb(breg[rs], k16, breg[rt]);
            break;
        case SH:
            printf("Executando SH...\n");
            sh(breg[rs], k16, breg[rt]);
            break;
        case BEQ:
            printf("Executando BEQ...\n");
            printf("Breg[rs] = %x\n", breg[rs]);
            printf("Breg[rt] = %x\n", breg[rt]);
            if(breg[rs] == breg[rt]){
                pc = pc + (k16 << 2);
            }
            break;
        case BNE:
            printf("Executando BNE...\n");
            if(breg[rs] != breg[rt])
                pc = pc + (k16 << 2);
            break;
        case BLEZ:
            printf("Executando BLEZ...\n");
            if(breg[rs] <= 0)
                pc = pc + (k16 << 2);
            break;
        case BGTZ:
            printf("Executando BGTZ...\n");
            if(breg[rs] > 0)
                pc = pc + (k16 << 2);
            break;
        case ADDI:
            printf("Executando ADDI...\n");
            printf("rs: %x\n", rs);
            printf("k16: %x\n", k16);
            breg[rt] = breg[rs] + k16;
            printf("rt: %x\n", rt);
            break;
        case SLTI:
            printf("Executando SLTI...\n");
            if(breg[rs] < k16)
                breg[rt] = 1;
            else
                breg[rt] = 0;
            break;
        case SLTIU:
            printf("Executando SLTIU...\n");
            if(((uint32_t)breg[rs]) < ((uint16_t)k16))
                breg[rt] = 1;
            else
                breg[rt] = 0;
            break;
        case ANDI:
            printf("Executando ANDI...\n");
            breg[rt] = breg[rs] & (((uint32_t)k16 << 16) >> 16);
            break;
        case ORI:
            printf("Executando ORI...\n");
            breg[rt] = breg[rs] | (((uint32_t)k16 << 16) >> 16);
            break;
        case XORI:
            printf("Executando XORI...\n");
            breg[rt] = breg[rs] ^ (((uint32_t)k16 << 16) >> 16);
            break;
        case J:
            printf("Executando J...\n");
            printf("PC: %x\n", pc);
            pc = (pc & 0xF0000000) | (k26 << 2);
            printf("PC: %x\n", pc);
            break;
        case JAL:
            printf("Executando JAL...\n");
            breg[31] = pc + 8;
            pc = (pc & 0xF0000000) | (k26 << 2);
            break;
        default:
            break;
    }
}

void step(){
    fetch();
    decode();
    execute();
}

void run(){
    while(pc < 0x2000){
        step();
        if(saidaaux == 10){
            printf("\nPrograma chegou ao fim\n");
            return;
        }
    }
}

void dump_mem(int start, int end, char format){
    if(format == 'h'){
        printf("\n\nImprimindo mem�ria em hexadecimal...\n\n");
        int i;
        for(i = start; i <= end; i+=4)
            printf("Posicao %d: %x \n", i/4, lw(i, 0));
    }else if(format == 'd'){
        printf("\n\nImprimindo mem�ria em decimal...\n\n");
        int i;
        for(i = start; i <= end; i+=4)
            printf("Posi��o %d: %d \n", start/4, lw(i, 0));
    }else{
        printf("Formato errado; Deve ser 'h' para hexadecimal ou 'd' para decimal.\n");
    }
}

void dump_reg(char format){
    if(format == 'h'){
        printf("\n\nImprimindo banco de registradores em hexadecimal...\n\n");
        int i;
        for(i = 0; i <= MEM_SIZE; i++)
            printf("Posicao %d: %x \n", i, breg[i/4]);
    }else if(format == 'd'){
        printf("\n\nImprimindo banco de registradores em decimal...\n\n");
        int i;
        for(i = 0; i <= MEM_SIZE; i++)
            printf("Posi��o %d: %d \n", i, breg[i/4]);
    }else{
        printf("Formato errado; Deve ser 'h' para hexadecimal ou 'd' para decimal.\n");
    }
}


int main(int argc, const char * argv[]) {
    
    FILE *code_ex, *data_ex, *code_fc, *data_fc;
    code_ex = fopen("code-exemplo.bin", "r");
    data_ex = fopen("data-exemplo.bin", "r");
    code_fc = fopen("code-fibonacci.bin", "r");
    data_fc = fopen("data-fibonacci.bin", "r");
    int i;
    
    for(i = 0; i < MEM_SIZE; i++){ /* Inicializa o array de mem�ria e o banco de registradores */
        mem[i] = 0;
        breg[i] = 0;
    }
    pc = 0;
    
    fread(mem, 4, 2048, code_ex); /* L� os arquivos gerados pelo MARS */
    fread(mem + 2048, 4, 2048, data_ex);
    run();
    
    for(i = 0; i < MEM_SIZE; i++){ /* Reinicializa o array de mem�ria e o banco de registradores*/
        mem[i] = 0;
        breg[i] = 0;
    }
    pc = 0;
    
    fread(mem, 4, 2048, code_fc); /* L� os arquivos gerados pelo MARS */
    fread(mem + 2048, 4, 2048, data_fc);
    run();
    
    fclose(code_ex);
    fclose(data_ex);
    fclose(code_fc);
    fclose(data_fc);
    return 0;
}

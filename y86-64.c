#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

const int MAX_MEM_SIZE  = (1 << 13);

void fetchStage(int *icode, int *ifun, int *rA, int *rB, wordType *valC, wordType *valP) {
  wordType pc = getPC();
  byteType byte = getByteFromMemory(pc);

  *icode = (byte >> 4) & 0xf;
  *ifun = byte & 0xf;

  if(*icode == HALT) { //HALT
    *valP = pc + 1;
    setStatus(STAT_HLT);
  }

  if(*icode == NOP) { //NOP
    *valP = pc + 1;
  }

  if(*icode == RRMOVQ) { //RRMOVQ
    byte = getByteFromMemory(pc + 1);
    *rA = (byte >> 4) & 0xf;
    *rB = byte & 0xf;
    *valP = pc + 2;
  }
  
  //CMOVXX
  
  if(*icode == IRMOVQ || RMMOVQ) { //IRMOVQ and RMMOVQ
    byte = getByteFromMemory(pc + 1);
    *rA = (byte >> 4) & 0xf;
    *rB = byte & 0xf;
    *valC = getWordFromMemory(pc + 2);
    *valP = pc + 10;
  }

  
  //MRMOVQ
  //OPQ (ADD, SUB, XOR, AND)
  //JXX
  //CALL
  //RET
  //PUSHQ
  //POPQ

  else {
    printf("ERROR - icode not implemented: %d", *icode);
  }
}

void decodeStage(int icode, int rA, int rB, wordType *valA, wordType *valB) {
  if(icode == RRMOVQ) { //RRMOVQ
    *valA = getRegister(rA);
  }

  if (icode == RMMOVQ) { //RMMOVQ
    *valA = getRegister(rA);
    *valB = getRegister(rB); 
  }
}

void executeStage(int icode, int ifun, wordType valA, wordType valB, wordType valC, wordType *valE, bool *Cnd) {
  if(icode == IRMOVQ || icode == RRMOVQ) { //IRMOVQ and RRMOVQ
    *valE = 0 + valC;
  }

  if(icode == RMMOVQ) { //RMMOVQ
    *valE = valB+ valC;
  }
}

void memoryStage(int icode, wordType valA, wordType valP, wordType valE, wordType *valM) {
  if(icode == RMMOVQ) { //RMMOVQ
    setWordInMemory(valE, valA); //Todo: check to see if this should be Word or Byte
  }
}

void writebackStage(int icode, wordType rA, wordType rB, wordType valE, wordType valM) {
  if(icode == IRMOVQ || icode == RRMOVQ) {
    setRegister(rB, valE);
  }
}

void pcUpdateStage(int icode, wordType valC, wordType valP, bool Cnd, wordType valM) {
  if(icode == HALT || icode == NOP || icode == IRMOVQ || icode == RRMOVQ || icode == RMMOVQ) {
    setPC(valP); 
  }
}

void stepMachine(int stepMode) {
  /* FETCH STAGE */
  int icode = 0, ifun = 0;
  int rA = 0, rB = 0;
  wordType valC = 0;
  wordType valP = 0;
 
  /* DECODE STAGE */
  wordType valA = 0;
  wordType valB = 0;

  /* EXECUTE STAGE */
  wordType valE = 0;
  bool Cnd = 0;

  /* MEMORY STAGE */
  wordType valM = 0;

  fetchStage(&icode, &ifun, &rA, &rB, &valC, &valP);
  applyStageStepMode(stepMode, "Fetch", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  decodeStage(icode, rA, rB, &valA, &valB);
  applyStageStepMode(stepMode, "Decode", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  executeStage(icode, ifun, valA, valB, valC, &valE, &Cnd);
  applyStageStepMode(stepMode, "Execute", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  memoryStage(icode, valA, valP, valE, &valM);
  applyStageStepMode(stepMode, "Memory", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  writebackStage(icode, rA, rB, valE, valM);
  applyStageStepMode(stepMode, "Writeback", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  pcUpdateStage(icode, valC, valP, Cnd, valM);
  applyStageStepMode(stepMode, "PC update", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  incrementCycleCounter();
}

/** 
 * main
 * */
int main(int argc, char **argv) {
  int stepMode = 0;
  FILE *input = parseCommandLine(argc, argv, &stepMode);

  initializeMemory(MAX_MEM_SIZE);
  initializeRegisters();
  loadMemory(input);

  applyStepMode(stepMode);
  while (getStatus() != STAT_HLT) {
    stepMachine(stepMode);
    applyStepMode(stepMode);
#ifdef DEBUG
    printMachineState();
    printf("\n");
#endif
  }
  printMachineState();
  return 0;
}

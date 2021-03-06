//C interface

#ifndef _OPTTREE_H_
#define _OPTTREE_H_

void* createOPTTree(void);
void readOPTTree(void* tree, char* path);
void removeOPTTree(void* tree);
long findNextInstruction(unsigned long cInst, long pN, void* tree);
#endif

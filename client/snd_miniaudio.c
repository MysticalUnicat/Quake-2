
// snddma_null.c
// all other sound mixing is portable

#include "client.h"
#include "snd_loc.h"

int SNDDMA_Init(void) { return 0; }

int SNDDMA_GetDMAPos(void) { return 0; }

void SNDDMA_Shutdown(void) {}

void SNDDMA_BeginPainting(void) {}

void SNDDMA_Submit(void) {}

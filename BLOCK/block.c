//prgm.c, aleph-bfin
#include "block.h"

//  data types
typedef struct _blockData {
    ModuleData super;

} blockData;

ModuleData *gModuleData;

//  pointer to SDRAM (all external memory)
blockData *data;

static u8 phase;


void module_init(void) {
    data = (blockData*)SDRAM_ADDRESS;
    gModuleData = &(data->super);
    strcpy(gModuleData->name, "aleph-block");
    phase = 0;
}

void module_pulse_in(void) {
    phase = !phase;
}

//  process frame
void module_process_frame(void) {
    if (phase) out[0] = FR32_MAX1_2;
    else out[0] = 0;
}

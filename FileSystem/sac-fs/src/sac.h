#ifndef AUX_SAC_H
#define AUX_SAC_H

#include "block.h"

void validate_input(int argc, char * argv[]);
void set_config    (char* path_config);
void load_bitmap   (t_block*** bitmap_blocks);
void unmap_bitmap  (t_block** bitmap_blocks);

#endif //AUX_SAC_H

#ifndef NVM_H
#define NVM_H

#include "Std_Types.h"

#define NVM_BLOCK_DEM_PRIMARY    (0x01U)
#define NVM_NVRAM_FILE           "dem_nvram.bin"
#define NVM_MAX_BLOCK_SIZE       (256U)

Std_ReturnType NvM_WriteBlock(uint8 blockId, const uint8 *data, uint16 len);
Std_ReturnType NvM_ReadBlock(uint8 blockId, uint8 *data, uint16 len);
void           NvM_Init(void);

#endif /* NVM_H */
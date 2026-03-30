#include "NvM.h"
#include <stdio.h>
#include <string.h>

void NvM_Init(void)
{
    printf("[NvM] Initialized - storage file: %s\n", NVM_NVRAM_FILE);
}

Std_ReturnType NvM_WriteBlock(uint8 blockId, const uint8 *data, uint16 len)
{
    FILE   *f;
    uint8   header[2];

    if (!data || len == 0U || len > NVM_MAX_BLOCK_SIZE) return E_NOT_OK;

    f = fopen(NVM_NVRAM_FILE, "wb");
    if (!f)
    {
        printf("[NvM] ERROR: Cannot open %s for write\n", NVM_NVRAM_FILE);
        return E_NOT_OK;
    }

    /* Write header: blockId + length */
    header[0] = blockId;
    header[1] = (uint8)len;
    fwrite(header, 1, 2, f);
    fwrite(data,   1, len, f);
    fclose(f);

    printf("[NvM] Block 0x%02X written (%d bytes) to %s\n",
           blockId, len, NVM_NVRAM_FILE);
    return E_OK;
}

Std_ReturnType NvM_ReadBlock(uint8 blockId, uint8 *data, uint16 len)
{
    FILE   *f;
    uint8   header[2];
    size_t  read_len;

    if (!data || len == 0U) return E_NOT_OK;

    f = fopen(NVM_NVRAM_FILE, "rb");
    if (!f)
    {
        printf("[NvM] No existing NVRAM file - starting fresh\n");
        return E_NOT_OK;
    }

    read_len = fread(header, 1, 2, f);
    if (read_len != 2 || header[0] != blockId || header[1] > len)
    {
        printf("[NvM] Block ID mismatch or corrupt file\n");
        fclose(f);
        return E_NOT_OK;
    }

    read_len = fread(data, 1, header[1], f);
    fclose(f);

    printf("[NvM] Block 0x%02X restored (%zu bytes) from %s\n",
           blockId, read_len, NVM_NVRAM_FILE);
    return E_OK;
}
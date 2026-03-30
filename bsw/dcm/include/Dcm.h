#ifndef DCM_H
#define DCM_H

#include "Std_Types.h"
#include "Dem.h"
#include "IsoTp.h"

/* UDS Service IDs */
#define UDS_SID_READ_DTC   0x19U
#define UDS_SID_CLEAR_DTC  0x14U
#define UDS_SID_POS_RESP   0x40U
#define UDS_NRC_SNS        0x11U  /* serviceNotSupported */
#define UDS_NRC_IMLOIF     0x22U  /* conditionsNotCorrect */

void           Dcm_Init(void);
void           Dcm_MainFunction(void);

#endif /* DCM_H */
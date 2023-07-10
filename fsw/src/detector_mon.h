/*
**  Copyright 2022 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
**  Purpose:
**    Monitor the detector for faults and take corrective action
**
**  Notes:
**    1. This serves as a solution to the payload manager project coding
**       exercise
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/
#ifndef _detector_mon_
#define _detector_mon_

/*
** Includes
*/

#include "app_cfg.h"
#include "pl_sim_lib.h"  /* See prologue notes */

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define DETECTOR_MON_DETECTED_FAULT_EID  (DETECTOR_MON_BASE_EID + 0)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** FAULT_MON_Class
*/

typedef struct
{

   bool    FaultPresent;
   uint16  ValidDataCnt;
   uint16  InvalidDataCnt;
   uint16  DetectorResetCnt;
   
} DETCTOR_MON_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: DETECTOR_MON_Constructor
**
** Initialize the intrument simulator to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void DETECTOR_MON_Constructor(DETCTOR_MON_Class_t *DetectorMonPtr);


/******************************************************************************
** Function: DETECTOR_MON_ValidData
**
** Check validity of a row of detector data
**
*/
bool DETECTOR_MON_CheckData(PL_SIM_LIB_DetectorRow_t *DetectorRow);


/******************************************************************************
** Function: DETECTOR_MON_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void DETECTOR_MON_ResetStatus(void);


#endif /* _detector_mon_ */

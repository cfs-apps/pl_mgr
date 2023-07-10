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
**    Implement the detector monitor object
**
**  Notes:
**    1. Information events are used in order to trace execution for
**       demonstrations.
**
**  References:
**    1. OpenSatKit Object-based Application Developer's Guide
**    2. cFS Application Developer's Guide
**
*/

/*
** Include Files:
*/

#include <ctype.h>

#include "app_cfg.h"
#include "detector_mon.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define HYSTERESIS_LIM  3


/**********************/
/** Global File Data **/
/**********************/

static DETCTOR_MON_Class_t *DetectorMon = NULL;


/*******************************/
/** Local Function Prototypes **/
/*******************************/


/******************************************************************************
** Function: DETECTOR_MON_Constructor
**
** Initialize the detector monitor to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void DETECTOR_MON_Constructor(DETCTOR_MON_Class_t *DetectorMonPtr)
{

   DetectorMon = DetectorMonPtr;
   
   DetectorMon->FaultPresent   = false;
   DetectorMon->ValidDataCnt   = 0;
   DetectorMon->InvalidDataCnt = 0;

} /* End DETECTOR_MON_Constructor() */



/******************************************************************************
** Function: DETECTOR_MON_CheckData
**
** Simple data validity check
**
** Notes:
**   1. This is the default data validity function that is replaced by
**      PL_MGR project exercise 1.
**
*/
bool DETECTOR_MON_CheckData(PL_SIM_LIB_DetectorRow_t *DetectorRow)
{
   // Simulation puts a character in first byte when a fault is present
   DetectorMon->FaultPresent = isalpha(DetectorRow->Data[0]); 

   return !DetectorMon->FaultPresent;
   
} /* End DETECTOR_MON_CheckData() */


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
void DETECTOR_MON_ResetStatus(void)
{

   DetectorMon->DetectorResetCnt = 0;

} /* End DETECTOR_MON_ResetStatus() */


/******************************************************************************
** Function: DETECTOR_MON_Exercise1
**
** A solution to PL_MAGR project exercise 1 to check the validity of 
** the detcor data and take corrective action.
**
** Notes:
**   1. This is a simple solution that relies on the detector reset to correct
**      the fault. Additional logic could be added that limits the number of
**      resets if the fault persists and the final action could be to power off
**      the payload.  
**   2. In a more complex system, the corrective action may be separated from
**      the fault detection because the action taken may need to consider the
**      state of other system components.
**
*/
bool DETECTOR_MON_Exercise1(PL_SIM_LIB_DetectorRow_t *DetectorRow)
{
   
   bool ValidData;
   
   // Simulation puts a character in first byte when a fault is present
   ValidData = !isalpha(DetectorRow->Data[0]); 


   if (ValidData)
   {
      DetectorMon->ValidDataCnt++;
      if (DetectorMon->ValidDataCnt >= HYSTERESIS_LIM)
      {
         DetectorMon->FaultPresent   = false;         
         DetectorMon->InvalidDataCnt = 0;
      }
   }
   else
   {
      DetectorMon->InvalidDataCnt++;
      if (DetectorMon->InvalidDataCnt >= HYSTERESIS_LIM)
      {
         DetectorMon->FaultPresent = true;
         CFE_EVS_SendEvent (DETECTOR_MON_DETECTED_FAULT_EID, CFE_EVS_EventType_ERROR,
                            "Detector fault persisted for %d cycles, sent detector reset",
                            DetectorMon->InvalidDataCnt);
         PL_SIM_LIB_DetectorReset();
         DetectorMon->DetectorResetCnt++;
         // No science data is produced while the detector is resetting so clearing
         // the counter won't cause this fault check to trip again
         DetectorMon->InvalidDataCnt = 0;
         
      }
   }

   return ValidData;
   
} /* End DETECTOR_MON_Exercise1() */


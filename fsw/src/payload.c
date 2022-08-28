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
**    Implement the payload management object
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

#include <string.h>

#include "app_cfg.h"
#include "payload.h"


/**********************/
/** Global File Data **/
/**********************/

static PAYLOAD_Class_t *Payload = NULL;


/*******************************/
/** Local Function Prototypes **/
/*******************************/


/******************************************************************************
** Function: PAYLOAD_Constructor
**
*/ 
void PAYLOAD_Constructor(PAYLOAD_Class_t *PayloadPtr, INITBL_Class_t *IniTbl)
{
 
   Payload = PayloadPtr;

   CFE_PSP_MemSet((void*)Payload, 0, sizeof(PAYLOAD_Class_t));
   
   Payload->CurrPower = PL_SIM_LIB_POWER_OFF;
   Payload->PrevPower = PL_SIM_LIB_POWER_OFF;
   
   SCI_FILE_Constructor(&Payload->SciFile, IniTbl);
         
} /* End PAYLOAD_Constructor() */


/******************************************************************************
** Functions: PAYLOAD_ManageData
**
** Reading detector data manage science files
**
** Notes:
**   1. This function is called every PL_MGR 'execution cycle' regardless of
**      the power and payload state.
**
*/
void PAYLOAD_ManageData(void)
{

   Payload->CurrPower = PL_SIM_LIB_ReadPowerState();
   
   if (Payload->CurrPower == PL_SIM_LIB_POWER_READY)
   {   
      PL_SIM_LIB_ReadDetector(&Payload->Detector);

      /* Simulation puts a character in first byte when a fault is present */
      Payload->DetectorFault = isalpha(Payload->Detector.Row.Data[0]);
         
      if (Payload->Detector.ReadoutRow == 0)
         SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_FIRST_ROW);
      else if (Payload->Detector.ReadoutRow >= (PL_SIM_LIB_DETECTOR_ROWS_PER_IMAGE-1))
         SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_LAST_ROW);
      else
         SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_ROW);
   }
   else
   {
      /* Check whether transitioned from READY to non-READY state */
      if (Payload->PrevPower == PL_SIM_LIB_POWER_READY)
      {
         if (Payload->SciFile.State == SCI_FILE_ENABLED)
         {
            CFE_EVS_SendEvent(PAYLOAD_SHUTDOWN_SCI_EID, CFE_EVS_EventType_ERROR, 
                              "Terminating science data collection. Payload power transitioned from %s to %s",
                              PL_SIM_LIB_GetPowerStateStr(Payload->PrevPower),
                              PL_SIM_LIB_GetPowerStateStr(Payload->CurrPower));
            SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_SHUTDOWN);
         }
      }
   }
   
   Payload->PrevPower = Payload->CurrPower;

} /* End PAYLOAD_ManageData() */


/******************************************************************************
** Function:  PAYLOAD_ResetStatus
**
*/
void PAYLOAD_ResetStatus(void)
{

   /* All state data managed by the science start and stop commands */
   
} /* End PAYLOAD_ResetStatus() */


/******************************************************************************
** Functions: PAYLOAD_StartSciCmd
**
** Start collecting and saving payload detector data to a file.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
bool PAYLOAD_StartSciCmd (void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = false;
   
   if (Payload->CurrPower == PL_SIM_LIB_POWER_READY)
   {
      
      if (SCI_FILE_Start() == true)
      {

         CFE_EVS_SendEvent (PAYLOAD_START_SCI_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                            "Start science data collection accepted");
      
         RetStatus = true;
      }
   }  
   else
   { 
   
      CFE_EVS_SendEvent (PAYLOAD_START_SCI_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                         "Start science data collection rejected. Payload in %s state and not the READY power state",
                        PL_SIM_LIB_GetPowerStateStr(Payload->CurrPower));
   
   }
   
   return RetStatus;

} /* End PAYLOAD_StartSciCmd() */



/******************************************************************************
** Functions: PAYLOAD_StopSciCmd
**
** Stop collecting and saving payload detector data to a file.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
bool PAYLOAD_StopSciCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   char EventStr[132];
   
   SCI_FILE_Stop(EventStr, 132);
   
   CFE_EVS_SendEvent (PAYLOAD_STOP_SCI_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                      "%s", EventStr);
                            
   return true;

} /* End PAYLOAD_StopSciCmd() */



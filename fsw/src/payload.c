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

//#include <string.h>

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
   
   Payload->PowerState     = PL_SIM_LIB_Power_OFF;
   Payload->PrevPowerState = PL_SIM_LIB_Power_OFF;
   
   SCI_FILE_Constructor(&Payload->SciFile, IniTbl);
   DETECTOR_MON_Constructor(&Payload->DetectorMon);
   
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

   Payload->PowerState = PL_SIM_LIB_ReadPowerState();
   
   if (Payload->PowerState == PL_SIM_LIB_Power_READY)
   {   
      if (PL_SIM_LIB_ReadDetector(&Payload->Detector))
      {

         DETECTOR_MON_CheckData(&Payload->Detector.Row); // EX1: Replace
         // EX1: DETECTOR_MON_CheckDataEx1(&Payload->Detector.Row);
           
         if (Payload->Detector.ReadoutRow == 0)
            SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_FIRST_ROW);
         else if (Payload->Detector.ReadoutRow >= (PL_SIM_LIB_DETECTOR_ROWS_PER_IMAGE-1))
            SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_LAST_ROW);
         else
            SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_ROW);
      
      } /* End if read data */
   }
   else
   {
      /* Check whether transitioned from READY to non-READY state */
      if (Payload->PrevPowerState == PL_SIM_LIB_Power_READY)
      {
         if (Payload->SciFile.State == SCI_FILE_ENABLED)
         {
            CFE_EVS_SendEvent(PAYLOAD_SHUTDOWN_SCI_EID, CFE_EVS_EventType_ERROR, 
                              "Terminating science data collection. Payload power transitioned from %s to %s",
                              PL_SIM_LIB_GetPowerStateStr(Payload->PrevPowerState),
                              PL_SIM_LIB_GetPowerStateStr(Payload->PowerState));
            SCI_FILE_WriteDetectorData(&Payload->Detector, SCI_FILE_SHUTDOWN);
         }
      }
   }
   
   Payload->PrevPowerState = Payload->PowerState;

} /* End PAYLOAD_ManageData() */


/******************************************************************************
** Functions: PAYLOAD_ResetDetectorCmd
**
** Initiate a detector reset. The PL_SIM_ExecuteStep() method defines how the
** simulator responds to a reset.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
bool PAYLOAD_ResetDetectorCmd (void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = false;

   if (Payload->PowerState == PL_SIM_LIB_Power_READY)
   {
      PL_SIM_LIB_DetectorReset();
      RetStatus = true;
   
   }  
   else
   { 
      CFE_EVS_SendEvent (PAYLOAD_RESET_DETECTOR_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                         "Resetcommand cmd rejected. Payload must be in power READY state and it's in the %s state.",
                         PL_SIM_LIB_GetPowerStateStr(Payload->PowerState));
   }
   
   return RetStatus;

} /* End PAYLOAD_ResetDetectorCmd() */


/******************************************************************************
** Function:  PAYLOAD_ResetStatus
**
** Notes:
**   1. All PAYLOAD state data is managed by commands so nothing to do except
**      call owned objects
** 
*/
void PAYLOAD_ResetStatus(void)
{

   DETECTOR_MON_ResetStatus();

      
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
   
   if (Payload->PowerState == PL_SIM_LIB_Power_READY)
   {
      
      PL_SIM_LIB_DetectorOn();      
      
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
                        PL_SIM_LIB_GetPowerStateStr(Payload->PowerState));
   
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
   
   PL_SIM_LIB_DetectorOff();
   SCI_FILE_Stop(EventStr, 132);
   
   CFE_EVS_SendEvent (PAYLOAD_STOP_SCI_CMD_EID, CFE_EVS_EventType_INFORMATION, 
                      "%s", EventStr);
                            
   return true;

} /* End PAYLOAD_StopSciCmd() */



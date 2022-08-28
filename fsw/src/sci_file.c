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
**    Implement the science file management object
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
#include "sci_file.h"


/**********************/
/** Global File Data **/
/**********************/

static SCI_FILE_Class_t*  SciFile = NULL;


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void InitFileState(void);
static void CreateCntFilename(uint16 ImageId);
static bool CreateFile(uint16 ImageId);
static void CloseFile(void);
static bool WriteDetectorRow(PL_SIM_LIB_DetectorRow_t *DetectorRow);


/******************************************************************************
** Function: SCI_FILE_Constructor
**
*/
void SCI_FILE_Constructor(SCI_FILE_Class_t *SciFilePtr, INITBL_Class_t *IniTbl)
{
 
   SciFile = SciFilePtr;

   CFE_PSP_MemSet((void*)SciFile, 0, sizeof(SCI_FILE_Class_t));
    
   /* Load initialization configurations */
   
   SciFile->Config.ImagesPerFile = INITBL_GetIntConfig(IniTbl, CFG_SCI_FILE_IMAGE_CNT);
   strncpy(SciFile->Config.PathBaseFilename,
           INITBL_GetStrConfig(IniTbl, CFG_SCI_FILE_PATH_BASE),
           OS_MAX_PATH_LEN);
   strncpy(SciFile->Config.FileExtension,
           INITBL_GetStrConfig(IniTbl, CFG_SCI_FILE_EXTENSION),
           SCI_FILE_EXT_MAX_CHAR);

   /* Initialize to a known state. Call after config parameters in case they're used */
   InitFileState();

} /* End SCI_FILE_Constructor() */


/******************************************************************************
** Functions: SCI_FILE_ConfigCmd
**
** Set configuration parameters for managing science files 
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**  2. TODO: Add error checks
**  3. TODO: PathBaseFilename max len must be less than OS_MAX_PATH_LEN
**           rest of filename and extension.
**
*/
bool SCI_FILE_ConfigCmd(void* DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   const  SCI_FILE_Config_t *ConfigCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, SCI_FILE_ConfigCmdMsg_t);
   
   SciFile->Config.ImagesPerFile = ConfigCmd->ImagesPerFile;
   
   strncpy(SciFile->Config.PathBaseFilename, ConfigCmd->PathBaseFilename, OS_MAX_PATH_LEN);
   SciFile->Config.PathBaseFilename[OS_MAX_PATH_LEN-1] = '\0';
   
   strncpy(SciFile->Config.FileExtension, ConfigCmd->FileExtension, SCI_FILE_EXT_MAX_CHAR);
   SciFile->Config.FileExtension[SCI_FILE_EXT_MAX_CHAR-1] = '\0';

   return true;
   
} /* End SCI_FILE_ConfigCmd() */


/******************************************************************************
** Function:  SCI_FILE_ResetStatus
**
*/
void SCI_FILE_ResetStatus(void)
{

   /* No state data should be changed */
   
} /* End SCI_FILE_ResetStatus() */


/******************************************************************************
** Functions: SCI_FILE_Start
**
** Start the science data collection process 
**
** Notes:
**   1. See SCI_FILE_WriteDetectorData() prologue for file creation and
**      detector image synchronization.
**
*/
bool SCI_FILE_Start(void)
{
   
   SciFile->State = SCI_FILE_ENABLED;
   SciFile->CreateNewFile = true;
   
   return true;

} /* End SciFile_Start() */


/******************************************************************************
** Functions: SCI_FILE_Stop
**
** Stop the science data collection process 
**
** Notes:
**   1. Always return true. Receiving the command even if sceince not in
**      progress allows automated cleanup procedures to execute without
**      generating errors.
**
*/
bool SCI_FILE_Stop(char *EventStr, uint16 MaxStrLen)
{
  
   if (SciFile->State == SCI_FILE_DISABLED)
   {
   
      strncpy(EventStr, "Stop science received with no science data collection in progress", MaxStrLen);
   
   }
   else
   {
      
      CloseFile();
      strncpy(EventStr, "Sucessfully stopped science", MaxStrLen);
   
   } /* End if science enabled */
      
   InitFileState();
         
   return true;
   
} /* End SciFile_Stop() */


/******************************************************************************
** Functions: SCI_FILE_WriteDetectorData
**
** Manage writing detector data to files
**
** Notes:
**   1. The Control input allows the caller to force a shutdown regardless
**      of the science data file processing state. The Detector data is
**      ignored and the science data file is closed. 
**   2. There is no synchronization between the start of a detector read out 
**      and the first file created after the initial start of writing science
**      data to a file. Therefore the first file after science collection is
**      started could have partial data. 
**
*/
void SCI_FILE_WriteDetectorData(PL_SIM_LIB_Detector_t *Detector, SCI_FILE_Control_t Control)
{

   bool SaveDetectorRow = true; 
   
   if (Control == SCI_FILE_SHUTDOWN)
   {
      CloseFile();
      InitFileState();
   }
   else
   {
      if (SciFile->State == SCI_FILE_ENABLED)     
      {

         if (SciFile->CreateNewFile)
         {
         
            /* Wait for first row before creating a file */
            if (Control == SCI_FILE_FIRST_ROW)
            {
               CreateFile(Detector->ImageCnt);
               SciFile->CreateNewFile = false;
            }
            else
            {
               SaveDetectorRow = false;
            }
         }

         if (SaveDetectorRow) WriteDetectorRow(&Detector->Row);         
         
         if (Control == SCI_FILE_LAST_ROW)
         {
            SciFile->ImageCnt++;
            if (SciFile->ImageCnt >= SciFile->Config.ImagesPerFile)
            {
               CloseFile();
               SciFile->CreateNewFile = true;
            }
            
         } /* End if SCI_FILE_SAVE_LAST_ROW */
      } /* End if SCI_FILE_ENABLED */
   }
   
} /* End SciFile_WriteDetectorData() */


/******************************************************************************
** Functions: CloseFile
**
** Close the current science file.
**
** Notes:
**   None
*/
static void CloseFile(void)
{
 
   if (SciFile->IsOpen)
   {
      
      OS_close(SciFile->Handle);
      
      CFE_EVS_SendEvent (SCI_FILE_CLOSE_EID, CFE_EVS_EventType_INFORMATION, 
                         "Closed science file %s",SciFile->Name);         
      
      SciFile->IsOpen = false;
      strcpy(SciFile->Name, SCI_FILE_UNDEF_FILE);

   }

} /* End SciFile_Close() */


/******************************************************************************
** Functions: CreateCntFilename
**
** Create a filename using the table-defined base path/filename, current image
** ID, and the table-defined extension. 
**
** Notes:
**   1. No string buffer error checking performed
*/
static void CreateCntFilename(uint16 ImageId)
{
   
   int i;
   
   char ImageIdStr[64];

   sprintf(ImageIdStr,"%03d",ImageId);

   strcpy (SciFile->Name, SciFile->Config.PathBaseFilename);

   i = strlen(SciFile->Name);  /* Starting position for image ID */
   strcat (&(SciFile->Name[i]), ImageIdStr);
   
   i = strlen(SciFile->Name);  /* Starting position for extension */
   strcat (&(SciFile->Name[i]), SciFile->Config.FileExtension);
   
} /* End CreateCntFilename() */


/******************************************************************************
** Functions: CreateFile
**
** Create a new science file using the ImageId in the filename
**
** Notes:
**   None
*/
static bool CreateFile(uint16 ImageId)
{

   bool          RetStatus = false;
   int32         SysStatus;
   os_err_name_t OsErrStr; 
   
   if (SciFile->IsOpen)
   {
      
      CFE_EVS_SendEvent (SCI_FILE_CREATE_ERR_EID, CFE_EVS_EventType_ERROR, 
                         "Create science file failed due to a file already being open: %s", SciFile->Name);         
   
   }
   else
   {
   
      CreateCntFilename(ImageId);
      
      SysStatus = OS_OpenCreate(&SciFile->Handle, SciFile->Name, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_READ_WRITE);
      
      if (SysStatus == OS_SUCCESS)
      {
      
         RetStatus = true;
         SciFile->ImageCnt = 0;
         SciFile->IsOpen = true;
         CFE_EVS_SendEvent (SCI_FILE_CREATE_EID, CFE_EVS_EventType_INFORMATION, 
                            "New science file created: %s",SciFile->Name);         

      }
      else
      {
         
         OS_GetErrorName(SysStatus, &OsErrStr);
         CFE_EVS_SendEvent (SCI_FILE_CREATE_ERR_EID, CFE_EVS_EventType_ERROR, 
                            "Error creating new science file %s. Return status %s",
                            SciFile->Name, OsErrStr);         
      
      }
   } /* End if no file currently open */
            
   return RetStatus;
   
} /* End CreateFile() */


/******************************************************************************
** Functions: InitFileState
**
** Initialize the SciFile object to a known state.
**
** Notess:
**   None
*/
static void InitFileState(void)
{
   
   SciFile->CreateNewFile = false;
   SciFile->State    = SCI_FILE_DISABLED;
   SciFile->Handle   = 0;
   SciFile->IsOpen   = false;
   SciFile->ImageCnt = 0;
   strcpy(SciFile->Name, SCI_FILE_UNDEF_FILE);
   
} /* End InitFileState() */


/******************************************************************************
** Functions: WriteDetectorRow
**
** Write a detector row to the current science file
**
** Notes:
**   None
*/
static bool WriteDetectorRow(PL_SIM_LIB_DetectorRow_t *DetectorRow)
{
   
   int32 WriteStatus = 0;
   bool  RetStatus = false;
   
   if (SciFile->IsOpen)
   {
     
      WriteStatus = OS_write(SciFile->Handle, DetectorRow->Data, strlen(DetectorRow->Data));
      
      RetStatus = (WriteStatus > 0);
        
   } /* End file open */

   if (RetStatus == false)
   {
   
      CFE_EVS_SendEvent (SCI_FILE_WRITE_ERR_EID, CFE_EVS_EventType_ERROR, 
                         "Error writing to science file %s. IsOpen=%d, WriteStatus=%d",
                         SciFile->Name, SciFile->IsOpen, WriteStatus);

   }
   
   return RetStatus;
   
} /* End WriteDetectorRow() */



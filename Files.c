/********************************************************************
 * COPYRIGHT -- Microsoft
 ********************************************************************
 * Program: Files
 * File: Files.c
 * Author: palmerk
 * Created: October 07, 2014
 ********************************************************************
 * Implementation of program Files
 ********************************************************************/

#include <bur/plctypes.h>
#include <fileio.h>
#include <string.h>

#ifdef _DEFAULT_INCLUDES
	#include <AsDefault.h>
#endif

_GLOBAL char *pch;
_GLOBAL UDINT fileOffset _VAR_INIT (0);
 char bOK;
_GLOBAL USINT byStep;
_GLOBAL USINT byErrorLevel;
 //_GLOBAL USINT byReadData[100], WriteData[100];
_GLOBAL STRING byReadData[1000], WriteData[1000];
 UINT Status, wError;
 UDINT dwIdent;
_GLOBAL STRING floppy[25][1000];
//_GLOBAL char floppy[2000]; 
  FileOpen_typ FOpen _VAR_INIT (0);
_LOCAL FileClose_typ FClose;
_LOCAL FileCreate_typ FCreate;
 FileRead_typ FRead _VAR_INIT (0);
_LOCAL FileWrite_typ FWrite;
_LOCAL FileDelete_typ FDelete;
_GLOBAL SINT ii _VAR_INIT (0);

void _INIT FilesInit(void)
{
	int i;
	/* Initialize variables */
	bOK = 0;
	byStep = 1;
	byErrorLevel = 0;
	/* Initialize read and write data */
	for (i = 0; i < 100; i ++)
	{
		WriteData[i]  = i + 1;
		byReadData[i] = 0;
	}

}

void _CYCLIC FilesCyclic(void)
{
	
	
	switch (byStep)
	{
		case 0: 
			
			/**** Error step ****/
			bOK = 0;
			break;
		
		case 1: 
			
			// Try to open existing file 
			// Initialize file open structrue 
			FOpen.enable      = 1;
			FOpen.pDevice   = (UDINT) "HD0";
			FOpen.pFile     = (UDINT) "testfile.txt";
			FOpen.mode      = fiREAD_ONLY;                        /* Read and write access */

			/* Call FUB */
			FileOpen(&FOpen);

			/* Get FBK output information */
			dwIdent = FOpen.ident;
			Status = FOpen.status;
			
			/* Verify status (20708 -> File doesn't exist) */
			if (Status == 20708)
			{
				byStep = 2;
			}
				//OK NO Error
			else if (Status == 0)
			{
				byStep = 3;
			}
				//65535 == FB is busy and still working
				//20798 == error in device manager
			else if (Status != 65535)
			{
				byErrorLevel = 1;
				byStep = 0;
			
				//system error
				if (Status == 20799)
				{
					wError = FileIoGetSysError();
				}
			}
			
			break;
		
		case 2:
		
			break;
		
		case 3:
			
			FRead.enable    = 1;
			FRead.ident     = dwIdent;//file to be read
			FRead.offset    = fileOffset;//where to start reading
		//	FRead.pDest     = (UDINT) &byReadData[0];
			FRead.pDest     = (UDINT) &byReadData;//target address where to copy data
			FRead.len       = sizeof (byReadData);
			
			/* Call FBK */
			FileRead(&FRead);
			
			/* Get status */
			Status = FRead.status;
			
			/* Verify status */
			if (Status == 0)
			{
				byStep = 5;
				pch    = strtok(byReadData, "\r\n");//carriage return then new line feed
			}
			else if (Status != 65535)
			{
				byErrorLevel = 4;
				byStep = 0;
			
				if (Status == 20799)
				{
					wError = FileIoGetSysError();
				}
			}
           
			break;
		
		case 4:
		
			break;
		
		case 5:
		
			
			if (pch != NULL)
			{
				strcpy(floppy[ii], pch);
				pch = strtok(NULL, "\r\n");//carriage return then new line feed
				ii++;
			}
			else
			{
				byStep = 6;
			}
			//FRead.enable = 0;
			//fileOffset   = strlen (byReadData);
			//strcpy(floppy, byReadData);
			//memcpy(byReadData, 0, fileOffset);
			//ii ++;
			//
			
			break;
		
		case 6:
		
			break;
	}

}

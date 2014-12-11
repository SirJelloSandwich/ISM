/********************************************************************
 * COPYRIGHT -- Microsoft
 ********************************************************************
 * Program: Torque
 * File: Torque.c
 * Author: palmerk
 * Created: August 20, 2014
 ********************************************************************
 * Implementation of program Torque
 ********************************************************************/

#include <bur/plctypes.h>

#ifdef _DEFAULT_INCLUDES
#include <AsDefault.h>
#endif


void _INIT TorqueInit(void)
{
	
	MC_BR_TorqueControl_0.Axis = (UDINT)(&gTorque.drive.ACP10AXIS);

	gTorque.drive.ServoDrv.AxisObj = (UDINT)&gTorque.drive.ACP10AXIS;
	
	general.state = INIT;
	
	gTorque.drive.ServoDrv.Acceleration = 50000.0;
	gTorque.drive.ServoDrv.Deceleration = 50000.0;
	gTorque.drive.ServoDrv.Velocity     = 5000.0;
	
}

void _CYCLIC TorqueCyclic(void)
{
	switch (general.state)
	{
	
		/********************STATE_READY*********************/
		case INIT :
			
			gTorque.drive.ServoDrv.POWER = 1;
			
			if(gTorque.drive.ServoDrv.DriveStatus.DriveEnable)
			{
				general.state = HOME;
			}
			
		case HOME:
		
			gTorque.drive.ServoDrv.HOME 	    = 1;
			gTorque.drive.ServoDrv.HomePosition = 0.0;
			gTorque.drive.ServoDrv.HomeMode     = mcHOME_ABSOLUTE_CORR; 
			general.state                       = HOME_DONE;
							
			break;
			
		case HOME_DONE:
			
			if(!(gTorque.drive.ServoDrv.DriveStatus.HomingOk))
			{
				general.state = HOME;
			}
			else if(gTorque.drive.ServoDrv.DriveStatus.HomingOk )
			{
				gTorque.drive.ServoDrv.POWER = 0;
				general.state = READY;
			}
						
			break;
			
		case READY:
						
			if (gHmi.key.tgl.WindEnableTgl)//(general.cmdActivate)
			{				
			
				gTorque.drive.ServoDrv.POWER = 1;
			
				if(!gTorque.drive.ServoDrv.Disabled) 
				{
					gHmi.key.tgl.WindEnableTgl = 0;
					general.state              = EXECUTE_FB;
				}
							
			}
			else if (general.cmdDisable)
			{ 
				general.cmdDisable = 0;
				general.state      = STOP_FB;
			}
			
			
			break;    
 
		/*********************************************/
		case EXECUTE_FB:
				
			MC_BR_TorqueControl_0.Enable         =1;
			MC_BR_TorqueControl_0.StartSignal    =1;    /* start with enable signal */
			MC_BR_TorqueControl_0.Torque         =(gUserInputWind[0] / 10.0);
			MC_BR_TorqueControl_0.TorqueRamp     =0.5;
			MC_BR_TorqueControl_0.PosMaxVelocity =20000.0;
			MC_BR_TorqueControl_0.NegMaxVelocity =-3000.0;
			MC_BR_TorqueControl_0.Acceleration   =50000.0;
			MC_BR_TorqueControl_0.TimeLimit      =3600.0;    /* Switch off after 3s if no load */
 
			MC_BR_TorqueControl_0.Mode= mcTIME_LIMIT;

		 	if (MC_BR_TorqueControl_0.InTorque)
			{ 
				MC_BR_TorqueControl_0.InitData = 0;
				general.state = READY;
			}
			else if (MC_BR_TorqueControl_0.Error) 	
			{  
				general.axis1ErrorID               = MC_BR_TorqueControl_0.ErrorID;
				MC_BR_TorqueControl_0.Enable = 0;
				MC_BR_TorqueControl_0.StartSignal =0;    /* start with enable signal */

				general.state                      = ERROR;
			}
			
			break;  
		      
		case STOP_FB:
			
			MC_BR_TorqueControl_0.Enable=0;
        
			if (MC_BR_TorqueControl_0.Active == 0)
			{
				MC_BR_TorqueControl_0.StartSignal=0;
				gTorque.drive.ServoDrv.POWER = 0;
				general.state = READY;
			}
			
			if (MC_BR_TorqueControl_0.Error) 	
				
			{  
				general.axis1ErrorID               = MC_BR_TorqueControl_0.ErrorID;
				MC_BR_TorqueControl_0.Enable = 0;
				MC_BR_TorqueControl_0.StartSignal =0;    /* start with enable signal */

				general.state                      = ERROR;
			}
			
			break; 
		
		case OFF_STOP:
			
			if(!gTorque.drive.ServoDrv.DriveStatus.DriveEnable)
			{
				general.state = LIMBO;
			}
			
			break;
		
		case ERROR:
			
					
			break;
		
		case LIMBO:
		
			break;
		/*********************************************/

	}  
	MC_BR_TorqueControl(&MC_BR_TorqueControl_0);
	
	ServoDrv(&gTorque.drive.ServoDrv);
	
	if (gUserInputWindOld[0] != gUserInputWind[0])
	{	
		MC_BR_TorqueControl_0.InitData       = 1;
		MC_BR_TorqueControl_0.Torque         =(gUserInputWind[0] / 10.0);
						
		gUserInputWindOld[0] = gUserInputWind[0] ;
	
	}
	
	if(MC_BR_TorqueControl_0.DataInitialized)
	{
		MC_BR_TorqueControl_0.InitData       = 0;
	}
 
}  

 /********************************************************************
 * COPYRIGHT --  
 ********************************************************************
 * Program: GAU18
 * File: GAU18.c
 * Author: ISM
 * Created: November 07, 2013

 ********************************************************************
 * Implementation of program MiniGunCtrl
 ********************************************************************/

#include <bur/plctypes.h>
#include "../../ISM_lib/magicnum.h"
#include "../../ISM_lib/Load_Constants.h"


#ifdef _DEFAULT_INCLUDES
	#include <AsDefault.h>
#endif

 static void GAU18_UpdateFire()//gWeaponAxisLeft
{

		
	InVar.bGAU18SafetyOff 		 = RawDigital.bRawGunSafety1;//These three vars are mapped to DI's in IO_Handler.c
	InVar.bGAU18LeftTrigger 	 = RawDigital.bRawGunTrigger3;
	InVar.bGAU18RightTrigger	 = RawDigital.bRawGunTrigger4;
		
	if (udpStatusFireReady[2] = (InVar.bGAU18SafetyOff && InVar.fire_Enable[0]) )// If the user switches the safety off on the gun and the instructor sends via UDP a fire enable flag
	{
		InVar.iGAU18FireEnable = (InVar.bGAU18LeftTrigger || InVar.bGAU18RightTrigger);//stores the trigger pulls
	}
	else
	{
		InVar.iGAU18FireEnable   = OFF;
		OutVar.bEnableIndicator  = OFF;// off when firing is not allowed
	}
			
	if(InVar.iGAU18FireEnable && !InVar.iGAU18FireEnableOLD)//if the trigger is pulled and previously it was not pulled.
	{
		OutVar.bEnableIndicator  = ON;//turn light on
		GAU18.state              = RAMP_UP;//advances the state to OSCILLATE the barrel
	}
	else if (InVar.iGAU18FireEnableOLD && !InVar.iGAU18FireEnable)//if the trigger is not pulled and previously it was pulled.
	{
		GAU18.state = RAMP_DOWN;//moves the state down to have the barrel return to zero position
	}
	
	if (gWeaponAxisLeft.ServoDrv.ErrorID)// if theres an error
	{
		gWeaponAxisLeft.ServoDrv.ErrorAcknowledge = 1;// clear it
	}
	
	systemError[0] = gWeaponAxisLeft.ServoDrv.ErrorID > 0 ? 1: 0;//stores the error to send via UDP
			
	switch (GAU18.state)
	{
		case IDLE:
				
			GAU18.state = DRIVE_ON;
				
			break;
			
		case DRIVE_ON:
				
			gWeaponAxisLeft.ServoDrv.POWER = 1;
				
			if(gWeaponAxisLeft.ServoDrv.DriveStatus.DriveEnable)//if the power on was successful
			{
				GAU18.state = DRIVE_HOME;
			}
				
			break;
			
		case DRIVE_HOME:
				
			gWeaponAxisLeft.ServoDrv.HOME     = 1; //start homing.  see ServoDrv.st
			gWeaponAxisLeft.ServoDrv.HomeMode = ncDIRECT; //type of homing
			GAU18.state                       = DRIVE_HOME_DONE;
				
			break;
			
		case DRIVE_HOME_DONE:
				
			if(gWeaponAxisLeft.ServoDrv.DriveStatus.HomingOk)// if the drive reports back a successful homing
			{
				GAU18.state = DRIVE_READY;
			}
				
			break;
			
		case DRIVE_READY://waitng for trigger pull state... machine sits in this state and waits.
				
			break;
			
		case RAMP_UP:
			
			gWeaponAxisLeft.ServoDrv.Acceleration = 500000.0;
			gWeaponAxisLeft.ServoDrv.Deceleration = 500000.0;
			gWeaponAxisLeft.ServoDrv.Velocity     = 50000.0;
			gWeaponAxisLeft.ServoDrv.Position     = GAU18.iRampTopPos;// 3000
			gWeaponAxisLeft.ServoDrv.Direction    = 0;//positive direction
			gWeaponAxisLeft.ServoDrv.MOVEABSOLUTE = TRUE;//command a movement
			GAU18.state                           = RAMP_UP_WAIT;
				
			break;
			
		case RAMP_UP_WAIT:
				
			if(gWeaponAxisLeft.ServoDrv.ActPosition == GAU18.iRampTopPos)// if the motor has moved to where we requested
			{					
				gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.Enable                          = 1;
				gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.InterpolationMode               = mcIPL_LINEAR;
				gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.AdvancedParameters.Acceleration = 2000000.0;
				gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.AdvancedParameters.Deceleration = 2000000.0;
				
				if (gAmmo.FireRate50cal != 0)// this allows the user through the GUI to input the RPM's they desire
				{
					gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.AdvancedParameters.Velocity     = 100000.0 * (gAmmo.FireRate50cal / 750);//If the user enters a RPM then this will determine the motor speed
				}
				GAU18.currentCycleTimeSec                                                   = 0.000001 * rt_info.cycle_time; //cycle time from CPU.sw converted from us to sec
				GAU18.Et                                                                    = 0.0;//initializes to zero
				GAU18.omega                                                                 = (K_TWO_PI_DP * GAU18.iFreq);// 2pi * hertz
				GAU18.oStart                                                                = (GAU18.iRampTopPos - GAU18.iAmp);//top pos = 3000 - amplitude
				GAU18.state                                                                 = OSCILLATE;
			}
				
			break;
				 
		case OSCILLATE:
				
			GAU18.newPos                                                     = (GAU18.iAmp * cos(GAU18.omega * GAU18.Et)) ;//constatnly generates a new motor position
			gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.CyclicPosition.Real  = GAU18.oStart +  GAU18.newPos ;//comands the motor to that pos.
			GAU18.Et += GAU18.currentCycleTimeSec;
			charged[0] = 0;// this resets the capture of the charging handle being pulled.
			break;
				
		case RAMP_DOWN:
				
			gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.Enable = 0;//stop the oscillation
				
			if(gWeaponAxisLeft.ServoDrv.StandStill)// is in openPLC standstill state
			{
				GAU18.state = RAMP_DOWN_EXT;		
			}
				
			break;
			
		case RAMP_DOWN_EXT:
			
				
			gWeaponAxisLeft.ServoDrv.Position     = 0.0;//go to zero pos which is the front of very front of the gun
			gWeaponAxisLeft.ServoDrv.Direction    = 1;// direction
			gWeaponAxisLeft.ServoDrv.MOVEABSOLUTE = TRUE;	//command
				
			if(gWeaponAxisLeft.ServoDrv.ActPosition == 0.0 && gAmmo.InitialRoundsGAU18 > 0 )//makes sure gun moved forward to zero pos and it is not out of bullets
			{
				GAU18.state = DRIVE_READY;
			}
				
			break;
	}
	InVar.iGAU18FireEnableOLD = InVar.iGAU18FireEnable;//last pass.  captures a change in trigger pulls
}

static void GAU18_UpdateFire_1()//gWeaponAxisRight
{
		
	InVar.bGAU18SafetyOff_1      = RawDigital.bRawGunSafety3;//These three vars are mapped to DI's in IO_Handler.c
	InVar.bGAU18LeftTrigger_1 	 = RawDigital.bRawGunTrigger7;
	InVar.bGAU18RightTrigger_1	 = RawDigital.bRawGunTrigger8;
		
	if (udpStatusFireReady[3] = (InVar.bGAU18SafetyOff_1 && InVar.fire_Enable[1]) )// If the user switches the safety off on the gun and the instructor sends via UDP a fire enable flag
	{
		InVar.iGAU18FireEnable_1 = (InVar.bGAU18LeftTrigger_1 || InVar.bGAU18RightTrigger_1);//stores the trigger pulls
	}
	else
	{
		InVar.iGAU18FireEnable_1   = OFF;//make sure triggers are set to zero
		OutVar.bEnableIndicator_1  = OFF;//turn light off
	}
			
	if(InVar.iGAU18FireEnable_1 && !InVar.iGAU18FireEnableOLD_1)//if the trigger is pulled and previously it was not pulled.
	{
		OutVar.bEnableIndicator_1  = ON;//turn light on
		GAU18.state_1              = RAMP_UP;
	}
	else if (InVar.iGAU18FireEnableOLD_1 && !InVar.iGAU18FireEnable_1)//if the trigger is not pulled and previously it was pulled.
	{
		GAU18.state_1 = RAMP_DOWN;
	}
	
	if (gWeaponAxisRight.ServoDrv.ErrorID)//if an error
	{
		gWeaponAxisRight.ServoDrv.ErrorAcknowledge = 1;//clear the error
	}
	
	systemError[1] = gWeaponAxisRight.ServoDrv.ErrorID > 0 ? 1: 0;//stores the error to send via UDP
			
	switch (GAU18.state_1)
	{
		case IDLE:
				
			GAU18.state_1 = DRIVE_ON;
				
			break;
			
		case DRIVE_ON:
				
			gWeaponAxisRight.ServoDrv.POWER = 1;
				
			if(gWeaponAxisRight.ServoDrv.DriveStatus.DriveEnable)//if the power on was successful
			{
				GAU18.state_1 = DRIVE_HOME;
			}
				
			break;
			
		case DRIVE_HOME:
				
			gWeaponAxisRight.ServoDrv.HOME     = 1;//homing command
			gWeaponAxisRight.ServoDrv.HomeMode = ncDIRECT; //type of homing
			GAU18.state_1                      = DRIVE_HOME_DONE;
				
			break;
			
		case DRIVE_HOME_DONE:
				
			if(gWeaponAxisRight.ServoDrv.DriveStatus.HomingOk)//if the homing was sucessful
			{
				GAU18.state_1 = DRIVE_READY;
			}
				
			break;
			
		case DRIVE_READY://waitng for trigger pull state... machine sits in this state and waits.
				
			break;
			
		case RAMP_UP:
					
					
			gWeaponAxisRight.ServoDrv.Acceleration = 500000.0;
			gWeaponAxisRight.ServoDrv.Deceleration = 500000.0;
			gWeaponAxisRight.ServoDrv.Velocity     = 50000.0;
			gWeaponAxisRight.ServoDrv.Position     = GAU18.iRampTopPos_1;// 3000
			gWeaponAxisRight.ServoDrv.Direction    = 0;//positive direction
			gWeaponAxisRight.ServoDrv.MOVEABSOLUTE = TRUE;//command a movement
			GAU18.state_1                           = RAMP_UP_WAIT;
				
			break;
			
		case RAMP_UP_WAIT:
				
			if(gWeaponAxisRight.ServoDrv.ActPosition == GAU18.iRampTopPos)// if the motor has moved to where we requested
			{					
				gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.Enable                           = 1;
				gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.InterpolationMode                = mcIPL_LINEAR;
				gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.AdvancedParameters.Acceleration  = 2000000.0;
				gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.AdvancedParameters.Deceleration  = 2000000.0;
				
				if (gAmmo.FireRate50cal != 0)// this allows the user through the GUI to input the RPM's they desire
				{
					gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.AdvancedParameters.Velocity     = 100000.0 * (gAmmo.FireRate50cal / 750);
				}
				GAU18.currentCycleTimeSec_1                                                   = 0.000001 * rt_info.cycle_time; //cycle time from CPU.sw converted from us to sec
				GAU18.Et_1                                                                    = 0.0;
				GAU18.omega_1                                                                 = (K_TWO_PI_DP * GAU18.iFreq_1);
				GAU18.oStart_1                                                                = (GAU18.iRampTopPos_1 - GAU18.iAmp_1);
				GAU18.state_1                                                                 = OSCILLATE;
			}
				
			break;
				 
		case OSCILLATE:
				
			GAU18.newPos_1                                                    = (GAU18.iAmp_1 * cos(GAU18.omega_1 * GAU18.Et_1)) ;//constatnly generates a new motor position
			gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.CyclicPosition.Real  = GAU18.oStart_1 +  GAU18.newPos_1 ;//comands the motor to that pos.
			GAU18.Et_1 += GAU18.currentCycleTimeSec_1;
			charged[1] = 0;// this resets the capture of the charging handle being pulled.
			break;
				
		case RAMP_DOWN:
				
			gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.Enable = 0;//stop the oscillation
				
			if(gWeaponAxisRight.ServoDrv.StandStill)// is in openPLC standstill state
			{
				GAU18.state_1 = RAMP_DOWN_EXT;		
			}
				
			break;
			
		case RAMP_DOWN_EXT:
			
			gWeaponAxisRight.ServoDrv.Acceleration = 500000.0;
			gWeaponAxisRight.ServoDrv.Deceleration = 500000.0;
			gWeaponAxisRight.ServoDrv.Velocity     = 50000.0;	
			gWeaponAxisRight.ServoDrv.Position     = 0.0;
			gWeaponAxisRight.ServoDrv.Direction    = 1;
			gWeaponAxisRight.ServoDrv.MOVEABSOLUTE = TRUE;	
				
			if(gWeaponAxisRight.ServoDrv.ActPosition == 0.0 && gAmmo.InitialRoundsGAU18_1 > 0 )//makes sure gun moved forward to zero pos and it is not out of bullets
			{
				GAU18.state_1 = DRIVE_READY;
			}
				
			break;
	}
	InVar.iGAU18FireEnableOLD_1 = InVar.iGAU18FireEnable_1;//last pass.  captures change in trigger pull
}

static void GAU18BarrelPositionAndShotsFired()//gWeaponAxisLeft
{
	
	gAmmo.BarrelPosition[2] = gWeaponAxisLeft.ServoDrv.ActPosition;//stores off the actual pos.
	
	if ((gAmmo.BarrelPosition[2] > (GAU18.iRampTopPos - 100)) && !gAmmo.gau18BulletKey)//increments the counter when the barrelis "pumping" or moving backward
	{
		gAmmo.BulletCounter[2]++;
		gAmmo.gau18BulletKey = 1;//flag		
	}
	if (gAmmo.BarrelPosition[2] < (GAU18.iRampTopPos - 500))//when the barrel is moving forward from the recoil, reset the above entry flag BulletKey
	{
		gAmmo.gau18BulletKey = 0;
	}
	
		
}

static void GAU18BarrelPositionAndShotsFired_1()//gWeaponAxisRight
{
	gAmmo.BarrelPosition[3] = gWeaponAxisRight.ServoDrv.ActPosition;//stores off the actual pos.
	
	if ((gAmmo.BarrelPosition[3] > (GAU18.iRampTopPos - 100)) && !gAmmo.gau18BulletKey_1)//increments the counter when the barrelis "pumping" or moving backward
	{
		gAmmo.BulletCounter[3]++;
		gAmmo.gau18BulletKey_1 = 1;	//flag		
	}
	if (gAmmo.BarrelPosition[3] < (GAU18.iRampTopPos - 500))//when the barrel is moving forward from the recoil, reset the above entry flag BulletKey
	{
		gAmmo.gau18BulletKey_1 = 0;
	}
	//gAmmo.Remainder50cal = floor(gAmmo.Remainder50cal);
	//gAmmo.Remainder50cal = brmfmod(gAmmo.BarrelPosition[1], 7000.00 );
}

static void GAU18DetermineRoundsRemaining()  
{	
	
	gAmmo.RoundsRemainingGAU18 = (gAmmo.InitialRoundsGAU18 - gAmmo.BulletCounter[2]);
	
	if (gAmmo.RoundsRemainingGAU18 < 0)
	{
		gAmmo.RoundsRemainingGAU18 = 0;
	}
			
	if (gAmmo.last_InitialRoundsGAU18 != gAmmo.InitialRoundsGAU18)//captures a re-entry of round amount from the user bia GUI
	{
		gAmmo.last_InitialRoundsGAU18 = gAmmo.InitialRoundsGAU18;
		
	}
	
	if(gAmmo.RoundsRemainingGAU18 <= 0 && gAmmo.last_InitialRoundsGAU18 > 0 &&  !Flags.entryFlag[0])//if we ran out of bullets, there was bullets, and the flag is not set
	{
				
		GAU18.state        = RAMP_DOWN;//move the barrel forward to the front pos.
		Flags.entryFlag[0] = 1;//set a one time flag
		ammoEmpty[2]       = 0;// UDP we are out of ammo.
	}
			
	if ( Flags.entryFlag[0])// if the flags was set meaning there was bullets and we ran out
	{				
		gAmmo.BulletCounter[2]     = 0;// reset ammo counter
		gAmmo.InitialRoundsGAU18   = 0;//set beginning rounds to zero
		gAmmo.RoundsRemainingGAU18 = 0;//remaining rounds to zero
		Flags.entryFlag[0]         = 0;// reset flag to allow entry into above if statement
	
	}
	
}

static void GAU18DetermineRoundsRemaining_1()  
{	
	gAmmo.RoundsRemainingGAU18_1 = (gAmmo.InitialRoundsGAU18_1 - gAmmo.BulletCounter[3]);
	
	if (gAmmo.RoundsRemainingGAU18_1 < 0)
	{
		gAmmo.RoundsRemainingGAU18_1 = 0;
	}
			
	if (gAmmo.last_InitialRoundsGAU18_1 != gAmmo.InitialRoundsGAU18_1) //captures a re-entry of round amount from the user via GUI
	{
		gAmmo.last_InitialRoundsGAU18_1 = gAmmo.InitialRoundsGAU18_1;
		
	}
	
	if(!gAmmo.RoundsRemainingGAU18_1)
	{
		InVar.iGAU18FireEnable_1 = 0;
	}
	
	if(gAmmo.RoundsRemainingGAU18_1 <= 0 && gAmmo.last_InitialRoundsGAU18_1 > 0 &&  !Flags.entryFlag[1])//if we ran out of bullets, there was bullets, and the flag is not set
	{
				
		GAU18.state_1      = RAMP_DOWN;//move the barrel forward to the front pos.
		Flags.entryFlag[1] = 1;//set a one time flag
		ammoEmpty[3]       = 0;// UDP we are out of ammo.
	}
			
	if ( Flags.entryFlag[1])
	{				
		gAmmo.BulletCounter[3]       = 0;// reset ammo counter
		gAmmo.InitialRoundsGAU18_1   = 0;//set beginning rounds to zero
		gAmmo.RoundsRemainingGAU18_1 = 0;//remaining rounds to zero
		Flags.entryFlag[1]           = 0;// reset flag to allow entry into above if statement
				
	}
	
}

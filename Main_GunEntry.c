/********************************************************************
 * COPYRIGHT -- ISM
 ********************************************************************
 * Program: Main_GunEntry
 * File: Main_GunEntry.c
 * Author: ISM
 * Created: August 01, 2014
 ********************************************************************
 * Implementation of program Main_GunEntry
 ********************************************************************/

#include <bur/plctypes.h>

#include "math.h"
#include "stdio.h"
#include "time.h"
#include "GAU18.c"
#include "M134.c"
#include "../../ISM_lib/magicnum.h"

#ifdef _DEFAULT_INCLUDES
	#include <AsDefault.h>
#endif

#define M_134 1001
#define GAU_18 1002

static void DeterminePitch();
static void DetermineRoll();

enum{
	BEGIN = 0,
	HOME_M134,
	DONE,
	PARTYTIME
}KP;

_GLOBAL USINT m134state _VAR_INIT (0);
_GLOBAL USINT m134state_1 _VAR_INIT (0);



//***************50cal***************
// 6 rev of B&R motor = 1 stroke of barrel. 
	

void _INIT Main_GunEntryInit(void)
{
	CalcVar.iTime       = 60;
	InVar.iRoundsPerMin = 150000; //60 = 1 round per min. 
	  
	gWeaponAxisLeft.enableDO    = ON;
	//gWeaponAxisRight.enableDO = 1;
	gAxis[10].enableDO          = ON;//This is connected to IF6.ST5 which was gAxis[10].enableDO, now gWeaponAxisRight.
	gTorque.enableDO            = ON;    
	  
	gWeaponAxisLeft.ServoDrv.AxisObj  = (UDINT)&gWeaponAxisLeft.ACP10AXIS;// the address for the left side gun
	gWeaponAxisRight.ServoDrv.AxisObj = (UDINT)&gWeaponAxisRight.ACP10AXIS; //address for right side gun
	
	gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt.Axis  = (UDINT)&gWeaponAxisLeft.ACP10AXIS;// the address for the left side gun
	gWeaponAxisRight.MC_BR_MoveCyclicPositionExt.Axis = (UDINT)&gWeaponAxisRight.ACP10AXIS; //address for right side gun
	
	gWeaponAxisLeft.ServoDrv.Acceleration = 500000.0;// this set of three variable sets a standard value for use in homing
	gWeaponAxisLeft.ServoDrv.Deceleration = 500000.0;
	gWeaponAxisLeft.ServoDrv.Velocity     = 50000.0;
	
	gWeaponAxisRight.ServoDrv.Acceleration = 500000.0;// this set of three variable sets a standard value for use in homing
	gWeaponAxisRight.ServoDrv.Deceleration = 500000.0;
	gWeaponAxisRight.ServoDrv.Velocity     = 50000.0;
	 		
	InVar.iGAU18FireEnable   = FALSE;// make sure the trigger pulling event gets set back to zero
	InVar.iGAU18FireEnable_1 = FALSE;// These variables capture both triggers on the gun left and right side grips	
	
	InVar.iM134FireEnable    = FALSE;// make sure the trigger pulling event gets set back to zero
	InVar.iM134FireEnable_1  = FALSE;// These vars capture both a hi or low speed trigger pull
	
	GAU18.state   = IDLE;//begin the state machine at the beginning
	GAU18.state_1 = IDLE;
	
	M134.state   = IDLE;//begin the state machine at the beginning
	M134_1.state = IDLE;
	
	rt_info.enable = ON;//allows us to access internal BR computer data.  Used in GAU18 cycle process time.  Variable is "info.rt_info.cycle_time"
	RTInfo(&rt_info);
		
	//	********************************************
	//	AMMO Init and Clearing is below
	//  ********************************************	
	
	gAmmo.BulletCounter[0] = 0;//reset all bullet counter
	gAmmo.BulletCounter[1] = 0;
	gAmmo.BulletCounter[2] = 0;
	gAmmo.BulletCounter[3] = 0;
	
	gAmmo.gau18BulletKey   = 0;// this is last pass.. It captures the GAU18 barrel pumping back and forth and allows the bullets to be counted based on that
	gAmmo.gau18BulletKey_1 = 0;
	
	//Should we set a intital default round amount?		
	//gAmmo.InitialRoundsGAU18_1 = 100;
	
	unsigned long pp = 0;// used for ulimited round counter found below
	
}

void _CYCLIC Main_GunEntryCyclic(void)
{
	/*********************************************
	**											**
	**           Check GAU18 presence           **
	**											**
	**********************************************/
	unsigned long pp;
	
	if(InVar.bGAU18Connected)//gWeaponAxisLeft
	{
		whichWeapon[0] = GAU_18;//1002 = GAU18 for UDP broadcast
		
		if((gAmmo.charged[0] != gAmmo.chargedLast[0]) && !charged[0])//captures the pulling back of the handle
		{
			charged[0]++;//sets a flag.  This is reset once the gun fires.  The reset is found in the OSCILLATE state
			gAmmo.charged[0] == gAmmo.chargedLast[0];//sets the vars equal to once again capture a change
		}		
		
		if(gAmmo.hatchClosed[2] && gAmmo.ammoPresent[2] && charged[0])//ammo cover closed, a round is loaded, and the charging handle was pulled back
		{
			GAU18_UpdateFire();//found in GAU18.c
			GAU18BarrelPositionAndShotsFired();//GAU18.c
			GAU18DetermineRoundsRemaining();//GAU18.c
			
		}
		
	}
	
	if(InVar.bGAU18Connected_1)//gWeaponAxisRight.  The _1 at the end signifies the right side weapon throught this code.
	{
		whichWeapon[1] = GAU_18;//UDP value that is broadcasted 1002 =  GAU18 for UDP
		
		if((gAmmo.charged[1] != gAmmo.chargedLast[1]) && !charged[1])//captures the pulling back of the handle
		{
			charged[1]++;//sets a flag.  This is reset once the gun fires.  The reset is found in the OSCILLATE state of the right side gun
			gAmmo.charged[1] == gAmmo.chargedLast[1];//sets the vars equal to once again capture a change
		}	
		if(gAmmo.hatchClosed[3] && gAmmo.ammoPresent[3] && charged[1])//ammo cover closed, a round is loaded, and the charging handle was pulled back
		{
			GAU18_UpdateFire_1();//found in GAU18.c
			GAU18BarrelPositionAndShotsFired_1();//found in GAU18.c
			GAU18DetermineRoundsRemaining_1();//found in GAU18.c
			
		}
	}
	
	
	/*********************************************
	**											**
	**           Check M134 presence            **
	**											**
	**********************************************/
	
		if(InVar.bM134Connected)//gWeaponAxisLeft
		{
			whichWeapon[0] = M_134;//1001 = M134
		
			switch(m134state)
			{
				case BEGIN:
					
					gWeaponAxisLeft.ServoDrv.POWER = ON;
					
					if(gWeaponAxisLeft.ServoDrv.DriveStatus.DriveEnable)//if the power on was successful
					{
						m134state = HOME;
					}
					
					break;
				
				case HOME_M134:
					
					gWeaponAxisLeft.ServoDrv.HOME     = ON; //start homing.  see ServoDrv.st
					gWeaponAxisLeft.ServoDrv.HomeMode = ncDIRECT; //type of homing
					m134state                       = DONE;
					
					break;
				
				case DONE:
					
					if(gWeaponAxisLeft.ServoDrv.DriveStatus.HomingOk)// if the drive reports back a successful homing
					{
						m134state = PARTYTIME;
					}
					
					break;
				
				case PARTYTIME://waitng for trigger pull state... machine sits in this state and waits.
					
					break;
			}
			if(gAmmo.ammoPresent[0] && gAmmo.hatchClosed[0] && gAmmo.fdInstalled[0])//round loaded, ammo cover closed, and feeder delinker installed
			{
				M134_UpdateFire();//found in M134.c
			}
		}
	
	if(InVar.bM134Connected_1)//gWeaponAxisRight
	{
		whichWeapon[1] = M_134;//1001 = M134
		
		switch(m134state_1)
		{
			case BEGIN:
				
				gWeaponAxisRight.ServoDrv.POWER = ON;
				
				if(gWeaponAxisRight.ServoDrv.DriveStatus.DriveEnable)//if the power on was successful
				{
					m134state_1 = HOME;
				}
				
				break;
			
			case HOME_M134:
				
				gWeaponAxisRight.ServoDrv.HOME     = ON; //start homing.  see ServoDrv.st
				gWeaponAxisRight.ServoDrv.HomeMode = ncDIRECT; //type of homing
				m134state_1                       = DONE;
				
				break;
			
			case DONE:
				
				if(gWeaponAxisRight.ServoDrv.DriveStatus.HomingOk)// if the drive reports back a successful homing
				{
					m134state_1 = PARTYTIME;
				}
				
				break;
			
			case PARTYTIME://waitng for trigger pull state... machine sits in this state and waits.
				
				break;
		
				if(gAmmo.ammoPresent[1] && gAmmo.hatchClosed[1] && gAmmo.fdInstalled[1])//round loaded, ammo cover closed, and feeder delinker installed
				{
					M134_UpdateFire_1();//found in M134.c
			
				}
			}
		}
	
		ServoDrv(&gWeaponAxisLeft.ServoDrv);// copy left side gun variables to function block.  See ServoDrv.st
		MC_BR_MoveCyclicPositionExt(&gWeaponAxisLeft.MC_BR_MoveCyclicPositionExt);
	
		ServoDrv(&gWeaponAxisRight.ServoDrv);//copy right side gun variables to function block
		MC_BR_MoveCyclicPositionExt(&gWeaponAxisRight.MC_BR_MoveCyclicPositionExt);
		
		if(gAmmo.unlimGAU18)//this handles free play mode...unlimited rounds. This can be tied to the GUI
		{
			//rounds remaining will always be greater than zero 
			pp++;
		
			gAmmo.RoundsRemainingGAU18   = pp;
			gAmmo.RoundsRemainingGAU18_1 = pp;
		
			// the below value is 1000 less than the max of a unsigned long
			// it simply resets the loop so rounds remaining is never zero.
			if (pp > 4294966295)
			{                   
				pp = 10;
			}
		
		
		}
	
		DetermineRoll();
	
		DeterminePitch();
	
}	

static void DeterminePitch()
{
	if (RawDigital.RawGunPitch > 0)
	{
		weapon_pitch_angle = (REAL) ((RawDigital.RawGunPitch * .0049) - 322.41) ;// need to standardize this offset and not hardcode.
	}
	else
	{
		weapon_pitch_angle = 0.0 ;
		
	}
	
	if (RawDigital.RawGunPitch_1 > 0)
	{
		weapon_pitch_angle_1 = (REAL) ((RawDigital.RawGunPitch_1 * .0049) - 322.41) ;// need to standardize this offset and not hardcode.
	}
	else
	{
		weapon_pitch_angle_1 = 0.0 ; 
	}
}

static void DetermineRoll()
{
	weapon_azimuth_angle = (REAL) (gTorque.drive.ServoDrv.ActPosition * 0.1) ;// need to standardize this offset and not hardcode.
	
	//weapon_azimuth_angle_1 = (REAL) (gTorque_1.drive.ServoDrv.ActPosition * 0.1) ;
}

 


 

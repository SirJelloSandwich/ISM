/********************************************************************
 * COPYRIGHT -- ISM
 ********************************************************************
 * Program: Main_GunEntry
 * File: M134.c
 * Author: ISM
 * Created: August 01, 2014
 *******************************************************************/

#include <bur/plctypes.h>
#include "../../ISM_lib/magicnum.h"
#include "../../ISM_lib/Load_Constants.h"


#ifdef _DEFAULT_INCLUDES
	#include <AsDefault.h>
#endif

#define HI 112500.0
#define LO 56250.0

//************//M134-GAU2C-7.62mm-MINI-GUN*************
 // 1 rev of B&R Barrel motor = 1 rev of 14 tooth gear = 0.388 rev of 36 tooth gear.  0.388 rev of 36 tooth gear = 0.388 rev of 17 tooth gear. 
 //0.388 rev of 17 tooth gear = 0.0758 rev of 87 tooth gear because 1 rev of 17 tooth gear/0.195 rev of 87 tooth gear.
//
 //  1 rev of the Barrel = 13.1926 rev of the B&R motor. 
//	One sixth rev of barrel: 1/6 = 0.16 rev.  0.16 rev of barrel = 2.198 rev of B&R motor. 2198 encoder units.
//	According to 8JSAaxisi.ax under parameter_if there are 1000 units/ 1 motor rev. 
//	Therefore, 13,192 encoder units equals 1 rotation of the barrel.  0.36 deg of resolution.*/
//Lo Rate trigger fires 2000 rounds/ min = 33 rounds/sec. = 5 rotations/ 33 rounds = 5 rotations/ sec = 13192 units/ 1 rotation = 66000 units / sec!

static void M134BarrelPositionAndShotsFired()
{
			
	gAmmo.BarrelPosition[0] = gWeaponAxisLeft.ServoDrv.ActPosition;//allows us to operate on the actual position.  

	//if (brmfmod(gAmmo.BarrelPosition[0]/2250
	
	if ( (gAmmo.BarrelPosition[0] > 2150.00) && (gAmmo.BarrelPosition[0] < 2190.00) )// if the barrel has spun once.  
	{	
		gAmmo.RemainderM134 = 0;
		
	}
	else
	{
		gAmmo.RemainderM134 = 1;
	}
		
	gAmmo.RemainderM134  = floor(gAmmo.RemainderM134);// round down the value.  
		
	if (gAmmo.RemainderM134 == 0)// if barrel spun 1/6 of 360 degrees.
	{
		gAmmo.BulletCounter[0]++;// 6 bullets fired.
	}
	gAmmo.BulletsFired = (gAmmo.BulletCounter[0] * 6);
	
}

static void M134DetermineRoundsRemaining()  
{	
	if(gAmmo.InitialRoundsM134 && gAmmo.BulletCounter[0])
	{
		gAmmo.RoundsRemainingM134 = (gAmmo.InitialRoundsM134 - ((UDINT) gAmmo.BulletCounter[0])) ;
	}
	
	if (gAmmo.RoundsRemainingM134 <= 0)
	{
		gAmmo.RoundsRemainingM134 = 0;
		ammoEmpty[0] = 0;
	}
	else
	{
		ammoEmpty[0] = 1;
	}
			
	if ((gAmmo.last_InitialRoundsM134 != gAmmo.InitialRoundsM134)) 
	{
		gAmmo.last_InitialRoundsM134 = gAmmo.InitialRoundsM134;
	}
	
	//if (!gAmmo.RoundsRemainingM134)
	//	{
	//	InVar.iM134FireEnable = 0;
	//	}
}

static void M134BarrelPositionAndShotsFired_1()//gWeaponAxisRight
{
			
	gAmmo.BarrelPosition[1] = gWeaponAxisRight.ServoDrv.ActPosition;

	if ( (gAmmo.BarrelPosition[1] > 2150.00) && (gAmmo.BarrelPosition[1] < 2250.00) )
	{	
		gAmmo.RemainderM134_1 = 0;
	}
	else
	{
		gAmmo.RemainderM134_1 = 1;
	}
		
	gAmmo.RemainderM134_1  = floor(gAmmo.RemainderM134_1);
		
	if (gAmmo.RemainderM134_1 == 0)
	{
		gAmmo.BulletCounter[1]++;
	}
	//gAmmo.BulletsFired_1 = gAmmo.BulletCounter[1] / 4.7;
	gAmmo.BulletsFired_1 = (gAmmo.BulletCounter[1] * 6);	
}

static void M134DetermineRoundsRemaining_1()  
{	
	if(gAmmo.InitialRoundsM134_1 && gAmmo.BulletCounter[1])
	{
		gAmmo.RoundsRemainingM134_1 = (gAmmo.InitialRoundsM134_1 - ((UDINT) gAmmo.BulletCounter[1])) ;
	}
	if (gAmmo.RoundsRemainingM134_1 <= 0)
	{
		gAmmo.RoundsRemainingM134_1 = 0;
		ammoEmpty[1] = 0 ;
	}
	else
	{
		ammoEmpty[1] = 1;
	}
			
	if ((gAmmo.last_InitialRoundsM134_1 != gAmmo.InitialRoundsM134_1)) 
	{
		gAmmo.last_InitialRoundsM134_1 = gAmmo.InitialRoundsM134_1;
	}

}

  static void M134_UpdateFire()
{	
	TON_01.IN = InVar.iM134FireEnable;//If a trigger is pulled on the right side then a timer starts
	TON_01.PT = 1000;//1 sec timer
	TON(&TON_01);
	gAmmo.TonOutput= TON_01.Q;
	gAmmo.ElapseTime = TON_01.ET;
			
	if (gAmmo.TonOutput)// if the timer has run for 1 sec then run the code below
	{
		M134BarrelPositionAndShotsFired();
		M134DetermineRoundsRemaining();
	}
	InVar.bM134SafetyOff     = RawDigital.bRawGunSafety;//These three vars a connected to DI's in IO_Handler.c
	InVar.bM134FireEnable_HI = RawDigital.bRawGunTrigger1; //Left trigger
	InVar.bM134FireEnable_LO = RawDigital.bRawGunTrigger2; //Right trigger
			
	if (InVar.bM134SafetyOff && InVar.fire_Enable[0])// if the user manually switched off the safety on the gun and the instructor sent a UDP enable flag across
	{
		udpStatusFireReady[0]          = 1;
		InVar.iM134FireEnable          = (InVar.bM134FireEnable_HI || InVar.bM134FireEnable_LO);
		//OutVar.bM134SolenoidArm        = ON;//
		OutVar.bEnableIndicator        = ON;// turn light on
		gWeaponAxisLeft.ServoDrv.POWER = ON;// power on the drive
	
		OutVar.bM134SolenoidEngage = gHmi.key.tgl.RealRounds;
					
	}
	else
	{
		InVar.iM134FireEnable          = OFF;//make sure everything is OFF if the safety is on and instructor has not sent flag
		OutVar.bEnableIndicator        = OFF;
		gWeaponAxisLeft.ServoDrv.POWER = OFF;
	}
	
	if(InVar.iM134FireEnable && !InVar.iM134FireEnableOLD && gWeaponAxisLeft.ServoDrv.StandStill && !gWeaponAxisLeft.ServoDrv.HALT)
	{
		gWeaponAxisLeft.ServoDrv.Velocity = (InVar.bM134FireEnable_HI > 0) ? HI : (InVar.bM134FireEnable_LO > 0) ? LO : 0.0;//Velocity gets set by which trigger is pulled.
		
		gWeaponAxisLeft.ServoDrv.MOVEVELOCITY = 1;//command
		
		gUdpAudioMsg.control[0]      = 2;// the below is for audio in the cockpit
		gUdpRecoilMiniGun.control[0] = 2;
		    
		gUdpAudioMsg.volume[0]       = 1.0;
		gUdpRecoilMiniGun.volume[0]  = 1.0;
		    
		gUdpAudioMsg.freq[0]         = 1.0;
		gUdpRecoilMiniGun.freq[0]    = 1.0;	
	}
			
	if ((!InVar.bM134FireEnable_HI && !InVar.bM134FireEnable_LO) && InVar.iM134FireEnableOLD && !gWeaponAxisLeft.ServoDrv.StandStill)//this stops the motor from spinning.  If the triggers are not pulled and previously were, then HALT
	{
		gWeaponAxisLeft.ServoDrv.HALT = 1;
		gUdpAudioMsg.control[0]       = 4;//audio off
		gUdpRecoilMiniGun.control[0]  = 4;
	}
	
	if (gWeaponAxisLeft.ServoDrv.ErrorID)//if error
	{
		gWeaponAxisLeft.ServoDrv.ErrorAcknowledge = 1;//clear it
	}
			
	systemError[0] = gWeaponAxisLeft.ServoDrv.ErrorID > 0 ?  1 : 0;//stores the error for UDP
	
	InVar.iM134FireEnableOLD = InVar.iM134FireEnable;//last pass
	
}

static void M134_UpdateFire_1()//gWeaponAxisRight
{
	
	TON_02.IN = InVar.iM134FireEnable_1;//If a trigger is pulled on the right side then a timer starts
	TON_02.PT = 1000;//1 sec timer
	TON(&TON_02);
	gAmmo.TonOutput_1 = TON_02.Q;
	gAmmo.ElapseTime_1 = TON_02.ET;
			
	if (gAmmo.TonOutput_1)// if the timer has run for 1 sec then run the code below
	{
		M134BarrelPositionAndShotsFired_1();
		M134DetermineRoundsRemaining_1();
	}
	
	InVar.bM134SafetyOff_1     = RawDigital.bRawGunSafety2 ;//These three vars a connected to DI's in IO_Handler.c
	InVar.bM134FireEnable_HI_1 = RawDigital.bRawGunTrigger5; //left tigger
	InVar.bM134FireEnable_LO_1 = RawDigital.bRawGunTrigger6; // right trigger
	
	if ( InVar.bM134SafetyOff_1 && InVar.fire_Enable[1))// if the user manually switched off the safety on the gun and the instructor sent a UDP enable flag across
	{
		udpStatusFireReady[1]            = 1;
		InVar.iM134FireEnable_1          = (InVar.bM134FireEnable_HI_1 || InVar.bM134FireEnable_LO_1);// stores the trigger pulls
	//	OutVar.bM134SolenoidArm_1        = ON;
		OutVar.bEnableIndicator_1        = ON;// turn light on
		gWeaponAxisRight.ServoDrv.POWER  = ON;//power the drive
		
		OutVar.bM134SolenoidEngage_1 = gHmi.key.tgl.RealRounds_1 ;
		
	}
	else
	{
		InVar.iM134FireEnable_1          = OFF;//make sure everything is OFF if the safety is on and instructor has not sent flag
		OutVar.bEnableIndicator_1        = OFF;
		gWeaponAxisRight.ServoDrv.POWER  = OFF;
	}
	
	if(InVar.iM134FireEnable_1 && !InVar.iM134FireEnableOLD_1 && gWeaponAxisRight.ServoDrv.StandStill && !gWeaponAxisRight.ServoDrv.HALT)
	{
		gWeaponAxisRight.ServoDrv.Velocity = (InVar.bM134FireEnable_HI_1 > 0) ? HI : (InVar.bM134FireEnable_LO_1   > 0) ? LO : 0.0;//Velocity gets set by which trigger is pulled.
		
		gWeaponAxisRight.ServoDrv.MOVEVELOCITY = 1;//command
		
		gUdpAudioMsg.control[0]      = 2;// the below is for audio in the cockpit
		gUdpRecoilMiniGun.control[0] = 2;
		    
		gUdpAudioMsg.volume[0]       = 1.0;
		gUdpRecoilMiniGun.volume[0]  = 1.0;
		    
		gUdpAudioMsg.freq[0]         = 1.0;
		gUdpRecoilMiniGun.freq[0]    = 1.0;	
	}
						
	if ((!InVar.bM134FireEnable_HI_1 && !InVar.bM134FireEnable_LO_1) && !InVar.iM134FireEnableOLD_1 && !gWeaponAxisRight.ServoDrv.StandStill)//this stops the motor from spinning.  If the triggers are not pulled and previously were, then HALT
	{
		gWeaponAxisRight.ServoDrv.HALT = 1;//stops motor
		gUdpAudioMsg.control[0]        = 4;
		gUdpRecoilMiniGun.control[0]   = 4;
	}

	if (gWeaponAxisRight.ServoDrv.ErrorID)
	{
		gWeaponAxisRight.ServoDrv.ErrorAcknowledge = 1;
	}
	
	systemError[1] = gWeaponAxisRight.ServoDrv.ErrorID > 0 ? 1: 0;//stores error for UDP
	
	InVar.iM134FireEnableOLD_1 = InVar.iM134FireEnable_1 ;//last pass
	
}


 

 


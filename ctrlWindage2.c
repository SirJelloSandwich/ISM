/********************************************************************
 * COPYRIGHT -- ISM
 ********************************************************************
 * Program: ctrlWindage2
 * File: ctrlWindage2.c
 * Author: ISM
 * Created: August 29, 2014
 ********************************************************************
 * Implementation of program ctrlWindage2
 ********************************************************************/

#include <bur/plctypes.h>

#ifdef _DEFAULT_INCLUDES
	#include <AsDefault.h>
#endif

#include <../../bglib/bgTimer1ms.c>

enum{
	ctrlZero = 0,
	ctrlInit,
	ctrlPause,
	ctrlHomeAxis,
	ctrlHomingPause,
	ctrlHomingMove1,
	ctrlHomingMove2,
	ctrlHomingMove3,
	ctrlHomingDone,
	ctrlIdle,
	ctrlErrorAck,
	ctrlErrorAck1,
	ctrlLast
}MOTIONSTATEMACHINE;

	
static const char* ctrlStateName(int state)
{
	static const char* stateNames[] = {
		"Zero",
		"Init",
		"Pause",
		"Homing-begin",
		"Homing- search pause",
		"Homing- move1",
		"Homing- move2",
		"Homing- move3",
		"Homing- done",
		"Idle",
		"Error ack",
		"Error ack1",
		};

	if( state >= ctrlLast)
		return "unknow";
	
	return stateNames[state];
};

typedef struct ctrlAllData_typ
{
	motionGunRecoilDrive_typ* pMot;
	gunStationConfig_typ* pCfg;	
	
} ctrlAllData_typ;

#if 1
#include <../../bglib/bgPVLogger.c>
#define LOG_ENTRY_COUNT 60	//max number of log entries
#define LOG_ENTRY_LENGTH  64 //max length of individual log message. Longer msg are truncated
_LOCAL STRING log[LOG_ENTRY_COUNT][LOG_ENTRY_LENGTH]; //log output visible from Automation Studio!
static bgPVLogger_typ lgr;	//PV logger object
#else
#define DBG(loggerInstance, format, args...) ;
#define DBGINIT(loggerInstance, logoutput, logEntryCount, logEntrySize)  ;
#endif

static void checkSetVariableValue(UDINT pVar, UDINT varType, REAL minVal, REAL maxVal, REAL defaultVal)
{
	switch(varType){
		case 0://REAL
			{
				REAL* pVarReal = (REAL*)pVar;
				if(*pVarReal > maxVal){
					*pVarReal = defaultVal;
				}		
			
				if(*pVarReal < minVal){
					*pVarReal = defaultVal;
				}		
			}
			break;
		case 1://UDINT
			{
				UDINT* pVarUDINT = (UDINT*)pVar;
				if(*pVarUDINT > (UDINT)maxVal){
					*pVarUDINT = (UDINT)defaultVal;
				}		
			
				if(*pVarUDINT < (UDINT)minVal){
					*pVarUDINT = (UDINT)defaultVal;
				}		
			}
			break;
		default://???
			{

			}
			break;
	}

}

static REAL getAbsoluteWithLimit(REAL var, REAL minVal, REAL maxVal)
{
	if(var < 0)
		var = var * (-1.0);

	if(var < minVal)
		var = minVal;

	if(var > maxVal)
		var = maxVal;

	return var;
}

static BOOL isMotionError(ctrlAllData_typ *p)
{
	return( p->pMot->drive.ServoDrv.ErrorID || 
		p->pMot->vMaster.ServoDrv.ErrorID);
}

static void logMotionError(ctrlAllData_typ *p)
{
	if(p->pMot->drive.ServoDrv.ErrorID)
	{
		DBG(lgr,"ServoDrv.ErrorID=%d", p->pMot->drive.ServoDrv.ErrorID);
	}

}

static void ctrlDrivePowerOn(ctrlAllData_typ *p)
{
	p->pMot->drive.ServoDrv.POWER = 1;
	p->pMot->vMaster.ServoDrv.POWER = 1;
}

static void ctrlDrivePowerOff(ctrlAllData_typ *p)
{
	p->pMot->drive.ServoDrv.POWER = 0;
	p->pMot->vMaster.ServoDrv.POWER = 0;
}

static BOOL isDriveEnabled(ctrlAllData_typ *p)
{
	return (!p->pMot->drive.ServoDrv.Disabled && !p->pMot->vMaster.ServoDrv.Disabled);	
}

static BOOL isDriveErrorStop(ctrlAllData_typ *p)
{
	return(p->pMot->drive.ServoDrv.ErrorStop ||  p->pMot->vMaster.ServoDrv.ErrorStop);
}

static BOOL isWindageAngleTorqueEnabled(ctrlAllData_typ *p)
{
	return (p->pCfg->windage.enabled & p->pCfg->windage.angleRelationEnabled);
}
static void reqWindageTorqueSetPointUpdate(ctrlAllData_typ *p, REAL tqSP, REAL tqRamp)
{
	p->pMot->prv.sm.event[0] = 1;//remember this event
	TimerStart(&p->pMot->prv.sm.timer_x1ms[1], 50);//start timer; when timer expires, reset above flag and rase InitData to 1 > transition 0>1 triggers FBK update
	p->pMot->drive.mcBRTorqueCtrl.InitData= 0;//creaet 0to1 transition on InitData in order to accept new torque values

	//IMPORTANT: all torque callculation are performed in a postive direction (1st quadrant)
	//Only at this point, when the torque is commanded to the FBK, the wind direction is taken into consideration

	//make sure commanded torque value is within the sanity range
	//tqSP = getAbsoluteWithLimit(tqSP, (-1.0)*p->pCfg->windage.angleTorqueMax, p->pCfg->windage.angleTorqueMax);
	checkSetVariableValue(&tqSP, 0, (-1.0)*p->pCfg->windage.torqueMax, p->pCfg->windage.torqueMax, 2.0);

	//p->pMot->drive.mcBRTorqueCtrl.Torque = tqSP * (p->pCfg->windage.windDirection ? (-1.0) : 1.0 );
	p->pMot->drive.mcBRTorqueCtrl.Torque = tqSP;

	//If ramp parametar negative, use default
	if(tqRamp >= 0)
	{
		p->pMot->drive.mcBRTorqueCtrl.TorqueRamp = tqRamp;
	}
	else
	{
		p->pMot->drive.mcBRTorqueCtrl.TorqueRamp = p->pCfg->windage.torqueRamp;
	}
		
	//Following velocity limit values are used during torque control and are currently not configurable
	p->pMot->drive.mcBRTorqueCtrl.Mode = 0;//Velocity limited mode is active
	if(tqSP < 0)
	{
		//Negative (CCW) torque 
		//p->pMot->drive.mcBRTorqueCtrl.PosMaxVelocity = 10000; //When tq is negative, axis goes positive only due to user force; as a result, set POS velocity high so user will not feel motor tq in that direction)
		//p->pMot->drive.mcBRTorqueCtrl.NegMaxVelocity = -3000;//Negative tq moves axis in NEG direction; Limit speed in NEG direction (to prevent banging at the hard stop)
		p->pMot->drive.mcBRTorqueCtrl.PosMaxVelocity = p->pCfg->windage.velocityWindOpositeDirection; //When tq is negative, axis goes positive only due to user force; as a result, set POS velocity high so user will not feel motor tq in that direction)
		p->pMot->drive.mcBRTorqueCtrl.NegMaxVelocity = (-1.0)*p->pCfg->windage.velocityWindDirection;//Negative tq moves axis in NEG direction; Limit speed in NEG direction (to prevent banging at the hard stop)

	}
	else
	{
		//Positive (CW) torque 
		//p->pMot->drive.mcBRTorqueCtrl.PosMaxVelocity = 3000; //Positive tq moves axis in POS direction; Limit speed in POS direction (to prevent banging at the hard stop)
		//p->pMot->drive.mcBRTorqueCtrl.NegMaxVelocity = -10000;//When tq is positive, axis goes negative only due to user force; as a result, set NEG velocity high so user will not feel motor tq in that direction)
		p->pMot->drive.mcBRTorqueCtrl.PosMaxVelocity = p->pCfg->windage.velocityWindDirection; //Positive tq moves axis in POS direction; Limit speed in POS direction (to prevent banging at the hard stop)
		p->pMot->drive.mcBRTorqueCtrl.NegMaxVelocity = (-1.0)*p->pCfg->windage.velocityWindOpositeDirection;;//When tq is positive, axis goes negative only due to user force; as a result, set NEG velocity high so user will not feel motor tq in that direction)
	}
	p->pMot->drive.mcBRTorqueCtrl.Acceleration= 0.0;
	
}

static REAL getAbsTorqueFromAngle(ctrlAllData_typ *p, REAL angle)
{
	REAL currentAngle = getAbsoluteWithLimit(angle, 0.0, 900.0);
	REAL absTq;

	//Torque set point is angle dependant
	absTq = p->pCfg->windage.angleTorqueMax - 
		((p->pCfg->windage.angleTorqueMax - p->pCfg->windage.angleTorqueMin)  / 900.0) * currentAngle;

	return absTq;
}


/**
   Motion control logic init - torque motor (windiage) axis
  
   @param[in,out] p ptr to structure that holds all accessible global data ptrs.
*/
static void ctrlMotionTorqueInit(ctrlAllData_typ *p)
{
	//make sure that motion control logic starts at a known point
	//p->pMot->prv.sm.state = ctrlZero;

	bgSSM_typ* pSm = &p->pMot->prv.sm;

	//make sure that motion control logic starts at a known point
	//p->pMot->prv.sm.state = ctrlZero;
	bgSSMInit(pSm, (UDINT)ctrlStateName, 0);
	
}

static void ctrlMotionTorqueCyclic(ctrlAllData_typ *p)
{
	bgSSM_typ* pSm = &p->pMot->prv.sm;

	//Always operate timers
	TimerExecute(&p->pMot->prv.sm.timer_x1ms[0]);
	TimerExecute(&p->pMot->prv.sm.timer_x1ms[1]);

	//continuosly check for motion errors
	if(isMotionError(p) && (pSm->state < ctrlErrorAck))
	{
		//in case of a drive error > force error state and automatic error recovery
		bgSSMStateNext(pSm, ctrlErrorAck);
	}

	/*execute control state machine cyclic handler*/		
	bgSSMCyclic(pSm);
	
	//show change of state in a log
	if( bgSSMIsStateInit(pSm) )
	{
		DBG(lgr, "tq=%s(%d)", bgSSMGetStateText(pSm), bgSSMGetState(pSm));
	}
	
	/*execute motion control state machine*/		
	switch( bgSSMGetState(pSm) )
	{
		/*######################################################*/
		case ctrlZero:
			//disable torque control block
			p->pMot->drive.mcBRTorqueCtrl.Enable = 0;
			p->pMot->drive.ServoDrv.HOME = 0;
			
			bgSSMStateNext(pSm, ctrlInit);
			//bgSSMStateNext(pSm, ctrlIdle);
			break;
		/*######################################################*/
		case ctrlInit:
			if( bgSSMIsStateInit(pSm) )
			{
				//Enable torque motor drive
				ctrlDrivePowerOn(p);
				TimerStart(&p->pMot->prv.sm.timer_x1ms[0], 2000);
				break;
			}

			if( !TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				break;
			}

			//Make sure that drive is off > power up behaivor for windage axis is Drive Enable=OFF
			if(isDriveEnabled(p))
			{						
				p->pMot->drive.ServoDrv.HOME = 1;

				//if(p->pCfg->windage.encoderOffset)

				if(p->pCfg->windage.encoderOffset >= 3600)
					p->pCfg->windage.encoderOffset = 0;
				if(p->pCfg->windage.encoderOffset <= -3600)
					p->pCfg->windage.encoderOffset = 0;
				
				p->pMot->drive.ServoDrv.HomePosition = p->pCfg->windage.encoderOffset * (-1.0);
				p->pMot->drive.ServoDrv.HomeMode = mcHOME_ABSOLUTE_CORR;
				bgSSMStateNext(pSm, ctrlPause);
			}
			break;
		/*######################################################*/
		case ctrlPause:
			
			if(!(gTorque_1.drive.ServoDrv.DriveStatus.HomingOk))
			{
				bgSSMStateNext(pSm, ctrlInit);
			}
			
			if( p->pMot->drive.ServoDrv.DriveStatus.HomingOk)
			{
				p->pMot->drive.ServoDrv.HOME = 0;
				ctrlDrivePowerOff(p);//Power up state is always drive OFF
				bgSSMStateNext(pSm, ctrlIdle);
			}	
			
			break;
		/*######################################################*/
		case ctrlHomingDone:
			if( TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				bgSSMStateNext(pSm, ctrlIdle);
			}
			break;
		/*######################################################*/
		case ctrlHomeAxis:
			if( bgSSMIsStateInit(pSm) )
			{		
				//Turn the drive off
				ctrlDrivePowerOn(p);
				p->pMot->drive.ServoDrv.HOME = 0;
				break;
			}

			// TODO: COMPLETE HOMING

			//Make sure that drive is on > ServoDrv doesn't work if this is not the case, problem that shall be fixed
			if(isDriveEnabled(p)){				
				bgSSMStateNext(pSm, ctrlHomingMove1);
			}
			break;			
		/*######################################################*/
		case ctrlHomingMove1:
			if( bgSSMIsStateInit(pSm) )
			{				
				//This application is using single turn EnDAT encoder
				//The axis move range is less then a 1 rotation of the encoder (less then 360 deg)
				//As a result, we are using Homing mode mcHOME_ABSOLUTE_CORR to take advantage of
				//this facts.
				//IMPORTANT: In order for this to work properly, SW limits for this axis have been set to the following:
				// -SWLimit=-179.0, +SWLimit=+179.0. That way, the total axis movement range is 358 deg which is less
				//the encoder total movement of 360. That way, a condition for this mode is satifsfied.
				//Homing procedure works as follows:
				//- user positions the torque (windage) axis to a desired position that is supposed to be a 0 azimuth position
				//- user then triggers homing event (i.e. presses an HMI button)
				//- this event is evaluated within the idle state of this state machine and then this homing procedure is 
				//- tirggered
				//On power up, homing mode is mcDIRECT and Home.Position == AcutalPosition! 
				p->pMot->drive.ServoDrv.HOME = 1;
				p->pMot->drive.ServoDrv.HomePosition = 0.0;
				p->pMot->drive.ServoDrv.HomeMode = mcHOME_ABSOLUTE_CORR;
				TimerStart(&p->pMot->prv.sm.timer_x1ms[0], 1500);
				break;
			}

			if( TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				p->pCfg->windage.encoderOffset = p->pMot->drive.ServoDrv.ActPosition;
				DBG(lgr, "EncOffset@Zero=%d", (int)p->pCfg->windage.encoderOffset);

				p->pMot->drive.ServoDrv.HOME = 0;
				bgSSMStateNext(pSm, ctrlHomingMove2);
			}	
			break;			
		/*######################################################*/
		case ctrlHomingMove2:
			if( bgSSMIsStateInit(pSm) )
			{				
				TimerStart(&p->pMot->prv.sm.timer_x1ms[0], 1500);
			}

			if( TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				bgSSMStateNext(pSm, ctrlHomingMove3);
			}

			break;			
		/*######################################################*/
		case ctrlHomingMove3:
			if( bgSSMIsStateInit(pSm) )
			{				
				p->pMot->drive.ServoDrv.HOME = 1;
				p->pMot->drive.ServoDrv.HomePosition = p->pCfg->windage.encoderOffset * (-1.0);
				p->pMot->drive.ServoDrv.HomeMode = mcHOME_ABSOLUTE_CORR;
				TimerStart(&p->pMot->prv.sm.timer_x1ms[0], 1500);
			}

			if( TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				p->pMot->drive.ServoDrv.HOME = 0;
				//Turn the drive off
				ctrlDrivePowerOff(p);
				DBG(lgr, "HomingDone pos=%d", (int)p->pMot->drive.ServoDrv.ActPosition);
				bgSSMStateNext(pSm, ctrlIdle);
			}
			

			break;			
		/*######################################################*/
		case ctrlIdle:
	
			if( bgSSMIsStateInit(pSm) )
			{
				//Enable FBK that reads actual torque
				p->pMot->drive.actualTorque.Enable = 1;
				p->pMot->prv.sm.event[0]           = 0;
			}

			//Detect windage OFF event
			if(!p->pCfg->windage.enabled && p->pMot->drive.mcBRTorqueCtrl.Enable)
			{
				p->pMot->drive.mcBRTorqueCtrl.Enable      = 0;
				p->pMot->drive.mcBRTorqueCtrl.InitData    = 0;
				p->pMot->drive.mcBRTorqueCtrl.StartSignal = 0;

				//Enable FBK that reads actual torque
				p->pMot->drive.actualTorque.Enable = 1;

				//Windage OFF turns the drive OFF				
				ctrlDrivePowerOff(p);
			}

			//Detect windage ON event; when windage gets enabled, trigger appropriate FBK-s
			if(p->pCfg->windage.enabled && !p->pMot->drive.mcBRTorqueCtrl.Enable )
			{
				//Turn on the drive
				ctrlDrivePowerOn(p);
				
				//Enable torque control block
				//IMPORTANT: don't enable torque block until drive gets enabled > it causes an error (controller not on)
				p->pMot->drive.mcBRTorqueCtrl.Enable = isDriveEnabled(p);
				p->pMot->drive.mcBRTorqueCtrl.StartSignal = 1;

				reqWindageTorqueSetPointUpdate(p, p->pCfg->windage.torqueDefault, -1);				
			}

			//when windage enabled (but windage angle feature is not active), enable normal windage (i.e. wo. angle dependancy) 
			if(!isWindageAngleTorqueEnabled(p)  && p->pCfg->windage.enabled && (p->pCfg->windage.torqueDefault != p->pMot->drive.mcBRTorqueCtrl.Torque))
			{
				reqWindageTorqueSetPointUpdate(p, p->pCfg->windage.torqueDefault, -1);
				break;
			}


			//FEATURE: angle dependant windage (torque) control			
			if( isWindageAngleTorqueEnabled(p) )
			{
				
				REAL tqDiff; 

				//Torque set point is angle dependant
				p->pMot->drive.windageAngleTorqueSP = getAbsTorqueFromAngle(p, p->pMot->drive.ServoDrv.ActPosition);

				//find the tq SP change diff
				tqDiff = getAbsoluteWithLimit(p->pMot->drive.windageAngleTorqueSP - p->pMot->drive.windageAngleTorqueSPOld, 0.0, 5.0*p->pCfg->windage.angleTorqueSensitivityDiff);

				
				if( tqDiff > p->pCfg->windage.angleTorqueSensitivityDiff)
				{
					//difference is sensible > remeber current value as the last value updated 
					p->pMot->drive.windageAngleTorqueSPOld = p->pMot->drive.windageAngleTorqueSP;
					
					//update torque FBK
					reqWindageTorqueSetPointUpdate(p, p->pMot->drive.windageAngleTorqueSP, -1);
				}

			}


			//Check for a request to Home windage axis
			if(p->pMot->motionIf.iHomingRequest)
			{
				p->pMot->motionIf.iHomingRequest = 0;
				DBG(lgr, "WindageAxis homing request!");
				bgSSMStateNext(pSm, ctrlHomeAxis);
			}

			
			//torque SP update logic > see also reqWindageTorqueSetPointUpdate()
			if(p->pMot->prv.sm.event[0] && TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[1]))
			{
				p->pMot->prv.sm.event[0] = 0;
				p->pMot->drive.mcBRTorqueCtrl.InitData= 1;//after timer expires, 0to1 transition is complete > new torque value will be set
			}	

			break;
		/*######################################################*/
		case ctrlErrorAck:
			if( bgSSMIsStateInit(pSm) )
			{	
				p->pMot->motionIf.oError = 1;

				//stop execution of certain fbk-s
				TimerStart(&p->pMot->prv.sm.timer_x1ms[0], 2000);
				
				logMotionError(p);
	
				if( isDriveErrorStop(p) )
				{
					ctrlDrivePowerOff(p);
				}

			}
			
			//No external signal to ack the alarm for 5 sec > try to recover automatcialy
			if(p->pMot->motionIf.iErrorAck || TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				//clear request
				p->pMot->motionIf.iErrorAck = 0;
				bgSSMStateNext(pSm, ctrlErrorAck1);
			}
			
			break;			
		/*######################################################*/
		case ctrlErrorAck1:
			if( bgSSMIsStateInit(pSm) )
			{	
				//ack error
				p->pMot->drive.ServoDrv.ErrorAcknowledge = 1;			
				p->pMot->vMaster.ServoDrv.ErrorAcknowledge = 1;
				
				TimerStart(&p->pMot->prv.sm.timer_x1ms[0], 2000);
			}

			//delay > keep clearing alarms until all are gone
			if(TimerIsTimeOut(&p->pMot->prv.sm.timer_x1ms[0]))
			{
				
				if(	isMotionError(p) )
				{
					//ack error
					p->pMot->drive.ServoDrv.ErrorAcknowledge = 0;			
					p->pMot->vMaster.ServoDrv.ErrorAcknowledge = 0;
					bgSSMStateNext(pSm, ctrlErrorAck);
					
				}
				else
				{
					p->pMot->motionIf.oError = 0;
					p->pMot->drive.ServoDrv.ErrorAcknowledge = 0;			
					p->pMot->vMaster.ServoDrv.ErrorAcknowledge = 0;

					// TODO: RESET TORQUE BLOCK TO RECOVER FROM ERROR
					p->pMot->drive.mcBRTorqueCtrl.Enable = 0;

					bgSSMStateNext(pSm, ctrlIdle);
					//bgSSMStateNext(pSm, ctrlZero);
					//bgSSMStateNext(pSm, 100);
				}
			}
			
			break;			
		/*######################################################*/
		case 100:
			if( bgSSMIsStateInit(pSm) )
			{	
				DBG(lgr,"stuck in error state");	
			}
			break;			
		/*######################################################*/
		default:
			bgSSMStateNext(pSm, ctrlInit);
			break;			
		/*######################################################*/
	}

}


/*
###############################################################################
Conversion function: y=f(x) where 
f(x) is defined as a CAM profile.
The CAM profile editor is a convenient method for entering/representing table 
based functions.
As a result, it used here as a generic converion function not specificly 
related to motion control.
A CAM curve must be defined in the project and the name is given as an input
to the init function.
###############################################################################
*/
static convertWindToTorqueInit(conversionFunction_typ* p, char* camCurveName, REAL minX, REAL maxX)
{
	p->state = 1;
	
	p->inValueMin = minX;
	p->inValueMax = maxX;
	
	p->MC_BR_GetCamSlavePosition.Execute = 0;
	p->MC_BR_GetCamSlavePosition.Master = 0;
	p->MC_BR_GetCamSlavePosition.Slave = 0;
	p->MC_BR_GetCamSlavePosition.MasterFactor = 1;
	p->MC_BR_GetCamSlavePosition.SlaveFactor = 1;
	strcpy(p->MC_BR_GetCamSlavePosition.DataObjectName, camCurveName);

}

static convertWindToTorqueCyclic(conversionFunction_typ* p)
{
	switch(p->state){
		case 0: //wait for value change
			if(p->inValue != p->inValueOld)
			{
				p->inValueOld = p->inValue;
				p->state = 1;
			}	
			break;
		case 1:
			//make sure input value is in the range acceptable for conversion
			if(p->inValue < p->inValueMin)
			{
				p->inValueInRange = p->inValueMin;
			}
			else if(p->inValue > p->inValueMax)
			{
				p->inValueInRange = p->inValueMax;
			}
			else
			{
				p->inValueInRange = p->inValue;
			}

			//trigger FBK
			p->MC_BR_GetCamSlavePosition.Execute = 1;
			//provide input to FBK
			p->MC_BR_GetCamSlavePosition.MasterPosition = p->inValueInRange;
			if(p->MC_BR_GetCamSlavePosition.Done)
			{
				p->MC_BR_GetCamSlavePosition.Execute = 0;
				//get output value from FBK
				p->outValue = p->MC_BR_GetCamSlavePosition.SlavePosition;
				//P-> outValue_1 = p->MC_BR_GetCamSlavePosition.SlavePosition;
				p->state = 0;			
			}
			if(p->MC_BR_GetCamSlavePosition.Error)
			{
				p->MC_BR_GetCamSlavePosition.Execute = 0;
				p->outValue = 0;
				p->state = 0;			
			}
			
			break;
		default:
			p->state = 0;
			break;
	}
	
	MC_BR_GetCamSlavePosition(&p->MC_BR_GetCamSlavePosition);
}

void _INIT ctrlWindage2Init(void)
{
	/* TODO: Add code here */
	gTorque_1.drive.ServoDrv.AxisObj = (UDINT)&gTorque_1.drive.ACP10AXIS;
	//init logger object
	DBGINIT(lgr, log, LOG_ENTRY_COUNT, LOG_ENTRY_LENGTH);

	//passing global structures/variables via single ptr
	ctrlAllData_typ p;

	//TORQUE AXIS
	//passing global structures/variables via single ptr
	p.pMot = &gTorque_1;
	p.pCfg = &gTorqueCfg_1;
	ctrlMotionTorqueInit(&p);
	
	//Wind to torque conversion init	
	convertWindToTorqueInit(&gWindToTorque_1, "windtq", 0.0, 145.0);
}

void _CYCLIC ctrlWindage2Cyclic(void)
{
	//passing global structures/variables via single ptr
	ctrlAllData_typ p;
	
	p.pMot = &gTorque_1;
	p.pCfg = &gTorqueCfg_1;
	
	ctrlMotionTorqueCyclic(&p);
	convertWindToTorqueCyclic(&gWindToTorque_1);	
	gWindToTorque_1.outValue   = gUserInputWind[1];
	gTorqueCfg_1.windage.torqueDefault = (-gWindToTorque_1.outValue / 10.0);
	gTorque_1.drive.ServoDrv.POWER = gHmi.key.tgl.WindEnableTgl_1;
	gTorqueCfg_1.windage.enabled = gHmi.key.tgl.WindEnableTgl_1 ;
		
	ServoDrv(&gTorque_1.drive.ServoDrv);
	
	gTorque_1.drive.mcBRTorqueCtrl.Axis = (UDINT)&gTorque_1.drive.ACP10AXIS;
	MC_BR_TorqueControl(&gTorque_1.drive.mcBRTorqueCtrl);	
	
	gTorque_1.drive.actualTorque.Axis = (UDINT)&gTorque_1.drive.ACP10AXIS;
	MC_ReadActualTorque(&gTorque_1.drive.actualTorque);
}

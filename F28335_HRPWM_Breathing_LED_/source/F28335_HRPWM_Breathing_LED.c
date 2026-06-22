//###########################################################################
//
// FILE:    F28335_HRPWM_Breathing_LED.c
//
// TITLE:   F28335 HRPWM breathing LED and SFO calibration project
//
// PURPOSE:
//   This project corresponds to the paper section:
//   "HRPWM high-resolution breathing LED project based on lab29-HRPWM_SFO".
//
// OUTPUT:
//   GPIO0 / EPWM1A : HRPWM output for LED brightness control.
//
// WATCH VARIABLES:
//   UpdateFine
//   DutyFine
//   DutyDirection
//   DutyMin
//   DutyMax
//   MEP_ScaleFactor[1]
//   EPwm1Regs.CMPA.all
//
//###########################################################################

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include "DSP2833x_EPwm_defines.h"
#include "SFO.h"

#define HRPWM_PERIOD_TBCLK      30U
#define DUTY_MIN_Q15            0x2300U
#define DUTY_MAX_Q15            0x7000U
#define DUTY_STEP_Q15           6U
#define SFO_REFRESH_COUNT       64U
#define BREATH_DELAY_US         350U

void EPwm1_HRPWM_Config(Uint16 period);
void UpdateHRPWMDuty(Uint16 dutyFine);
void RefreshSFO(Uint16 count);

Uint16 UpdateFine;
Uint16 DutyFine;
Uint16 DutyDirection;
Uint16 DutyMin;
Uint16 DutyMax;
Uint16 DutyStep;
Uint16 CurrentCmpa;
Uint16 CurrentCmpahr;
Uint32 CurrentCmpaAll;

int16 MEP_ScaleFactor[5];

void main(void)
{
    InitSysCtrl();
    InitEPwm1Gpio();

    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    UpdateFine = 1;
    DutyFine = DUTY_MIN_Q15;
    DutyDirection = 1;
    DutyMin = DUTY_MIN_Q15;
    DutyMax = DUTY_MAX_Q15;
    DutyStep = DUTY_STEP_Q15;

    MEP_ScaleFactor[0] = 0;
    MEP_ScaleFactor[1] = 0;
    MEP_ScaleFactor[2] = 0;
    MEP_ScaleFactor[3] = 0;
    MEP_ScaleFactor[4] = 0;

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    while (MEP_ScaleFactor[1] == 0)
    {
        SFO_MepDis(1);
    }
    MEP_ScaleFactor[0] = MEP_ScaleFactor[1];

    EPwm1_HRPWM_Config(HRPWM_PERIOD_TBCLK);
    UpdateHRPWMDuty(DutyFine);

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    for (;;)
    {
        if (DutyDirection)
        {
            if (DutyFine >= (DutyMax - DutyStep))
            {
                DutyFine = DutyMax;
                DutyDirection = 0;
            }
            else
            {
                DutyFine += DutyStep;
            }
        }
        else
        {
            if (DutyFine <= (DutyMin + DutyStep))
            {
                DutyFine = DutyMin;
                DutyDirection = 1;
            }
            else
            {
                DutyFine -= DutyStep;
            }
        }

        UpdateHRPWMDuty(DutyFine);
        RefreshSFO(SFO_REFRESH_COUNT);
        DELAY_US(BREATH_DELAY_US);
    }
}

void EPwm1_HRPWM_Config(Uint16 period)
{
    EPwm1Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;
    EPwm1Regs.TBPRD = period;
    EPwm1Regs.TBPHS.all = 0;
    EPwm1Regs.TBCTR = 0;

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 3;

    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm1Regs.AQCTLB.all = 0;
    EPwm1Regs.DBCTL.all = 0;

    EALLOW;
    EPwm1Regs.HRCNFG.all = 0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_FEP;
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;
    EPwm1Regs.HRCNFG.bit.HRLOAD = HR_CTR_ZERO;
    EDIS;
}

void UpdateHRPWMDuty(Uint16 dutyFine)
{
    int16 cmpaRegVal;
    int16 cmpahrRegVal;
    int32 temp;

    if (UpdateFine)
    {
        cmpaRegVal = ((long)dutyFine * EPwm1Regs.TBPRD) >> 15;
        temp = ((long)dutyFine * EPwm1Regs.TBPRD);
        temp = temp - ((long)cmpaRegVal << 15);
        cmpahrRegVal = (temp * MEP_ScaleFactor[1]) >> 15;
        cmpahrRegVal = cmpahrRegVal << 8;
        cmpahrRegVal += 0x0180;

        EPwm1Regs.CMPA.all = ((long)cmpaRegVal << 16) | cmpahrRegVal;
        CurrentCmpa = (Uint16)cmpaRegVal;
        CurrentCmpahr = (Uint16)cmpahrRegVal;
        CurrentCmpaAll = EPwm1Regs.CMPA.all;
    }
    else
    {
        cmpaRegVal = ((long)dutyFine * EPwm1Regs.TBPRD) >> 15;
        EPwm1Regs.CMPA.half.CMPA = cmpaRegVal;
        EPwm1Regs.CMPA.half.CMPAHR = 0;
        CurrentCmpa = (Uint16)cmpaRegVal;
        CurrentCmpahr = 0;
        CurrentCmpaAll = EPwm1Regs.CMPA.all;
    }
}

void RefreshSFO(Uint16 count)
{
    Uint16 n;

    for (n = 0; n < count; n++)
    {
        SFO_MepEn(1);
    }

    MEP_ScaleFactor[0] = MEP_ScaleFactor[1];
}

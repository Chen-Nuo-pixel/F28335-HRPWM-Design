//###########################################################################
//
// FILE:    F28335_HRPWM_Multi_Frequency.c
//
// TITLE:   F28335 HRPWM four-frequency automatic switching project
//
// PURPOSE:
//   This project corresponds to the paper section:
//   "HRPWM multi-frequency automatic switching project based on lab30-HRPWM".
//
// OUTPUT:
//   GPIO0 / EPWM1A : HRPWM output, 50% duty.
//
// HIGH-FREQUENCY MODE:
//   100 kHz -> 500 kHz -> 1 MHz -> 1.5 MHz
//
// WATCH VARIABLES:
//   current_freq_hz
//   current_tbprd
//   current_cmpa
//   current_mode_index
//   MEP_ScaleFactor[1]
//   EPwm1Regs.TBPRD
//   EPwm1Regs.CMPA.all
//
//###########################################################################

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include "DSP2833x_EPwm_defines.h"
#include "SFO.h"

#if (CPU_FRQ_150MHZ)
  #define SYSCLK_HZ        150000000L
#elif (CPU_FRQ_100MHZ)
  #define SYSCLK_HZ        100000000L
#else
  #define SYSCLK_HZ        150000000L
#endif

#define FREQ_COUNT             4U
#define SEGMENT_HOLD_SEC       8U
#define SFO_REFRESH_COUNT      128U

typedef struct
{
    Uint32 freqHz;
    Uint16 tbprd;
    Uint16 cmpa;
} HRPWM_FREQ_STEP;

const HRPWM_FREQ_STEP HrpwmFreqTable[FREQ_COUNT] =
{
    {100000L,  1500U, 750U},
    {500000L,   300U, 150U},
    {1000000L,  150U,  75U},
    {1500000L,  100U,  50U}
};

void EPwm1_HRPWM_Config(Uint16 tbprd, Uint16 cmpa);
void ApplyFrequencyStep(Uint16 index);
void HoldSegment(Uint16 seconds);
void RefreshSFO(Uint16 count);

Uint32 current_freq_hz;
Uint16 current_tbprd;
Uint16 current_cmpa;
Uint16 current_mode_index;
Uint32 current_cmpa_all;

int16 MEP_ScaleFactor[5];

void main(void)
{
    Uint16 i;

    InitSysCtrl();
    InitEPwm1Gpio();

    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

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

    ApplyFrequencyStep(0);

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    for (;;)
    {
        for (i = 0; i < FREQ_COUNT; i++)
        {
            ApplyFrequencyStep(i);
            HoldSegment(SEGMENT_HOLD_SEC);
        }
    }
}

void ApplyFrequencyStep(Uint16 index)
{
    Uint16 tbprd;
    Uint16 cmpa;

    if (index >= FREQ_COUNT)
    {
        index = 0;
    }

    tbprd = HrpwmFreqTable[index].tbprd;
    cmpa = HrpwmFreqTable[index].cmpa;

    if (SYSCLK_HZ != 150000000L)
    {
        tbprd = (Uint16)(SYSCLK_HZ / HrpwmFreqTable[index].freqHz);
        cmpa = tbprd / 2;
    }

    current_mode_index = index;
    current_freq_hz = HrpwmFreqTable[index].freqHz;
    current_tbprd = tbprd;
    current_cmpa = cmpa;

    EPwm1_HRPWM_Config(tbprd, cmpa);
}

void EPwm1_HRPWM_Config(Uint16 tbprd, Uint16 cmpa)
{
    EPwm1Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;
    EPwm1Regs.TBPRD = tbprd;
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

    EPwm1Regs.CMPA.half.CMPA = cmpa;
    EPwm1Regs.CMPA.half.CMPAHR = 0x0180;
    EPwm1Regs.CMPB = cmpa;
    current_cmpa_all = EPwm1Regs.CMPA.all;

    EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLB.all = 0;
    EPwm1Regs.DBCTL.all = 0;

    EALLOW;
    EPwm1Regs.HRCNFG.all = 0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_REP;
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;
    EPwm1Regs.HRCNFG.bit.HRLOAD = HR_CTR_ZERO;
    EDIS;
}

void HoldSegment(Uint16 seconds)
{
    Uint16 s;

    for (s = 0; s < seconds; s++)
    {
        RefreshSFO(SFO_REFRESH_COUNT);
        DELAY_US(1000000L);
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

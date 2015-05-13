#include "stdafx.h"
#include "CBus.h"


CCBus::CCBus(void)
{
	nofconnection = 0;
	// standard PQ buses have 2 power flow equations
	nofequation = 2; 
	pload = 0;
	qload = 0;
	pgen = 0;
	qgen = 0;
	nofpv = 0;
	nofpq = 0;
	voltageR = 1.0; // flat start
	voltageI = 0;
	absvoltage = 1.0;
	gnk = 0;
	bnk = 0;
}


CCBus::~CCBus(void)
{
}

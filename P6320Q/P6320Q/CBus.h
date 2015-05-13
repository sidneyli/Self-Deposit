#pragma once
class CCBus
{
public:
	CCBus(void);
	~CCBus(void);

public:
	int nofconnection;
	int* connect;
	int nofequation;
	double pload, qload;
	double pgen, qgen;
	double voltageR, voltageI, absvoltage;
	double gnk,bnk;
	// tag the number of pv buses and pq buses for all the buses with smaller indices
	int nofpv,nofpq;
};


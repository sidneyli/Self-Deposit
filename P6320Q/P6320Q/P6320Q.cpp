// P6320.cpp : 定义控制台应用程序的入口点。
//

#include "StdAfx.h"
#include <iostream>
#include <math.h>  
#include <Eigen/Dense>
#include "CBus.h"

using Eigen::MatrixXd;
using namespace Eigen;
using namespace Eigen::internal;
using namespace Eigen::Architecture;
using namespace std;

// Number of buses
#define N 9

// System Parametres
MatrixXd g(N,N),b(N,N),gs(N,N),bs(N,N);
VectorXd gnk(N),bnk(N);

// System Buses
CCBus bus[N];

// System states and intermediate variables
MatrixXd jacobi(16,16);
VectorXd qf(16),x(16),dx(16);

int AssembleSys()
{
	// cout << N;
	// Initialize
	for (int i=0;i<N;i++){
		for (int j=0;j<N;j++){
			g(i,j) = 0;
			b(i,j) = 0;
			gs(i,j) = 0;
			bs(i,j) = 0;
		}
	}

	// assemble system parametres
	g.block(0,1,1,2) << 1.9802, 1.2376;
	g.block(1,3,1,1) << 1.2376;
	g.block(2,3,1,2) << 3.0769, 6.09756;
	g.block(3,7,1,1) << 8.25083;
	g.block(4,5,1,1) << 1.9802;
	g.block(5,6,1,1) << 0.295082;
	g.block(6,8,1,1) << 0.315738;
	g.block(7,8,1,1) << 1.9802;

	b.block(0,1,1,2) << -19.802, -12.3762;
	b.block(1,3,1,1) << -12.3762;
	b.block(2,3,1,2) << -24.6154, -54.878;
	b.block(3,7,1,1) << -82.5083;
	b.block(4,5,1,1) << -19.802;
	b.block(5,6,1,1) << -3.2459;
	b.block(6,8,1,1) << -3.47311;
	b.block(7,8,1,1) << -19.802;


	// make the parametre matrix symmetric
	for (int i=N-1;i>=0;i--){
		for (int j=0;j<i;j++){
			g(i,j)=g(j,i);
			b(i,j)=b(j,i);
		}
	}

	gs(8,6) = -0.0206557, gs(6,8) = 0.0221016;
	bs(8,6) = 0.227213, bs(6,8) = -0.243118;

	//cout << g << endl << b <<endl;
	//cout << gs << endl << bs << endl;
	return 0;
}

int TopologySys()
{
	for (int i=0;i<N;i++){
		for (int j=i+1;j<N;j++){
			if (b(i,j)!=0) {
				bus[i].nofconnection++;
				bus[j].nofconnection++;
			}
		}
	}
	int k; // count
	for (int i=0;i<N;i++){
		bus[i].connect = new int[bus[i].nofconnection];
		k = 0;
		for (int j=0;j<N;j++){
			if (b(i,j)!=0) {
				bus[i].connect[k] = j;
				k++;
			}
			if (k == bus[i].nofconnection) continue;
		}
	}
	// reset number of equations for slack buses and PV buses
	// bus 0 is a slack bus
	bus[0].nofequation = 0;
	// bus 1 is a PV bus
	bus[1].nofequation = 1;

	// counting the pv & pq buses with smaller indices
	for (int i=0;i<N-1;i++)
	{
		if(bus[i].nofequation){
			if(bus[i].nofequation==1) {
				bus[i+1].nofpv = bus[i].nofpv + 1;
				bus[i+1].nofpq = bus[i].nofpq;		
			}
			else{
				bus[i+1].nofpq = bus[i].nofpq + 1;
				bus[i+1].nofpv = bus[i].nofpv;
			}
		}	
	}
	return 0;
}

int Initialize()
{
	bus[0].voltageR = 1.03;
	bus[0].absvoltage = 1.03*1.03;
	bus[1].voltageR = 1.03;
	bus[1].absvoltage = 1.03*1.03;

	for (int i=0;i<16;i++) {
		for (int j=0;j<16;j++) {
			jacobi(i,j) = 0;
		}
		qf(i) = 0;
		x(i) = 0;
		dx(i) = 0;
	}

	// combine the unkonws into one vector
	for (int i=0;i<N;i++){
		if (bus[i].nofequation != 0){ // 2 variables induced when bus i is not a slack bus
			x(2*(bus[i].nofpq+bus[i].nofpv)) = bus[i].voltageR;
			x(2*(bus[i].nofpq+bus[i].nofpv)+1) = bus[i].voltageI;
		}
	}
	return 0;
}

int updateSys()
{
	for (int i=0;i<N;i++){
		if (bus[i].nofequation != 0){ // slack bus does not update
			if (bus[i].nofequation == 1){ // i is a PV bus, update voltageR and voltage I
				bus[i].voltageR = x(2*(bus[i].nofpq+bus[i].nofpv));
				bus[i].voltageI = x(2*(bus[i].nofpq+bus[i].nofpv)+1);
			}
			else{ // i is a PQ bus, update voltageR, voltageI and absvoltage
				bus[i].voltageR = x(2*(bus[i].nofpq+bus[i].nofpv));
				bus[i].voltageI = x(2*(bus[i].nofpq+bus[i].nofpv)+1);
				bus[i].absvoltage = bus[i].voltageI*bus[i].voltageI + bus[i].voltageR*bus[i].voltageR;
			}
		}
	}
	return 0;
}

int main()
{
	// Initialize everything
	AssembleSys();
	// construct topology within the system
	TopologySys();
	Initialize();

	// generating and load insert HERE!
	bus[1].pgen = 4.5;
	// Loading condition A
	bus[3].pload = 4.0;
	bus[3].qload = -1.0;
	bus[6].pload = 0.6;
	bus[6].qload = 0.1;
	bus[8].pload = 0.5;
	bus[8].qload = 0.2;

	for (int i=0;i<N;i++){
		gnk(i) = bus[i].pload-bus[i].pgen;
		bnk(i) = -(bus[i].qload-bus[i].qgen);
	}

	// check the topology
	for (int i = 0; i<9; i++){
		cout << bus[i].nofconnection;
		for (int j=0;j<bus[i].nofconnection;j++){
			cout << bus[i].connect[j];
		}
		cout << endl;
	}

	////////////////////////////////////quadratized power flow calculation//////////////////////////////////////////////////////////
	int k; // count
	int m;
	int count = 0;

	qf(0) = 1; // to start the iteration
	// iteration starts here
	while(qf.lpNorm<Infinity>()>1e-6){
		// cout << qf;
		// now we want to assemble jacobi matrix and qf residue vector

		// assemble quadratized power flow equations, every PQ and PV bus has exactly 2 quadratized power flow equations
		k = 0;
		double u1,u2;
		for (int i=0;i<N;i++){
			m = -1;
			u2 = bus[i].absvoltage;
			u1 = 1/u2 -1;
			if (bus[i].nofequation!=0){ // bus[i] is NOT a slack bus so qf equation exists
				if (bus[i].nofequation == 2){ // bus[i] is a PQ bus
					qf(k) = (1+u1)*(gnk(i)*bus[i].voltageR-bnk(i)*bus[i].voltageI); // real part
					qf(k+1) = (1+u1)*(gnk(i)*bus[i].voltageI+bnk(i)*bus[i].voltageR); // imagine part
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						qf(k) += gs(i,m)*bus[i].voltageR - bs(i,m)*bus[i].voltageI + g(i,m)*(bus[i].voltageR-bus[m].voltageR) - b(i,m)*(bus[i].voltageI-bus[m].voltageI);
						qf(k+1) += gs(i,m)*bus[i].voltageI + bs(i,m)*bus[i].voltageR + b(i,m)*(bus[i].voltageR-bus[m].voltageR) + g(i,m)*(bus[i].voltageI-bus[m].voltageI);
					}
				}
				else { // bus[i] is a PV bus
					qf(k) = 0; // real power part
					qf(k+1) = bus[i].voltageI*bus[i].voltageI + bus[i].voltageR*bus[i].voltageR - bus[i].absvoltage; // voltage regulation part
					cout << "pv bus " << qf(k+1); 
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						qf(k) += (gs(i,m)+g(i,m))*bus[i].absvoltage-g(i,m)*(bus[i].voltageR*bus[m].voltageR+bus[i].voltageI*bus[m].voltageI)+b(i,m)*(bus[i].voltageR*bus[m].voltageI-bus[i].voltageI*bus[m].voltageR);
					}
					qf(k) = qf(k) - bus[i].pgen + bus[i].pload;
				}
				k = k+2;
			}		
		}
		// we now get the power flow equation residues at this iteration step
		//cout << qf;
		//cout << qf << endl;
		//cout << qf(1) << endl << bus[1].voltageI*bus[1].voltageI + bus[1].voltageR*bus[1].voltageR << " "<< bus[1].absvoltage << bus[1].voltageI*bus[1].voltageI + bus[1].voltageR*bus[1].voltageR - bus[1].absvoltage;
		///////////////////////////////////////////jacobi matrix////////////////////////////////////////
		// thereafter, we want to cope with the jacobi matrix for the system
		k = 0;	
		for (int i=0;i<N;i++){
			m = -1;
			u2 = bus[i].absvoltage;
			u1 = 1/u2 -1;
			if (bus[i].nofequation!=0){ // bus[i] is NOT a slack bus so 2 qf equations exist	
				if (bus[i].nofequation == 2){  // bus[i] is a PQ bus use branch model
					jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) = (1 + u1)*gnk(i); // R vs Vi,real
					jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) = -(1 + u1)*bnk(i); // R vs Vi,imagine
					jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)) = (1 + u1)*bnk(i); // I vs Vi,real
					jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)+1) = (1 + u1)*gnk(i); // I vs Vi,imagine

					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						if (bus[m].nofequation == 0){ // m is a slack bus, no new term needs update
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) += g(i,m) + gs(i,m);// R vs Vi,real
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) += -bs(i,m) - b(i,m); // R vs Vi,imagine
							jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)) += bs(i,m) + b(i,m);// I vs Vi,real
							jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)+1) += g(i,m) + gs(i,m); // I vs Vi,imagine
						}
						else{ // m is NOT a slack bus, 2 new terms need update each row
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) += g(i,m) + gs(i,m);// R vs Vi,real
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) += -bs(i,m) - b(i,m); // R vs Vi,imagine
							jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)) += bs(i,m) + b(i,m);// I vs Vi,real
							jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)+1) += g(i,m) + gs(i,m); // I vs Vi,imagine

							jacobi(k,2*(bus[m].nofpq+bus[m].nofpv)) = -g(i,m);// R vs Vm,real
							jacobi(k,2*(bus[m].nofpq+bus[m].nofpv)+1) = b(i,m); // R vs Vm,imagine
							jacobi(k+1,2*(bus[m].nofpq+bus[m].nofpv)) = -b(i,m);// I vs Vm,real
							jacobi(k+1,2*(bus[m].nofpq+bus[m].nofpv)+1) = -g(i,m); // I vs Vm,imagine  
						}
					}		
					//  u1 and u2 are variables respect to voltages,thus some more terms partial u1 vs voltage are needed
					jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) += -2*bus[i].voltageR/bus[i].absvoltage/bus[i].absvoltage*(gnk(i)*bus[i].voltageR-bnk(i)*bus[i].voltageI); // R vs Vi,real
					jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) += -2*bus[i].voltageI/bus[i].absvoltage/bus[i].absvoltage*(gnk(i)*bus[i].voltageR-bnk(i)*bus[i].voltageI); // R vs Vi,imagine
					jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)) += -2*bus[i].voltageR/bus[i].absvoltage/bus[i].absvoltage*(gnk(i)*bus[i].voltageI+bnk(i)*bus[i].voltageR); // I vs Vi,real
					jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)+1) += -2*bus[i].voltageI/bus[i].absvoltage/bus[i].absvoltage*(gnk(i)*bus[i].voltageI+bnk(i)*bus[i].voltageR); // I vs Vi,imagine					
				}
				else{ // bus[i] is a PV bus
					jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) = 0; // Real power vs Vi,real
					jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) = 0; // Real power vs Vi,imagine
					jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)) = 2*bus[i].voltageR; // VR vs Vi,real
					jacobi(k+1,2*(bus[i].nofpq+bus[i].nofpv)+1) = 2*bus[i].voltageI; // VR vs Vi,imagine
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						if (bus[m].nofequation == 0){ // m is a slack bus, no new term needs update
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) += -g(i,m)*bus[m].voltageR + b(i,m)*bus[m].voltageI;// Real power vs Vi,real
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) += -g(i,m)*bus[m].voltageI - b(i,m)*bus[m].voltageR; // Real power vs Vi,imagine
						}
						else{ // m is NOT a slack bus, 2 new terms need update each row 
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)) += -g(i,m)*bus[m].voltageR + b(i,m)*bus[m].voltageI;// Real power vs Vi,real
							jacobi(k,2*(bus[i].nofpq+bus[i].nofpv)+1) += -g(i,m)*bus[m].voltageI - b(i,m)*bus[m].voltageR; // Real power vs Vi,imagine
							jacobi(k,2*(bus[m].nofpq+bus[m].nofpv)) = -g(i,m)*bus[i].voltageR - b(i,m)*bus[i].voltageI;// Real power vs Vm,real
							jacobi(k,2*(bus[m].nofpq+bus[m].nofpv)+1) = b(i,m)*bus[i].voltageR - g(i,m)*bus[i].voltageI; // Real power vs Vm,imagine
						}
					}
				}
				k = k+2;
			}
		}

		//cout << jacobi << endl;
		// for (int i=0;i<16;i++)	cout <<i<<endl<< jacobi.row(i)<<endl;
		// solves the N-R problem
		dx = jacobi.colPivHouseholderQr().solve(qf);
		//cout << endl<<dx.lpNorm<Infinity>();
		x = x - dx;


		// update the voltages and angles for new iteration
		// this needs special care, different from NR method
		updateSys();
		for (int i=0;i<N;i++){
			cout << bus[i].voltageR << endl << bus[i].voltageI << endl;
			cout << "abs:" << bus[i].absvoltage << " " << bus[i].voltageR*bus[i].voltageR + bus[i].voltageI*bus[i].voltageI << endl;
		}

		count++;

	};

	return 0;
}
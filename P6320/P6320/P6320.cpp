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

// System Buses
CCBus bus[N];

// System states and intermediate variables
MatrixXd jacobi(15,15);
VectorXd pf(15),x(15),dx(15);
VectorXd voltage(N);
VectorXd angle(N);

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

int Initialize()
{
	for (int i=0;i<N;i++) {
		angle(i) = 0;
		voltage(i) = 1;
	}

	voltage(0) = 1.03;
	voltage(1) = 1.03;

	for (int i=0;i<15;i++) {
		for (int j=0;j<15;j++) {
			jacobi(i,j) = 0;
		}
		pf(i) = 0;
		x(i) = 0;
		dx(i) = 0;
	}

	// combine the unkonws into one vector
	x << angle.block(1,0,8,1), voltage.block(2,0,7,1);
	return 0;
}

int main()
{
	// read the number of pq buses and pv buses
	int nofpq = 0;
	int nofpv = 0;

	// Initialize everything
	AssembleSys();
	Initialize();
	// Initialize Complete

	// construct topology within the system

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

	// counting for the pv & pq buses with smaller indices
	for (int i=0;i<N-1;i++)
	{
		if(bus[i].nofequation){
			if(bus[i].nofequation==1) {
				bus[i+1].nofpv = bus[i].nofpv + 1;
				bus[i+1].nofpq = bus[i].nofpq;
				nofpv++;
			}
			else{
				bus[i+1].nofpq = bus[i].nofpq + 1;
				bus[i+1].nofpv = bus[i].nofpv;
				nofpq++;
			}
		}	
	}
	if (bus[N-1].nofequation==1) nofpv++;
	else if (bus[N-1].nofequation==2) nofpq++;

	/*for (int i=0;i<9;i++){
		cout << bus[i].nofpq << " " << bus[i].nofpv << endl;
	}*/
	
	// generating and load insert HERE!
	bus[1].pgen = 4.5;
// Loading condition A
	bus[3].pload = 4.0;
	bus[3].qload = -1.0;
	bus[6].pload = 0.6;
	bus[6].qload = 0.1;
	bus[8].pload = 0.5;
	bus[8].qload = 0.2;

	 
/*	// check the topology
	for (int i = 0; i<9; i++){
	cout << bus[i].nofconnection;
	for (int j=0;j<bus[i].nofconnection;j++){
	cout << bus[i].connect[j];
	}
	cout << endl;
	}*/
	

////////////////////////////////////power flow calculation//////////////////////////////////////////////////////////

	int m;
	double alpha,beta;
	int count = 0;
	double residue = 1 ;
	pf(0) = 1; // to start the iteration
	// iteration starts here
	//while(pf.lpNorm<Infinity>()>1e-6){
	while(abs(dx.lpNorm<Infinity>()-residue)>1e-4){
		residue = dx.lpNorm<Infinity>();
		//cout << pf;
		// now we want to assemble jacobi matrix and pf residue vector

		// assemble quadratized power flow equations 
		k = 0;
		for (int i=0;i<N;i++){
			m = -1;
			alpha = 0;
			beta = 0;
			if (bus[i].nofequation!=0){ // bus[i] is NOT a slack bus so pf equation exists
				if (bus[i].nofequation==1){ // there is one real pf equation in this case, bus[i] is a PV bus
					pf(k) = voltage(i)*voltage(i)*g(i,i);
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						alpha = g(i,m)*cos(angle(i)-angle(m)) + b(i,m)*sin(angle(i)-angle(m));
						pf(k) += voltage(i)*voltage(i)*(g(i,m)+gs(i,m)) - voltage(i)*voltage(m)*alpha; 
					}
					pf(k) = pf(k) - bus[i].pgen + bus[i].pload; 
					k++;
				}
				else{  // there are two pf equations in this case, bus[i] is a PQ bus
					pf(k) = voltage(i)*voltage(i)*g(i,i);
					pf(k+1) = -voltage(i)*voltage(i)*b(i,i);
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						alpha = g(i,m)*cos(angle(i)-angle(m)) + b(i,m)*sin(angle(i)-angle(m));
						beta = g(i,m)*sin(angle(i)-angle(m)) - b(i,m)*cos(angle(i)-angle(m));
						pf(k) += voltage(i)*voltage(i)*(g(i,m)+gs(i,m)) - voltage(i)*voltage(m)*alpha; 
						pf(k+1) += -voltage(i)*voltage(i)*(b(i,m)+bs(i,m)) - voltage(i)*voltage(m)*beta; 					
					}
					pf(k) = pf(k) - bus[i].pgen + bus[i].pload; 
					pf(k+1) = pf(k+1) - bus[i].qgen + bus[i].qload; 
					k = k+2;
				}
			}			
		}
		// we now get the power flow equation residues at this iteration step
		
		// cout << pf;
		
///////////////////////////////////////////jacobi matrix////////////////////////////////////////
		// thereafter, we want to cope with the jacobi matrix for the system
		// which is really a pain 
		k = 0;
		double dalpha,dbeta;
		for (int i=0;i<N;i++){
			m = -1;
			alpha = 0;
			beta = 0;
			dalpha = 0;
			dbeta = 0; 
			if (bus[i].nofequation!=0){ // bus[i] is NOT a slack bus so pf equation exists
				if (bus[i].nofequation==1){ // there is one real pf equation in this case, bus[i] is a PV bus, one row in jacobi needs update
					jacobi(k,bus[i].nofpq+bus[i].nofpv) = 0; // P vs angle(i)
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						if (bus[m].nofequation == 0){ // m is a slack bus, no new term needs to be updated
							dalpha = -g(i,m)*sin(angle[i]-angle[m]) + b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(i)
							jacobi(k,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dalpha; //  P vs angle(i)
						}
						else if (bus[m].nofequation == 1){ // m is a PV bus, one term needs to be updated 
							dalpha = -g(i,m)*sin(angle[i]-angle[m]) + b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(i)
							jacobi(k,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dalpha; //  P vs angle(i)
							dalpha = g(i,m)*sin(angle[i]-angle[m]) - b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(m)
							jacobi(k,bus[m].nofpq+bus[m].nofpv) = -voltage[i]*voltage[m]*dalpha; //  P vs angle(m)
						}
						else{  // m is a PQ bus, two terms need update
							dalpha = -g(i,m)*sin(angle[i]-angle[m]) + b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(i)
							jacobi(k,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dalpha; // P vs angle(i)
							dalpha = g(i,m)*sin(angle[i]-angle[m]) - b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(m)
							jacobi(k,bus[m].nofpq+bus[m].nofpv) = -voltage[i]*voltage[m]*dalpha; // P vs angle(m)
							alpha = g(i,m)*cos(angle(i)-angle(m)) + b(i,m)*sin(angle(i)-angle(m)); // alpha
							jacobi(k,nofpq+nofpv+bus[m].nofpq) = -voltage(i)*alpha; // P vs voltage(m)
							
						}
					}
					k++;
				}
				else{ // there are two pf equations in this case, bus[i] is a PQ bus, two rows in jacobi needs update
					jacobi(k,bus[i].nofpq+bus[i].nofpv) = 0; // P vs angle(i)
					jacobi(k,nofpq+nofpv+bus[i].nofpq) = 2*voltage(i)*g(i,i); // P vs voltage(i)
					jacobi(k+1,bus[i].nofpq+bus[i].nofpv) = 0; // Q vs angle(i)
					jacobi(k+1,nofpq+nofpv+bus[i].nofpq) = -2*voltage(i)*b(i,i); // Q vs voltage(i)
					for (int j=0;j<bus[i].nofconnection;j++){
						m = bus[i].connect[j];
						if (bus[m].nofequation == 0){ // m is a slack bus, no new term needs to be updated
							dalpha = -g(i,m)*sin(angle[i]-angle[m]) + b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(i)
							jacobi(k,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dalpha; // P vs angle(i)
							alpha = g(i,m)*cos(angle(i)-angle(m)) + b(i,m)*sin(angle(i)-angle(m)); // alpha
							jacobi(k,nofpq+nofpv+bus[i].nofpq) += 2*voltage(i)*(g(i,m)+gs(i,m)) - alpha*voltage(m); // P vs voltage(i)

							dbeta = g(i,m)*cos(angle[i]-angle[m]) + b(i,m)*sin(angle(i)-angle(m)); // beta vs angle(i)
							jacobi(k+1,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dbeta; // Q vs angle(i)
							beta = g(i,m)*sin(angle(i)-angle(m)) - b(i,m)*cos(angle(i)-angle(m)); // beta
							jacobi(k+1,nofpq+nofpv+bus[i].nofpq) += -2*voltage(i)*(b(i,m)+bs(i,m)) - beta*voltage(m); // Q vs voltage(i)							
						}
						else if (bus[m].nofequation == 1){ // m is a PV bus, one term needs to be updated 
							dalpha = -g(i,m)*sin(angle[i]-angle[m]) + b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(i)
							jacobi(k,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dalpha; // P vs angle(i)
							dalpha = g(i,m)*sin(angle[i]-angle[m]) - b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(m)
							jacobi(k,bus[m].nofpq+bus[m].nofpv) = -voltage[i]*voltage[m]*dalpha; //  P vs angle(m)
							alpha = g(i,m)*cos(angle(i)-angle(m)) + b(i,m)*sin(angle(i)-angle(m)); // alpha
							jacobi(k,nofpq+nofpv+bus[i].nofpq) += 2*voltage(i)*(g(i,m)+gs(i,m)) - alpha*voltage(m); // P vs voltage(i)
							
							dbeta = g(i,m)*cos(angle[i]-angle[m]) + b(i,m)*sin(angle(i)-angle(m)); // beta vs angle(i)
							jacobi(k+1,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dbeta; // Q vs angle(i)
							dbeta = -g(i,m)*cos(angle[i]-angle[m]) - b(i,m)*sin(angle(i)-angle(m)); // beta vs angle(m)
							jacobi(k+1,bus[m].nofpq+bus[m].nofpv) = -voltage[i]*voltage[m]*dbeta; //  Q vs angle(m)
							beta = g(i,m)*sin(angle(i)-angle(m)) - b(i,m)*cos(angle(i)-angle(m)); // beta
							jacobi(k+1,nofpq+nofpv+bus[i].nofpq) += -2*voltage(i)*(b(i,m)+bs(i,m)) - beta*voltage(m); // Q vs voltage(i)	
						}
						else{  // m is a PQ bus, two more terms need update
							dalpha = -g(i,m)*sin(angle[i]-angle[m]) + b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(i)
							jacobi(k,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dalpha; // P vs angle(i)
							dalpha = g(i,m)*sin(angle[i]-angle[m]) - b(i,m)*cos(angle(i)-angle(m)); // alpha vs angle(m)
							jacobi(k,bus[m].nofpq+bus[m].nofpv) = -voltage[i]*voltage[m]*dalpha; //  P vs angle(m)
							alpha = g(i,m)*cos(angle(i)-angle(m)) + b(i,m)*sin(angle(i)-angle(m)); // alpha
							jacobi(k,nofpq+nofpv+bus[i].nofpq) += 2*voltage(i)*(g(i,m)+gs(i,m)) - alpha*voltage(m); // P vs voltage(i)
							jacobi(k,nofpq+nofpv+bus[m].nofpq) = -voltage(i)*alpha; // P vs voltage(m)
							
							dbeta = g(i,m)*cos(angle[i]-angle[m]) + b(i,m)*sin(angle(i)-angle(m)); // beta vs angle(i)
							jacobi(k+1,bus[i].nofpq+bus[i].nofpv) += -voltage[i]*voltage[m]*dbeta; // Q vs angle(i)
							dbeta = -g(i,m)*cos(angle[i]-angle[m]) - b(i,m)*sin(angle(i)-angle(m)); // beta vs angle(m)
							jacobi(k+1,bus[m].nofpq+bus[m].nofpv) = -voltage[i]*voltage[m]*dbeta; //  Q vs angle(m)
							beta = g(i,m)*sin(angle(i)-angle(m)) - b(i,m)*cos(angle(i)-angle(m)); // beta
							jacobi(k+1,nofpq+nofpv+bus[i].nofpq) += -2*voltage(i)*(b(i,m)+bs(i,m)) - beta*voltage(m); // Q vs voltage(i)
							jacobi(k+1,nofpq+nofpv+bus[m].nofpq) = -voltage(i)*beta; // Q vs voltage(m)
						}
					}
					k = k+2;
				}
			}
		}
	
		
		//for (int i=0;i<15;i++)	cout <<i<<endl<< jacobi.row(i)<<endl;
		// solves the N-R problem
		dx = jacobi.colPivHouseholderQr().solve(pf);
		//cout << endl<<dx.lpNorm<Infinity>();
		x = x - dx;
	    // update the voltages and angles for new iteration
		angle.block(1,0,8,1) << x.block(0,0,8,1);
		voltage.block(2,0,7,1) << x.block(8,0,7,1);
		
		cout << count << endl << "angle =" << endl << angle << endl << "voltage =" << endl << voltage << endl;
		cout << pf << endl;
		count++;
		
	};


	return 0;
}
/*
 * main.cpp
 *
 *  Created on: 19 Apr 2018
 *      Author: User
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <string>
#include "pin.H"
#include <list>
#include <map>
#include <vector>

using namespace std;

ofstream outFile;
typedef struct RtnCount {
    ADDRINT curr_addr;
	ADDRINT target_addr;
	UINT32 _icount;
	string RTN_name;
	UINT64 localCounter;
	UINT64 lastCounter;
	UINT64 _CountSeen; //EX2
	UINT64 _CountLoopInvoked; //EX2
	UINT64 _MeanTaken;//EX2
	UINT64 _DiffCount;//EX2
	ADDRINT _routineAddress;//EX2
	UINT64 _routineName;//EX2
	UINT64 _routineInvocationsCount;//EX2
} RTN_COUNT;

list<RTN_COUNT*> newList;
typedef map<ADDRINT, RTN_COUNT*> myMap;
typedef map<ADDRINT,UINT64> rtnMap;
myMap newMap;
rtnMap myRtns;


VOID docount(UINT64 * counter ) {
	(*counter)++;
}



VOID docountIterations(RTN_COUNT* rc) {
	rc->_CountSeen++;
	rc->localCounter++;
}




VOID docountInvocations(RTN_COUNT* rc) {
	
	rc->_CountLoopInvoked++;
	if(rc->localCounter != rc->lastCounter && rc->lastCounter != 0)
	{
		rc->_DiffCount++;
	}
	rc->lastCounter=rc->localCounter;
	rc->localCounter=0;
}



bool vecCompare(RTN_COUNT* first, RTN_COUNT* second)
{
	if (first->_CountSeen >  second->_CountSeen)
	{
		return true;
	}
	return false;
}



bool compare_fun(const RTN_COUNT* first, const RTN_COUNT* last) {

	if (first->_CountSeen > last->_CountSeen) {
		return true;

	}
	return false;
}

VOID Routine(RTN rtn, VOID *v) {
	

	RTN_Open(rtn);
	std::map<ADDRINT, RTN_COUNT*>::iterator it2 = newMap.begin();
	std::map<ADDRINT, UINT64>::iterator it3 = myRtns.begin();
	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
			if(INS_IsDirectBranch (ins) && INS_HasFallThrough(ins) ) {//&&  expression to determine if the current instruction is a conditional branchEX2
				INT32 insCndBr = INS_Category(ins);
				if((insCndBr == XED_CATEGORY_COND_BR))
				{
					ADDRINT currentAddress = INS_Address(ins);
					ADDRINT targetAddress = INS_DirectBranchOrCallTargetAddress(ins);
					if(targetAddress < currentAddress)
					{	
						it2 = newMap.find(currentAddress);
						if (it2 == newMap.end()) {
					
								RTN_COUNT* rc = new RTN_COUNT;
								rc->RTN_name = RTN_Name(rtn);
								rc->_routineAddress = RTN_Address(rtn);
								rc->curr_addr = currentAddress;
								rc->target_addr = targetAddress;
								rc->_CountSeen = 0;
								rc->_CountLoopInvoked = 0;
								rc->localCounter = 0;
								rc->lastCounter = 0;
								rc->_DiffCount = 0;
								rc->_icount=RTN_NumIns(rtn);
								newMap.insert(pair<ADDRINT, RTN_COUNT*>(currentAddress,rc));
								it2 = newMap.find(currentAddress);
						}
						
						
							INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docountIterations, IARG_PTR,it2->second,IARG_BRANCH_TAKEN,IARG_END);
							INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) docountInvocations, IARG_PTR ,it2->second , IARG_END);

					}
				}

			}//EX2zz
			
			if(INS_IsRet(ins))
			{
				it3 = myRtns.find(RTN_Address(rtn));
				if (it3 == myRtns.end()) {
					myRtns.insert(pair<ADDRINT, UINT64>(RTN_Address(rtn),0));
					it3 = myRtns.find(RTN_Address(rtn));
				}
				
				INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) docount, IARG_PTR,
				&((it3->second)), IARG_END);
			}				
	}
	RTN_Close(rtn);

}

VOID Fini(INT32 code, VOID *v) {
	
	vector<RTN_COUNT*> myVec;
	std::map<ADDRINT, UINT64>::iterator it3 = myRtns.begin();
	for (std::map<ADDRINT, RTN_COUNT*>::iterator it = newMap.begin(); it != newMap.end(); ++it)
	{
		myVec.push_back(it->second);
	}
	std::sort (myVec.begin(), myVec.end(), vecCompare);
	
	for(std::vector<RTN_COUNT*>::iterator it = myVec.begin(); it != myVec.end(); ++it) {
		double meanTaken=0;
		RTN_COUNT *rc = *it;
		if(rc->_CountLoopInvoked==0)
		{
			meanTaken=rc->_CountSeen;
		}
		else
		{
			meanTaken=(double)((double)rc->_CountSeen/(double)rc->_CountLoopInvoked);
		}
		it3 = myRtns.find(rc->_routineAddress);
		if (rc && (rc->_CountSeen) > 0 ) {
			outFile << "0x" << hex << rc->target_addr << "," << dec << rc->_CountSeen << "," << rc->_CountLoopInvoked << "," << meanTaken << "," << rc->_DiffCount <<  "," << rc->RTN_name << ","  << "0x"  << hex << rc->_routineAddress << "," << dec <<  it3->second << endl;
		}
	}
}

INT32 Usage() {
	cerr << "Error"
			<< endl;
	cerr << "Error" << endl;
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char * argv[]) {
	PIN_InitSymbols();
	outFile.open("loop-count.csv");
	if (PIN_Init(argc, argv))
		return Usage();
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);
	PIN_StartProgram();

	return 0;
}

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
#include "pin.H"
#include <list>
using namespace std;

ofstream outFile;
typedef struct RtnCount {
	ADDRINT image_address;
	string image_name;
	ADDRINT RTN_address;
	string RTN_name;
	UINT64 _icount;
	RTN _rtn;
} RTN_COUNT;

list<RTN_COUNT*> newList;

VOID docount(UINT64 * counter) {
	(*counter)++;
}

bool compare_fun(const RTN_COUNT* first, const RTN_COUNT* last) {

	if (first->_icount > last->_icount) {
		return true;

	}
	return false;
}

VOID Routine(RTN rtn, VOID *v) {
	RTN_COUNT* rc = new RTN_COUNT;

	rc->RTN_name = RTN_Name(rtn);
	rc->image_name = IMG_Name(SEC_Img(RTN_Sec(rtn)));
	rc->image_address = IMG_StartAddress(SEC_Img(RTN_Sec(rtn)));
	rc->RTN_address = RTN_Address(rtn);
	rc->_icount = 0;
	newList.push_front(rc);

	RTN_Open(rtn);
	std::list<RTN_COUNT *>::iterator it = newList.begin();
	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) docount, IARG_PTR,
				&((*it)->_icount), IARG_END);
	}

	RTN_Close(rtn);
}

VOID Fini(INT32 code, VOID *v) {
	newList.sort(compare_fun);
	for (std::list<RTN_COUNT *>::iterator it = newList.begin();
			it != newList.end(); ++it) {
		RTN_COUNT *rc = *it;
		if (rc && (rc->_icount) > 0) {
			outFile << hex << "0x" << rc->image_address << "," << rc->image_name
					<< ",0x" << hex << rc->RTN_address << "," << rc->RTN_name
					<< "," << dec << (rc->_icount) << endl;
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
	outFile.open("rtn-output.csv");
	if (PIN_Init(argc, argv))
		return Usage();
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);
	PIN_StartProgram();

	return 0;
}

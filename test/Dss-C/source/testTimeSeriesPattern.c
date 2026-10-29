#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "heclib.h"
#include "hecdssInternal.h"
#include "TestDssC.h"


	//  Regular interval floats, basic

int testTimeSeriesPattern(long long *ifltab)
{
	zStructTimeSeries *tss1, *tss2;
	float fvalues[200];
	double dvalues[200];
	int times[200];
	int i;
	int status;
	int zero = 0;

	for (i=0; i<200; i++) {
		fvalues[i] = (float)i;
		dvalues[i] = (double)i;
		times[i] = 10000 + i * 10;
	}

	printf("enter testTimeSeriesPattern\n");
	tss1 = zstructTsNewRegFloats("/Basin/Location/Flow/TS-Pattern/1Hour/Float Pattern/", fvalues, 200, "21Jan2001", "1200", "cfs", "Inst-Val");
	tss1->startJulianDate = UNDEFINED_TIME;
	tss1->startTimeSeconds = -1;
	tss1->dataType = DATA_TYPE_RTS_PATTERN;
	status = ztsStore(ifltab, tss1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 1, store status ")) return status; 

	tss2 = zstructTsNew("/Basin/Location/Flow/TS-Pattern/1Hour/Float Pattern/"); 
	status = ztsRetrieve(ifltab, tss2, 0, 0, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 2, retrieve status ")) return status; 

	status = zcompareDataSets(ifltab, tss1, tss2, 1, 0, tss1->pathname, "Fail in testTimeSeriesPattern, Location 3");
	if (status) return status;

	zstructFree(tss1);
	zstructFree(tss2);

	//  Other variations not supported by DSS-6
	if (zgetVersion(ifltab) == 7) {
		tss1 = zstructTsNewRegDoubles("/Basin/Location/Flow/TS-Pattern/1Hour/Double Pattern/", dvalues, 200, "21Jan2001", "1200", "cfs", "Inst-Val");
		tss1->startJulianDate = UNDEFINED_TIME;
		tss1->startTimeSeconds = -1;
		status = ztsStore(ifltab, tss1, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 11, store status ")) return status; 

		tss2 = zstructTsNew("/Basin/Location/Flow/TS-Pattern/1Hour/Double Pattern/"); 
		status = ztsRetrieve(ifltab, tss2, 0, 0, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 12, retrieve status ")) return status; 

		status = zcompareDataSets(ifltab, tss1, tss2, 1, 0, tss1->pathname, "Fail in testTimeSeriesPattern, Location 13");
		if (status) return status;

		zstructFree(tss1);
		zstructFree(tss2);

	/*				int retrieveDoublesFlag - 4th argument
	*					A flag indicating if floats or doubles should be returned.  This is independent of
	*					what is actually stored on disk!  Values will be converted to the requested type.
	*						0:  Return data as stored.  If missing, will return as doubles.
	*						1:  Return floats
	*						2:  Return doubles
	*/

		//  test double to float conversion
		tss1 = zstructTsNew("/Basin/Location/Flow/TS-Pattern/1Hour/Float Pattern/"); 
		status = ztsRetrieve(ifltab, tss1, 0, 1, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 21, retrieve status ")) return status; 

		tss2 = zstructTsNew("/Basin/Location/Flow/TS-Pattern/1Hour/Double Pattern/"); 
		status = ztsRetrieve(ifltab, tss2, 0, 1, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 22, retrieve status ")) return status; 

		//  We need to over-ride the obvious differences between the structs
		tss2->dataType = 101;

		status = zcompareDataSets(ifltab, tss1, tss2, 1, 0, tss1->pathname, "Fail in testTimeSeriesPattern, Location 23");
		if (status) return status;

		zstructFree(tss1);
		zstructFree(tss2);


		//  test float to double conversion
		tss1 = zstructTsNew("/Basin/Location/Flow/TS-Pattern/1Hour/Float Pattern/"); 
		status = ztsRetrieve(ifltab, tss1, 0, 2, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 31, retrieve status ")) return status; 

		tss2 = zstructTsNew("/Basin/Location/Flow/TS-Pattern/1Hour/Double Pattern/"); 
		status = ztsRetrieve(ifltab, tss2, 0, 2, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 32, retrieve status ")) return status; 

		//  We need to over-ride the obvious differences between the structs
		tss1->dataType = 106;

		status = zcompareDataSets(ifltab, tss1, tss2, 1, 0, tss1->pathname, "Fail in testTimeSeriesPattern, Location 33");
		if (status) return status;

		zstructFree(tss1);
		zstructFree(tss2);

	}


	/*
	tss1 = zstructTsNewRegFloats("/Basin/Location/Flow//1Hour/TSS-Floats/", fvalues, 200, "21Jan2001", "1200", "cfs", "Inst-Val");
	status = ztsStore(ifltab, tss1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 1, store status ")) return status; 


	tss2 = zstructTsNew("/Basin/Location/Flow/01Jan2001/1Hour/TSS-Floats/"); 
	status = ztsRetrieve(ifltab, tss2, -1, 1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 2, retrieve status ")) return status; 

	status = zcompareDataSets(ifltab, tss1, tss2, 1, 0, tss1->pathname, "Fail in testTimeSeriesPattern, Location 3");
	if (status) return status;

	zstructFree(tss1);
	zstructFree(tss2);
	/*

	tss1 = zstructTsNewRegFloats("/Basin/Location/Flow//1Hour/User Header/", fvalues, 200, "21Jan2001", "1200", "cfs", "Inst-Val");

	tss1->userHeader = (int *)calloc(5,4);
	tss1->userHeader[0] = 1;
	tss1->userHeader[1] = 2;
	tss1->userHeader[2] = 3;
	tss1->userHeader[3] = 4;
	tss1->userHeader[4] = 5;
	tss1->userHeaderNumber = 5;

	status = ztsStore(ifltab, tss1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 10, store status ")) return status; 


	tss2 = zstructTsNew("/Basin/Location/Flow/01Jan2001/1Hour/User Header/"); 
	status = ztsRetrieve(ifltab, tss2, -1, 1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPattern Loc 11, retrieve status ")) return status; 

	status = zcompareDataSets(ifltab, tss1, tss2, 1, 0, tss1->pathname, "Fail in testTimeSeriesPattern, Location 12");
	if (status) return status;

	free(tss1->userHeader);
	tss1->userHeader = 0;
	zstructFree(tss1);
	zstructFree(tss2);

	*/

	return 0; 
}

//  Rewrite a stored time series record with a different internal header
//  (as another writer may produce), bypassing ztsStore().
static int rewriteTimeSeriesHeader(long long *ifltab, const char *pathname, int *internalHeader, int numberInts)
{
	zStructTransfer *ztransferRead;
	zStructTransfer *ztransferWrite;
	int status;

	ztransferRead = zstructTransferNew(pathname, 1);
	if (!ztransferRead) return -1;
	status = zread(ifltab, ztransferRead);
	if (status != STATUS_RECORD_FOUND) {
		zstructFree(ztransferRead);
		return -1;
	}
	ztransferWrite = zstructTransferNew(pathname, 0);
	if (!ztransferWrite) {
		zstructFree(ztransferRead);
		return -1;
	}
	ztransferWrite->dataType = ztransferRead->dataType;
	ztransferWrite->internalHeader = internalHeader;
	ztransferWrite->internalHeaderNumber = numberInts;
	ztransferWrite->header2 = ztransferRead->header2;
	ztransferWrite->header2Number = ztransferRead->header2Number;
	ztransferWrite->values1 = ztransferRead->values1;
	ztransferWrite->values1Number = ztransferRead->values1Number;
	ztransferWrite->values2 = ztransferRead->values2;
	ztransferWrite->values2Number = ztransferRead->values2Number;
	ztransferWrite->values3 = ztransferRead->values3;
	ztransferWrite->values3Number = ztransferRead->values3Number;
	ztransferWrite->numberValues = ztransferRead->numberValues;
	ztransferWrite->logicalNumberValues = ztransferRead->logicalNumberValues;
	status = zwrite(ifltab, ztransferWrite);
	zstructFree(ztransferWrite);
	zstructFree(ztransferRead);
	return status;
}


//  Units text in a time series internal header must be read only from the
//  stored header.  Text that fills the header without a null terminator
//  ends at the end of the header; type and time zone are then not set.
int testTimeSeriesUnitsShortHeader(long long *ifltab)
{
	zStructTimeSeries *tss;
	float fvalues[24];
	int internalHeader[20];
	int numberInts;
	int i;
	int status;
	const char *expected;
	const char *pathname = "/Basin/Location/Flow/TS-Pattern/1Hour/Unterminated Units/";

	for (i = 0; i < 24; i++) {
		fvalues[i] = (float)i;
	}

	for (numberInts = INT_HEAD_units + 2; numberInts <= INT_HEAD_units + 3; numberInts++) {
		tss = zstructTsNewRegFloats(pathname, fvalues, 24, "21Jan2001", "1200", "cfs", "Inst-Val");
		tss->startJulianDate = UNDEFINED_TIME;
		tss->startTimeSeconds = -1;
		tss->dataType = DATA_TYPE_RTS_PATTERN;
		status = ztsStore(ifltab, tss, 0);
		zstructFree(tss);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesUnitsShortHeader Loc 1, store status ")) return status;

		//  Word INT_HEAD_units is the blank padding word; the text follows it, with no null
		for (i = 0; i < 20; i++) {
			internalHeader[i] = 0;
		}
		internalHeader[INT_HEAD_timeGranularity] = 1;
		if (numberInts == INT_HEAD_units + 2) {
			charLong("ABCD", &internalHeader[INT_HEAD_units + 1], 4, 4, 1, 0);
			expected = "ABCD";
		}
		else {
			charLong("ABCDEFGH", &internalHeader[INT_HEAD_units + 1], 8, 8, 1, 0);
			expected = "ABCDEFGH";
		}
		status = rewriteTimeSeriesHeader(ifltab, pathname, internalHeader, numberInts);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesUnitsShortHeader Loc 2, write status ")) return status;

		tss = zstructTsNew(pathname);
		status = ztsRetrieve(ifltab, tss, 0, 1, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesUnitsShortHeader Loc 3, retrieve status ")) {
			zstructFree(tss);
			return status;
		}
		if ((tss->numberValues != 24) || !tss->units || strcmp(tss->units, expected) ||
			tss->type || tss->timeZoneName) {
			printf("Fail in testTimeSeriesUnitsShortHeader Loc 4, %d int header: numberValues %d, units \"%s\", type \"%s\", time zone \"%s\"; expected 24, \"%s\", (null), (null)\n",
				numberInts, tss->numberValues, tss->units ? tss->units : "(null)",
				tss->type ? tss->type : "(null)", tss->timeZoneName ? tss->timeZoneName : "(null)", expected);
			zstructFree(tss);
			return -1;
		}
		zstructFree(tss);
	}

	return 0;
}


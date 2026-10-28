#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "heclib.h"
#include "TestDssC.h"


//  Write a regular interval pattern record whose internal header holds only
//  numberInts ints (as other writers may produce), bypassing ztsStore().
static int writePatternRecord(long long *ifltab, const char *pathname, float *values,
							  int numberValues, int *internalHeader, int numberInts)
{
	zStructTransfer* ztransfer;
	int status;

	ztransfer = zstructTransferNew(pathname, 0);
	if (!ztransfer) return -1;
	ztransfer->dataType = DATA_TYPE_RTS_PATTERN;
	ztransfer->numberValues = numberValues;
	ztransfer->values1 = (int *)values;
	ztransfer->values1Number = numberValues;
	ztransfer->internalHeader = internalHeader;
	ztransfer->internalHeaderNumber = numberInts;
	status = zwrite(ifltab, ztransfer);
	zstructFree(ztransfer);
	return status;
}


//  A pattern record with an internal header too short to hold the time offset
//  (fewer than INT_HEAD_timeOffset + 1 ints) must read as if the offset were zero,
//  not pick up whatever follows the header in memory.
//  The time offset of a header long enough to hold it is still applied.
//  ztsGetSizes() on a regular interval block with a short header reports zeros
//  for the slots that are not stored.
int testTimeSeriesPatternShortHeader(long long *ifltab)
{
	zStructTimeSeries *tss;
	zStructRecordSize *recordSize;
	zStructTransfer* ztransfer;
	float fvalues[24];
	int internalHeader[4];  //  Whole 64-bit words; heclib stores headers on 8-byte boundaries
	int startJulian;
	int startSeconds;
	int numberInts;
	int i;
	int status;
	char path[100];

	for (i = 0; i < 24; i++) {
		fvalues[i] = (float)i;
	}

	//  Reference: the same pattern stored normally, with a zero offset
	tss = zstructTsNewRegFloats("/Basin/Location/Flow/TS-Pattern/15Minute/Full Header/", fvalues, 24, "21Jan2001", "1200", "cfs", "Inst-Val");
	tss->startJulianDate = UNDEFINED_TIME;
	tss->startTimeSeconds = -1;
	tss->dataType = DATA_TYPE_RTS_PATTERN;
	status = ztsStore(ifltab, tss, 0);
	zstructFree(tss);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 1, store status ")) return status;

	tss = zstructTsNew("/Basin/Location/Flow/TS-Pattern/15Minute/Full Header/");
	status = ztsRetrieve(ifltab, tss, 0, 1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 2, retrieve status ")) return status;
	startJulian = tss->startJulianDate;
	startSeconds = tss->startTimeSeconds;
	zstructFree(tss);

	//  Headers of 1 and 2 ints have no time offset word
	for (numberInts = 1; numberInts <= INT_HEAD_timeOffset; numberInts++) {
		for (i = 0; i < 4; i++) {
			internalHeader[i] = 0;
		}
		snprintf(path, sizeof(path), "/Basin/Location/Flow/TS-Pattern/15Minute/Header %d/", numberInts);
		status = writePatternRecord(ifltab, path, fvalues, 24, internalHeader, numberInts);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 10, write status ")) return status;

		tss = zstructTsNew(path);
		status = ztsRetrieve(ifltab, tss, 0, 1, 0);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 11, retrieve status ")) return status;
		if ((tss->numberValues != 24) || (tss->timeOffsetSeconds != 0) ||
			(tss->startJulianDate != startJulian) || (tss->startTimeSeconds != startSeconds)) {
			printf("Fail in testTimeSeriesPatternShortHeader Loc 12, %d int header: numberValues %d, timeOffsetSeconds %d, start %d %d, expected 24, 0, %d %d\n",
				numberInts, tss->numberValues, tss->timeOffsetSeconds, tss->startJulianDate, tss->startTimeSeconds,
				startJulian, startSeconds);
			zstructFree(tss);
			return -1;
		}
		zstructFree(tss);
	}

	//  A 3 int header holds the time offset, which is applied
	for (i = 0; i < 4; i++) {
		internalHeader[i] = 0;
	}
	internalHeader[INT_HEAD_timeOffset] = 180;
	status = writePatternRecord(ifltab, "/Basin/Location/Flow/TS-Pattern/15Minute/Header 3/", fvalues, 24, internalHeader, INT_HEAD_timeOffset + 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 20, write status ")) return status;

	tss = zstructTsNew("/Basin/Location/Flow/TS-Pattern/15Minute/Header 3/");
	status = ztsRetrieve(ifltab, tss, 0, 1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 21, retrieve status ")) return status;
	if (tss->timeOffsetSeconds != 180) {
		printf("Fail in testTimeSeriesPatternShortHeader Loc 22, timeOffsetSeconds %d, expected 180\n", tss->timeOffsetSeconds);
		zstructFree(tss);
		return -1;
	}
	zstructFree(tss);

	//  Sizes for a regular interval block whose internal header holds only 2 ints:
	//  slots past the stored header must read as zero, not stack contents.
	for (i = 0; i < 4; i++) {
		internalHeader[i] = 0;
	}
	ztransfer = zstructTransferNew("/Basin/Location/Flow/01Jan2001/1Hour/Short Header/", 0);
	if (!ztransfer) return -1;
	ztransfer->dataType = DATA_TYPE_RTS;
	ztransfer->numberValues = 24;
	ztransfer->values1 = (int *)fvalues;
	ztransfer->values1Number = 24;
	ztransfer->internalHeader = internalHeader;
	ztransfer->internalHeaderNumber = 2;
	status = zwrite(ifltab, ztransfer);
	zstructFree(ztransfer);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 30, write status ")) return status;

	tss = zstructTsNewTimes("/Basin/Location/Flow//1Hour/Short Header/", "01Jan2001", "0100", "02Jan2001", "0000");
	recordSize = zstructRecordSizeNew("/Basin/Location/Flow//1Hour/Short Header/");
	ztsProcessTimes(ifltab, tss, 0);
	status = ztsGetSizes(ifltab, tss, recordSize);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesPatternShortHeader Loc 31, ztsGetSizes status ")) return status;
	if ((recordSize->tsTimeOffset != 0) || (recordSize->tsProfileDepthsNumber != 0) ||
		(recordSize->tsBlockStartPosition != 0) || (recordSize->tsBlockEndPosition != 0) ||
		(recordSize->tsValueSize != 0) || (recordSize->tsValueElementSize != 0) ||
		(recordSize->tsCnotesLength != 0)) {
		printf("Fail in testTimeSeriesPatternShortHeader Loc 32, sizes from a 2 int header: timeOffset %d, profileDepths %d, blockStart %d, blockEnd %d, valueSize %d, valueElementSize %d, cnotesLength %d; expected all 0\n",
			recordSize->tsTimeOffset, recordSize->tsProfileDepthsNumber, recordSize->tsBlockStartPosition,
			recordSize->tsBlockEndPosition, recordSize->tsValueSize, recordSize->tsValueElementSize,
			recordSize->tsCnotesLength);
		zstructFree(tss);
		zstructFree(recordSize);
		return -1;
	}
	zstructFree(tss);
	zstructFree(recordSize);

	return 0;
}


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

		status = testTimeSeriesPatternShortHeader(ifltab);
		if (status) return status;
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


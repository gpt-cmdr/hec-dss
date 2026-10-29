#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "heclib.h"
#include "TestDssC.h"


//  Copy a stored time series record to toPath, keeping only the first
//  numberInts ints of its internal header (as another writer may produce).
//  The data and compression arrays are copied unchanged.
static int rewriteWithShortHeader(long long *ifltab, const char *fromPath, const char *toPath, int numberInts)
{
	zStructTransfer *ztransferIn;
	zStructTransfer *ztransferOut;
	int internalHeader[INT_HEAD_SIZE];
	int i;
	int status;

	ztransferIn = zstructTransferNew(fromPath, 1);
	if (!ztransferIn) return -1;
	status = zread(ifltab, ztransferIn);
	if (status != STATUS_OKAY) {
		zstructFree(ztransferIn);
		return -1;
	}
	if (ztransferIn->internalHeaderNumber < numberInts) {
		zstructFree(ztransferIn);
		return -1;
	}
	for (i = 0; i < INT_HEAD_SIZE; i++) {
		internalHeader[i] = 0;
	}
	for (i = 0; i < numberInts; i++) {
		internalHeader[i] = ztransferIn->internalHeader[i];
	}

	ztransferOut = zstructTransferNew(toPath, 0);
	if (!ztransferOut) {
		zstructFree(ztransferIn);
		return -1;
	}
	ztransferOut->dataType = ztransferIn->dataType;
	ztransferOut->numberValues = ztransferIn->numberValues;
	ztransferOut->logicalNumberValues = ztransferIn->logicalNumberValues;
	ztransferOut->values1 = ztransferIn->values1;
	ztransferOut->values1Number = ztransferIn->values1Number;
	ztransferOut->header2 = ztransferIn->header2;
	ztransferOut->header2Number = ztransferIn->header2Number;
	ztransferOut->internalHeader = internalHeader;
	ztransferOut->internalHeaderNumber = numberInts;
	status = zwrite(ifltab, ztransferOut);
	zstructFree(ztransferOut);
	zstructFree(ztransferIn);
	return status;
}


//  Compare a retrieved series with the reference.  When valuesStored is 0 the
//  header does not hold the values slots, so every value must be missing.
static int checkShortHeaderSeries(zStructTimeSeries *tss, zStructTimeSeries *tssRef, int doubles,
								  int valuesStored, const char *label, int numberInts)
{
	int i;
	int missing;

	if (tss->numberValues != tssRef->numberValues) {
		printf("Fail in testTimeSeriesShortHeader, %s, %d int header: numberValues %d, expected %d\n",
			label, numberInts, tss->numberValues, tssRef->numberValues);
		return -1;
	}
	if ((doubles && !tss->doubleValues) || (!doubles && !tss->floatValues)) {
		printf("Fail in testTimeSeriesShortHeader, %s, %d int header: no %s values returned\n",
			label, numberInts, doubles ? "double" : "float");
		return -1;
	}
	if (tssRef->times) {
		if (!tss->times || (tss->julianBaseDate != tssRef->julianBaseDate) ||
			(tss->timeGranularitySeconds != tssRef->timeGranularitySeconds)) {
			printf("Fail in testTimeSeriesShortHeader, %s, %d int header: times not returned as stored\n",
				label, numberInts);
			return -1;
		}
	}
	for (i = 0; i < tss->numberValues; i++) {
		if (tssRef->times && (tss->times[i] != tssRef->times[i])) {
			printf("Fail in testTimeSeriesShortHeader, %s, %d int header: time %d is %d, expected %d\n",
				label, numberInts, i, tss->times[i], tssRef->times[i]);
			return -1;
		}
		if (doubles) {
			missing = zisMissingDouble(tss->doubleValues[i]);
		}
		else {
			missing = zisMissingFloat(tss->floatValues[i]);
		}
		if (valuesStored) {
			if (missing || (doubles && (tss->doubleValues[i] != (double)tssRef->floatValues[i])) ||
				(!doubles && (tss->floatValues[i] != tssRef->floatValues[i]))) {
				printf("Fail in testTimeSeriesShortHeader, %s, %d int header: value %d is %f, expected %f\n",
					label, numberInts, i, doubles ? tss->doubleValues[i] : (double)tss->floatValues[i],
					(double)tssRef->floatValues[i]);
				return -1;
			}
		}
		else if (!missing) {
			printf("Fail in testTimeSeriesShortHeader, %s, %d int header: value %d is %f, expected missing\n",
				label, numberInts, i, doubles ? tss->doubleValues[i] : (double)tss->floatValues[i]);
			return -1;
		}
	}
	if ((tss->qualityElementSize != 0) || (tss->inoteElementSize != 0) || (tss->cnotesLengthTotal != 0)) {
		printf("Fail in testTimeSeriesShortHeader, %s, %d int header: qualityElementSize %d, inoteElementSize %d, cnotesLengthTotal %d; expected 0\n",
			label, numberInts, tss->qualityElementSize, tss->inoteElementSize, tss->cnotesLengthTotal);
		return -1;
	}
	return 0;
}


//  Regular and irregular interval records whose internal header holds 5 to 16 ints
//  (the readers accept 5 or more) must read the slots that are not stored as zero,
//  not whatever follows the header in memory.
//  A 16 int header holds every slot used to unpack the data (all but the cnotes length),
//  so its values read as stored.  Headers of 5 and 6 ints hold no values information,
//  so every value reads as missing; the times of an irregular series still read as stored.
int testTimeSeriesShortHeader(long long *ifltab)
{
	zStructTimeSeries *tss;
	zStructTimeSeries *tssReg;
	zStructTimeSeries *tssIrreg;
	float fvalues[24];
	int itimes[24];
	int headerLengths[3] = { 5, 6, 16 };
	int numberInts;
	int doubles;
	int i, j;
	int status;
	char path[100];

	printf("enter testTimeSeriesShortHeader\n");
	for (i = 0; i < 24; i++) {
		fvalues[i] = (float)(i + 1) * 10.0f;
		itimes[i] = 60 + (i * 90);  //  minutes from 01Jan2001
	}

	//  Reference records, stored normally
	tss = zstructTsNewRegFloats("/Short Header/Reg/Flow//1Hour/Full/", fvalues, 24, "01Jan2001", "0100", "cfs", "Inst-Val");
	status = ztsStore(ifltab, tss, 0);
	zstructFree(tss);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 1, regular store status ")) return status;

	tss = zstructTsNewIrregFloats("/Short Header/Irr/Flow//IR-Month/Full/", fvalues, 24, itimes, MINUTE_GRANULARITY, "01Jan2001", "cfs", "Inst-Val");
	status = ztsStore(ifltab, tss, 0);
	zstructFree(tss);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 2, irregular store status ")) return status;

	tssReg = zstructTsNewTimes("/Short Header/Reg/Flow//1Hour/Full/", "01Jan2001", "0100", "02Jan2001", "0000");
	status = ztsRetrieve(ifltab, tssReg, 0, 1, 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 3, regular retrieve status ")) return status;

	tssIrreg = zstructTsNewTimes("/Short Header/Irr/Flow//IR-Month/Full/", "01Jan2001", "0000", "31Jan2001", "2400");
	status = ztsRetrieve(ifltab, tssIrreg, 0, 1, 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 4, irregular retrieve status ")) return status;

	if ((tssReg->numberValues != 24) || (tssIrreg->numberValues != 24) || !tssIrreg->times) {
		printf("Fail in testTimeSeriesShortHeader Loc 5, reference numberValues %d and %d, expected 24\n",
			tssReg->numberValues, tssIrreg->numberValues);
		return -1;
	}

	for (j = 0; j < 3; j++) {
		numberInts = headerLengths[j];

		//  Regular interval
		snprintf(path, sizeof(path), "/Short Header/Reg/Flow/01Jan2001/1Hour/Header %d/", numberInts);
		status = rewriteWithShortHeader(ifltab, "/Short Header/Reg/Flow/01Jan2001/1Hour/Full/", path, numberInts);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 10, regular write status ")) return status;
		snprintf(path, sizeof(path), "/Short Header/Reg/Flow//1Hour/Header %d/", numberInts);
		for (doubles = 0; doubles < 2; doubles++) {
			tss = zstructTsNewTimes(path, "01Jan2001", "0100", "02Jan2001", "0000");
			status = ztsRetrieve(ifltab, tss, 0, doubles ? 2 : 1, 1);
			if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 11, regular retrieve status ")) {
				zstructFree(tss);
				return status;
			}
			status = checkShortHeaderSeries(tss, tssReg, doubles, numberInts > INT_HEAD_valueElementSize, "regular", numberInts);
			zstructFree(tss);
			if (status) return status;
		}

		//  Irregular interval
		snprintf(path, sizeof(path), "/Short Header/Irr/Flow/01Jan2001/IR-Month/Header %d/", numberInts);
		status = rewriteWithShortHeader(ifltab, "/Short Header/Irr/Flow/01Jan2001/IR-Month/Full/", path, numberInts);
		if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 20, irregular write status ")) return status;
		snprintf(path, sizeof(path), "/Short Header/Irr/Flow//IR-Month/Header %d/", numberInts);
		for (doubles = 0; doubles < 2; doubles++) {
			tss = zstructTsNewTimes(path, "01Jan2001", "0000", "31Jan2001", "2400");
			status = ztsRetrieve(ifltab, tss, 0, doubles ? 2 : 1, 1);
			if (zcheckStatus(ifltab, status, 1, "Fail in testTimeSeriesShortHeader Loc 21, irregular retrieve status ")) {
				zstructFree(tss);
				return status;
			}
			status = checkShortHeaderSeries(tss, tssIrreg, doubles, numberInts > INT_HEAD_valueElementSize, "irregular", numberInts);
			zstructFree(tss);
			if (status) return status;
		}
	}

	zstructFree(tssReg);
	zstructFree(tssIrreg);
	return 0;
}

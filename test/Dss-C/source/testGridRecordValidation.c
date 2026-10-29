#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "heclib.h"
#include "hecdss7.h"
#include "hecdssInternal.h"
#include "TestDssC.h"
#include "zStructSpatialGrid.h"
#include "zStructSpatialTin.h"


#define GRID_VALIDATION_CELLS_X 4
#define GRID_VALIDATION_CELLS_Y 3
#define GRID_VALIDATION_RANGES 2


static int isInvalidRecordHeader(int status)
{
	int highFunction, lowFunction, dssError, errorStatus;
	if (status == STATUS_OKAY) return 0;
	zerrorDecode(status, &highFunction, &lowFunction, &dssError, &errorStatus);
	return dssError == zdssErrorCodes.INVALID_RECORD_HEADER;
}


//  Store a 4 x 3 grid with 2 ranges, cell i holding i
static int storeValidationGrid(long long *ifltab, const char *pathname, int compressionMethod)
{
	zStructSpatialGrid *grid;
	float *data;
	float *rangeLimits;
	int *rangeCounts;
	int numberCells = GRID_VALIDATION_CELLS_X * GRID_VALIDATION_CELLS_Y;
	int i;
	int status;

	grid = zstructSpatialGridNew(pathname);
	if (!grid) return -1;
	grid->_type = 420;
	grid->_dataSource = mallocAndCopy("INTERNAL");
	grid->_version = 1;
	grid->_dataUnits = mallocAndCopy("mm");
	grid->_dataType = PER_AVER;
	grid->_numberOfCellsX = GRID_VALIDATION_CELLS_X;
	grid->_numberOfCellsY = GRID_VALIDATION_CELLS_Y;
	grid->_cellSize = 5.0f;
	grid->_compressionMethod = compressionMethod;
	grid->_numberOfRanges = GRID_VALIDATION_RANGES;
	rangeLimits = (float *)calloc(GRID_VALIDATION_RANGES, sizeof(float));
	rangeCounts = (int *)calloc(GRID_VALIDATION_RANGES, sizeof(int));
	data = (float *)calloc(numberCells, sizeof(float));
	grid->_minDataValue = calloc(1, sizeof(float));
	grid->_maxDataValue = calloc(1, sizeof(float));
	grid->_meanDataValue = calloc(1, sizeof(float));
	grid->_rangeLimitTable = rangeLimits;
	grid->_numberEqualOrExceedingRangeLimit = rangeCounts;
	grid->_data = data;
	if (!rangeLimits || !rangeCounts || !data || !grid->_minDataValue || !grid->_maxDataValue || !grid->_meanDataValue) {
		zstructFree(grid);
		return -1;
	}
	for (i = 0; i < GRID_VALIDATION_RANGES; i++) {
		rangeLimits[i] = (float)i * 5.0f;
		rangeCounts[i] = numberCells - (i * 5);
	}
	for (i = 0; i < numberCells; i++) {
		data[i] = (float)i;
	}
	*(float *)grid->_minDataValue = 0.0f;
	*(float *)grid->_maxDataValue = (float)(numberCells - 1);
	*(float *)grid->_meanDataValue = (float)(numberCells - 1) / 2.0f;
	grid->_srsDefinitionType = 1;
	grid->_srsName = mallocAndCopy("SRS");
	grid->_srsDefinition = mallocAndCopy("TEST");
	grid->_xCoordOfGridCellZero = 10.5f;
	grid->_yCoordOfGridCellZero = 20.5f;
	grid->_nullValue = -1.0f;
	grid->_timeZoneID = mallocAndCopy("UTC");
	status = zspatialGridStore(ifltab, grid);
	zstructFree(grid);
	return status;
}


//  Rewrite a stored grid record with one internal header word changed
//  (headerSlot < 0 leaves the header as stored) and, if values3Number >= 0,
//  only the first values3Number ints of the cell data.
static int rewriteValidationGrid(long long *ifltab, const char *pathname,
								 int headerSlot, int headerValue, int values3Number)
{
	zStructTransfer *ztransferRead;
	zStructTransfer *ztransferWrite;
	int internalHeader[INT_HEAD_grid_internalHeaderNumber];
	int i;
	int status;

	ztransferRead = zstructTransferNew(pathname, 1);
	if (!ztransferRead) return -1;
	status = zread(ifltab, ztransferRead);
	if ((status != STATUS_RECORD_FOUND) || (ztransferRead->internalHeaderNumber != INT_HEAD_grid_internalHeaderNumber)) {
		zstructFree(ztransferRead);
		return -1;
	}
	for (i = 0; i < INT_HEAD_grid_internalHeaderNumber; i++) {
		internalHeader[i] = ztransferRead->internalHeader[i];
	}
	if (headerSlot >= 0) {
		internalHeader[headerSlot] = headerValue;
	}
	ztransferWrite = zstructTransferNew(pathname, 0);
	if (!ztransferWrite) {
		zstructFree(ztransferRead);
		return -1;
	}
	ztransferWrite->dataType = ztransferRead->dataType;
	ztransferWrite->internalHeader = internalHeader;
	ztransferWrite->internalHeaderNumber = INT_HEAD_grid_internalHeaderNumber;
	ztransferWrite->header2 = ztransferRead->header2;
	ztransferWrite->header2Number = ztransferRead->header2Number;
	ztransferWrite->userHeader = ztransferRead->userHeader;
	ztransferWrite->userHeaderNumber = ztransferRead->userHeaderNumber;
	ztransferWrite->userHeaderMode = 2;  //  Store the user header as given (no string padding)
	ztransferWrite->values1 = ztransferRead->values1;
	ztransferWrite->values1Number = ztransferRead->values1Number;
	ztransferWrite->values2 = ztransferRead->values2;
	ztransferWrite->values2Number = ztransferRead->values2Number;
	ztransferWrite->values3 = ztransferRead->values3;
	ztransferWrite->values3Number = ztransferRead->values3Number;
	if ((values3Number >= 0) && (values3Number < ztransferRead->values3Number)) {
		ztransferWrite->values3Number = values3Number;
	}
	ztransferWrite->numberValues = ztransferRead->numberValues;
	ztransferWrite->logicalNumberValues = ztransferRead->logicalNumberValues;
	status = zwrite(ifltab, ztransferWrite);
	zstructFree(ztransferWrite);
	zstructFree(ztransferRead);
	return status;
}


//  A grid written by zspatialGridStore reads back as stored
static int checkValidationGrid(long long *ifltab, const char *pathname, const char *label)
{
	zStructSpatialGrid *grid;
	float *data;
	int numberCells = GRID_VALIDATION_CELLS_X * GRID_VALIDATION_CELLS_Y;
	int i;
	int status;

	grid = zstructSpatialGridNew(pathname);
	status = zspatialGridRetrieve(ifltab, grid, 1);
	if (status != STATUS_OKAY) {
		printf("Fail in testGridRecordValidation, %s grid: retrieve status %d\n", label, status);
		zstructFree(grid);
		return -1;
	}
	if ((grid->_numberOfCellsX != GRID_VALIDATION_CELLS_X) || (grid->_numberOfCellsY != GRID_VALIDATION_CELLS_Y) ||
		(grid->_numberOfRanges != GRID_VALIDATION_RANGES) || !grid->_data ||
		!grid->_rangeLimitTable || !grid->_numberEqualOrExceedingRangeLimit ||
		!grid->_minDataValue || !grid->_maxDataValue || !grid->_meanDataValue) {
		printf("Fail in testGridRecordValidation, %s grid: cells %d x %d, ranges %d, or arrays not returned\n",
			label, grid->_numberOfCellsX, grid->_numberOfCellsY, grid->_numberOfRanges);
		zstructFree(grid);
		return -1;
	}
	data = (float *)grid->_data;
	for (i = 0; i < numberCells; i++) {
		if (data[i] != (float)i) {
			printf("Fail in testGridRecordValidation, %s grid: cell %d is %f, expected %d\n", label, i, data[i], i);
			zstructFree(grid);
			return -1;
		}
	}
	for (i = 0; i < GRID_VALIDATION_RANGES; i++) {
		if ((((float *)grid->_rangeLimitTable)[i] != (float)i * 5.0f) ||
			(grid->_numberEqualOrExceedingRangeLimit[i] != numberCells - (i * 5))) {
			printf("Fail in testGridRecordValidation, %s grid: range %d is %f, %d\n", label, i,
				((float *)grid->_rangeLimitTable)[i], grid->_numberEqualOrExceedingRangeLimit[i]);
			zstructFree(grid);
			return -1;
		}
	}
	if ((*(float *)grid->_minDataValue != 0.0f) || (*(float *)grid->_maxDataValue != (float)(numberCells - 1)) ||
		(*(float *)grid->_meanDataValue != (float)(numberCells - 1) / 2.0f)) {
		printf("Fail in testGridRecordValidation, %s grid: min, max, mean %f %f %f\n", label,
			*(float *)grid->_minDataValue, *(float *)grid->_maxDataValue, *(float *)grid->_meanDataValue);
		zstructFree(grid);
		return -1;
	}
	zstructFree(grid);
	return 0;
}


//  The retrieve of an inconsistent record must report INVALID_RECORD_HEADER
static int checkRejected(long long *ifltab, const char *pathname, int boolRetrieveData, const char *label)
{
	zStructSpatialGrid *grid;
	int status;

	grid = zstructSpatialGridNew(pathname);
	status = zspatialGridRetrieve(ifltab, grid, boolRetrieveData);
	zstructFree(grid);
	if (!isInvalidRecordHeader(status)) {
		printf("Fail in testGridRecordValidation, %s: retrieve status %d, expected an invalid record header error\n",
			label, status);
		return -1;
	}
	return 0;
}


//  zspatialGridRetrieve and zspatialTinRetrieve check the counts in a record's
//  header against the arrays stored with it, and reject a record whose counts
//  would index past them.  Records written by zspatialGridStore read as before.
int testGridRecordValidation(long long *ifltab)
{
	zStructTransfer *ztransfer;
	zStructTransfer *ztransferRead;
	zStructSpatialTin *tin;
	int tinHeader[12];
	int tinHeader2[4] = { 0, 0, 0, 0 };
	int tinValues[6] = { 0, 0, 0, 0, 0, 0 };
	int compressedInts;
	int status;
	int i;
	const char *pathPlain = "/SHG/Validation/Precip/01Jan2001:0100/01Jan2001:0200/Plain/";
	const char *pathZlib = "/SHG/Validation/Precip/01Jan2001:0100/01Jan2001:0200/Zlib/";
	const char *pathTin = "/A/Validation/Elev/01Jan2001:0100//Points/";

	printf("enter testGridRecordValidation\n");

	//  Grids written by zspatialGridStore round-trip
	status = storeValidationGrid(ifltab, pathPlain, NO_COMPRESSION);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 1, store status ")) return status;
	if (checkValidationGrid(ifltab, pathPlain, "uncompressed")) return -1;
	status = storeValidationGrid(ifltab, pathZlib, ZLIB_COMPRESSION);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 2, store status ")) return status;
	if (checkValidationGrid(ifltab, pathZlib, "zlib")) return -1;

	//  The same records rewritten unchanged still read as stored
	status = rewriteValidationGrid(ifltab, pathPlain, -1, 0, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 3, write status ")) return status;
	if (checkValidationGrid(ifltab, pathPlain, "rewritten uncompressed")) return -1;
	status = rewriteValidationGrid(ifltab, pathZlib, -1, 0, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 4, write status ")) return status;
	if (checkValidationGrid(ifltab, pathZlib, "rewritten zlib")) return -1;

	//  More ranges than values1 holds (7 ints stored for 2 ranges)
	status = rewriteValidationGrid(ifltab, pathPlain, INT_HEAD_grid_numberOfRanges, 40, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 10, write status ")) return status;
	if (checkRejected(ifltab, pathPlain, 1, "40 ranges")) return -1;
	if (checkRejected(ifltab, pathPlain, 0, "40 ranges, header only")) return -1;
	status = rewriteValidationGrid(ifltab, pathPlain, INT_HEAD_grid_numberOfRanges, -1, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 11, write status ")) return status;
	if (checkRejected(ifltab, pathPlain, 1, "-1 ranges")) return -1;

	//  More uncompressed cells than stored (40 x 3 cells, 12 stored)
	status = storeValidationGrid(ifltab, pathPlain, NO_COMPRESSION);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 20, store status ")) return status;
	status = rewriteValidationGrid(ifltab, pathPlain, INT_HEAD_grid_numberOfCellsX, 40, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 21, write status ")) return status;
	if (checkRejected(ifltab, pathPlain, 1, "uncompressed 40 x 3 cells")) return -1;
	status = rewriteValidationGrid(ifltab, pathPlain, INT_HEAD_grid_numberOfCellsX, -4, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 22, write status ")) return status;
	if (checkRejected(ifltab, pathPlain, 1, "uncompressed -4 x 3 cells")) return -1;

	//  A compressed size larger than the bytes stored: keep the header, store 2 fewer ints
	status = storeValidationGrid(ifltab, pathZlib, ZLIB_COMPRESSION);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 30, store status ")) return status;
	ztransferRead = zstructTransferNew(pathZlib, 1);
	status = zread(ifltab, ztransferRead);
	compressedInts = ztransferRead->values3Number;
	zstructFree(ztransferRead);
	if ((status != STATUS_RECORD_FOUND) || (compressedInts < 4)) {
		printf("Fail in testGridRecordValidation Loc 31, read status %d, compressed ints %d\n", status, compressedInts);
		return -1;
	}
	status = rewriteValidationGrid(ifltab, pathZlib, -1, 0, compressedInts - 2);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 32, write status ")) return status;
	if (checkRejected(ifltab, pathZlib, 1, "zlib size past stored bytes")) return -1;

	//  More cells than the compressed data holds (40 x 3 cells, 12 compressed)
	status = storeValidationGrid(ifltab, pathZlib, ZLIB_COMPRESSION);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 40, store status ")) return status;
	status = rewriteValidationGrid(ifltab, pathZlib, INT_HEAD_grid_numberOfCellsX, 40, -1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 41, write status ")) return status;
	if (checkRejected(ifltab, pathZlib, 1, "zlib 40 x 3 cells")) return -1;

	//  A TIN header claiming 10 points with values for 2 (zspatialTinStore stores 3 ints per point)
	for (i = 0; i < 12; i++) {
		tinHeader[i] = 0;
	}
	tinHeader[0] = 10;
	ztransfer = zstructTransferNew(pathTin, 0);
	if (!ztransfer) return -1;
	ztransfer->dataType = DATA_TYPE_SPATIAL_TIN;
	ztransfer->internalHeader = tinHeader;
	ztransfer->internalHeaderNumber = 12;
	ztransfer->header2 = tinHeader2;
	ztransfer->header2Number = 4;
	ztransfer->values1 = tinValues;
	ztransfer->values1Number = 6;
	ztransfer->numberValues = 2;
	status = zwrite(ifltab, ztransfer);
	zstructFree(ztransfer);
	if (zcheckStatus(ifltab, status, 1, "Fail in testGridRecordValidation Loc 50, write status ")) return status;
	tin = zstructSpatialTinNew(pathTin);
	status = zspatialTinRetrieve(ifltab, tin, 1);
	zstructFree(tin);
	if (!isInvalidRecordHeader(status)) {
		printf("Fail in testGridRecordValidation Loc 51, TIN with 10 points and 6 values: status %d, expected an invalid record header error\n", status);
		return -1;
	}

	return 0;
}

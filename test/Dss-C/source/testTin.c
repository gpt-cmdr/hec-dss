#include "stdio.h"
#include "string.h"

#include "zdssKeys.h"
#include "heclib.h"
#include "hecdss7.h"
#include "hecdssInternal.h"
#include "TestDssC.h"
#include "zStructAllocation.h"

#include "zStructSpatialTin.h"

char getaCharacter(int i) {
	int j;

	j = i/26;
	i = i - (j * 26);

	i += 97;
	if ((i >= 97) && (i <= 122)) {
		return i;
	}
	return 97;
}

int getStr(char *str, int pos) {
	int i, j;

	i = pos /12;
	j = pos - (i * 12);
	if (j < 0) j = -j;
	if (j < 5) j += 5;
	for (i=0; i<j; i++) {
		str[i] = getaCharacter(pos+i);
	}
	str[j+1] = '\0';
	return j+1;
}

char* getLabelPtr(char* labels, int labelIndex, int maxIndex){
	int i = 0, j = 0;
	char* thePtr = labels;
	while (i < maxIndex){
		if (j == labelIndex) return thePtr;
		if (*thePtr == '\0') j++;
		i++;
		thePtr++;
	}
	return NULL;
}

int testTin(long long *ifltab)
{

	int DEBUG=0;
	int status=0;
	int* connectTo;
	zStructSpatialTin *tinStructStore;
	
	// Building a SHG-type grid as a TIN.
	// Origin is at 283,000 m east, 1,785,000 m north or center of cell (141, 892)
	float xMin = 283000.0, yMin = 1785000.0;
    // 2000m spacing for 40 rows and 50 columns
	int row = 0, column = 0, nrows = 40, ncols = 50, maxRowIndex = 39, maxColIndex = 49;
	float cellsize = 2000.0;
	int nconns = 0, nedges = 0, connIndex = 0;
	// maximum data value = 5 mm precip
	float maxVal = 5;
	int done = 0, nodeIndex = -1;


	if (zgetVersion(ifltab) != 7) {		
		return 0;
	}
	return 0;   //  Needs to be updated by Tom E.

	tinStructStore = zstructSpatialTinNew("/a/b/c/01jan2001:1200/01jan2001:1300/f/");

	//   Gen data
	tinStructStore->numberPoints = 2000;
	tinStructStore->slendernessRatio = 0.0;
	tinStructStore->SRSType = 0; // WKT string to define coordinate system

	tinStructStore->xCoordinate =     (float *)calloc(tinStructStore->numberPoints, 4);
	tinStructStore->yCoordinate     = (float *)calloc(tinStructStore->numberPoints, 4);
	tinStructStore->value           = (float *)calloc(tinStructStore->numberPoints, 4);
	tinStructStore->numberConnections = (int *)calloc(tinStructStore->numberPoints, 4);

	// temporary array to hold the connections while they're being figured out.
	connectTo = (int *)calloc((tinStructStore->numberPoints) * 6, 4); // more than we need, by a bit, actually

	tinStructStore->allocated[zSTRUCT_TIN_xCoordinate] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_yCoordinate] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_value] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_pointType] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_numberConnections] = 1;
/*
	
	for (i=0; i<tinStructStore->numberPoints; i++) {
		row = i / ncols;
		column = i % ncols;

		// easy stuff (x, y, value) first
		tinStructStore->xCoordinate[i] = (float)(xMin + column * cellsize);
		tinStructStore->yCoordinate[i] = (float)(yMin + row * cellsize);
		tinStructStore->value[i] = maxVal * (1 - (float)abs(nrows / 2 - row) / (float)(nrows / 2));
		tinStructStore->pointType[i] = 2; // using 2 to indicate a cell in a grid

		// trickier topological stuff
		tinStructStore->connection[i] = connIndex;
		if (column != 0 && row != 0 && row != maxRowIndex && column != maxColIndex){
			// this is an interior point in the grid
			tinStructStore->numberConnections[i] = 6;
			nedges += 6;
			connectTo[connIndex++] = i - ncols - 1; // node to southwest (down one and over one)
			connectTo[connIndex++] = i - ncols; // node to south
			connectTo[connIndex++] = i - 1; // node to west
			connectTo[connIndex++] = i + 1; // node to east
			connectTo[connIndex++] = i + ncols; // node to north
			connectTo[connIndex++] = i + ncols + 1; // node to northeast (up one and over one)
		}
		else if (row == 0){
			// south edge of grid
			if (column == 0){
				//southwest corner
				tinStructStore->numberConnections[i] = 3;
				nedges += 3;
				connectTo[connIndex++] = i + 1; // node to east
				connectTo[connIndex++] = i + ncols; // node to north
				connectTo[connIndex++] = i + ncols + 1; // node to northeast (up one and over one)
			}
			else if (column == maxColIndex){
				//southeast corner
				tinStructStore->numberConnections[i] = 2;
				nedges += 2;
				connectTo[connIndex++] = i - 1; // node to west
				connectTo[connIndex++] = i + ncols; // node to north
			}
			else{
				//not on corner
				tinStructStore->numberConnections[i] = 4;
				nedges += 4;
				connectTo[connIndex++] = i - 1; // node to west
				connectTo[connIndex++] = i + 1; // node to east
				connectTo[connIndex++] = i + ncols; // node to north
				connectTo[connIndex++] = i + ncols + 1; // node to northeast (up one and over one)
			}
		}
		else if (row == maxRowIndex){
			// north edge of grid
			if (column == 0){
				//northwest corner
				tinStructStore->numberConnections[i] = 2;
				nedges += 2;
				connectTo[connIndex++] = i - ncols; // node to south
				connectTo[connIndex++] = i + 1; // node to north
			}
			else if (column == maxColIndex){
				//northeast corner
				tinStructStore->numberConnections[i] = 3;
				nedges += 3;
				connectTo[connIndex++] = i - ncols - 1; // node to southwest (down one and over one)
				connectTo[connIndex++] = i - ncols; // node to south
				connectTo[connIndex++] = i - 1; // node to west
			}
			else{
				//not on corner
				tinStructStore->numberConnections[i] = 4;
				nedges += 4;
				connectTo[connIndex++] = i - ncols - 1; // node to southwest (down one and over one)
				connectTo[connIndex++] = i - ncols; // node to south
				connectTo[connIndex++] = i - 1; // node to west
				connectTo[connIndex++] = i + 1; // node to east
			}
		}
		else if (column == 0){
			// west edge of grid, excluding corners
			tinStructStore->numberConnections[i] = 4;
			nedges += 4;
			connectTo[connIndex++] = i - ncols; // node to south
			connectTo[connIndex++] = i + 1; // node to east
			connectTo[connIndex++] = i + ncols; // node to north
			connectTo[connIndex++] = i + ncols + 1; // node to northeast (up one and over one)
		}
		else {
			// all that's left is the east edge, excluding the two corners
			tinStructStore->numberConnections[i] = 4;
			nedges += 4;
			connectTo[connIndex++] = i - ncols - 1; // node to southwest (down one and over one)
			connectTo[connIndex++] = i - ncols; // node to south
			connectTo[connIndex++] = i - 1; // node to west
			connectTo[connIndex++] = i + ncols; // node to north
		}

		// tinStructStore->numberConnections[i] = i + 1000;
		// tinStructStore->connectionIndex[i]   = i + 10000;
		//tinStructStore->labelIndex[i]        = i;
		// tinStructStore->connectTo[i]         = i + 1000000;
	}

	tinStructStore->connection = (int *)calloc(nedges, sizeof(int));
	for (i = 0; i < nedges; i++){
		tinStructStore->connection[i] = connectTo[i];
	}
	free(connectTo);
	//tinStructStore->connectTableNumber = nedges;
	//tinStructStore->allocated[zSTRUCT_TIN_connectTo] = 1;

	tinStructStore->SpatialReferenceSystem = mallocAndCopy("PROJCS[\"USA_Contiguous_Albers_Equal_Area_Conic_USGS_version\",GEOGCS[\"GCS_North_American_1983\",DATUM[\"D_North_American_1983\",SPHEROID[\"GRS_1980\",6378137.0,298.257222101]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Albers\"],PARAMETER[\"False_Easting\",0.0],PARAMETER[\"False_Northing\",0.0],PARAMETER[\"Central_Meridian\",-96.0],PARAMETER[\"Standard_Parallel_1\",29.5],PARAMETER[\"Standard_Parallel_2\",45.5],PARAMETER[\"Latitude_Of_Origin\",23.0],UNIT[\"Meter\",1.0]]");
	tinStructStore->SRSName = mallocAndCopy("CONUS_ALBERS_USGS");
	tinStructStore->SRSUnits = mallocAndCopy("Meters");
	tinStructStore->units = mallocAndCopy("mm");
	tinStructStore->type = mallocAndCopy("PER-CUM");
	tinStructStore->timeZoneName = mallocAndCopy("UTC");

	tinStructStore->allocated[zSTRUCT_TIN_SpatialReferenceSystem] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_SRSName] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_SRSUnits] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_units] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_type] = 1;
	tinStructStore->allocated[zSTRUCT_TIN_timeZoneName] = 1;

	labels = (char *)calloc(tinStructStore->numberPoints * 16, sizeof(char));
	count = 0;
	for (i=0; i<tinStructStore->numberPoints; i++) {
		count += getStr(&labels[count], i);
	}
	tinStructStore->labelLen = count;
	tinStructStore->label = (char*)calloc(count, sizeof(char));
	for (i = 0; i < count; i++){
		//tinStructStore->pointLabel[i] = '\0';
		tinStructStore->label[i] = labels[i];
	}
	// free(labels);
	tinStructStore->allocated[zSTRUCT_TIN_label] = 1;

	
	printf("\nEnter an index number (non-numerical value exits): ");
	done = 10;
	nodeIndex = 123;
	while (!done){		
			printf(" Node %d is at (%4.1f, %4.1f) and has a value of %3.1f\n", nodeIndex, tinStructStore->xCoordinate[nodeIndex],
				tinStructStore->yCoordinate[nodeIndex], tinStructStore->value[nodeIndex]);
			printf(" Its first connection is located at index %d.\n", tinStructStore->connection[nodeIndex]);
			labelForShow = getLabelPtr(tinStructStore->label, tinStructStore->label[nodeIndex], tinStructStore->labelLen);
			if (labelForShow != NULL){
				printf(" Its label is: %s\n", labelForShow);
			}
			printf(" It connects to %d nodes: ", tinStructStore->numberConnections[nodeIndex]);
			for (i = 0; i < tinStructStore->numberConnections[nodeIndex] - 1; i++){
				printf(" %d, ", tinStructStore->connectTo[tinStructStore->connectionIndex[nodeIndex] + i]);
			}
			printf("and %d.\n", tinStructStore->connectTo[tinStructStore->connectionIndex[nodeIndex] + i]);
			printf("\nNext: ");
			done--;
			nodeIndex++;
		
	}
	//fflush(stdin);

	// zstructFree(tinStructStore);
	// return;

	status = zspatialTinStore(ifltab, tinStructStore);

	tinStructRetrieve = zstructSpatialTinNew("/a/b/c/01jan2001:1200/01jan2001:1300/f/");
	status = zspatialTinRetrieve(ifltab, tinStructRetrieve, 1);

	done = 10;
	nodeIndex = 123;
	printf("\nEnter an index number (non-numerical value exits): ");
	while (!done){
			printf(" Node %d is at (%4.1f, %4.1f) and has a value of %3.1f\n", nodeIndex, tinStructRetrieve->xCoordinate[nodeIndex],
				tinStructRetrieve->yCoordinate[nodeIndex], tinStructRetrieve->value[nodeIndex]);
			labelForShow = getLabelPtr(tinStructRetrieve->pointLabel, tinStructRetrieve->labelIndex[nodeIndex], tinStructRetrieve->pointLabelLen);
			if (labelForShow != NULL){
				printf(" Its label is: %s\n", labelForShow);
			}
			printf(" Its first connection is located at index %d.\n", tinStructRetrieve->connectionIndex[nodeIndex]);
			printf(" It connects to %d nodes: ", tinStructRetrieve->numberConnections[nodeIndex]);
			for (i = 0; i < tinStructRetrieve->numberConnections[nodeIndex] - 1; i++){
				printf(" %d, ", tinStructRetrieve->connectTo[tinStructRetrieve->connectionIndex[nodeIndex] + i]);
			}
			printf("and %d.\n", tinStructRetrieve->connectTo[tinStructRetrieve->connectionIndex[nodeIndex] + i]);
			printf("\nNext: ");
			done--;
			nodeIndex++;
	}

	free(labels);
	zstructFree(tinStructStore);
	zstructFree(tinStructRetrieve);
*/	
	return status;
}


//  Rewrite a stored record with a different internal header, user header or
//  header2 (as another writer may produce), bypassing the store functions.
static int rewriteSpatialRecord(long long *ifltab, const char *pathname,
	int *internalHeader, int internalHeaderNumber,
	int userHeaderNumber, int boolKeepHeader2)
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
	if (internalHeader) {
		ztransferWrite->internalHeader = internalHeader;
		ztransferWrite->internalHeaderNumber = internalHeaderNumber;
	}
	else {
		ztransferWrite->internalHeader = ztransferRead->internalHeader;
		ztransferWrite->internalHeaderNumber = ztransferRead->internalHeaderNumber;
	}
	if (boolKeepHeader2) {
		ztransferWrite->header2 = ztransferRead->header2;
		ztransferWrite->header2Number = ztransferRead->header2Number;
	}
	ztransferWrite->userHeader = ztransferRead->userHeader;
	ztransferWrite->userHeaderNumber = ztransferRead->userHeaderNumber;
	if ((userHeaderNumber >= 0) && (userHeaderNumber < ztransferRead->userHeaderNumber)) {
		ztransferWrite->userHeaderNumber = userHeaderNumber;
	}
	ztransferWrite->userHeaderMode = 2;  //  Store the user header as given (no string padding)
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


static int isInvalidHeaderError(int status)
{
	int highFunction, lowFunction, dssError, errorStatus;
	if (status == STATUS_OKAY) return 0;
	zerrorDecode(status, &highFunction, &lowFunction, &dssError, &errorStatus);
	return dssError == zdssErrorCodes.INVALID_RECORD_HEADER;
}


static int storeSmallGrid(long long *ifltab, const char *pathname)
{
	zStructSpatialGrid *grid;
	float *data;
	int idx;
	int status;

	grid = zstructSpatialGridNew(pathname);
	if (!grid) return -1;
	grid->_type = 420;
	grid->_dataSource = mallocAndCopy("INTERNAL");
	grid->_version = 1;
	grid->_dataUnits = mallocAndCopy("mm");
	grid->_dataType = PER_AVER;
	grid->_numberOfCellsX = 4;
	grid->_numberOfCellsY = 3;
	grid->_cellSize = 5.0f;
	grid->_compressionMethod = NO_COMPRESSION;
	grid->_numberOfRanges = 2;
	grid->_rangeLimitTable = (float *)calloc(2, sizeof(float));
	grid->_numberEqualOrExceedingRangeLimit = (int *)calloc(2, sizeof(int));
	grid->_srsDefinitionType = 1;
	grid->_srsName = mallocAndCopy("SRS");
	grid->_srsDefinition = mallocAndCopy("TEST");
	grid->_xCoordOfGridCellZero = 10.5f;
	grid->_yCoordOfGridCellZero = 20.5f;
	grid->_nullValue = -1.0f;
	grid->_timeZoneID = mallocAndCopy("UTC");
	grid->_maxDataValue = calloc(1, sizeof(float));
	grid->_minDataValue = calloc(1, sizeof(float));
	grid->_meanDataValue = calloc(1, sizeof(float));
	data = (float *)calloc(12, sizeof(float));
	if (!data) {
		zstructFree(grid);
		return -1;
	}
	for (idx = 0; idx < 12; idx++) {
		data[idx] = (float)idx;
	}
	grid->_data = data;
	status = zspatialGridStore(ifltab, grid);
	zstructFree(grid);
	return status;
}


//  Grid and TIN records whose headers are shorter than the reader expects:
//  a short internal header is rejected as an invalid header instead of being
//  read past its end; a short grid user header sets only the values it holds;
//  a grid without header2 text reads with empty strings.
int testSpatialShortHeader(long long *ifltab)
{
	zStructSpatialGrid *grid;
	zStructSpatialTin *tin;
	zStructTransfer *ztransfer;
	int internalHeader[2] = { VERSION_100, 0 };
	int values[4] = { 0, 0, 0, 0 };
	int version;
	int status;
	const char *gridPath = "/SHG/Basin/Precip/01Jan2001:0100/01Jan2001:0200/Short Header/";
	const char *tinPath = "/A/B/Elev/01Jan2001:0100//Short Header/";

	//  Grid internal header of 2 ints
	status = storeSmallGrid(ifltab, gridPath);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 1, grid store status ")) return status;
	status = rewriteSpatialRecord(ifltab, gridPath, internalHeader, 2, -1, 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 2, write status ")) return status;
	version = 0;
	status = zspatialGridRetrieveVersion(ifltab, gridPath, &version);
	if ((status != STATUS_RECORD_FOUND) || (version != -VERSION_100)) {
		printf("Fail in testSpatialShortHeader Loc 3, version status %d, version %d\n", status, version);
		return -1;
	}
	grid = zstructSpatialGridNew(gridPath);
	status = zspatialGridRetrieve(ifltab, grid, 1);
	zstructFree(grid);
	if (!isInvalidHeaderError(status)) {
		printf("Fail in testSpatialShortHeader Loc 4, 2 int grid header: status %d, expected an invalid header error\n", status);
		return -1;
	}

	//  Grid user header of 1 int: only the cell size is stored
	status = storeSmallGrid(ifltab, gridPath);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 10, grid store status ")) return status;
	status = rewriteSpatialRecord(ifltab, gridPath, 0, 0, 1, 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 11, write status ")) return status;
	grid = zstructSpatialGridNew(gridPath);
	status = zspatialGridRetrieve(ifltab, grid, 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 12, retrieve status ")) {
		zstructFree(grid);
		return status;
	}
	if ((grid->_cellSize != 5.0f) || (grid->_xCoordOfGridCellZero != 0.0f) ||
		(grid->_yCoordOfGridCellZero != 0.0f) || (grid->_nullValue != 0.0f)) {
		printf("Fail in testSpatialShortHeader Loc 13, 1 int user header: cell size %f, x %f, y %f, null %f; expected 5, 0, 0, 0\n",
			grid->_cellSize, grid->_xCoordOfGridCellZero, grid->_yCoordOfGridCellZero, grid->_nullValue);
		zstructFree(grid);
		return -1;
	}
	zstructFree(grid);

	//  Grid without header2 (no units, source, SRS or time zone text)
	status = storeSmallGrid(ifltab, gridPath);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 20, grid store status ")) return status;
	status = rewriteSpatialRecord(ifltab, gridPath, 0, 0, -1, 0);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 21, write status ")) return status;
	grid = zstructSpatialGridNew(gridPath);
	status = zspatialGridRetrieve(ifltab, grid, 1);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 22, retrieve status ")) {
		zstructFree(grid);
		return status;
	}
	if (!grid->_dataUnits || strlen(grid->_dataUnits) || !grid->_timeZoneID || strlen(grid->_timeZoneID) ||
		(grid->_numberOfCellsX != 4) || (grid->_numberOfCellsY != 3)) {
		printf("Fail in testSpatialShortHeader Loc 23, grid without header2: units \"%s\", time zone \"%s\", cells %d x %d\n",
			grid->_dataUnits ? grid->_dataUnits : "(null)", grid->_timeZoneID ? grid->_timeZoneID : "(null)",
			grid->_numberOfCellsX, grid->_numberOfCellsY);
		zstructFree(grid);
		return -1;
	}
	zstructFree(grid);

	//  TIN internal header of 2 ints (zspatialTinStore writes 12)
	ztransfer = zstructTransferNew(tinPath, 0);
	if (!ztransfer) return -1;
	internalHeader[0] = 0;
	ztransfer->dataType = DATA_TYPE_SPATIAL_TIN;
	ztransfer->internalHeader = internalHeader;
	ztransfer->internalHeaderNumber = 2;
	ztransfer->values1 = values;
	ztransfer->values1Number = 4;
	ztransfer->numberValues = 4;
	status = zwrite(ifltab, ztransfer);
	zstructFree(ztransfer);
	if (zcheckStatus(ifltab, status, 1, "Fail in testSpatialShortHeader Loc 30, write status ")) return status;
	tin = zstructSpatialTinNew(tinPath);
	status = zspatialTinRetrieve(ifltab, tin, 0);
	zstructFree(tin);
	if (!isInvalidHeaderError(status)) {
		printf("Fail in testSpatialShortHeader Loc 31, 2 int TIN header: status %d, expected an invalid header error\n", status);
		return -1;
	}

	return 0;
}

/*
 * scanner.h
 *
 *  Created on: 2014. 2. 13.
 *      Author: kos
 */

#ifndef SCANNER_H_
#define SCANNER_H_

#include <stdio.h>

#include <string>
#include <vector>

#include "demux.h"
#include "filter.h"
//------------------------------------------------------------------------

#define SCAN_BUFFER_SIZE 4096
#define SCAN_TIMEOUT     2000
typedef struct _ait_info_t {
	int section_no;
	int org_id;
	int app_id;
	int control_code;
	short profile_code;
	std::string url;
	std::string name;
} AITInfo;
typedef std::vector<AITInfo*> AITInfoVector;
//------------------------------------------------------------------------

typedef std::pair<int, ApplicationInformationSection*> ApplicationInformationSectionMultiPair;
typedef std::vector<ApplicationInformationSectionMultiPair> ApplicationInformationSectionMultiVector;

void Scan(eDemux& demux, int param_pmtid, int param_sid);
//------------------------------------------------------------------------

#endif /* SCANNER_H_ */

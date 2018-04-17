/*
 * scanner.cpp
 *
 *  Created on: 2014. 2. 13.
 *      Author: kos
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>

#include <list>
#include <vector>
#include <string>
#include <utility>

#include "log.h"
#include "filter.h"
#include "scanner.h"

using namespace std;
//------------------------------------------------------------------------

#define PACK_VERSION(major,minor,micro) (((major) << 16) + ((minor) << 8) + (micro))
#define UNPACK_VERSION(version,major,minor,micro) { \
	major = (version)&0xff; \
	minor = (version>>8)&0xff; \
	micro = (version>>16)&0xff; \
}
//------------------------------------------------------------------------

AITInfo* NewAITInfo(int section_no, int org_id, int app_id, int control_code, short profile_code, const char* url, const char* name);
void SaveAITInfo(AITInfoVector &ait_infos, FILE* fp);

int ScanPAT(eDemux& demux, ProgramAssociationSectionList &pat_list, char* buffer);
int ScanPMT(eDemux& demux, long pmtid, long sid, ProgramMapSectionList &pmt_list, char* buffer);
int ScanAIT(eDemux& demux, long aitid, ApplicationInformationSectionMultiVector &ait_vector, char* buffer);
void ParseAIT(ApplicationInformationSectionMultiVector &ait_vector, AITInfoVector &ait_infos);

void ClearPAT(ProgramAssociationSectionList &pat_list);
void ClearPMT(ProgramMapSectionList &pmt_list);
void ClearAIT(ApplicationInformationSectionMultiVector &ait_list);
void ClearAITInfo(AITInfoVector &ait_infos);
//------------------------------------------------------------------------

void Scan(eDemux& demux, int param_pmtid, int param_sid)
{
	AITInfoVector ait_infos;

	char buffer[SCAN_BUFFER_SIZE] = {0};
	ProgramAssociationSectionList pat_list;
	int length = ScanPAT(demux, pat_list, buffer);
	if (length <= 0) {
		exit(1);
	}
	for (ProgramAssociationSectionConstIterator pat_iter = pat_list.begin(); pat_iter != pat_list.end() ; ++pat_iter) {
		ProgramAssociationConstIterator pat_container = (*pat_iter)->getPrograms()->begin();
		for (; pat_container != (*pat_iter)->getPrograms()->end(); ++pat_container) {
			int pmtid = (*pat_container)->getProgramMapPid();
			int sid   = (*pat_container)->getProgramNumber();
			DA_DEBUG("PMTID : %x, SID : %x", pmtid, sid);

			if (param_pmtid != 0 && param_sid != 0) {
				if (pmtid != param_pmtid || sid != param_sid) {
					continue;
				}
			}

			ProgramMapSectionList pmt_list;
			length = ScanPMT(demux, pmtid, sid, pmt_list, buffer);
			if (length <= 0) {
				continue;
			}
			for(ProgramMapSectionList::iterator pmt_iter = pmt_list.begin(); pmt_iter != pmt_list.end() ; ++pmt_iter) {
				ElementaryStreamInfoConstIterator esi_iter = (*pmt_iter)->getEsInfo()->begin();
				for(; esi_iter != (*pmt_iter)->getEsInfo()->end(); ++esi_iter) {
					if((*esi_iter)->getType() != 0x05)
						continue;
					DescriptorConstIterator desc_iter = (*esi_iter)->getDescriptors()->begin();
					for(; desc_iter != (*esi_iter)->getDescriptors()->end(); ++desc_iter) {
						DA_DEBUG("DESC TYPE : %X", (*desc_iter)->getTag());
						switch ((*desc_iter)->getTag()) {
						case APPLICATION_SIGNALLING_DESCRIPTOR: {
								int aitid = (*esi_iter)->getPid();
								ApplicationInformationSectionMultiVector ait_vector;
								int length = ScanAIT(demux, aitid, ait_vector, buffer);
								DA_DEBUG("AIT read count : %d : %d", length, ait_vector.size());
								if (length > 0) {
									ParseAIT(ait_vector, ait_infos);
								}
								ClearAIT(ait_vector);
							}
							break;
						}
					}
				}
			}
			ClearPMT(pmt_list);
		}
	}
	ClearPAT(pat_list);

	SaveAITInfo(ait_infos, stdout);
	ClearAITInfo(ait_infos);
}
//------------------------------------------------------------------------

int ScanPAT(eDemux& demux, ProgramAssociationSectionList &pat_list, char* buffer)
{
	int len = 0;
	demux.Start(eDVBPATSpec());
	struct pollfd ufds;
	ufds.fd = demux.GetFd();
	ufds.events = POLLIN;

	for(int try_count = 0; ; try_count++) {
		if (poll(&ufds, 1, SCAN_TIMEOUT) <= 0) {
			return 0;
		}
		if((len = ::read(demux.GetFd(), buffer, SCAN_BUFFER_SIZE)) <= 0) {
			if(try_count == 5)
				break;
			continue;
		}
		unsigned int section_number = buffer[6];
		unsigned int last_section_number = buffer[7];
		if (try_count > 5 * (last_section_number + 1))
			break;
		if(!len)
			break;
#if ENABLE_DEBUG
		HexDump("PAT SECTION", (char*)buffer, len);
#endif /*ENABLE_DEBUG*/
		pat_list.push_back(new ProgramAssociationSection((const uint8_t*) buffer));
		DA_DEBUG("PAT section no : try count : %d, section_no : %d, last_section_no : %d (list size : %d)", try_count, section_number, last_section_number, pat_list.size());

		if(last_section_number == 0 && section_number == 0)
			break;
	}
	demux.Stop();
	return len;
}
//------------------------------------------------------------------------

int ScanPMT(eDemux& demux, long pmtid, long sid, ProgramMapSectionList &pmt_list, char* buffer)
{
	int len;
	demux.Start(eDVBPMTSpec(pmtid, sid));
	struct pollfd ufds;
	ufds.fd = demux.GetFd();
	ufds.events = POLLIN;

	for(int try_count = 0; ; try_count++) {
		if (poll(&ufds, 1, SCAN_TIMEOUT) <= 0) {
			return 0;
		}
		if((len = ::read(demux.GetFd(), buffer, SCAN_BUFFER_SIZE)) <= 0) {
			if(try_count == 5)
				break;
			continue;
		}
		unsigned int section_number = buffer[6];
		unsigned int last_section_number = buffer[7];
		if (try_count > 5 * (last_section_number + 1))
			break;
		if(!len)
			break;
#if ENABLE_DEBUG
		HexDump("PMT SECTION", (char*)buffer, len);
#endif /*ENABLE_DEBUG*/
		pmt_list.push_back(new ProgramMapSection((const uint8_t*) buffer));
		DA_DEBUG("PMT section no : try count : %d, section_no : %d, last_section_no : %d (list size : %d)", try_count, section_number, last_section_number, pmt_list.size());

		if(last_section_number == 0 && section_number == 0)
			break;
	}
	demux.Stop();
	return len;
}
//------------------------------------------------------------------------

int ScanAIT(eDemux& demux, long aitid, ApplicationInformationSectionMultiVector &ait_vector, char* buffer)
{
	struct pollfd ufds;
	ufds.fd = demux.GetFd();
	ufds.events = POLLIN;

	char filename[256]  = {0};
	int section_nos[10] = {0};

	demux.Start(eDVBAITSpec(aitid));
	for(int try_count = 0; ; try_count++) {
		if (poll(&ufds, 1, SCAN_TIMEOUT) <= 0) {
			return 0;
		}
		int len = ::read(demux.GetFd(), buffer, SCAN_BUFFER_SIZE);
		if(len <= 0) {
			if(try_count == 5)
				break;
			continue;
		}
		int section_number = buffer[6];
		int last_section_number = buffer[7];
		if (try_count > 5 * (last_section_number + 1))
			break;

#if ENABLE_DEBUG
		HexDump("AIT SECTION", (char*)buffer, len);
#endif /*ENABLE_DEBUG*/
		if (section_nos[section_number] == 0) {
			ApplicationInformationSection *ais = new ApplicationInformationSection((const uint8_t*) buffer);
			std::list<ApplicationInformation *>::const_iterator pp = ais->getApplicationInformation()->begin();
			for (; pp != ais->getApplicationInformation()->end(); ++pp) {
				int controlCode = (*pp)->getApplicationControlCode();
				const ApplicationIdentifier* applicationIdentifier = (*pp)->getApplicationIdentifier();
				int orgId = applicationIdentifier->getOrganisationId();
				if(controlCode == 1) {
#ifdef SUPPORT_MULTIPLE_SECTION
					sprintf(filename, "/tmp/ait.%d.%d", orgId, section_number);
#else
					sprintf(filename, "/tmp/ait.%d", orgId);
#endif /*SUPPORT_MULTIPLE_SECTION*/
					if (buffer[6] > 0) {
						DA_DEBUG("section_number %d > 0", buffer[6]);
						buffer[6] = 0;
					}
					if (buffer[7] > buffer[6]) {
						DA_DEBUG("last_section_number %d > section_number %d", buffer[7], buffer[6]);
						buffer[7] = buffer[6];
					}
					int fd = ::open(filename, O_RDWR | O_CREAT | O_TRUNC);
					if(fd >= 0) {
						::write(fd, buffer, len);
						::close(fd);
						DA_INFO("ait section saved. '%s'", filename);
					}
				}
			}
			ait_vector.push_back(std::make_pair(section_number, ais));
			section_nos[section_number] = len;
		}
		//DA_DEBUG("AIT section no : try count : %d, section_no : %d, last_section_no : %d (list size : %d)", try_count, section_number, last_section_number, list.size());

		bool exit_ok = true;
		for (int i = 0; i <= last_section_number; i++) {
			DA_DEBUG("section_nos[%d] : %d", i, section_nos[i]);
			if (section_nos[i] == 0) {
				exit_ok = false;
			}
		}
		if (exit_ok) {
			break;
		}
	}
	demux.Stop();
	return ait_vector.size();
}
//------------------------------------------------------------------------

void ParseAIT(ApplicationInformationSectionMultiVector &ait_vector, AITInfoVector &ait_infos)
{
	bool save_ait = false;
	bool already_exist = false;

	std:string applicaionName = "", hbbtvUrl = "", boundaryExtension = "";
	int profileCode = 0, orgId = 0, appId = 0, controlCode = 0, profileVersion = 0;

	ApplicationInformationSectionMultiVector::iterator ait_top_iter = ait_vector.begin();
	for(; ait_top_iter != ait_vector.end(); ++ait_top_iter){
		ApplicationInformationSectionMultiPair aismp = (*ait_top_iter);

		int section_no = aismp.first;
		ApplicationInformationSection *ais = aismp.second;
		DA_DEBUG("start parse ait!! -  %d", ais->getApplicationInformation()->size());

		std::list<ApplicationInformation *>::const_iterator pp = ais->getApplicationInformation()->begin();
		for (; pp != ais->getApplicationInformation()->end(); ++pp) {
			controlCode = (*pp)->getApplicationControlCode();

			const ApplicationIdentifier* applicationIdentifier = (*pp)->getApplicationIdentifier();
			orgId = applicationIdentifier->getOrganisationId();
			appId = applicationIdentifier->getApplicationId();

			DA_DEBUG("control code : %d", controlCode);

			if(controlCode == 1 || controlCode == 2) {
				DescriptorConstIterator desc = (*pp)->getDescriptors()->begin();

				for(; desc != (*pp)->getDescriptors()->end(); ++desc) {
					switch ((*desc)->getTag()) {
					case APPLICATION_DESCRIPTOR: {
							ApplicationDescriptor* applicationDescriptor = (ApplicationDescriptor*)(*desc);
							const ApplicationProfileList* applicationProfiles = applicationDescriptor->getApplicationProfiles();
							ApplicationProfileConstIterator interactionit = applicationProfiles->begin();
							for(; interactionit != applicationProfiles->end(); ++interactionit) {
								profileCode = (*interactionit)->getApplicationProfile();
								profileVersion = PACK_VERSION(
									(*interactionit)->getVersionMajor(),
									(*interactionit)->getVersionMinor(),
									(*interactionit)->getVersionMicro()
								);
							}
						}
						break;
					case APPLICATION_NAME_DESCRIPTOR: {
							ApplicationNameDescriptor *nameDescriptor  = (ApplicationNameDescriptor*)(*desc);
							ApplicationNameConstIterator interactionit = nameDescriptor->getApplicationNames()->begin();
							for(; interactionit != nameDescriptor->getApplicationNames()->end(); ++interactionit) {
								applicaionName = (*interactionit)->getApplicationName();
								break;
							}
						}
						break;
					case TRANSPORT_PROTOCOL_DESCRIPTOR: {
							TransportProtocolDescriptor *transport = (TransportProtocolDescriptor*)(*desc);
							switch (transport->getProtocolId())
							{
							case 1: /* object carousel */
								break;
							case 2: /* ip */
								break;
							case 3: { /* interaction */
									InterActionTransportConstIterator interactionit = transport->getInteractionTransports()->begin();
									for(; interactionit != transport->getInteractionTransports()->end(); ++interactionit) {
										hbbtvUrl = (*interactionit)->getUrlBase()->getUrl();
										break;
									}
									break;
								}
							}
						}
						break;
					case GRAPHICS_CONSTRAINTS_DESCRIPTOR:
						break;
					case SIMPLE_APPLICATION_LOCATION_DESCRIPTOR: {
							SimpleApplicationLocationDescriptor *applicationlocation = (SimpleApplicationLocationDescriptor*)(*desc);
							hbbtvUrl += applicationlocation->getInitialPath();
						}
						break;
					case APPLICATION_USAGE_DESCRIPTOR:
						break;
					case SIMPLE_APPLICATION_BOUNDARY_DESCRIPTOR: {
							SimpleApplicationBoundaryDescriptor *boundaryDescriptor  = (SimpleApplicationBoundaryDescriptor*)(*desc);
							const BoundaryExtensionList* boundaryList = boundaryDescriptor->getBoundaryExtensions();
							BoundaryExtensionConstIterator interactionit = boundaryList->begin();
							for(; interactionit != boundaryList->end(); ++interactionit) {
								boundaryExtension = (*interactionit)->getBoundaryExtension();
								if(!boundaryExtension.empty()) {
									break;
								}
							}
						}
						break;
					}
				}
			}

			DA_DEBUG("\n"\
					"\t- SectionNo=%d\n"\
					"\t- ApplicaionName=%s\n"\
					"\t- HbbtvUrl=%s\n"\
					"\t- BoundaryExtension=%s\n"\
					"\t- ProfileCode=%d\n"\
					"\t- OrgId=%x\n"\
					"\t- AppId=%x\n"\
					"\t- ControlCode=%x\n"\
					"\t- ProfileVersion=%x\n"\
					"------------------------------------------------------------------\n",
					section_no, applicaionName.c_str(), hbbtvUrl.c_str(), boundaryExtension.c_str(), profileCode, orgId, appId, controlCode, profileVersion
					);

			if(!hbbtvUrl.empty()) {
				int control_code_flag = 0;
				const char* uu = hbbtvUrl.c_str();
				if(!strncmp(uu, "http://", 7) || !strncmp(uu, "dvb://", 6) || !strncmp(uu, "https://", 8)) {
					switch(profileVersion) {
					case 65793:
					case 66049:	control_code_flag =  1; break;
					case 1280:
					case 65538:
					default:	control_code_flag = -1; break;
					}
				}
				else if (!boundaryExtension.empty()) {
					if(boundaryExtension.at(boundaryExtension.length()-1) != '/') {
						boundaryExtension += "/";
					}
					boundaryExtension += hbbtvUrl;
					switch(profileVersion) {
					case 65793:
					case 66049: control_code_flag =  1; break;
					case 1280:
					case 65538:
					default:	control_code_flag = -1; break;
					}
				}
				already_exist = false;
				if (control_code_flag != 0) {
					AITInfoVector::iterator ait_info_iter = ait_infos.begin();
					for (; ait_info_iter != ait_infos.end(); ++ait_info_iter) {
						AITInfo* info = (*ait_info_iter);
						if (strcmp(info->url.c_str(), uu) == 0) {
							already_exist = true;
							break;
						}
					}
					if (already_exist) {
						DA_WARNING("already exist ait info.");
						break;
					}
					ait_infos.push_back(NewAITInfo(section_no, orgId, appId, control_code_flag * controlCode, profileCode, hbbtvUrl.c_str(), applicaionName.c_str()));
					DA_INFO("ait info pushed.");
				}
			}
		}
	}
}
//------------------------------------------------------------------------

AITInfo* NewAITInfo(int section_no, int org_id, int app_id, int control_code, short profile_code, const char* url, const char* name)
{
	AITInfo* ait_info = new AITInfo;
	ait_info->section_no   = section_no;
	ait_info->org_id       = org_id;
	ait_info->app_id       = app_id;
	ait_info->control_code = control_code;
	ait_info->profile_code = profile_code;
	ait_info->url          = url;
	ait_info->name         = name;
	return ait_info;
}
//------------------------------------------------------------------------

void SaveAITInfo(AITInfoVector &ait_infos, FILE* fp)
{
	fprintf(fp, "<applications>\n");
	for (AITInfoVector::iterator iter = ait_infos.begin(); iter != ait_infos.end(); ++iter) {
		AITInfo* info = (*iter);
		fprintf(fp, "<application>\n");
		fprintf(fp, "       <secno>%d</secno>\n", info->section_no);
		fprintf(fp, "       <control>%d</control>\n", info->control_code);
		fprintf(fp, "       <orgid>%d</orgid>\n", info->org_id);
		fprintf(fp, "       <appid>%d</appid>\n", info->app_id);
		fprintf(fp, "       <name>%s</name>\n", info->name.c_str());
		fprintf(fp, "       <url>%s</url>\n", info->url.c_str());
		fprintf(fp, "       <profile>%d</profile>\n", info->profile_code);
		fprintf(fp, "</application>\n");
	}
	fprintf(fp, "</applications>\n");
	fflush(fp);
}
//------------------------------------------------------------------------

void ClearPAT(ProgramAssociationSectionList &pat_list)
{
	ProgramAssociationSectionList::iterator iter = pat_list.begin();
	for (; iter != pat_list.end(); ++iter) {
		if (*iter) {
			delete *iter;
		}
	}
	pat_list.clear();
}
//------------------------------------------------------------------------

void ClearPMT(ProgramMapSectionList &pmt_list)
{
	ProgramMapSectionList::iterator iter = pmt_list.begin();
	for (; iter != pmt_list.end(); ++iter) {
		if (*iter) {
			delete *iter;
		}
	}
	pmt_list.clear();
}
//------------------------------------------------------------------------

void ClearAIT(ApplicationInformationSectionMultiVector &ait_vector)
{
	ApplicationInformationSectionMultiVector::iterator iter = ait_vector.begin();
	for (; iter != ait_vector.end(); ++iter) {
		ApplicationInformationSectionMultiPair p = *iter;
		if (p.second) {
			delete (p.second);
		}
	}
	ait_vector.clear();
}
//------------------------------------------------------------------------

void ClearAITInfo(AITInfoVector &ait_infos)
{
	AITInfoVector::iterator iter = ait_infos.begin();
	for (; iter != ait_infos.end(); ++iter) {
		if (*iter) {
			delete *iter;
		}
	}
	ait_infos.clear();
}
//------------------------------------------------------------------------

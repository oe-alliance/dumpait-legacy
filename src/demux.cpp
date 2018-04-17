/*
 * Demux.cpp
 *
 *  Created on: 2014. 2. 13.
 *      Author: kos
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "log.h"
#include "demux.h"
//------------------------------------------------------------------------

eDemux::eDemux()
	: m_demux_fd(-1), m_is_activated(false)
{
}
//------------------------------------------------------------------------

eDemux::~eDemux()
{
	Close();
}
//------------------------------------------------------------------------

eDVBSectionFilterMask& eDemux::FilterToMask(const eDVBTableSpec &table)
{
	static eDVBSectionFilterMask mask;

	memset(&mask, 0, sizeof(mask));
	mask.pid   = table.pid;
	mask.flags = 0;

	if (table.flags & eDVBTableSpec::tfCheckCRC)
		mask.flags |= eDVBSectionFilterMask::rfCRC;

	if (table.flags & eDVBTableSpec::tfHaveTID) {
		mask.data[0] = table.tid;
		if (table.flags & eDVBTableSpec::tfHaveTIDMask)
			mask.mask[0] = table.tid_mask;
		else
			mask.mask[0] = 0xFF;
	}

	if (table.flags & eDVBTableSpec::tfHaveTIDExt) {
		mask.data[1] = table.tidext >> 8;
		mask.data[2] = table.tidext;
		if (table.flags & eDVBTableSpec::tfHaveTIDExtMask) {
			mask.mask[1] = table.tidext_mask >> 8;
			mask.mask[2] = table.tidext_mask;
		} else {
			mask.mask[1] = 0xFF;
			mask.mask[2] = 0xFF;
		}
	}

	if (!(table.flags & eDVBTableSpec::tfAnyVersion)) {
		mask.data[3] |= (table.version << 1)|1;
		mask.mask[3] |= 0x3f;
		if (!(table.flags & eDVBTableSpec::tfThisVersion))
			mask.mode[3] |= 0x3e;
	}
	return mask;
}
//------------------------------------------------------------------------

bool eDemux::Open(const char* path)
{
	m_demux_fd = ::open(path, O_RDWR);
	if(m_demux_fd < 0) {
		m_demux_fd = -1;
		DA_ERROR("demux opening fail : '%s', errno : '%d'", path, errno);
		return false;
	}

	DA_DEBUG("demux opening success '%s' : '%d'", path, m_demux_fd);
	return true;
}
//------------------------------------------------------------------------

void eDemux::Close()
{
	Stop();
	if (m_demux_fd > -1) {
		close(m_demux_fd);
		m_demux_fd = -1;
	}
}
//------------------------------------------------------------------------

bool eDemux::Start(const eDVBTableSpec &table)
{
	int rc = -1;
	if (m_demux_fd >= 0) {
		dmx_sct_filter_params sct;
		eDVBSectionFilterMask mask = FilterToMask(table);

		sct.pid		= mask.pid;
		sct.timeout	= 0;
		sct.flags	= DMX_IMMEDIATE_START;

		memcpy(sct.filter.filter, mask.data, DMX_FILTER_SIZE);
		memcpy(sct.filter.mask, mask.mask, DMX_FILTER_SIZE);
		memcpy(sct.filter.mode, mask.mode, DMX_FILTER_SIZE);
		::ioctl(m_demux_fd, DMX_SET_BUFFER_SIZE, 8192 * 8);
		rc = ::ioctl(m_demux_fd, DMX_SET_FILTER, &sct);
	}
	m_is_activated = (rc < 0) ? false : true;
	DA_DEBUG("demux setting result '%s'", m_is_activated ? "SUCCESS" : "FAIL");
	return m_is_activated;
}
//------------------------------------------------------------------------

void eDemux::Stop()
{
	if (m_is_activated) {
		::ioctl(m_demux_fd, DMX_STOP);
		m_is_activated = false;
		DA_DEBUG("demux stoped.");
	}
}
//------------------------------------------------------------------------

/*
 * Demux.h
 *
 *  Created on: 2014. 2. 13.
 *      Author: kos
 */

#ifndef DEMUX_H_
#define DEMUX_H_

#include "filter.h"
//------------------------------------------------------------------------

class eDemux
{
protected:
	eDVBSectionFilterMask& FilterToMask(const eDVBTableSpec &table);

public:
	eDemux();
	virtual ~eDemux();

	bool Open(const char* path);
	void Close();

	bool Start(const eDVBTableSpec &table);
	void Stop();

	int GetFd() { return m_demux_fd; }

private:
	int m_demux_fd;
	bool m_is_activated;
};
//------------------------------------------------------------------------

#endif /* DEMUX_H_ */

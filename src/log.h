/*
 * log.h
 *
 *  Created on: 2014. 2. 13.
 *      Author: kos
 */

#ifndef LOG_H_
#define LOG_H_
//------------------------------------------------------------------------

#define ENABLE_DEBUG   0
#define ENABLE_INFO    0
#define ENABLE_WARNING 0
#define ENABLE_ERROR   0
//------------------------------------------------------------------------

#define _DA_LOG(level, fmt, ...) \
	do { \
		fprintf(stderr, "DUMPAIT "level" : "fmt" (%s:%d::%s)\n", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
	} while (0)
//------------------------------------------------------------------------

#if ENABLE_DEBUG
#	define DA_DEBUG(fmt, ...)   _DA_LOG("DEBUG  ", fmt, ##__VA_ARGS__)
void HexDump(const char *pTagName, char *pHexData, int nLen);
#else
#	define DA_DEBUG(fmt, ...)
#endif
//------------------------------------------------------------------------

#if ENABLE_INFO
#	define DA_INFO(fmt, ...)    _DA_LOG("INFO   ", fmt, ##__VA_ARGS__)
#else
#	define DA_INFO(fmt, ...)
#endif
//------------------------------------------------------------------------

#if ENABLE_WARNING
#	define DA_WARNING(fmt, ...) _DA_LOG("WARNING", fmt, ##__VA_ARGS__)
#else
#	define DA_WARNING(fmt, ...)
#endif
//------------------------------------------------------------------------

#if ENABLE_ERROR
#	define DA_ERROR(fmt, ...)   _DA_LOG("ERROR  ", fmt, ##__VA_ARGS__)
#else
#	define DA_ERROR(fmt, ...)
#endif
//------------------------------------------------------------------------

#endif /* LOG_H_ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>

#include "log.h"
#include "demux.h"
#include "filter.h"
#include "scanner.h"

#include <string>

using namespace std;
//------------------------------------------------------------------------

#define PV "2.2"
#define usage(PN)   { \
	printf("Usage: %s [OPTIONS]\n"\
	"  Version %s\n"\
	"      --demux={HEXA}\n"\
	"      --pmtid={HEXA}\n"\
	"      --serviceid={HEXA}\n"\
	"      --help\n"\
	"  ex> %s --demux=0 --pmtid=1b3b --serviceid=9a7b\n"\
	"\n", PN, PV, PN); exit(0);\
	};
//------------------------------------------------------------------------

int htoi(char *str);
bool is_hex_str(char* str);
//------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int param_dmxid   = -1;
	int param_pmtid = -1;
	int param_sid   = -1;

	static struct option long_options[] = {
		{"help",	0, NULL, 'h'},
		{"demux",	1, NULL, 'd'},
		{"pmtid",	1, NULL, 'p'},
		{"serviceid",	1, NULL, 's'},
		{0, 0, 0, 0}
	};

	int opt = 0;
	do {
		opt = getopt_long(argc, argv, "d:p:s:h", long_options, 0);
		switch (opt) {
		case 'd': if(is_hex_str(optarg)) param_dmxid = htoi(optarg); break;
		case 'p': if(is_hex_str(optarg)) param_pmtid = htoi(optarg); break;
		case 's': if(is_hex_str(optarg)) param_sid   = htoi(optarg); break;
		case 'h': usage(argv[0]); break;
		}
	} while(opt != -1);

	if (param_dmxid < 0 || param_pmtid < 0 || param_sid < 0) {
		usage(argv[0]);
	}

	char demux_path[256] = {0};
	sprintf(demux_path, "/dev/dvb/adapter0/demux%d", param_dmxid);

	eDemux demux;
	if (demux.Open(demux_path) == false) {
		exit(1);
	}

	Scan(demux, param_pmtid, param_sid);

	return 0;
}

bool is_hex_str(char* str)
{
	char* temp = str;
	while(*temp) {
		switch(*temp)
		{
		case '0': break;
		case '1': break;
		case '2': break;
		case '3': break;
		case '4': break;
		case '5': break;
		case '6': break;
		case '7': break;
		case '8': break;
		case '9': break;
		case 'a': break;
		case 'b': break;
		case 'c': break;
		case 'd': break;
		case 'e': break;
		case 'f': break;
		case 'A': break;
		case 'B': break;
		case 'C': break;
		case 'D': break;
		case 'E': break;
		case 'F': break;
		default:  return false;
		}
		++temp;
	}
	return true;
}
//------------------------------------------------------------------------

int htoi(char *str)
{
	int   degit	= 0;
	char* hexa	= str;

	while(*hexa) {
		degit *= 16;
		switch(*hexa)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			degit += *hexa - '0';
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			degit += *hexa - 'a' + 10;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			degit += *hexa - 'A' + 10;
			break;
		}
		++hexa;
	}
	return degit;
}
//------------------------------------------------------------------------

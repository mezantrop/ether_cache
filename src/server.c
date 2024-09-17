/* -------------------------------------------------------------------------- */
/*
  "THE BEER-WARE LICENSE" (Revision 42):
  zmey20000@yahoo.com wrote this file. As long as you retain this notice you
  can do whatever you want with this stuff. If we meet some day, and you think
  this stuff is worth it, you can buy me a beer in return Mikhail Zakharov
*/

/* -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <err.h>
#include <errno.h>

#include "ec.h"
#include "crc32.h"

/* -------------------------------------------------------------------------- */
void usage(char *emsg) {
	fprintf(stderr,
		"Ethernet message caching server\n"
		"Error:\n"
		"\t%s\n"
		"Usage:\n"
		"\tserver NIC\n"
		"\n"
		"Example:\n"
		"\tserver enp0s3\n",
		*emsg ? emsg : "General fault");
        exit(1);
}

/* -------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
	struct ifreq ifrq;
	struct sockaddr_ll ll_addr, sl_addr;
	int sl_addr_len = 0;
	char *smac = NULL;
	struct ether_addr mac;
	int s = -1;
	struct eth_cache ec;

	char buffer[128][1494] = {0};
	int mid;


	if (argc != 2)
		usage("Wrong number of arguments specified");
	
	if ((s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_802_3))) == -1)
		err(1, "Unable to create a socket");

	memset(&ifrq, 0, sizeof(struct ifreq));
	strncpy(ifrq.ifr_name, argv[1], IFNAMSIZ - 1);
	if (ioctl(s, SIOCGIFINDEX, &ifrq) < 0)
		err(1, "Unable to get NIC Index by name");

	/* packet(7) */
	memset(&ll_addr, 0,  sizeof(struct sockaddr_ll));
	ll_addr.sll_family = AF_PACKET;
	ll_addr.sll_protocol = htons(ETH_P_802_3);
	ll_addr.sll_ifindex = ifrq.ifr_ifindex;		/* bind socket 0 - any;
							// 1 - loopback;
							// 2 - first nic. */
	if (ioctl(s, SIOCGIFHWADDR, &ifrq) < 0)
		err(1, "Unable to get MAC by NIC name");
	memcpy(mac.ether_addr_octet, (struct ether_addr *)&ifrq.ifr_hwaddr.sa_data,
		sizeof(struct ether_addr));

	if (bind(s, (struct sockaddr*)&ll_addr, sizeof(struct sockaddr_ll)) ==1)
		err(1, "Unable to bind");

	while (1) {
		memset(&ec, 0, sizeof(ec));
		memset(&sl_addr, 0, sizeof(sl_addr));

		sl_addr_len = sizeof(sl_addr);
		if (recvfrom(s, (char *)&ec, sizeof(ec), 0,
			(struct sockaddr *)&sl_addr, &sl_addr_len) == -1)
				err(1, "Unable to receive data");
		smac = ether_ntoa((struct ether_addr *)&sl_addr.sll_addr);

                if (!verify_crc32((char *)&ec, sizeof(ec)))
                        fprintf(stderr,
                                "Warning: Received frame with Incorrect FCS! %x\n",
                                ec.crc);

		if (ec.fm.pl.cmd < 0x80) {
			/* Save in cache */
			memset(buffer[ec.fm.pl.cmd], 0,
				sizeof(buffer[ec.fm.pl.cmd]));
			memcpy(buffer[ec.fm.pl.cmd], ec.fm.pl.pl,
				sizeof(ec.fm.pl.pl));

			printf("FROM:\t%s #%03d: [%s]\n", smac, ec.fm.pl.cmd,
				ec.fm.pl.pl[0] ? (char*)ec.fm.pl.pl : "<Free slot>");

		} else {
			/* Fetch from cache */
			memcpy(&ec.fm.eh.ether_dhost,
				(struct ether_addr *)&sl_addr.sll_addr, ETH_ALEN);
			memcpy(&ec.fm.eh.ether_shost, &mac, ETH_ALEN);
			mid = ec.fm.pl.cmd &~ 0x80;
			memcpy(&ec.fm.pl.pl, buffer[mid], sizeof(ec.fm.pl.pl));
			/* ec.crc = computeCrc32((char *)&ec.fm, sizeof(ec.fm)); */
			ec.crc = htonl(calk_crc32((char *)&ec.fm, sizeof(ec.fm)));
			/* if (!verifyEthernetFrameFcs((char *)&ec, sizeof(ec))) */
			if (!verify_crc32((char *)&ec, sizeof(ec)))
				fprintf(stderr,
					"Warning: Incorrect Frame Check Sequence!\n");

			if (sendto(s,
				(char *)&ec, sizeof(ec), 0,
				(struct sockaddr*)&sl_addr,
				sizeof(struct sockaddr_ll)) < 0)
					err(1, "Unable to send data");
		
			printf("TO:\t%s #%03d: [%s]\n", smac, mid,
				ec.fm.pl.pl[0] ? (char*)ec.fm.pl.pl : "<Free slot>");

		}

	}

	/* We will never get there */
	close(s);
	return 0;
}

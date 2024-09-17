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
		"Ethernet message caching client\n\n"
		"Error:\n"
		"\t%s\n" 
		"Usage:\n"
		"\tclient SRC_NIC MAC_DST 0-127 'Message to send'\n"
		"\tclient SRC_NIC MAC_DST 0-127\n"
		"\n"
		"For example, to send:\n"
		"\tclient enp0s3 22:22:22:22:22:22 34 'foo bar baz bam'\n"
		"And to retrieve:\n"
		"\tclient enp0s3 22:22:22:22:22:22 56\n",
		*emsg ? emsg : "General fault");
	exit(1);
}

/* -------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
	struct ether_addr src;
	struct ether_addr *dst = NULL;
	int mid;
	int s = -1;
	struct eth_cache ec;
	struct sockaddr_ll dst_addr;
	int dst_addr_len = 0;
	struct ifreq ifrq;

	if (argc < 4)
		usage("Wrong number of arguments specified");

	if (!(dst = ether_aton(argv[2])))
		usage("Wrong destination ethernet address");

	mid = strtol(argv[3], (char **)NULL, 10);
	if (mid < 0 || mid > 0x7F || mid == 0 && errno == EINVAL)
		usage("Unsupported message ID");


	if ((s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_802_3))) == -1)
		err(1, "Unable to create a socket");

	memset(&ifrq, 0, sizeof(struct ifreq));
	strncpy(ifrq.ifr_name, argv[1], IFNAMSIZ - 1);
	if (ioctl(s, SIOCGIFHWADDR, &ifrq) < 0)
		err(1, "Unable to get MAC by NIC name");
	memcpy(src.ether_addr_octet, (struct ether_addr *)&ifrq.ifr_hwaddr.sa_data,
			sizeof(struct ether_addr));

	/* printf("DEBUG SRC MAC: %s\n", ether_ntoa(&src)); */

	memset(&ec, 0, sizeof(ec));
	memcpy(&ec.fm.eh.ether_dhost, dst, ETH_ALEN);
	memcpy(&ec.fm.eh.ether_shost, &src, ETH_ALEN);
	/* Ethernet type < 1500 == 802.3 frame */
	ec.fm.eh.ether_type = htons(sizeof(ec.fm.pl)); 
	ec.fm.pl.pp = 0xFFFF;			/* Novell raw 802.3 */


	if (argv[4]) {				/* Send a message to a server */
		ec.fm.pl.cmd = mid;
		memcpy(&ec.fm.pl.pl, argv[4], strnlen(argv[4], ETH_DATA_LEN));
	} else		 			/* Retrieve a message by ID */
		ec.fm.pl.cmd = mid | 0x80;
	
	/* ec.crc = computeCrc32((char *)&ec.fm, sizeof(ec.fm)); */
	ec.crc = htonl(calk_crc32((char *)&ec.fm, sizeof(ec.fm)));
	/* if (!verifyEthernetFrameFcs((char *)&ec, sizeof(ec)))  */
	if (!verify_crc32((char *)&ec, sizeof(ec))) 
		fprintf(stderr,
			"Warning: Incorrect Frame Check Sequence! %x\n", ec.crc);

	if (ioctl(s, SIOCGIFINDEX, &ifrq) < 0)
		err(1, "Unable to get NIC Index by name");

	/* packet(7) */
	dst_addr.sll_family = AF_PACKET;
	dst_addr.sll_protocol = htons(ETH_P_802_3);
	dst_addr.sll_ifindex = ifrq.ifr_ifindex;	/* to bind socket 0 - any
						   	1 - loopback; 
							2 - first nic. */
	/* dst_addr.sll_hatype = 1; */	/* ARPHRD_ETHER from linux/if_arp.h */
	/* dst_addr.sll_pkttype */	/* make sense only for receivin */
	dst_addr.sll_halen = ETH_ALEN;
	memcpy(dst_addr.sll_addr, dst, ETH_ALEN);

/*
	printf("DEBUG ET      : %d\n", ec.fm.eh.ether_type);
	printf("DEBUG EC SZ   : %ld\n", sizeof(ec));
	printf("DEBUG FM SZ   : %ld\n", sizeof(ec.fm));
	printf("DEBUG CRC     : %x\n", ec.crc);
	printf("DEBUG PL SZ   : %ld\n", sizeof(ec.fm.pl));
	printf("DEBUG PL PP SZ: %ld\n", sizeof(ec.fm.pl.pp));
	printf("DEBUG PL PL SZ: %ld\n", sizeof(ec.fm.pl.pl));
*/
	if (ec.fm.pl.cmd > 0x7F)
	      	if (bind(s, (struct sockaddr*)&dst_addr,
			sizeof(struct sockaddr_ll)) == -1)
        	       		err(1, "Unable to bind");

	if (sendto(s,
		(char *)&ec, sizeof(ec), 0, 
		(struct sockaddr*)&dst_addr,
		sizeof(struct sockaddr_ll)) < 0)
			err(1, "Unable to send data");

	if (ec.fm.pl.cmd > 0x7F) {
		/* Get a reply from server with a stored message */
                dst_addr_len = sizeof(dst_addr);
                if (recvfrom(s, (char *)&ec, sizeof(ec), 0,
			(struct sockaddr *)&dst_addr, &dst_addr_len) == -1)
				err(1, "Failed receiving a reply");

		if (!verify_crc32((char *)&ec, sizeof(ec))) 
			fprintf(stderr,
				"Warning: Received frame with Incorrect FCS! %x\n",
				ec.crc);


		printf("FROM:\t%s #%03d: [%s]\n",
			ether_ntoa((struct ether_addr *)&dst_addr.sll_addr),
			mid,
			ec.fm.pl.pl[0] ? (char*)ec.fm.pl.pl : "<Free slot>");
	}

	/* Good manners - always close sockets */
	close(s);
	return 0;
}

/* Program to put network interface in promiscuous mode
 * and read all packets
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if.h>

#define MTU	1536

char ifname[10];
struct ifreq	ethreq;
int	sock, pkt_num;

void my_cleanup( void )
{
	// turn off the interface's 'promiscuous' mode
	ethreq.ifr_flags &= ~IFF_PROMISC;
	if ( ioctl( sock, SIOCSIFFLAGS, &ethreq ) < 0 )
		{ perror( "ioctl: set ifflags" ); exit(1); }
}


void my_handler( int signo)
{
	// This function executes when the user hits <CTRL-C>.
	printf("exiting..");
	exit(0);
}


void display_packet( char *buf, int n )
{
	unsigned char	ch;

	printf( "\npacket #%d ", ++pkt_num );
	for (int i = 0; i < n; i+=16) {
		printf( "\n%04X: ", i );
		//print hex value
		for (int j = 0; j < 16; j++) {
			ch = ( i + j < n ) ? buf[ i+j ] : 0;
			if ( i + j < n ) printf( "%02X ", ch );
			else	printf( "   " );
		}

		// print ascii value
		for (int j = 0; j < 16; j++) {
			ch = ( i + j < n ) ? buf[ i+j ] : ' ';
			if (( ch < 0x20 )||( ch > 0x7E )) ch = '.';
			printf( "%c", ch );
		}
	}
	printf( "\n%d bytes read\n-------\n", n );
}


int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("usage: %s <ifname>\n", argv[0]);
		exit(1);
	}
	strncpy(ifname, argv[1], strlen(argv[1]));
	printf("network interafe name: %s\n", ifname);

	// create an unnamed socket for reception of ethernet packets 
	sock = socket( PF_PACKET, SOCK_RAW, htons( ETH_P_ALL ) ); 
	if ( sock < 0 ) { perror( "socket" ); exit( 1 ); }

	// enable 'promiscuous mode' for the selected socket interface
	strncpy( ethreq.ifr_name, ifname, IFNAMSIZ );
	if ( ioctl( sock, SIOCGIFFLAGS, &ethreq ) < 0 )
		{ perror( "ioctl: get ifflags" ); exit(1); }
	ethreq.ifr_flags |= IFF_PROMISC;  // enable 'promiscuous' mode
	if ( ioctl( sock, SIOCSIFFLAGS, &ethreq ) < 0 )
		{ perror( "ioctl: set ifflags" ); exit(1); }

	// make sure 'promiscuous mode' will get disabled upon termination
	atexit( my_cleanup );
	signal( SIGINT, my_handler );

	// main loop to intercept and display the ethernet packets
	char	buffer[ MTU ];
	printf( "\nMonitoring all packets on interface \'%s\' \n", ifname );
	do	{
		int	n = recvfrom( sock, buffer, MTU, 0, NULL, NULL );
		display_packet( buffer, n );
		}
	while (true);
}

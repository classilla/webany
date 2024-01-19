#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

int porta;
struct termios ttya, ttya_saved;
struct sigaction hothotsig;
fd_set master;

void
usend(int j,...)
{
	size_t i=0;
	int rv;
	fd_set wfd;
	va_list ap;
	char d;

	va_start(ap, j);

	fprintf(stderr, "-- sending --\n");
	for(;;) {
		wfd=master;
		rv = select(porta+1, NULL, &wfd, NULL, NULL);
		if (rv < 0) {
			perror("select(send)");
			exit(255);
		}
		if (rv > 0) break;
	}
	for(i=0;i<j;i++) {
		char c = (char)va_arg(ap, int);
		fprintf(stderr, " %02x ", c);
		write(porta, &c, 1);
	}
	fprintf(stderr, "\n");
}

void
uwait(char c)
{
	fd_set rfd;
	struct timeval tv;
	char d;
	int rv;

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	fprintf(stderr, "-- waiting for %02x:", c);
	for(;;) {
		for(;;) {
			rfd=master;
			rv = select(porta+1, &rfd, NULL, NULL, &tv);
			if (rv < 0) {
				perror("select(send)");
				exit(255);
			}
			if (rv == 0) {
				if (tv.tv_sec > 0 || tv.tv_usec > 0) continue;
				perror("timeout on select");
				exit(255);
			}
			if (rv > 0) break;
		}
		read(porta, &d, 1);
		fprintf(stderr, " %02x ", d);
		if (d == c) {
			fprintf(stderr, "--\n");
			return;
		}
	}
	fprintf(stderr, "unreachable\n"); exit(255);
}

void
cleanup()
{
	(void)tcsetattr(porta, TCSAFLUSH, &ttya_saved);
	(void)close(porta);
}

void
hotsigaction(int hotsig)
{
	fprintf(stdout, "\n\nexiting on signal\n");
	cleanup();
	exit(0);
}

int
main(int argc, char **argv)
{
	fd_set rfd, wfd;
	struct timeval tv;
	unsigned char c;
	int i;

	if (argc == 2)
		porta = open(argv[1], O_RDWR | O_NOCTTY);
	else
		porta = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
	if (tcgetattr(porta, &ttya_saved)) {
		perror("tcgetattr (A)");
		close(porta);
		return 1;
	}

	atexit(cleanup);
	(void)memset(&hothotsig, 0, sizeof(hothotsig));
	hothotsig.sa_handler = hotsigaction;
	sigaction(SIGINT, &hothotsig, NULL);
	sigaction(SIGTERM, &hothotsig, NULL);
	sigaction(SIGHUP, &hothotsig, NULL);

	memset(&ttya, 0, sizeof(ttya));
	ttya.c_cflag = (CS8 | CREAD | CLOCAL);
ttya.c_cflag |= (PARENB | CSTOPB); /* 9600bps 8E2 no flow control */
	ttya.c_cc[VTIME] = 5;
	ttya.c_cc[VMIN] = 1;
	cfsetospeed(&ttya, B9600);
	cfsetispeed(&ttya, B9600);
	tcsetattr(porta, TCSANOW, &ttya);

	FD_ZERO(&master);
	FD_SET(porta, &master);
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	setvbuf(stdout, NULL, _IONBF, 0); /* select(STDOUT); $|++; */

fprintf(stderr, "port open, sending in 5 seconds\n");
sleep(5);

usend(10,0xaa,0x55,0x00,0x00,0x00,0x42,0x00,0x00,0x01,0x42);
uwait(0xaa);
usend(129,
0x1e, 0x16, 0x08, 0x38, 0x3d, 0x05, 0x30, 0x1b, 0x05, 0x12, 0x08, 0x38, 0x27, 0x2d, 0x16, 0x3a, 0x3a, 0x01, 0x3c, 0x1f, 0x16, 0x05, 0x1f, 0x30, 0x38, 0x38, 0x30, 0x38, 0x4e, 0x05, 0x3c, 0x1f, 0x16, 0x05, 0x1f, 0x30, 0x38, 0x38, 0x30, 0x38, 0x4e, 0x01, 0x2a, 0x21, 0x3a, 0x3d, 0x08, 0x1f, 0x05, 0x26, 0x40, 0x38, 0x3d, 0x4c, 0x62, 0x05, 0x1f, 0x16, 0x05, 0x13, 0x16, 0x08, 0x13, 0x63, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
113);
uwait(0xaa);
usend(10, 0xAA, 0x55, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE);
uwait(0xAA);

	fprintf(stderr, "\nok\n");
	return 0;
}

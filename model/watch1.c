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

usend(10,
0xAA, 0x55, 0x00, 0x00, 0x00,
0x10, 0x00, 0x00, 0x01, 0x10
); uwait(0xAA);

usend(129,
0x3D, 0x16, 0x3A, 0x3D, 0x01, 0x3D, 0x16, 0x3A, 0x3D, 0x66,
0x46, 0x08, 0x3D, 0x10, 0x1F, 0x00, 0x4A, 0xF5, 0x54, 0x7F,
0xBF, 0x32, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, 0x20,
0xE0, 0x06, 0x4A, 0xF5, 0x00, 0x00, 0xC0, 0x15, 0x02, 0x00,
0xD7, 0x05, 0xD3, 0x00, 0xA0, 0xF0, 0x7A, 0x00, 0x89, 0x25,

0x41, 0x00, 0x78, 0x0A, 0x02, 0x00, 0xF7, 0xBD, 0x00, 0x00,
0x00, 0x00, 0x00, 0x40, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00,
0x00, 0x00, 0xCB, 0xCD, 0x02, 0x00, 0x00, 0x00, 0x00, 0x40,
0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x24, 0x2C, 0x7F,

0x43, 0x02, 0x18, 0x02, 0x49, 0x02, 0x19, 0x02, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x43, 0x01,
0x24, 0x02, 0x0B, 0x02, 0x68, 0xEF, 0x7A, 0x00, 0x46
); uwait(0xAA);

usend(10,
0xAA, 0x55, 0x00, 0x00, 0x00,
0x25, 0x00, 0x00, 0x01, 0x25
); uwait(0xAA);

usend(129,
0x3D, 0x16, 0x3A, 0x3D, 0x52, 0x01, 0x3D, 0x16, 0x3A, 0x3D,
0x66, 0x46, 0x08, 0x3D, 0x10, 0x1F, 0x52, 0x66, 0x2D, 0x30,
0x05, 0x2B, 0x40, 0x29, 0x3D, 0x21, 0x34, 0x29, 0x16, 0x05,
0x28, 0x21, 0x2D, 0x16, 0x3A, 0x4E, 0x00, 0x15, 0x02, 0x00,
0xD7, 0x05, 0xD3, 0x00, 0xA0, 0xF0, 0x7A, 0x00, 0x89, 0x25,

0x41, 0x00, 0x78, 0x0A, 0x02, 0x00, 0xF7, 0xBD, 0x00, 0x00,
0x00, 0x00, 0x00, 0x40, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00,
0x00, 0x00, 0xCB, 0xCD, 0x02, 0x00, 0x00, 0x00, 0x00, 0x40,
0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x24, 0x2C, 0x7F,

0x43, 0x02, 0x18, 0x02, 0x49, 0x02, 0x19, 0x02, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x43, 0x01,
0x24, 0x02, 0x0B, 0x02, 0x68, 0xEF, 0x7A, 0x00, 0xC6
); uwait(0xAA);

usend(10,
0xAA, 0x55, 0xFF, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFE
); uwait(0xAA);

	fprintf(stderr, "\nok\n");
	return 0;
}

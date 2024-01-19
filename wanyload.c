#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

/*
 *
 * Web-@nywhere uploader
 * Takes 10-129* format binary files (with checksums) and uploads them
 * to a connected Web-@nywhere watch. Pass multiple files for multiple
 * entries, such as for the Browser.
 *
 * Use -p /path/to/port as the first argument to use something other than
 * /dev/ttyUSB0.
 *
 * Compiles with gcc -O3 -o wanyload wanyload.c or some such.
 * Only needs POSIX and termios.h.
 *
 * Copyright (C) 2024 Cameron Kaiser. All rights reserved.
 * BSD license.
 * https://oldvcr.blogspot.com/
 * https://github.com/classilla/webany
 *
 */

int porta;
struct termios ttya, ttya_saved;
struct sigaction hothotsig;
fd_set master;

void
usend(size_t j,unsigned char *w)
{
	size_t i=0;
	int rv;
	fd_set wfd;

	fprintf(stderr, "-- sending %d bytes --\n", j);
	for(;;) {
		wfd=master;
		rv = select(porta+1, NULL, &wfd, NULL, NULL);
		if (rv < 0) {
			/* fatal */
			perror("select(send)");
			exit(255);
		}
		if (rv > 0) break;
	}
	for(i=0;i<j;i++) {
		fprintf(stderr, " %02x ", w[i]);
		write(porta, &(w[i]), 1);
	}
	fprintf(stderr, "\n");
}

int
uwait(char c)
{
	fd_set rfd;
	struct timeval tv;
	char d;
	int rv;

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	fprintf(stderr, "-- waiting for %02x:", c);
	for(;;) {
		for(;;) {
			rfd=master;
			rv = select(porta+1, &rfd, NULL, NULL, &tv);
			if (rv < 0) {
				/* fatal */
				perror("select(send)");
				exit(255);
			}
			if (rv == 0) {
				if (tv.tv_sec > 0 || tv.tv_usec > 0) continue;

				/* try again? */
				perror("timeout on select");
				return 0;
			}
			if (rv > 0) break;
		}
		read(porta, &d, 1);
		fprintf(stderr, " %02x ", d);
		if (d == c) {
			fprintf(stderr, "--\n");
			return 1;
		}
	}

	/* fatal */
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

/* attention signal, end type, 0 length, 0 packets, checksum 254 */
char eot[10] = { 0xAA, 0x55, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE };

int
main(int argc, char **argv)
{
	FILE *f;
	int i, p, argp;
	unsigned char c[129];

	argp = 1;
	if (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'p') {
		fprintf(stderr, "(using %s as port)\n", argv[2]);
		porta = open(argv[2], O_RDWR | O_NOCTTY); 
		argp += 2;
	} else
		porta = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
	if (argp == argc) {
		fprintf(stderr, "usage: %s [-p port] files ...\n", argv[0]);
		close(porta);
		return 1;
	}
	if (tcgetattr(porta, &ttya_saved)) {
		perror("tcgetattr");
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
	/* 9600bps 8E2 no flow control */
	ttya.c_cflag = (CS8 | CREAD | CLOCAL | PARENB | CSTOPB);
	ttya.c_cc[VTIME] = 5;
	ttya.c_cc[VMIN] = 1;
	cfsetospeed(&ttya, B9600);
	cfsetispeed(&ttya, B9600);
	tcsetattr(porta, TCSANOW, &ttya);

	FD_ZERO(&master);
	FD_SET(porta, &master);

	setvbuf(stdout, NULL, _IONBF, 0); /* select(STDOUT); $|++; */

	while (argp < argc) {
		fprintf(stderr, "\n(next file: %s)\n", argv[argp]);
		if(!(f = fopen(argv[argp], "r"))) {
			perror("fopen");
			return 255;
		}

		/* header */
		if (fread(&c, 1, 10, f) != 10) {
			fprintf(stderr, "unable to read header\n");
			return 254;
		}
		if (feof(f)) {
			fprintf(stderr, "file has only a header\n");
			return 254;
		}

		/* wait for watch to enter download mode */
		for(i=0;i<10;i++) {
			usend(10, (unsigned char *)&c);
			if (uwait(0xaa)) break;
		}
		if (i==10) {
			fprintf(stderr, "watch is not responding\n");
			return 253;
		}

		/* send data */
		p = 0;
		while (!feof(f)) {
			if ((i = fread(&c, 1, 129, f)) < 1) /* assume end */
				break;
			fprintf(stderr, "(packet %d)\n", ++p);
			if (i != 129) {
				fprintf(stderr, "incomplete data packet, got %d bytes\n", i);
				return 254;
			}
			usend(129, (unsigned char *)&c);
			if (!uwait(0xaa)) {
				fprintf(stderr, "transmission failure\n");
				return 253;
			}
		}
		fclose(f);
		argp++;
	}

	/* send EOT */
	usend(10, (unsigned char *)&eot);
	if (!uwait(0xaa)) {
		fprintf(stderr, "transmission failure\n");
		return 253;
	}
	fprintf(stderr, "\nsuccessful!\n");
	return 0;
}

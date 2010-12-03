/*
 * sonderbar - read barcode data from an input device
 *
 * by Stefan Tomanek <stefan.tomanek@wertarbyte.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>

#define MAX_BUFFER 1024
#define FINAL_KEY KEY_ENTER

static char buffer[MAX_BUFFER];
static int tainted;

void reset_buffer(void) {
	tainted = 0;
	buffer[0] = '\0';
}

void append_char(char c) {
	if (1 + strlen(buffer) <= (MAX_BUFFER-1)) {
		strncat(buffer, &c, 1);
	} else {
		fprintf(stderr, "Buffer overflow!\n");
		tainted = 1;
	}
}

int read_event(int fd) {
	// printf("Buffer: <%s>\n", buffer);
	struct input_event ev;
	int n = read( fd, &ev, sizeof(ev) );
	if ( n != sizeof(ev) ) {
		fprintf(stderr, "Error reading event from device!\n");
		return 0;
	}

	if (ev.type == EV_KEY && ev.value == 1) {
		/*
		 * This is an ugly hack, but it works for now
		 */
		if (ev.code == FINAL_KEY) {
			if (!tainted) {
				printf("%s\n", buffer);
			}
			reset_buffer();
		} else if (tainted) {
			// do nothing and consider the barcode spoiled
		} else if (ev.code >= KEY_1 && ev.code <= KEY_0) {
			char new = '0' + (ev.code+1-KEY_1)%10;
			append_char(new);
		} else {
			switch(ev.code) {
				case KEY_SPACE:
					append_char(' ');
					break;
				default:
					/*
			       		* Unknown key?
					* we now consider the input tainted and will not process it further.
					*/
					fprintf(stderr, "Unknown keycode: %d\n", ev.code);
					tainted = 1;
			}
		}
	}
	return 1;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "No input device specified\n");
		return EXIT_FAILURE;
	} 

	reset_buffer();

	const char *device = argv[1];
	
	int dev_fd = open(device, O_RDONLY);

	if (!dev_fd) {
		fprintf(stderr, "Unable to open device\n");
		return EXIT_FAILURE;
	}
	/*
	 * get exclusive access to the device
	 */
	if (ioctl(dev_fd, EVIOCGRAB, 1) == -1) {
		fprintf(stderr, "Unable to grab device exclusively\n");
		return EXIT_FAILURE;
	}

	while (read_event(dev_fd));
	close(dev_fd);

	return EXIT_SUCCESS;
}

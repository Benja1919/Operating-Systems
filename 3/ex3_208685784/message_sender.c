#include "message_slot.h"

#include <fcntl.h>      
#include <unistd.h>     
#include <sys/ioctl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*********************************Some Explainations about the code************************************/
/*
                                        id 208685784
                    Explenations are descried in "message_slot.c" file
                    Additional Defines are given in "message_slot.h" file
								                                                                      */
/******************************************************************************************************/

int main(int argc, char** argv)
{
    /*success - successful sending.
     if errors occurs - handling like described in the assingment file and exiting with the correct status.
         printing to the screen uses "strerror as requested in the assingment
    */
	int fd, value;
	char *deliver = argv[3];

	if (argc != 4)
    {
		fprintf(stderr, "The number of arguments is incorrect: %s\n", strerror(EINVAL));
		exit(EXIT_STATUS);
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0)
    {
		fprintf(stderr, "Error opening the device's file: %s\n Error: %s\n", argv[1], strerror(errno));
		exit(EXIT_STATUS);
	}

	value = ioctl(fd, MSG_SLOT_CHANNEL, atoi(argv[2]));
	if (value != 0)
    {
		fprintf(stderr, "ioctl failed Error: %s\n", strerror(errno));
		exit(EXIT_STATUS);
	}
	value = write(fd, deliver, strlen(deliver));
	if (value != strlen(deliver))
    {
		fprintf(stderr, "Writing failed: %s\n", strerror(errno));
		exit(EXIT_STATUS);
	}
	close(fd);
	exit(0);
}

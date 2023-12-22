#include "message_slot.h"    

#include <fcntl.h>   
#include <errno.h>
#include <unistd.h>     
#include <sys/ioctl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************************Some Explainations about the code************************************/
/*
                                         id 208685784
                        Explenations are descried in "message_slot.c" file
                        Additional Defines are given in "message_slot.h" file
								                                                                      */
/******************************************************************************************************/

int main(int argc, char** argv)
{
    /*success - successful reading.
     if errors occurs - handling like described in the assingment file and exiting with the correct status.
         printing to the screen uses "strerror as requested in the assingment
    */
    char buffer [BUFFER_SIZE];
    int fd;
    int value;

    if (argc != 3)
    {
        fprintf(stderr, "The number of arguments is incorrect: %s\n", strerror(errno));
        exit(EXIT_STATUS_ERR);
    }

    fd = open( argv[1], O_RDONLY );
    if( fd < 0 ) 
    {
        fprintf(stderr, "Error opening the device's file: %s\n", strerror(errno));
        exit(EXIT_STATUS);
    }

    value = ioctl( fd, MSG_SLOT_CHANNEL, atoi(argv[2]));
    if (value !=0)
    {
        fprintf(stderr, "ioctl failed Error: %s\n", strerror(errno));
        exit(EXIT_STATUS);
    }
    value = read( fd, buffer,BUFFER_SIZE);
    if (value <= 0)
    {
        fprintf(stderr, "Reading failed: %s\n", strerror(errno));
        exit(EXIT_STATUS);
    }

    if (write(STDOUT_FILENO, buffer, value) != value)
    {
        fprintf(stderr, "printing failed Error: %s\n", strerror(errno));
		exit(EXIT_STATUS);
	}
    close(fd);
    return 0;
}

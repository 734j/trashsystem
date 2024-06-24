#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define USAGE "to be decided"

struct trashsys_log_info {
	uint64_t ts_log_id;
	char ts_log_filename[255];
	size_t ts_log_filesize;
	time_t ts_log_trashtime;
	char ts_log_originalpath[4096];
	bool ts_log_tmp;
};

int main (int argc, char *argv[]) {

	if (argc == 1) {
        fprintf(stderr, "%s: please specify a filename\n%s\n", argv[0], USAGE);
        return EXIT_FAILURE;
    }

	bool y_used = false;
	bool n_used = false;
	bool v_used = false;
	bool f_used = false;
	bool a_used = false;
	bool t_used = false;
	bool l_used = false;
	bool L_used = false;
	bool c_used = false;
	bool C_used = false;
	bool R_used = false;
	
    int opt;
    int returnval;
    char *optarg_copy = NULL; // We will copy optarg to this
    while ((opt = getopt(argc, argv, "ynvfatlLcCR:")) != -1) {
        switch (opt) {
        case 'y':

			y_used = true;

        break;
        case 'n':

            n_used = true;

        break;
        case 'v':

            v_used = true;

        break;
        case 'f':

            f_used = true;

        break;
		case 'a':
			
			a_used = true;

		break;
	    case 't':

			t_used = true;
			
		break;
		case 'l':
			
			l_used = true;
			
		break;
		case 'L':
			
			L_used = true;
			
		break;
		case 'c':
			
			c_used = true;
			
		break;
		case 'C':
			
			C_used = true;
			
		break;
		case 'R':
			
			R_used = true;
			
		break;
        }
    }
	

    if (n_used == true && y_used == true) { // If both YES and NO are used print usage and exit
        fprintf(stderr, "%s", USAGE);
        return EXIT_FAILURE;
    }
	return 0;
}

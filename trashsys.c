#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <stdarg.h>

#define USAGE "to be decided"
#define MODE_NORMAL -1
#define MODE_YES 0
#define MODE_NO 1
#define ENVVAR_HOME "HOME"

bool v_cvm_fprintf = false;

struct trashsys_log_info {
	uint64_t ts_log_id;
	char ts_log_filename[255];
	size_t ts_log_filesize;
	time_t ts_log_trashtime;
	char ts_log_originalpath[4096];
	bool ts_log_tmp;
};

struct initial_path_info { // Initial useful strings to create before we start checking if certain directories or files exist.
	char *ts_path_user_home;
	char *ts_path_trashsys;
	char *ts_path_log;
	char *ts_path_trashed;
};

int cvm_fprintf(bool ONOROFF, FILE *stream, const char *format, ...) {
    // a sort of debug fprintf
    if (ONOROFF == false) {
        return 0; // Return 0 to indicate no characters were printed
    }

    // If condition is true, proceed with fprintf
    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);

    return result; // Return the result from fprintf
}

char *concat_str(char *final, ssize_t rem_size, const char *from) {
	// IF you use this function PLEASE know this:
	// rem_size is the amount of characters left in final
	// rem_size should NOT include \0
	// So if you were to have 5 remaining characters then 5 is what you put as the argument
	// from is calculated and then we add +1 to account for the \0 character. 
	if (final == NULL || from == NULL) {
		return NULL;
	}
	
	ssize_t from_len = strlen(from);

	if (from_len+1 > rem_size) {
		cvm_fprintf(v_cvm_fprintf, stderr, "IF: from_len: %li\nIF: rem_size: %li\n", from_len+1, rem_size);
		return NULL;
	}
	cvm_fprintf(v_cvm_fprintf, stderr, "Ffrom_len: %li\nRrem_size: %li\n", from_len+1, rem_size);
	strcat(final, from);
	return final;
}

void free_ipi(struct initial_path_info *ipi) { // Free all info in initial_path_info created from fill_ipi
	free(ipi->ts_path_user_home);
	free(ipi->ts_path_trashsys);
	free(ipi->ts_path_log);
	free(ipi->ts_path_trashed);
	free(ipi);
}

struct initial_path_info *fill_ipi() { // Function for filling out initial_path_info so it can be used later

	struct initial_path_info *ipi = malloc(sizeof(struct initial_path_info));
	ipi->ts_path_user_home = malloc(sizeof(char) * 4096);
	ipi->ts_path_trashsys = malloc(sizeof(char) * 4096);
	ipi->ts_path_log = malloc(sizeof(char) * 4096);
	ipi->ts_path_trashed = malloc(sizeof(char) * 4096);
	
	ipi->ts_path_user_home[0] = '\0';
	ipi->ts_path_trashsys[0] = '\0';
	ipi->ts_path_log[0] = '\0';
	ipi->ts_path_trashed[0] = '\0';
	char *homepath;
	homepath = getenv(ENVVAR_HOME);

	if (homepath == NULL) {
		fprintf(stderr, "fill_ipi(): getenv failed");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}
	// top level =  "/.trashsys"
	// log = "/log"
	// trashed = "/trashed"
	//char *concat_str(char *final, ssize_t rem_size, const char *from);
   	//concat_str(ipi->ts_path_user_home, PATH_MAX, homepath);
	strcat(ipi->ts_path_user_home, homepath); // Fill home path
	strcat(ipi->ts_path_trashsys, homepath); // fill toplevel ts path
	strcat(ipi->ts_path_trashsys, "/.trashsys"); // 2nd step to fill toplevel ts path

	strcat(ipi->ts_path_log, ipi->ts_path_trashsys); // fill log path
	strcat(ipi->ts_path_log, "/log"); // 2nd step fill log path

	strcat(ipi->ts_path_trashed, ipi->ts_path_trashsys); // fill trashed path
	strcat(ipi->ts_path_trashed, "/trashed"); // 2nd step fill trashed path
	return ipi;
}

int check_create_ts_dirs(struct initial_path_info *ipi) { // 1. Check if trashsys toplevel exists 2. Check if log exists 3. Check if trashed exists 

	int mkd;
  	mkd = mkdir(ipi->ts_path_trashsys, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { fprintf(stderr, ".trashsys exists\n"); } else { return -1; }
	}
	
	mkd = mkdir(ipi->ts_path_log, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { fprintf(stderr, "log exists\n"); } else { return -1; }
	}
	
	mkd = mkdir(ipi->ts_path_trashed, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { fprintf(stderr, "trashed exists\n"); } else { return -1; }
	}
    
	return 0;
}
 
uint64_t find_highest_id (struct initial_path_info *ipi) { // Find highest id and then return it, because we will create the new log entry as highestID + 1

	// We need to check whether a file is a directory or just a file. 
	struct dirent *ddd;
	DIR *dir = opendir(ipi->ts_path_log);
	if (dir == NULL) { return -1; }
	while ((ddd = readdir(dir)) != NULL) {
		
		// MUST BE TESTED MORE!!!
		int path_max_int = PATH_MAX; // Temporary variable, good for testing
		char stat_fullpath[path_max_int];
		stat_fullpath[0] = '\0';

		ssize_t sf_sz = path_max_int;
		if(concat_str(stat_fullpath, sf_sz, ipi->ts_path_log) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			exit(EXIT_FAILURE);
		}

		sf_sz = sf_sz - strlen(stat_fullpath);
   		//fprintf(stdout, "%ld\n", sf_sz);
		if(concat_str(stat_fullpath, sf_sz, "/") == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			exit(EXIT_FAILURE);
		}

		sf_sz = sf_sz - strlen(stat_fullpath);
		//fprintf(stdout, "%ld\n", sf_sz);		
		if(concat_str(stat_fullpath, sf_sz, ddd->d_name) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			exit(EXIT_FAILURE);
		}
		
		struct stat d_or_f;
		stat(stat_fullpath, &d_or_f);
		
		if(S_ISREG(d_or_f.st_mode)) { // check if given file is actually a file
			fprintf(stdout, "%s\n", ddd->d_name);
		}
		
	}
	closedir(dir);
	return 0;
}

int tli_fill_info (struct trashsys_log_info *tli, char* filename, bool log_tmp, struct initial_path_info *ipi) {
	/*	
struct trashsys_log_info {
	uint64_t ts_log_id;
	char ts_log_filename[255]; X
	size_t ts_log_filesize; X
	time_t ts_log_trashtime; X
	char ts_log_originalpath[4096]; X
	bool ts_log_tmp; X
};
	*/
	char *rp;
	rp = realpath(filename, NULL); // get full entire path of the file
	tli->ts_log_originalpath[0] = '\0';
	tli->ts_log_filename[0] = '\0';
	strcat(tli->ts_log_originalpath, rp);
	free(rp);
	strcat(tli->ts_log_filename, basename(filename)); // record filename and basename it

	tli->ts_log_tmp = log_tmp; // tmp or not?
	tli->ts_log_trashtime = time(NULL); // record current time

	FILE *file = fopen(filename, "r"); // We get the filesize in bytes
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fclose(file);
	tli->ts_log_filesize = (size_t)filesize;

	find_highest_id(ipi);
	
	cvm_fprintf(v_cvm_fprintf, stdout, "fullpath: %s\nfilename: %s\ntime: %ld\ntmp: %d\nsize: %ld\n", tli->ts_log_originalpath, tli->ts_log_filename, tli->ts_log_trashtime, tli->ts_log_tmp, tli->ts_log_filesize);

	return 0;
}

/*
int trash_file(struct trashsys_log_info tli, struct initial_path_info *ipi) {

	// name
	// full path
	// filesize
	// time of deletion
	// other info?
	return 0;
}
*/
int choice(int mode) {

    char choice;
    char modechoice;

    do {

    if (mode == MODE_NORMAL) { fputs("[Y / N] ? ", stdout); }
    if (mode == MODE_YES) { fputs("[Y / n] ? ", stdout); }
    if (mode == MODE_NO) { fputs("[y / N] ? ", stdout); }

    choice = getchar();
    if (choice == '\n' && mode == MODE_YES) { modechoice = 'Y'; choice = modechoice; goto modeskip;}
    if (choice == '\n' && mode == MODE_NO) { modechoice = 'N'; choice = modechoice; goto modeskip;}
    if (choice == '\n' && mode == MODE_NORMAL) { continue; }

    while ('\n' != getchar());

    } while ( (choice != 'Y') && (choice != 'y') && (choice != 'N') && (choice != 'n') );

    modeskip:

    if ( (choice == 'Y') || (choice == 'y') ) 
    {
        return 0;
    }

    if ((choice == 'N') || (choice == 'n') )
    {
        return 1;
    }

    return EXIT_FAILURE;
}

int main (int argc, char *argv[]) {

	if (argc == 1) {
        fprintf(stderr, "%s: please specify a filename\n%s\n", argv[0], USAGE);
        return EXIT_FAILURE;
    }

	struct initial_path_info *ipi_m;
	int cctd;
	ipi_m = fill_ipi();
	cvm_fprintf(v_cvm_fprintf, stdout, "%s\n%s\n%s\n%s\n", ipi_m->ts_path_user_home, ipi_m->ts_path_trashsys, ipi_m->ts_path_log, ipi_m->ts_path_trashed);
	cctd = check_create_ts_dirs(ipi_m);
	if(cctd == -1) {
		fprintf(stderr, "check_create_ts_dirs(): Cannot create directories\n");
		free_ipi(ipi_m);
		return EXIT_FAILURE;
	}

	struct trashsys_log_info tli_m;
	tli_fill_info(&tli_m , "../myfile.img", false, ipi_m);
	free_ipi(ipi_m);
	
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
    //int returnval;
    //char *optarg_copy = NULL; // We will copy optarg to this
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

	if(false) { // This is just so the compiler wont complain
		fprintf(stdout, "%d%d%d%d%d%d%d%d%d%d%d", R_used, C_used, c_used, L_used, l_used, t_used, a_used, f_used, v_used, n_used, y_used);
	}
	
    if (n_used == true && y_used == true) { // If both YES and NO are used print usage and exit
        fprintf(stderr, "%s", USAGE);
        return EXIT_FAILURE;
    }
	return 0;
}

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
#define MODE_FORCE 2
#define ENVVAR_HOME "HOME"
#define NOFILE 3

bool v_cvm_fprintf = false;
int choice_mode = MODE_NORMAL;

struct trashsys_log_info {
	uint64_t ts_log_id;
	char ts_log_filename[FILENAME_MAX]; // doublecheck this!
	size_t ts_log_filesize;
	time_t ts_log_trashtime;
	char ts_log_originalpath[PATH_MAX];
	bool ts_log_tmp;
};

struct dynamic_paths {
	char old_trashfile_path[PATH_MAX];
	char new_trashfile_path[PATH_MAX];
	char new_logfile_path_incl_name[PATH_MAX];
};

struct initial_path_info { // Initial useful strings to create before we do anything. Super useful when programming.
	char ts_path_user_home[PATH_MAX];
	char ts_path_trashsys[PATH_MAX];
	char ts_path_log[PATH_MAX];
	char ts_path_trashed[PATH_MAX];
	char ts_path_user_home_withslash[PATH_MAX];
	char ts_path_trashsys_withslash[PATH_MAX];
	char ts_path_log_withslash[PATH_MAX];
	char ts_path_trashed_withslash[PATH_MAX];
};

int handle_ynf(bool y_used, bool n_used, bool f_used) { // Will handle cases for y, n and f. Exits if any of these are used together.

	int choice_mode_ynf = MODE_NORMAL;

	if (n_used == true && y_used == true) { // If both YES and NO are used print usage and exit
        fprintf(stderr, "%s", USAGE);
        exit(EXIT_FAILURE);
    } else if (n_used == true && f_used == true) { fprintf(stderr, "%s", USAGE); exit(EXIT_FAILURE); }
	else if (y_used == true && f_used == true) { fprintf(stderr, "%s", USAGE); exit(EXIT_FAILURE); }

	if (n_used == true) { choice_mode_ynf = MODE_NO; }
   	if (y_used == true) { choice_mode_ynf = MODE_YES; }
	if (f_used == true) { choice_mode_ynf = MODE_FORCE; }
	
	
	return choice_mode_ynf;
}

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
	// rem_size should NOT include \0 in the size
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
	free(ipi);
	cvm_fprintf(v_cvm_fprintf, stderr, "initial_path_info free'd\n");
}

struct initial_path_info *fill_ipi() { // Function for filling out initial_path_info so it can be used later

	#define MY_PATH_MAX PATH_MAX
	char *ts_toplevel = "/.trashsys";
	char *ts_log = "/log";	
   	char *ts_trashed = "/trashed";
   	char *ts_toplevel_withslash = "/.trashsys/";
	char *ts_log_withslash = "/log/";	
   	char *ts_trashed_withslash = "/trashed/";
	char *homepath;
	struct initial_path_info *ipi = malloc(sizeof(struct initial_path_info)); // malloc memory to struct
	
	ipi->ts_path_user_home[0] = '\0'; // Add null character to all of them because we'll be using concat_str (basically strcat) later
	ipi->ts_path_trashsys[0] = '\0';
	ipi->ts_path_log[0] = '\0';
	ipi->ts_path_trashed[0] = '\0';
   	ipi->ts_path_user_home_withslash[0] = '\0'; 
	ipi->ts_path_trashsys_withslash[0] = '\0';
	ipi->ts_path_log_withslash[0] = '\0';
	ipi->ts_path_trashed_withslash[0] = '\0';
	
	homepath = getenv(ENVVAR_HOME); // Get the home path of the current user

	if (homepath == NULL) {
		fprintf(stderr, "fill_ipi(): getenv failed");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}
   	ssize_t remaining_size = MY_PATH_MAX;
	ssize_t remaining_size_2 = MY_PATH_MAX;
	// /home/john
	// /home/john/
   	if(concat_str(ipi->ts_path_user_home, MY_PATH_MAX, homepath) == NULL
	   || concat_str(ipi->ts_path_user_home_withslash, MY_PATH_MAX, homepath) == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}
	remaining_size = remaining_size - strlen("/");
	if(concat_str(ipi->ts_path_user_home_withslash, remaining_size, "/") == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}

	// /home/john/.trashsys
	// /home/john/.trashsys/
	if(concat_str(ipi->ts_path_trashsys, MY_PATH_MAX, homepath) == NULL
	   || concat_str(ipi->ts_path_trashsys_withslash, MY_PATH_MAX, homepath) == NULL) { 
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}
	remaining_size = MY_PATH_MAX;
	remaining_size = remaining_size - strlen(ts_toplevel);
	remaining_size_2 = remaining_size_2 - strlen(ts_toplevel_withslash);
	if(concat_str(ipi->ts_path_trashsys, remaining_size, ts_toplevel) == NULL 
	   || concat_str(ipi->ts_path_trashsys_withslash, remaining_size_2, ts_toplevel_withslash) == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}

	// /home/john/.trashsys/log
	// /home/john/.trashsys/log/
	if(concat_str(ipi->ts_path_log, MY_PATH_MAX, ipi->ts_path_trashsys) == NULL
	   || concat_str(ipi->ts_path_log_withslash, MY_PATH_MAX, ipi->ts_path_trashsys) == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}
	remaining_size = MY_PATH_MAX;
	remaining_size_2 = MY_PATH_MAX;
	remaining_size = remaining_size - strlen(ts_log);
	remaining_size_2 = remaining_size_2 - strlen(ts_log_withslash);
	if(concat_str(ipi->ts_path_log, remaining_size, ts_log) == NULL
	   || concat_str(ipi->ts_path_log_withslash, remaining_size_2, ts_log_withslash) == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}

	// /home/john/.trashsys/trashed
	// /home/john/.trashsys/trashed/
	if(concat_str(ipi->ts_path_trashed, MY_PATH_MAX, ipi->ts_path_trashsys) == NULL
	   || concat_str(ipi->ts_path_trashed_withslash, MY_PATH_MAX, ipi->ts_path_trashsys) == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}
	remaining_size = MY_PATH_MAX;
	remaining_size_2 = MY_PATH_MAX;
	remaining_size = remaining_size - strlen(ts_trashed);
	remaining_size_2 = remaining_size_2 - strlen(ts_trashed_withslash);
	if(concat_str(ipi->ts_path_trashed, remaining_size, ts_trashed) == NULL
	   || concat_str(ipi->ts_path_trashed_withslash, remaining_size_2, ts_trashed_withslash) == NULL) {
		fprintf(stderr, "fill_ipi: path is too long\n");
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}

	cvm_fprintf(v_cvm_fprintf, stdout, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n"
				, ipi->ts_path_user_home
				, ipi->ts_path_trashsys
				, ipi->ts_path_log
				, ipi->ts_path_trashed
				, ipi->ts_path_user_home_withslash
				, ipi->ts_path_trashsys_withslash
				, ipi->ts_path_log_withslash
				, ipi->ts_path_trashed_withslash
				);

	return ipi;
}

int check_create_ts_dirs(struct initial_path_info *ipi) { // 1. Check if trashsys toplevel exists 2. Check if log exists 3. Check if trashed exists 

	int mkd;
  	mkd = mkdir(ipi->ts_path_trashsys, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { cvm_fprintf(v_cvm_fprintf, stderr, ".trashsys exists\n"); } else { return -1; }
	} else { cvm_fprintf(v_cvm_fprintf, stderr, "%s was created\n", ipi->ts_path_trashsys); }
	
	mkd = mkdir(ipi->ts_path_log, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { cvm_fprintf(v_cvm_fprintf, stderr, "log exists\n"); } else { return -1; }
	} else { cvm_fprintf(v_cvm_fprintf, stderr, "%s was created\n", ipi->ts_path_log); }
	
	mkd = mkdir(ipi->ts_path_trashed, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { cvm_fprintf(v_cvm_fprintf, stderr, "trashed exists\n"); } else { return -1; }
	} else { cvm_fprintf(v_cvm_fprintf, stderr, "%s was created\n", ipi->ts_path_trashed); }
    
	return 0;
}
 
uint64_t find_highest_id (struct initial_path_info *ipi) { // Find highest id and then return it, because we will create the new log entry as highestID + 1

	// We need to check whether a file is a directory or just a file. 
	uint64_t id = 1;
	struct dirent *ddd;
	DIR *dir = opendir(ipi->ts_path_log);
	if (dir == NULL) {
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	} // Return here or exit() ??
	while ((ddd = readdir(dir)) != NULL) {
		
		// MUST BE TESTED MORE!!!
		char stat_fullpath[PATH_MAX];
		stat_fullpath[0] = '\0';

		ssize_t remaining_size = PATH_MAX;
		if(concat_str(stat_fullpath, PATH_MAX, ipi->ts_path_log_withslash) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			free_ipi(ipi);
			exit(EXIT_FAILURE);
		}

		remaining_size = remaining_size - strlen(stat_fullpath);
		if(concat_str(stat_fullpath, remaining_size, ddd->d_name) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			free_ipi(ipi);
			exit(EXIT_FAILURE);
		}
		
		struct stat d_or_f;
		stat(stat_fullpath, &d_or_f);
		
		if(S_ISREG(d_or_f.st_mode)) { // check if given file is actually a file
			cvm_fprintf(v_cvm_fprintf, stdout, "is regular file: %s\nstat_fullpath: %s\n", ddd->d_name, stat_fullpath);
			char *endptr;
			uint64_t strtoull_ID = strtoull(ddd->d_name, &endptr, 10);
			if(ddd->d_name == endptr) {
				cvm_fprintf(v_cvm_fprintf, stdout, "d_name == endptr | d_name: %p | endptr: %p | d_name string: %s\n", ddd->d_name, endptr, ddd->d_name);
				continue;
			}
			if(*endptr != ':') {
				cvm_fprintf(v_cvm_fprintf, stdout, "':' not found for file: %s\n", ddd->d_name);
				continue;
			}
			if(strtoull_ID > id) { // If id is bigger then update it
				id = strtoull_ID;
				cvm_fprintf(v_cvm_fprintf, stdout, "found higher ID: %d\n", id);
			}
		}
		
	}
	closedir(dir);
	return id;
}

int tli_fill_info (struct trashsys_log_info *tli, char* filename, bool log_tmp, struct initial_path_info *ipi) { 
	// This function will be the main function that gathers and fills out info that will be in the log file for a file a user wants to trash
	char *rp;
	time_t curtime;
	rp = realpath(filename, NULL); // get full entire path of the file
	if (rp == NULL) {
		return NOFILE;
	}
	tli->ts_log_originalpath[0] = '\0';
	tli->ts_log_filename[0] = '\0';

	if(concat_str(tli->ts_log_originalpath, PATH_MAX, rp) == NULL) {
		free_ipi(ipi);
		free(rp);
		exit(EXIT_FAILURE);
	}
	free(rp);
	if(concat_str(tli->ts_log_filename, FILENAME_MAX, basename(filename)) == NULL) {
		free_ipi(ipi);
		exit(EXIT_FAILURE);
	}

	tli->ts_log_tmp = log_tmp; // tmp or not?
	curtime = time(NULL);
	if (curtime == -1) { free_ipi(ipi); exit(EXIT_FAILURE); }
	tli->ts_log_trashtime = curtime;
	FILE *file = fopen(filename, "r"); // We get the filesize in bytes
	if(file == NULL) { return NOFILE; }
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fclose(file);
	tli->ts_log_filesize = (size_t)filesize;

	uint64_t ID = find_highest_id(ipi);
	tli->ts_log_id = ID + 1; // +1 because if we are making a new file we need to give it one above highest ID.
	
	return 0;
}

/*
int prepare_log_paths(struct initial_path_info *ipi, struct trashsys_log_info *tli) {

}
*/
int fill_dynamic_paths (struct initial_path_info *ipi, struct trashsys_log_info *tli, struct dynamic_paths *dp) {
	/*
	struct dynamic_paths {
		char old_trashfile_path[PATH_MAX];
		char new_trashfile_path[PATH_MAX];
		char new_logfile_path_incl_name[PATH_MAX];
	};
	*/
	ssize_t remaining_size = PATH_MAX;
	dp->old_trashfile_path[0] = '\0';
	dp->new_trashfile_path[0] = '\0';
	dp->new_logfile_path_incl_name[0] = '\0';
	// /path/to/my/file.txt
	if(concat_str(dp->old_trashfile_path, PATH_MAX, tli->ts_log_originalpath) == NULL) { return -1; }

	// /home/john/.trashsys/trashed/file.txt
	if(concat_str(dp->new_trashfile_path, PATH_MAX, ipi->ts_path_trashed_withslash) == NULL) { return -1; }
	remaining_size = remaining_size - strlen(tli->ts_log_filename);
	if(concat_str(dp->new_trashfile_path, remaining_size, tli->ts_log_filename) == NULL) { return -1; }

	// /home/john/.trashsys/log/35:file.txt
	remaining_size = PATH_MAX;
	char idstr[23];
	snprintf(idstr, 23, "%ld:", tli->ts_log_id);
	concat_str(dp->new_logfile_path_incl_name, PATH_MAX, ipi->ts_path_log_withslash);
	remaining_size = remaining_size - strlen(idstr);
   	concat_str(dp->new_logfile_path_incl_name, remaining_size, idstr);
	remaining_size = PATH_MAX;
	remaining_size = remaining_size - strlen(tli->ts_log_filename);
   	concat_str(dp->new_logfile_path_incl_name, remaining_size, tli->ts_log_filename);

	cvm_fprintf(v_cvm_fprintf, stdout, "%s\n%s\n%s\n"
			    , dp->old_trashfile_path
				, dp->new_trashfile_path
				, dp->new_logfile_path_incl_name
				);
	return 0;
}

int write_log_file(struct dynamic_paths *dp, struct trashsys_log_info *tli, bool t_used_aka_tmp) {

	char *tmp_path = "/tmp/";

	if (t_used_aka_tmp == true) {
		fprintf(stdout, "%s", tmp_path);
	}

	fprintf(stdout, "plcf: %s\n", dp->new_logfile_path_incl_name);
	FILE *file = fopen(dp->new_logfile_path_incl_name, "w");
	if(file == NULL) {
		printf("%s\n", strerror(errno));
		return -1;
	}
	fprintf(file, ";\n%ld\n%s\n%ld\n%ld\n%s\n%d\n;\n", tli->ts_log_id, tli->ts_log_filename, tli->ts_log_filesize, tli->ts_log_trashtime, tli->ts_log_originalpath, tli->ts_log_tmp);
	
	fclose(file);

	return 0;
}

int choice(int mode) {

    char choice;
    char modechoice;

    do {

    if (mode == MODE_NORMAL) { fputs("[Y / N] ? ", stdout); }
    if (mode == MODE_YES) { fputs("[Y / n] ? ", stdout); }
    if (mode == MODE_NO) { fputs("[y / N] ? ", stdout); }
	if (mode == MODE_FORCE) { return 0; }
	
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

			y_used = true; // YES on enter

        break;
        case 'n':

            n_used = true; // NO on enter

        break;
        case 'v':

            v_used = true; // Verbose debug mode

        break;
        case 'f':

            f_used = true; // choice will not ask, it will just say yes by default thus basically "forcing" it

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
	if (v_used == true) { v_cvm_fprintf = true; } // Verbose mode

	choice_mode = handle_ynf(y_used, n_used, f_used);
    choice(choice_mode);
	
	struct initial_path_info *ipi_m; // _m because i just want to keep in mind that we're in main which is a bit easier for me
	int cctd;

	ipi_m = fill_ipi(); // Fill out ipi struct
	cctd = check_create_ts_dirs(ipi_m); // check for or create directories
	if(cctd == -1) {
		fprintf(stderr, "check_create_ts_dirs(): Cannot create directories\n");
		free_ipi(ipi_m);
		return EXIT_FAILURE;
	}

	int index;
	for (index = optind ; index < argc ; index++) {
		cvm_fprintf(v_cvm_fprintf, stdout, "%s\n", argv[index]);
   		struct trashsys_log_info tli_m;
		struct dynamic_paths dp;

		if(tli_fill_info(&tli_m , argv[index], false, ipi_m) == NOFILE) {
			fprintf(stderr, "%s: error '%s': No such file or directory\n", basename(argv[0]), basename(argv[index]));
			continue;
		}

		if(fill_dynamic_paths(ipi_m, &tli_m, &dp) == -1) {
			fprintf(stderr, "%s: cannot process paths\n", basename(argv[0]));
			continue;
		}
		if(write_log_file(&dp, &tli_m, t_used) == -1) {
			fprintf(stderr, "%s: cannot create logfile\n", basename(argv[0]));
			continue;
		}
		
		if(rename(dp.old_trashfile_path, dp.new_trashfile_path) == -1) {
			continue;
		}
		
		cvm_fprintf(v_cvm_fprintf, stdout, "ID: %ld\nfullpath: %s\nfilename: %s\ntime: %ld\ntmp: %d\nsize: %ld\n", tli_m.ts_log_id, tli_m.ts_log_originalpath, tli_m.ts_log_filename, tli_m.ts_log_trashtime, tli_m.ts_log_tmp, tli_m.ts_log_filesize);
	
	}

	free_ipi(ipi_m);
	
	return 0;
}

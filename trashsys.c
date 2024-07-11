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
#define FUNCTION_FAILURE -1
#define FUNCTION_SUCCESS 0
#define BYTES 10
#define KiB 210
#define MiB 220
#define GiB 230
#define REM_SZ(remsz, final) (remsz - strlen(final)) 
#define USAGE_OUT(stream) (fprintf(stream, "%s", USAGE))

bool v_cvm_fprintf = false;
int choice_mode = MODE_NORMAL;

struct trashsys_log_info {
	int64_t ts_log_id;
	char ts_log_filename[FILENAME_MAX];
	size_t ts_log_filesize;
	time_t ts_log_trashtime;
	char ts_log_originalpath[PATH_MAX];
	bool ts_log_tmp;
};

struct list_file_content {
	char ID[PATH_MAX];
	char filename[PATH_MAX];
	char trashed_filename[PATH_MAX];
	char filesize[PATH_MAX];
	char time[PATH_MAX];
	char originalpath[PATH_MAX];
	char tmp[PATH_MAX];
	struct list_file_content *next;
};

struct dynamic_paths {
	char old_trashfile_path[PATH_MAX];
	char new_trashfile_path[PATH_MAX];
	char new_trashfile_filename[FILENAME_MAX];
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

int handle_ynf(const bool y_used, const bool n_used, const bool f_used) { // Will handle cases for y, n and f. Exits if any of these are used together.

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

int get_line(const char *filename, long focus, char **line, size_t *start) { // taken from 7Editor and modified slightly
    
    FILE *file;
    file = fopen(filename,"r"); // Open file

    if (file == NULL) { // Check if you can open file
        fprintf(stderr, "Cannot open file get_line.\n");
        return FUNCTION_FAILURE;
    }
    
    if (focus == 1) {
        int c1_count = 0;
        while (1) {
            char c = fgetc(file);
            if (c == '\n') {
                c1_count++;
                break;
            } else if (c == EOF) {
                break;
            } else {
                c1_count++;
            }
        }                       // checks how many characters are in the first line
        char c1buf[c1_count+1];
        fseek(file, 0, SEEK_SET);
        
        int i = 0;

        for (; i < c1_count ; i++) {
            c1buf[i] = fgetc(file);
        }
        c1buf[i] = '\0';
        *line = (char *)malloc(strlen(c1buf) + 1);

        if (*line != NULL) {
            strcpy(*line, c1buf); // Return line 1
        }
		
        *start = 0; // Is start the start of where line

    } else {

        focus--;
        size_t line_count = 0; // Counter starting at 0
        size_t save_i = 0;
        for (size_t i = 0; ; i++) {
            char c = fgetc(file);
            if (feof(file)) { // If end of file is encountered then break
                break; 
            }
            if (c == '\n') {
                line_count++;
                if (line_count == (size_t)focus) {
                    save_i = i;
                    break;
                }
            }
        }
        fseek(file, save_i+1, SEEK_SET);
        
        int c2_count = 0;
        while (1) {
            char c = fgetc(file);
            if (c == '\n') {
                c2_count++;
                break;
            } else if (c == EOF) {
                break;
            } else {
                c2_count++;
            }
        }
        
        fseek(file, save_i+1, SEEK_SET);
        char c2buf[c2_count+1];
        int i = 0;
        for (; i < c2_count ; i++) {
            c2buf[i] = fgetc(file);
        }
        c2buf[i] = '\0';
        *line = (char *)malloc(strlen(c2buf) + 1);

        if (*line != NULL) {
            strcpy(*line, c2buf);
        }
        *start = save_i+1; // not sure but i think it saves the start position of the line
        
    }
    
    fclose(file);
    return FUNCTION_SUCCESS;
}

int cvm_fprintf(const bool ONOROFF, FILE *stream, const char *format, ...) {
    
    if (ONOROFF == false) {
        return FUNCTION_SUCCESS; 
    }

    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);

    return result;
} 

char *concat_str(char *final, const ssize_t rem_size, const char *from) {
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
		cvm_fprintf(v_cvm_fprintf, stdout, "IF: from_len: %li\nIF: rem_size: %li\n", from_len+1, rem_size);
		return NULL;
	}
	cvm_fprintf(v_cvm_fprintf, stdout, "Ffrom_len: %li\nRrem_size: %li\n", from_len+1, rem_size);
	strcat(final, from);
	return final;
}

int fill_ipi(const bool t_used, struct initial_path_info *ipi) { // Function for filling out initial_path_info so it can be used later

	const char *ts_toplevel = "/.trashsys";
	const char *ts_log = "/log";	
   	const char *ts_trashed = "/trashed";
   	const char *ts_toplevel_withslash = "/.trashsys/";
	const char *ts_log_withslash = "/log/";	
   	const char *ts_trashed_withslash = "/trashed/";
	char *homepath;

	const char *ts_tmp = "/tmp";
	const char *ts_tmp_toplevel = "/tmp/.trashsys";
	const char *ts_tmp_log = "/tmp/.trashsys/log";
	const char *ts_tmp_trashed = "/tmp/.trashsys/trashed";
   	const char *ts_tmp_withslash = "/tmp/";
	const char *ts_tmp_toplevel_withslash = "/tmp/.trashsys/";
	const char *ts_tmp_log_withslash = "/tmp/.trashsys/log/";
	const char *ts_tmp_trashed_withslash = "/tmp/.trashsys/trashed/";
	
	ipi->ts_path_user_home[0] = '\0'; // Add null character to all of them because we'll be using concat_str (basically strcat) later
	ipi->ts_path_trashsys[0] = '\0';
	ipi->ts_path_log[0] = '\0';
	ipi->ts_path_trashed[0] = '\0';
   	ipi->ts_path_user_home_withslash[0] = '\0'; 
	ipi->ts_path_trashsys_withslash[0] = '\0';
	ipi->ts_path_log_withslash[0] = '\0';
	ipi->ts_path_trashed_withslash[0] = '\0';
	
	if (t_used == false) {
		homepath = getenv(ENVVAR_HOME); // Get the home path of the current user

		if (homepath == NULL) {
			fprintf(stderr, "fill_ipi(): getenv failed");
			return FUNCTION_FAILURE;
		}

		// /home/john
		// /home/john/
		if(concat_str(ipi->ts_path_user_home, PATH_MAX, homepath) == NULL
		   || concat_str(ipi->ts_path_user_home_withslash, PATH_MAX, homepath) == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}
		
		if(concat_str(ipi->ts_path_user_home_withslash, REM_SZ(PATH_MAX, ipi->ts_path_user_home_withslash), "/") == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
		    return FUNCTION_FAILURE;
		}

		// /home/john/.trashsys
		// /home/john/.trashsys/
		if(concat_str(ipi->ts_path_trashsys, PATH_MAX, homepath) == NULL
		   || concat_str(ipi->ts_path_trashsys_withslash, PATH_MAX, homepath) == NULL) { 
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}
	    
		if(concat_str(ipi->ts_path_trashsys, REM_SZ(PATH_MAX, ipi->ts_path_trashsys), ts_toplevel) == NULL 
		   || concat_str(ipi->ts_path_trashsys_withslash, REM_SZ(PATH_MAX, ipi->ts_path_trashsys_withslash), ts_toplevel_withslash) == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}

		// /home/john/.trashsys/log
		// /home/john/.trashsys/log/
		if(concat_str(ipi->ts_path_log, PATH_MAX, ipi->ts_path_trashsys) == NULL
		   || concat_str(ipi->ts_path_log_withslash, PATH_MAX, ipi->ts_path_trashsys) == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}

		if(concat_str(ipi->ts_path_log, REM_SZ(PATH_MAX, ipi->ts_path_log), ts_log) == NULL
		   || concat_str(ipi->ts_path_log_withslash, REM_SZ(PATH_MAX, ipi->ts_path_log_withslash), ts_log_withslash) == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}

		// /home/john/.trashsys/trashed
		// /home/john/.trashsys/trashed/
		if(concat_str(ipi->ts_path_trashed, PATH_MAX, ipi->ts_path_trashsys) == NULL
		   || concat_str(ipi->ts_path_trashed_withslash, PATH_MAX, ipi->ts_path_trashsys) == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}

		if(concat_str(ipi->ts_path_trashed, REM_SZ(PATH_MAX, ipi->ts_path_trashed), ts_trashed) == NULL
		   || concat_str(ipi->ts_path_trashed_withslash, REM_SZ(PATH_MAX, ipi->ts_path_trashed_withslash), ts_trashed_withslash) == NULL) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}
	
	} else if (t_used == true) { // If -t flag is specified we fill ipi with /tmp paths instead
		if(concat_str(ipi->ts_path_user_home, PATH_MAX, ts_tmp) == NULL ||
		   concat_str(ipi->ts_path_trashsys, PATH_MAX, ts_tmp_toplevel) == NULL ||
		   concat_str(ipi->ts_path_log, PATH_MAX, ts_tmp_log) == NULL ||
		   concat_str(ipi->ts_path_trashed, PATH_MAX, ts_tmp_trashed) == NULL ||
		   concat_str(ipi->ts_path_user_home_withslash, PATH_MAX, ts_tmp_withslash) == NULL ||
		   concat_str(ipi->ts_path_trashsys_withslash, PATH_MAX, ts_tmp_toplevel_withslash) == NULL ||
		   concat_str(ipi->ts_path_log_withslash, PATH_MAX, ts_tmp_log_withslash) == NULL ||
		   concat_str(ipi->ts_path_trashed_withslash, PATH_MAX, ts_tmp_trashed_withslash) == NULL
		   ) {
			fprintf(stderr, "fill_ipi: path is too long\n");
			return FUNCTION_FAILURE;
		}

		
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

	return FUNCTION_SUCCESS;
}

int check_create_ts_dirs(const struct initial_path_info *ipi) { // 1. Check if trashsys toplevel exists 2. Check if log exists 3. Check if trashed exists 

	int mkd;
  	mkd = mkdir(ipi->ts_path_trashsys, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { cvm_fprintf(v_cvm_fprintf, stdout, ".trashsys exists\n"); } else { return FUNCTION_FAILURE; }
	} else { cvm_fprintf(v_cvm_fprintf, stdout, "%s was created\n", ipi->ts_path_trashsys); }
	
	mkd = mkdir(ipi->ts_path_log, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { cvm_fprintf(v_cvm_fprintf, stdout, "log exists\n"); } else { return FUNCTION_FAILURE; }
	} else { cvm_fprintf(v_cvm_fprintf, stdout, "%s was created\n", ipi->ts_path_log); }
	
	mkd = mkdir(ipi->ts_path_trashed, 0755);
	if (mkd < 0) {
		if (errno == EEXIST) { cvm_fprintf(v_cvm_fprintf, stdout, "trashed exists\n"); } else { return FUNCTION_FAILURE; }
	} else { cvm_fprintf(v_cvm_fprintf, stdout, "%s was created\n", ipi->ts_path_trashed); }
    
	return FUNCTION_SUCCESS;
}
 
int64_t find_highest_id (const struct initial_path_info *ipi) { 
	// We need to check whether a file is a directory or just a file. 
	int64_t id = 0;
	struct dirent *ddd;
	DIR *dir = opendir(ipi->ts_path_log);
	if (dir == NULL) {
		return FUNCTION_FAILURE;
	}
	while ((ddd = readdir(dir)) != NULL) {
		
		// MUST BE TESTED MORE!!!
		char stat_fullpath[PATH_MAX];
		stat_fullpath[0] = '\0';

		if(concat_str(stat_fullpath, PATH_MAX, ipi->ts_path_log_withslash) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			closedir(dir);
			return FUNCTION_FAILURE;
		}

		if(concat_str(stat_fullpath, REM_SZ(PATH_MAX, stat_fullpath), ddd->d_name) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			closedir(dir);
			return FUNCTION_FAILURE;
		}
		
		struct stat d_or_f;
		stat(stat_fullpath, &d_or_f);
		
		if(S_ISREG(d_or_f.st_mode)) { // check if given file is actually a file
			cvm_fprintf(v_cvm_fprintf, stdout, "is regular file: %s\nstat_fullpath: %s\n", ddd->d_name, stat_fullpath);
			char *endptr;
			int64_t strtoll_ID = strtoull(ddd->d_name, &endptr, 10);
			if(ddd->d_name == endptr) {
				cvm_fprintf(v_cvm_fprintf, stdout, "d_name == endptr | d_name: %p | endptr: %p | d_name string: %s\n", ddd->d_name, endptr, ddd->d_name);
				continue;
			}
			if(*endptr != ':') {
				cvm_fprintf(v_cvm_fprintf, stdout, "':' not found for file: %s\n", ddd->d_name);
				continue;
			}
			if(strtoll_ID > id) { // If id is bigger then update it
				id = strtoll_ID;
				cvm_fprintf(v_cvm_fprintf, stdout, "found higher ID: %d\n", id);
			}
		}
		
	}
	closedir(dir);
	return id;
}

int tli_fill_info (struct trashsys_log_info *tli, char* filename, const bool log_tmp, struct initial_path_info *ipi) { 
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
		free(rp);
		return FUNCTION_FAILURE;
	}
	free(rp);
	if(concat_str(tli->ts_log_filename, FILENAME_MAX, basename(filename)) == NULL) {
		return FUNCTION_FAILURE;
	}

	tli->ts_log_tmp = log_tmp; // tmp or not?
	curtime = time(NULL);
	if (curtime == -1) { return FUNCTION_FAILURE; }
	tli->ts_log_trashtime = curtime;
	FILE *file = fopen(filename, "r"); // We get the filesize in bytes
	if(file == NULL) { return NOFILE; }
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fclose(file);
	tli->ts_log_filesize = (size_t)filesize;

	int64_t ID = find_highest_id(ipi);
	if (ID == FUNCTION_FAILURE) {
		return FUNCTION_FAILURE;
	}
	tli->ts_log_id = ID + 1; // +1 because if we are making a new file we need to give it one above highest ID.
	
	return FUNCTION_SUCCESS;
}

int fill_dynamic_paths (struct initial_path_info *ipi, struct trashsys_log_info *tli, struct dynamic_paths *dp) {
	
	dp->old_trashfile_path[0] = '\0';
	dp->new_trashfile_path[0] = '\0';
	dp->new_logfile_path_incl_name[0] = '\0';
	dp->new_trashfile_filename[0] = '\0';
	// /path/to/my/file.txt
	if(concat_str(dp->old_trashfile_path, PATH_MAX, tli->ts_log_originalpath) == NULL) { return FUNCTION_FAILURE; }

	// filename ID eg. '35:'
	char idstr[23];
	snprintf(idstr, 23, "%ld:", tli->ts_log_id);
	
	// /home/john/.trashsys/trashed/35:file.txt
	if(concat_str(dp->new_trashfile_path, PATH_MAX, ipi->ts_path_trashed_withslash) == NULL) { return FUNCTION_FAILURE; }
	if(concat_str(dp->new_trashfile_path, REM_SZ(PATH_MAX, dp->new_trashfile_path), idstr) == NULL) { return FUNCTION_FAILURE; }
	if(concat_str(dp->new_trashfile_path, REM_SZ(PATH_MAX, dp->new_trashfile_path), tli->ts_log_filename) == NULL) { return FUNCTION_FAILURE; }
	
	// /home/john/.trashsys/log/35:file.txt.log
	if(concat_str(dp->new_logfile_path_incl_name, PATH_MAX, ipi->ts_path_log_withslash) == NULL) { return FUNCTION_FAILURE; }
   	if(concat_str(dp->new_logfile_path_incl_name, REM_SZ(PATH_MAX, dp->new_logfile_path_incl_name), idstr) == NULL) { return FUNCTION_FAILURE; }
   	if(concat_str(dp->new_logfile_path_incl_name, REM_SZ(PATH_MAX, dp->new_logfile_path_incl_name), tli->ts_log_filename) == NULL) { return FUNCTION_FAILURE; }
   	if(concat_str(dp->new_logfile_path_incl_name, REM_SZ(PATH_MAX, dp->new_logfile_path_incl_name), ".log") == NULL) { return FUNCTION_FAILURE; }
	
	// 35:file.txt
	if(concat_str(dp->new_trashfile_filename, PATH_MAX, basename(dp->new_trashfile_path)) == NULL) { return FUNCTION_FAILURE; }
    
	cvm_fprintf(v_cvm_fprintf, stdout, "%s\n%s\n%s\n%s\n"
			    , dp->old_trashfile_path
				, dp->new_trashfile_path
				, dp->new_trashfile_filename
				, dp->new_logfile_path_incl_name
				);
	
	return FUNCTION_SUCCESS;
}

int write_log_file(struct dynamic_paths *dp, struct trashsys_log_info *tli, const bool t_used_aka_tmp) {

	char *tmp_path = "/tmp/";

	if (t_used_aka_tmp == true) {
		fprintf(stdout, "%s", tmp_path);
	}

	cvm_fprintf(v_cvm_fprintf, stdout, "logfile path: %s\n", dp->new_logfile_path_incl_name);
	FILE *file = fopen(dp->new_logfile_path_incl_name, "w");
	if(file == NULL) {
		printf("%s\n", strerror(errno));
		return -1;
	}

	/*this fprintf is what WRITES in to the logfile*/
	fprintf(file, "%ld\n%s\n%s\n%ld\n%ld\n%s\n%d\n"
			, tli->ts_log_id
			, tli->ts_log_filename
			, dp->new_trashfile_filename
			, tli->ts_log_filesize
			, tli->ts_log_trashtime
			, tli->ts_log_originalpath
			, tli->ts_log_tmp);
	
	fclose(file);

	return FUNCTION_SUCCESS;
}

char *rawtime_to_readable(time_t rawtime) {

	struct tm *tmp;
	char *pretty_time = malloc(sizeof(char) * 512);
	tmp = localtime(&rawtime);
	if(strftime(pretty_time, 512, "%F", tmp) == 0) {
		free(pretty_time);
		return NULL;
	}

	return pretty_time;
}

char *bytes_to_readable_str(size_t bytes, char *str, size_t str_len) {

	char tmp_str[str_len];
	double f_bytes = (double)bytes;
	int count = 0;
	char *BKMG[] = {"B", "KiB", "MiB", "GiB"};
	   
	while (f_bytes >= 1024) {
		f_bytes = f_bytes / 1024;
		count++;
	}
	
	snprintf(tmp_str, str_len, "%0.1f", f_bytes);
	if(concat_str(str, str_len, tmp_str) == NULL) {
		return NULL;
	}
	
	return BKMG[count];
}

int lfc_formatted (struct list_file_content *lfc, const bool L_used) {

	time_t rawtime;
    size_t filesize_bytes;
	char *endptr;
	char *endptr2;
	char *pretty_time;


	rawtime = (time_t)strtoll(lfc->time, &endptr, 10);
	if (errno == ERANGE || lfc->time == endptr) {
		fprintf(stdout, "strtoll fail\n");
		return FUNCTION_FAILURE;
	}
	filesize_bytes = (size_t)strtoul(lfc->filesize, &endptr2, 10);
	if (errno == ERANGE || lfc->filesize == endptr) {
		fprintf(stdout, "strtoul fail\n");
		return FUNCTION_FAILURE;
	}
	pretty_time = rawtime_to_readable(rawtime);
	if(pretty_time == NULL){
		fprintf(stdout, "Cannot convert time to readable\n");
		return FUNCTION_FAILURE;
	}
	char *fff;
	size_t str_len = 1024;
	char readable_mib_str[str_len];
	readable_mib_str[0] = '\0';
  	fff = bytes_to_readable_str(filesize_bytes, readable_mib_str, str_len);
	if (L_used == true) {
		fprintf(stdout, "ID: %s    %s    %s %s    %s    Trashed at: %s (unixtime: %s)    originalpath: %s\n"
				, lfc->ID
				, lfc->filename
				, readable_mib_str
				, fff
				, lfc->filesize
				, pretty_time
				, lfc->time
				, lfc->originalpath
				);
		free(pretty_time);
		return FUNCTION_SUCCESS;
	}
	fprintf(stdout, "ID: %s    %s    %s %s    Trashed at: %s\n"
			, lfc->ID
			, lfc->filename
			, readable_mib_str
			, fff
			, pretty_time
			);
	free(pretty_time);
	return FUNCTION_SUCCESS;
}

void free_lfc (struct list_file_content *lfc) {
	struct list_file_content *save;
    while (lfc != NULL) {
        save = lfc;
        lfc = lfc->next;
        free(save);
    }
}

struct list_file_content *fill_lfc (struct initial_path_info *ipi) {

	struct dirent *ddd;
	DIR *dir = opendir(ipi->ts_path_log);
	if (dir == NULL) {
		return NULL;
	}

	struct list_file_content *lfc = malloc(sizeof(struct list_file_content)); // first node
	struct list_file_content *lfc_head = lfc;
	bool first = true;
	while ((ddd = readdir(dir)) != NULL) {
		
		// must BE TESTED MORE!!!
		char stat_fullpath[PATH_MAX];
		stat_fullpath[0] = '\0';

		if(concat_str(stat_fullpath, PATH_MAX, ipi->ts_path_log_withslash) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handle
			free_lfc(lfc_head);
			closedir(dir);
			return NULL;
		}

		if(concat_str(stat_fullpath, REM_SZ(PATH_MAX, stat_fullpath), ddd->d_name) == NULL) {
			fprintf(stderr, "Path is too long\n"); // rare case but at least its handled
			free_lfc(lfc_head);
			closedir(dir);
			return NULL;
		}
		
		struct stat d_or_f;
		stat(stat_fullpath, &d_or_f);
		
		if(S_ISREG(d_or_f.st_mode)) { // check if given file is actually a file
			if(first == false) {
				lfc->next = malloc(sizeof(struct list_file_content));
				lfc = lfc->next;
			} else {
				first = false;
			}
			cvm_fprintf(v_cvm_fprintf, stdout, "is regular file: %s\nstat_fullpath: %s\n", ddd->d_name, stat_fullpath);
			FILE *file = fopen(stat_fullpath, "r");
			if (file == NULL) {
				free_lfc(lfc_head);
				closedir(dir);
				return NULL;
			}
			
			char *lfc_a[7];
			lfc->ID[0] = '\0';
			lfc->filename[0] = '\0';
			lfc->trashed_filename[0] = '\0';
			lfc->filesize[0] = '\0';
			lfc->time[0] = '\0';
			lfc->originalpath[0] = '\0';
			lfc->tmp[0] = '\0';
			lfc_a[0] = lfc->ID;
			lfc_a[1] = lfc->filename;
			lfc_a[2] = lfc->trashed_filename;
			lfc_a[3] = lfc->filesize;
			lfc_a[4] = lfc->time;
			lfc_a[5] = lfc->originalpath;
			lfc_a[6] = lfc->tmp;
			int i = 0;
			int linenum = 1;
			for ( ; i < 7 ; i++, linenum++) {
				char *line;
				size_t start;
				if(get_line(stat_fullpath, linenum, &line, &start) == FUNCTION_FAILURE) {
					free_lfc(lfc_head);
					closedir(dir);
					return NULL;
				}
				if(concat_str(lfc_a[i], PATH_MAX, line) == NULL) {
					free_lfc(lfc_head);
					free(line);
					closedir(dir);
					return NULL;
				}
				for(int si = 0 ;; si++) {
					if(lfc_a[i][si] == '\n') {
						lfc_a[i][si] = '\0';
						break;
					}
				}
				free(line);
			}
			fclose(file); 
		}
	}
	if(first == true) {
		free_lfc(lfc_head);
		closedir(dir);
		return NULL;
	}
	lfc = NULL;
	closedir(dir);
	return lfc_head;

}
	
int choice(const int mode) {

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

    return FUNCTION_FAILURE; // Should never happen
}

int main (int argc, char *argv[]) {

	if (argc == 1) {
		USAGE_OUT(stderr);
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
    while ((opt = getopt(argc, argv, "ynvfatlLcCR:h")) != -1) {
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
		case 'h':
			USAGE_OUT(stdout);
			return EXIT_SUCCESS;
		break;
        }
    }
	if(optind == argc && (l_used || L_used) == false) {
		USAGE_OUT(stderr);
		return EXIT_FAILURE;
	}
	if (v_used == true) { v_cvm_fprintf = true; } // Verbose mode
	cvm_fprintf(v_cvm_fprintf, stdout, "options RCcLltafvny: %d%d%d%d%d%d%d%d%d%d%d\n", R_used, C_used, c_used, L_used, l_used, t_used, a_used, f_used, v_used, n_used, y_used);
	choice_mode = handle_ynf(y_used, n_used, f_used);
	
	struct initial_path_info ipi_m; // _m because i just want to keep in mind that we're in main which is a bit easier for me
	int cctd;
	if(fill_ipi(t_used, &ipi_m) == FUNCTION_FAILURE) {
		fprintf(stderr, "fill_ipi error, exiting...\n");
		return EXIT_FAILURE;
	}
	cctd = check_create_ts_dirs(&ipi_m); // check for or create directories
	if(cctd == FUNCTION_FAILURE) {
		fprintf(stderr, "check_create_ts_dirs() error: Cannot create directories\n");
		return EXIT_FAILURE;
	}

	if(l_used == true || L_used == true) {
		struct list_file_content *lfc = fill_lfc(&ipi_m);
		struct list_file_content *walk;
		int i = 1;
		//int lfc_formatted (struct list_file_content *lfc, const bool L_used)
		if(lfc == NULL) { return EXIT_SUCCESS; }
		for(walk = lfc ; walk != NULL ; walk = walk->next, i++) {
			lfc_formatted(walk, L_used);
		}
		free_lfc(lfc);
		return EXIT_SUCCESS;
	}
	
	int index;
	for (index = optind ; index < argc ; index++) {
   		struct trashsys_log_info tli_m;
		struct dynamic_paths dp;
		int tli_fi_r = tli_fill_info(&tli_m , argv[index], false, &ipi_m);
		if(tli_fi_r == NOFILE || tli_fi_r == FUNCTION_FAILURE) {
			fprintf(stderr, "%s: error '%s': No such file or directory\n", basename(argv[0]), basename(argv[index]));
			continue;
		}

		if(fill_dynamic_paths(&ipi_m, &tli_m, &dp) == -1) {
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
		
		cvm_fprintf(v_cvm_fprintf, stdout, "ID: %ld\nfull original path: %s\noriginal filename: %s\ntime: %ld\ntmp: %d\nsize: %ld\nnew trashed filename: %s\n"
					, tli_m.ts_log_id
					, tli_m.ts_log_originalpath
					, tli_m.ts_log_filename
					, tli_m.ts_log_trashtime
					, tli_m.ts_log_tmp
					, tli_m.ts_log_filesize
					, dp.new_trashfile_filename
					);
	
	}
	
	return EXIT_SUCCESS;
}

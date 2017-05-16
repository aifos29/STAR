/*tar.h 
 * FILE VERSION 000
 * April 23, 2017
 * 
 * Operative Systems's Principles
 * Project II
 *
 */


#define BLOCKSIZE       512
#define BLOCKING_FACTOR 20
#define RECORDSIZE      10240

// file type values (1 octet)
#define REGULAR          0
#define NORMAL          '0'
#define HARDLINK        '1'
#define SYMLINK         '2'
#define CHAR            '3'
#define BLOCK           '4'
#define DIRECTORY       '5'
#define FIFO            '6'
#define CONTIGUOUS      '7'




#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>



// tar entry metadata structure (singly-linked list)
struct tar_t {
    char original_name[100];                // original filenme; only available when writing into a tar
    unsigned int begin;                     // location of data in file (including metadata)
    union {
        union {
            // Pre-POSIX.1-1988 format
            struct {
                char name[100];             // file name
                char mode[8];               // permissions
                char uid[8];                // user id (octal)
                char gid[8];                // group id (octal)
                char size[12];              // size (octal)
                char mtime[12];             // modification time (octal)
                char check[8];              // sum of unsigned characters in block block, with spaces in the check field while calculation is done (octal)
                char link;                  // link indicator
                char link_name[100];        // name of linked file
            };

            // UStar format (POSIX IEEE P1003.1)
            struct {
                char old[156];              // first 156 octets of Pre-POSIX.1-1988 format
                //char type;                  // file type
                char also_link_name[100];   // name of linked file
                char ustar[8];              // ustar\000
                char owner[32];             // user name (string)
                char group[32];             // group name (string)
                char major[8];              // device major number
                char minor[8];              // device minor number
                char prefix[155];
            };
        };

        char block[512];                    // raw memory (500 octets of actual data, padded to 1 block)
    };

    struct tar_t * next;
};

int MAX(int x,int y) {
    if (x > y)
    {
        return x;
    }
    return y;
}


struct tar_t * exists(struct tar_t * archive, const char * filename, const char ori){
    while (archive){
        if (ori){
            if (!strncmp(archive -> original_name, filename, MAX(strlen(archive -> original_name), strlen(filename)) + 1)){
                return archive;
            }
        }
        else{
            if (!strncmp(archive -> name, filename, MAX(strlen(archive -> name), strlen(filename)) + 1)){
                return archive;
            }
        }
        archive = archive -> next;
    }
    return NULL;
}



unsigned int calculate_checksum(struct tar_t * entry){
    // use 8 spaces (' ', char 0x20) in place of checksum string
    memset(entry -> check, ' ', sizeof(char) * 8);

    // sum of entire metadata
    unsigned int check = 0;
    for(int i = 0; i < 500; i++){
        check += (unsigned char) entry -> block[i];
    }
    sprintf(entry -> check, "%06o", check);

    entry -> check[6] = '\0';
    entry -> check[7] = ' ';
    return check;
}

int format_tar_data(struct tar_t * entry, const char * filename, const char verbosity){
    if (!entry){
        return -1;
    }

    struct stat st;
    if (lstat(filename, &st)){
        printf("ERROR \n");

        return -1;
    }


    // start putting in new data
    memset(entry, 0, sizeof(struct tar_t));
    strncpy(entry -> original_name, filename, 100);
    strncpy(entry -> name, filename, 100);

    sprintf(entry -> size, "%011o", (int) st.st_size);
    sprintf(entry -> mtime, "%011o", (int) st.st_mtime);






    // get the checksum
    calculate_checksum(entry);

    return 0;
}

unsigned int oct2uint(char * oct, unsigned int size){
    unsigned int out = 0;
    int i = 0;
    while ((i < size) && oct[i]){
        out = (out << 3) | (unsigned int) (oct[i++] - '0');
    }
    return out;
}

int write_size(int fd, char * buf, int size){
    int wrote = 0, rc;
    while ((wrote < size) && ((rc = write(fd, buf + wrote, size - wrote)) > 0)){
        wrote += rc;
    }
    return wrote;
}

int read_size(int fd, char * buf, int size){
    int got = 0, rc;
    while ((got < size) && ((rc = read(fd, buf + got, size - got)) > 0)){
        got += rc;
    }
    return got;
}



int write_entries(const int fd, struct tar_t ** archive, struct tar_t ** head, const size_t filecount, const char * files[], int * offset, const char verbosity){
    printf("File Count:%d\n",filecount);


    // add new data
    struct tar_t ** tar = archive;  // current entry
    char buf[512];              // one block buffer
    for(unsigned int i = 0; i < filecount; i++){
        *tar = malloc(sizeof(struct tar_t));

        // stat file
        if (format_tar_data(*tar, files[i], verbosity) < 0){
            printf("Error: Failed to stat %s\n", files[i]);
        }

        if (!i){
            *archive = *tar;  // store first address
        }

        (*tar) -> begin = *offset;

            if (write_size(fd, (*tar) -> block, 512) != 512){
                printf("Error: Failed to write metadata to archive\n");
            }



        int f = open((*tar) -> name, O_RDONLY);
        if (f < 0){
            printf("Error: Could not open %s\n", files[i]);
        }

        int r = 0;
        while ((r = read_size(f, buf, 512)) > 0){
            if (write_size(fd, buf, r) != r){

                printf("No se puede escribir en el archivo\n");
            }
        }

        close(f);



            // pad data to fill block
            const unsigned int size = oct2uint((*tar) -> size, 11);
            const unsigned int pad = 512 - size % 512;
            if (pad != 512){
                for(unsigned int j = 0; j < pad; j++){
                    if (write_size(fd, "\0", 1) != 1){
                        printf("Error: Could not write padding data\n");
                    }
                }
                *offset += pad;
            }
            *offset += size;
            tar = &((*tar) -> next);


        // add metadata size
        *offset += 512;
    }

    return 0;
}





int write_end_data(const int fd, int size, const char verbosity){
    if (fd < 0){
        return -1;
    }

    // complete current record
    const int pad = RECORDSIZE - (size % RECORDSIZE);
    for(int i = 0; i < pad; i++){
        if (write(fd, "\0", 1) != 1){
            printf( "Error: Unable to close tar file\n");
            return -1;
        }
    }

    // if the current record does not have 2 blocks of zeros, add a whole other record
    if (pad < (2 * BLOCKSIZE)){
        for(int i = 0; i < RECORDSIZE; i++){
            if (write(fd, "\0", 1) != 1){
                printf("Error: Unable to close tar file\n");
                return -1;
            }
        }
        return pad + RECORDSIZE;
    }

    return pad;
}

int tar_write(const int fd, struct tar_t ** archive, const size_t filecount, const char * files[], const char verbosity){
    if (fd < 0){
        return -1;
    }

    if (!archive){
        return -1;
    }

    // where file descriptor offset is
    int offset = 0;

    // if there is old data
    struct tar_t ** tar = archive;


    // write entries first
    if (write_entries(fd, tar, archive, filecount, files, &offset, verbosity) < 0){
        printf( "Error: Failed to write entries\n");
        return -1;
    }

    // write ending data
    if (write_end_data(fd, offset, verbosity) < 0){
        printf( "Error: Failed to write end data");
        return -1;
    }

    // clear original names from data
    tar = archive;
    while (*tar){
        memset((*tar) -> name, 0, 100);
        tar = &((*tar) -> next);
    }
    return offset;
}

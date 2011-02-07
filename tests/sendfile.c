/*
  Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
  This file is part of Rozo.

  Rozo is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 3 of the License,
  or (at your option) any later version.

  Rozo is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include <limits.h>

#include <rozo_client.h>

static char host[255];

static char export[255];

static rozo_client_t rozo_client;

int main(int argc, char *argv[]) {
    rozo_file_t *file;
    int fd;
    uint32_t len;
    uint64_t off;
    char * buffer;
    int buffer_size;
    char output_path[PATH_MAX];

    // Check args
    if (argc < 5) {
        printf("Usage: sendfile <host> <export> <input file> <output file> <buffer size>\n");
        exit(1);
    }

    strcpy(host, argv[1]);
    strcpy(export, argv[2]);

    if (rozo_initialize() != 0) {
        perror("rozo_initialize failed");
        exit(1);
    }

    // Initialize the API
    if (rozo_client_initialize(&rozo_client, host, export) != 0) {
        perror("rozo_client_initialize failed");
        exit(1);
    }

    // Get the buffer size
    if ((buffer_size = atoi(argv[5])) <= 0) {
        perror("buffer size failed");
        exit(1);
    }

    // Allocate buffer
    buffer = malloc(sizeof (char) * buffer_size);

    // Get the output path
    output_path[0] = '/';
    strncat(output_path, argv[4], sizeof (output_path) - 1);

    // Open the input file
    if ((fd = open(argv[3], O_RDONLY)) < 0) {
        perror("open failed");
    }

    // Create the output file in the MDS
    if (rozo_client_mknod(&rozo_client, output_path, S_IFREG) != 0) {
        perror("rozo_client_mknod failed");
        exit(1);
    }

    // Open the output file
    if ((file = rozo_client_open(&rozo_client, output_path, O_RDWR)) == NULL) {
        perror("rozo_client_open failed");
        exit(1);
    }

    // Set the begin offset at 0
    off = 0;

    // Read the input file per block of buffer_size bytes
    while ((len = read(fd, buffer, buffer_size)) > 0) {

        // Write the output file per block of len bytes
        if (rozo_client_write(&rozo_client, file, off, buffer, len) < 0) {
            perror("api_write failed");
            exit(1);
        }

        // Update the offset
        off += len;
    }

    // Close the input and output files
    rozo_client_close(file);
    close(fd);

    // Free the buffer
    free(buffer);
    exit(0);
}

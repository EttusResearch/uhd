// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "tlv_eeprom.h"

struct tlv_eeprom *tlv_eeprom_read_from_file(const char *path)
{
	struct tlv_eeprom *ep;
	int fd;
	uint8_t *ptr;
	size_t len;
	ssize_t rd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("couldn't open file");
		return NULL;
	}


	ep = malloc(sizeof(struct tlv_eeprom));
	if (!ep) {
		perror("alloc failed");
		return NULL;
	}

	len = sizeof(struct tlv_eeprom);
	ptr = (uint8_t *)ep;

	while (len) {
		rd = read(fd, ptr, len);
		if (rd < 0) {
			perror("read failed");
			goto out;
		}

		len -= rd;
		ptr += rd;

		if (!rd)
			break;
	}

	return ep;
out:
	free(ep);
	return NULL;
}

int tlv_eeprom_write_to_file(const struct tlv_eeprom *e, const char *path)
{
	int fd, rv = 0;
	uint8_t *ptr;
	size_t len;
	ssize_t wr;

	fd = open(path, O_WRONLY | O_CREAT, 0666);
	if (fd < 0) {
		perror("couldn't open file");
		return -1;
	}

	len = sizeof(*e);
	ptr = (uint8_t *)e;

	while (len) {
		wr = write(fd, ptr, len);
		if (wr < 0) {
			perror("error writing file");
			goto out;
		}
		len -= wr;
		ptr += wr;
	}

out:
	close(fd);
	return rv;
}

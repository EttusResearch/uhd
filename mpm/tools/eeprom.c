//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <stdio.h>
#include <stdlib.h>

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>


#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include "eeprom.h"


static const u32 USRP_EEPROM_MAGIC = 0xF008AD10;
static const u32 USRP_EEPROM_DB_MAGIC = 0xF008AD11;
static const u32 USRP_EEPROM_VERSION = 2;
static const u32 USRP_EEPROM_DB_VERSION = 2;
static const u32 USRP_EEPROM_DEFAULT_MCU_FLAGS[4] = {0x0, 0x0, 0x0, 0x0};


/*static const size_t TOTAL_SIZE=256;*/
static const u8 EEPROM_I2C_ADDR = 0x50;

static const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *p;

	p = buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

static int __eth_addr_parse(uint8_t *out, const char *in)
{
	/* 00:00:00:00:00:00 */
	char tmp[18];
	char *tok;
	unsigned int ul, i = 0;

	if (strlen(in) < 17)
		return -1;

	strncpy(tmp, in, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = '\0';

	for (tok = strtok(tmp, ":"); tok && (i < ETH_ALEN); tok = strtok(NULL, ":")) {
		ul = strtoul(tok, NULL, 16);
		out[i++] = ul & 0xff;
	}

	return 0;
}

struct usrp_sulfur_eeprom *usrp_sulfur_eeprom_new(const u32 *mcu_flags,
						  const u16 pid,
						  const u16 rev,
						  const char *serial,
						  const char *eth_addr0,
						  const char *eth_addr1,
						  const char *eth_addr2,
						  const u16 dt_compat,
						  const u16 mcu_compat)
{
	struct usrp_sulfur_eeprom *ep;
	int i;

	ep = malloc(sizeof(*ep));
	if (!ep) {
		perror("Failed to allocate eeprom struct\n");
		return NULL;
	}

	/* it's an eeprom, so just make it 0xff */
	memset(ep, 0, sizeof(*ep));

	ep->magic = htonl(USRP_EEPROM_MAGIC);
	ep->version = htonl(USRP_EEPROM_VERSION);

	if (!mcu_flags)
		mcu_flags = &USRP_EEPROM_DEFAULT_MCU_FLAGS[0];

	for (i = 0; i < 4; ++i)
		ep->mcu_flags[i] = htonl(mcu_flags[i]);

	ep->pid = htons(pid);
	ep->rev = htons(rev);

	if (strlen(serial) > 8) {
		fprintf(stderr, "Serial# too long\n");
		free(ep);
		return NULL;
	}

	memset(ep->serial, '\0', 8);
	strncpy(ep->serial, serial, 8);

	if (eth_addr0)
		__eth_addr_parse(ep->eth_addr0, eth_addr0);

	if (eth_addr1)
		__eth_addr_parse(ep->eth_addr1, eth_addr1);

	if (eth_addr2)
		__eth_addr_parse(ep->eth_addr2, eth_addr2);

	ep->dt_compat = htons(dt_compat);
	ep->mcu_compat = htons(mcu_compat);

	ep->crc = htonl(crc32(0, &ep->magic, sizeof(*ep)-4));

	return ep;
}

static int __usrp_sulfur_eeprom_check_crc(const struct usrp_sulfur_eeprom *ep)
{
	uint32_t crc;

	crc = crc32(0, &ep->magic, sizeof(*ep) - 4);

	return (crc != ep->crc);
}

static int __usrp_sulfur_db_eeprom_check_crc(const struct usrp_sulfur_db_eeprom *ep)
{
	uint32_t crc;

	crc = crc32(0, &ep->magic, sizeof(*ep) - 4);

	return (crc != ep->crc);
}

void usrp_sulfur_eeprom_print(const struct usrp_sulfur_eeprom *ep)
{
	int i;

	if (!ep) {
		fprintf(stderr, "ep not valid!\n");
		return;
	}

	printf("-- PID/REV: %04x %04x\n", ntohs(ep->pid), ntohs(ep->rev));
	for (i = 0; i < 4; ++i)
		printf("-- MCU_FLAGS[%u]: %08x\n", i, ntohl(ep->mcu_flags[i]));
	printf("-- Serial: %s\n", ep->serial);

	printf("-- eth_addr0: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       ep->eth_addr0[0], ep->eth_addr0[1], ep->eth_addr0[2],
	       ep->eth_addr0[3], ep->eth_addr0[4], ep->eth_addr0[5]);
	printf("-- eth_addr1: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       ep->eth_addr1[0], ep->eth_addr1[1], ep->eth_addr1[2],
	       ep->eth_addr1[3], ep->eth_addr1[4],
	       ep->eth_addr1[5]);
	printf("-- eth_addr2: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       ep->eth_addr2[0], ep->eth_addr2[1], ep->eth_addr2[2],
	       ep->eth_addr2[3], ep->eth_addr2[4], ep->eth_addr2[5]);

	if (ntohl(ep->version) == 2)
		printf("-- DT-Compat/MCU-Compat: %04x %04x\n",
		       ntohs(ep->dt_compat), ntohs(ep->mcu_compat));

	printf("-- CRC: %08x (%s)\n", ntohl(ep->crc),
	       __usrp_sulfur_eeprom_check_crc(ep) ? "matches": "doesn't match!");
}

struct usrp_sulfur_eeprom *usrp_sulfur_eeprom_from_file(const char *path)
{
	int fd;
	struct usrp_sulfur_eeprom *ep;
	size_t len = sizeof(*ep);
	size_t got = 0;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Could not open file:\n");
		return NULL;
	}

	ep = malloc(sizeof(*ep));
	if (!ep) {
		perror("Could not allocate struct");
		return ep;
	}

	u8 *rptr = (u8*)ep;

	while(len) {
		got = read(fd, rptr, len);
		len -= got;
		rptr += got;
	}

	close(fd);
	if (ep->magic != ntohl(USRP_EEPROM_MAGIC)) {
		free(ep);
		return NULL;
	}

	return ep;
}

void usrp_sulfur_eeprom_to_file(struct usrp_sulfur_eeprom *ep, const char *path)
{
	int fd;
	size_t len = sizeof(*ep);
	size_t got = 0;

	fd = open(path, O_WRONLY | O_CREAT);
	if (fd < 0) {
		perror("Could not open file:\n");
		return;
	}

	u8 *rptr = (u8*)ep;

	while(len) {
		got = write(fd, rptr, len);
		len -= got;
		rptr += got;
	}

	close(fd);

	return;
}

void usrp_sulfur_db_eeprom_to_file(struct usrp_sulfur_db_eeprom *ep, const char *path)
{
	int fd;
	size_t len = sizeof(*ep);
	size_t got = 0;

	fd = open(path, O_WRONLY | O_CREAT);
	if (fd < 0) {
		perror("Could not open file:\n");
		return;
	}

	u8 *rptr = (u8*)ep;

	while(len) {
		got = write(fd, rptr, len);
		len -= got;
		rptr += got;
	}

	close(fd);

	return;
}


static int set_i2c_reg(int fd, const u8 addr, const u8 reg,
		       const u8 val)
{
	int ret;
	u8 outbuf[2];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];

	messages[0].addr = addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(outbuf);
	messages[0].buf = outbuf;

	outbuf[0] = reg;
	outbuf[1] = val;

	packets.msgs = messages;
	packets.nmsgs = 1;

	ret = ioctl(fd, I2C_RDWR, &packets);
	if (ret < 0) {
		fprintf(stderr, "Failed to set register %u\n", reg);
		return ret;
	}

	usleep(5 * 1000);

	return 0;
}

static int get_i2c_reg(int fd, const u8 addr, const u8 reg, u8 *val)
{
	int ret;
	u8 outbuf, inbuf;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];

	messages[0].addr = addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(outbuf);
	messages[0].buf = &outbuf;

	outbuf = reg;

	messages[1].addr = addr;
	messages[1].flags = I2C_M_RD;
	messages[1].len = sizeof(inbuf);
	messages[1].buf = &inbuf;

	packets.msgs = messages;
	packets.nmsgs = 2;

	ret = ioctl(fd, I2C_RDWR, &packets);
	if (ret < 0) {
		fprintf(stderr, "Failed to get register %u\n", reg);
		return ret;
	}

	*val = inbuf;

	return 0;
}

void usrp_sulfur_eeprom_recrc(struct usrp_sulfur_eeprom *ep)
{
	ep->crc = htonl(crc32(0, &ep->magic, sizeof(*ep)-4));
}

void usrp_sulfur_eeprom_to_i2c(struct usrp_sulfur_eeprom *ep, const char *path)
{
	int fd;
	size_t len = sizeof(*ep);
	size_t i;

	fd = open(path, O_WRONLY | O_CREAT);
	if (fd < 0) {
		perror("Could not open file:\n");
		return;
	}

	u8 *rptr = (u8*)ep;

	for (i = 0; i < len; i++)
		set_i2c_reg(fd, EEPROM_I2C_ADDR, i, *(rptr+i));

	close(fd);

	return;
}

struct usrp_sulfur_db_eeprom *usrp_sulfur_db_eeprom_new(const u16 pid,
						     const u16 rev,
						     const char *serial,
						     const u16 dt_compat)
{
	struct usrp_sulfur_db_eeprom *ep;
	int i;

	ep = malloc(sizeof(*ep));
	if (!ep) {
		perror("Failed to allocate eeprom struct\n");
		return NULL;
	}

	/* it's an eeprom, so just make it 0xff */
	memset(ep, 0, sizeof(*ep));

	ep->magic = htonl(USRP_EEPROM_DB_MAGIC);
	ep->version = htonl(USRP_EEPROM_DB_VERSION);

	ep->pid = htons(pid);
	ep->rev.v2_rev.rev = rev;
	ep->rev.v2_rev.dt_compat = dt_compat;

	if (strlen(serial) > 8) {
		fprintf(stderr, "Serial# too long\n");
		free(ep);
		return NULL;
	}

	memset(ep->serial, '\0', 8);
	strncpy(ep->serial, serial, 8);

	ep->crc = htonl(crc32(0, &ep->magic, sizeof(*ep)-4));

	return ep;
}

struct usrp_sulfur_db_eeprom *usrp_sulfur_db_eeprom_from_file(const char *path)
{
	int fd;
	struct usrp_sulfur_db_eeprom *ep;
	size_t len = sizeof(*ep);
	size_t got = 0;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return NULL;
	}

	ep = malloc(sizeof(*ep));
	if (!ep) {
		perror("Could not allocate struct");
		return ep;
	}

	u8 *rptr = (u8*)ep;

	while(len) {
		got = read(fd, rptr, len);
		len -= got;
		rptr += got;
	}

	close(fd);
	if (ep->magic != ntohl(USRP_EEPROM_DB_MAGIC)) {
		free(ep);
		return NULL;
	}

	return ep;
}

void usrp_sulfur_db_eeprom_print(const struct usrp_sulfur_db_eeprom *ep)
{
	if (!ep) {
		fprintf(stderr, "ep not valid!\n");
		return;
	}

	if (ntohl(ep->version) == 1)
		printf("-- PID/REV: %04x %04x\n", ntohs(ep->pid), ntohs(ep->rev.v1_rev));
	else
		printf("-- PID/REV: %04x %02x\n", ntohs(ep->pid), ep->rev.v2_rev.rev);
	printf("-- Serial: %s\n", ep->serial);
	if (ntohl(ep->version) == 2)
		printf("-- DT-Compat: %02x\n", ep->rev.v2_rev.dt_compat);
	printf("-- CRC: %08x (%s)\n", ntohl(ep->crc),
	       __usrp_sulfur_db_eeprom_check_crc(ep) ? "matches": "doesn't match!");
}


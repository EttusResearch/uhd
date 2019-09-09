// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tlv_eeprom.h"
#include "tlv_eeprom_io.h"
#include "usrp_eeprom.h"

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((*x)))

int parse_board_info(int argc, char **argv, void *data)
{
	struct usrp_eeprom_board_info *info = data;

	if (!argc) {
		sprintf(data, "<pid> <rev> <compat_rev> <serial>");
		return 0;
	}

	assert(argc >= 4);

	info->pid = strtoul(argv[0], NULL, 0);
	info->rev = strtoul(argv[1], NULL, 0);
	info->compat_rev = strtoul(argv[2], NULL, 0);
	strncpy(info->serial, argv[3], 8);
	info->serial[7] = '\0';

	return 4;
}
static struct usrp_eeprom_board_info board_info;

int parse_module_info(int argc, char **argv, void *data)
{
	struct usrp_eeprom_module_info *info = data;

	if (!argc) {
		sprintf(data, "<pid> <rev> <serial>");
		return 0;
	}

	assert(argc >= 3);

	info->pid = strtoul(argv[0], NULL, 0);
	info->rev = strtoul(argv[1], NULL, 0);
	strncpy(info->serial, argv[2], 8);
	info->serial[7] = '\0';

	return 3;
}
static struct usrp_eeprom_module_info module_info;

int parse_mac(int argc, char **argv, void *data)
{
	struct usrp_eeprom_mac_addr *mac = data;
	int ret;

	if (!argc) {
		sprintf(data, "<mac_addr>");
		return 0;
	}

	assert(argc >= 1);

	ret = sscanf(*argv, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
	             mac->addr + 0, mac->addr + 1, mac->addr + 2,
	             mac->addr + 3, mac->addr + 4, mac->addr + 5);
	assert(ret == 6);

	return 1;
}
static struct usrp_eeprom_mac_addr mac_addrs[USRP_EEPROM_MAX_MAC_ADDRS];

int parse_db_pwr_seq(int argc, char **argv, void *data)
{
	struct usrp_eeprom_db_pwr_seq *seq = data;
	uint8_t i;

	if (!argc) {
		sprintf(data, "<nsteps> [delay supply_mask]...");
		return 0;
	}

	assert(argc >= 1);
	seq->nsteps = strtoul(argv[0], NULL, 0);
	assert(argc >= 1 + (2 * seq->nsteps));

	argv++;
	for (i = 0; i < seq->nsteps; i++) {
		seq->steps[i].delay = strtoul(*argv++, NULL, 0);
		seq->steps[i].supply_mask = strtoul(*argv++, NULL, 0);
	}

	return 1 + 2 * i;
}
static struct usrp_eeprom_db_pwr_seq db_pwr_seq;

int parse_mcu_flags(int argc, char **argv, void *data)
{
	struct usrp_eeprom_mcu_flags *flags = data;
	unsigned long tmp;
	int i;

	if (!argc) {
		sprintf(data, "<flags[0]> <flags[1]> <flags[2]> "
		              "<flags[3]> <flags[4]> <flags[5]>");
		return 0;
	}

	assert(argc >= ARRAY_SIZE(flags->flags));
	for (i = 0; i < ARRAY_SIZE(flags->flags); i++) {
		tmp = strtoul(argv[i], NULL, 0);
		assert(tmp <= 0xFF);

		flags->flags[i] = tmp;
	}

	return i;
}
static struct usrp_eeprom_mcu_flags mcu_flags;

int parse_fan_limits(int argc, char **argv, void *data)
{
	struct usrp_eeprom_fan_limits *fan_limits = data;
	int i;

	if (!argc) {
		sprintf(data, "<fan min rpm> <fan start rpm> <fan max rpm>");
		return 0;
	}

	assert(argc >= 3);
	fan_limits->min = strtoul(argv[0], NULL, 0);
	fan_limits->start = strtoul(argv[1], NULL, 0);
	fan_limits->max = strtoul(argv[2], NULL, 0);

	return 3;
}
static struct usrp_eeprom_fan_limits fan_limits;

int parse_fan_fixed_capacity(int argc, char **argv, void *data)
{
	struct usrp_eeprom_fan_fixed_capacity *fan_fixed_capacity = data;

	if (!argc) {
		sprintf(data, "<fan fixed capacity>");
		return 0;
	}

	fan_fixed_capacity->capacity = strtoul(argv[0], NULL, 0);

	return 1;
}
static struct usrp_eeprom_fan_fixed_capacity fan_fixed_capacity;

int parse_clkaux_tuning_word(int argc, char **argv, void *data)
{
	struct usrp_eeprom_clkaux_tuning_word *clkaux_tuning_word = data;

	if (!argc) {
		sprintf(data, "<dac tuning word>");
		return 0;
	}

	clkaux_tuning_word->tuning_word = strtoul(argv[0], NULL, 0);

	return 1;
}
static struct usrp_eeprom_clkaux_tuning_word clkaux_tuning_word;

struct arg_parser {
	const char *arg;
	const char *alias;
	void *store;
	size_t size;
	uint8_t tag;
	int (*parse)(int, char **, void *);
	int dirty;
};

#define PARSER_(name_, alias_, data_, tag_, parser_) { \
	.arg = #name_, \
	.alias = alias_, \
	.store = &(data_), \
	.size = sizeof((data_)), \
	.tag = (tag_), \
	.parse = (parser_), \
}

#define PARSER(param_, tag_) PARSER_(param_, NULL, param_, tag_, parse_ ##param_)
#define MAC_PARSER_ALIAS(idx_, alias_) PARSER_( mac_addr_ ## idx_, (alias_), mac_addrs[(idx_)], USRP_EEPROM_MAC_ADDR_TAG((idx_)), parse_mac)
#define MAC_PARSER(idx_) MAC_PARSER_ALIAS(idx_, NULL)

static struct arg_parser parsers[] = {
	PARSER(board_info, USRP_EEPROM_BOARD_INFO_TAG),
	PARSER(module_info, USRP_EEPROM_MODULE_INFO_TAG),
	MAC_PARSER_ALIAS(0, "eth0_mac"),
	MAC_PARSER_ALIAS(1, "qsfp0_mac"),
	MAC_PARSER_ALIAS(2, "qsfp1_mac"),
	MAC_PARSER(3),
	MAC_PARSER(4),
	MAC_PARSER(5),
	MAC_PARSER(6),
	MAC_PARSER(7),
	MAC_PARSER(8),
	MAC_PARSER(9),
	MAC_PARSER(10),
	MAC_PARSER(11),
	MAC_PARSER(12),
	PARSER(db_pwr_seq, USRP_EEPROM_DB_PWR_SEQ_TAG),
	PARSER(mcu_flags, USRP_EEPROM_MCU_FLAGS),
	PARSER(fan_limits, USRP_EEPROM_FAN_LIMITS),
	PARSER(fan_fixed_capacity, USRP_EEPROM_FAN_FIXED_CAPACITY),
	PARSER(clkaux_tuning_word, USRP_EEPROM_CLKAUX_TUNING_WORD),
};

static struct arg_parser *parser_lookup(const char *arg)
{
	for (size_t i = 0; i < ARRAY_SIZE(parsers); i++) {
		if (!strcmp(parsers[i].arg, arg))
			return parsers + i;
		if (parsers[i].alias && !strcmp(parsers[i].alias, arg))
			return parsers + i;
	}

	return NULL;
}

#ifdef TLV_EEPROM_UPDATE
static void tlv_update(uint8_t tag, uint8_t len, const void *val)
{
	struct arg_parser *parser = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(parsers); i++) {
		if (parsers[i].tag == tag)
			parser = parsers + i;
	}

	if (!parser) {
		fprintf(stderr, "found unknown tag %02x len %u", tag, len);
		fprintf(stderr, "cannot use this utility to update\n");
		exit(EXIT_FAILURE);
	}

	assert(len == parser->size);
	memcpy(parser->store, val, len);
	parser->dirty = 1;
}

static void handle_update(const char *filename)
{
	struct tlv_eeprom *eeprom;

	eeprom = tlv_eeprom_read_from_file(filename);
	if (!eeprom) {
		perror("failed to read");
		exit(EXIT_FAILURE);
	}

	if (tlv_eeprom_validate(eeprom, USRP_EEPROM_MAGIC)) {
		fprintf(stderr, "contents invalid, cannot update\n");
		exit(EXIT_FAILURE);
	}

	tlv_for_each(eeprom->tlv, eeprom->size, tlv_update);
	free(eeprom);
}
#else
static void handle_update(const char *filename)
{
}
#endif

static void validate_file_matches(const struct tlv_eeprom *eeprom,
				  const char *filename)
{
	struct tlv_eeprom *file_eeprom;

	file_eeprom = tlv_eeprom_read_from_file(filename);
	if (!file_eeprom) {
		perror("failed to read for validate");
		exit(EXIT_FAILURE);
	}

	if (memcmp(file_eeprom, eeprom, sizeof(*eeprom))) {
		fprintf(stderr, "eeprom validation failed! "
				"values read do not match what was written. "
				"perhaps the eeprom was write-protected?\n");
		exit(EXIT_FAILURE);
	}

	free(file_eeprom);
}

static void usage(const char *argv0)
{
	const struct arg_parser *p;
	char buffer[4096];

	fprintf(stderr, "usage: %s <output_file>\n", argv0);
	for (size_t i = 0; i < ARRAY_SIZE(parsers); i++) {
		p = parsers + i;
		p->parse(0, NULL, buffer);
		fprintf(stderr, "\t--%s %s\n", p->arg, buffer);
	}
}

int main(int argc, char **argv)
{
	struct arg_parser *parser;
	const char *output_file;
	struct tlv_eeprom eeprom;
	uint8_t *ptr;
	int ret;

	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	output_file = argv[1];
	handle_update(output_file);

	argc -= 2;
	argv += 2;

	while (argc > 0) {
		if (strncmp("--", *argv, 2)) {
			fprintf(stderr, "unexpected arg\n");
			return 1;
		}

		*argv += 2;
		parser = parser_lookup(*argv);
		if (!parser) {
			fprintf(stderr, "unknown type: %s\n", *argv);
			return 1;
		}

		argc--;
		argv++;

		ret = parser->parse(argc, argv, parser->store);
		if (ret < 0) {
			fprintf(stderr, "parsing failed\n");
			return 1;
		}
		parser->dirty = 1;

		argc -= ret;
		argv += ret;
	}

	/*
	 * Now that the data's been parsed, write back any changed fields
	 */
	memset(&eeprom, 0, sizeof(eeprom));
	ptr = eeprom.tlv;
	for (size_t i = 0; i < ARRAY_SIZE(parsers); i++) {
		parser = parsers + i;
		if (!parser->dirty)
			continue;
		ptr += tlv_write(ptr, parser->tag, parser->size, parser->store);
	}

	tlv_eeprom_seal(&eeprom, USRP_EEPROM_MAGIC, ptr - eeprom.tlv);
	tlv_eeprom_write_to_file(&eeprom, output_file);
	validate_file_matches(&eeprom, output_file);

	return 0;
}

/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "btk_help.h"
#include "mods/output.h"
#include "mods/opts.h"
#include "mods/error.h"

static void print_general_help(void) {
    printf("Bitcoin Toolkit (btk) - A command line tool for Bitcoin operations\n\n");
    printf("Usage: btk [command] [options]\n\n");
    printf("Commands:\n");
    printf("  privkey     Generate or manipulate private keys\n");
    printf("  pubkey      Generate or display public keys\n");
    printf("  address     Generate or validate Bitcoin addresses\n");
    printf("  vanity      Generate Bitcoin vanity addresses\n");
    printf("  node        Bitcoin node operations\n");
    printf("  balance     Check address balances\n");
    printf("  config      Configure toolkit settings\n");
    printf("  version     Show version information\n");
    printf("  help        Show this help message\n\n");
    printf("Use 'btk help [command]' for more information about a specific command\n");
}

static void print_command_help(const char* command) {
    if (strcmp(command, "privkey") == 0) {
        printf("btk privkey - Generate or manipulate private keys\n\n");
        printf("Usage: btk privkey [options]\n");
        printf("Options:\n");
        printf("  -C        Use compressed public key format\n");
        printf("  -U        Use uncompressed public key format\n");
    } else if (strcmp(command, "pubkey") == 0) {
        printf("btk pubkey - Generate or display public keys\n\n");
        printf("Usage: btk pubkey [options]\n");
        printf("Options:\n");
        printf("  -C        Use compressed format\n");
        printf("  -U        Use uncompressed format\n");
    } else if (strcmp(command, "address") == 0) {
        printf("btk address - Generate or validate Bitcoin addresses\n\n");
        printf("Usage: btk address [options]\n");
        printf("Options:\n");
        printf("  --bech32   Generate Bech32 address\n");
        printf("  --legacy   Generate Legacy address\n");
    } else if (strcmp(command, "vanity") == 0) {
        printf("btk vanity - Generate Bitcoin vanity addresses\n\n");
        printf("Usage: btk vanity [mode] [pattern] [options]\n\n");
        printf("Modes:\n");
        printf("  prefix    Match pattern at start of address\n");
        printf("  suffix    Match pattern at end of address\n");
        printf("  anywhere  Match pattern anywhere (default)\n\n");
        printf("Options:\n");
        printf("  -i        Case insensitive match (default)\n");
        printf("  -t N      Number of threads to use (default: 1)\n\n");
        printf("Examples:\n");
        printf("  btk vanity abc              # Match 'abc' anywhere\n");
        printf("  btk vanity prefix abc       # Match 'abc' at start\n");
        printf("  btk vanity suffix xyz       # Match 'xyz' at end\n");
        printf("  btk vanity -t 8 abc         # Use 8 threads\n");
    } else {
        printf("Unknown command '%s'. Use 'btk help' for a list of commands.\n", command);
    }
}

int btk_help_main(output_item *output, opts_p opts, unsigned char *input, size_t input_len)
{
	assert(opts);

	(void)input;
	(void)input_len;

	if (opts->input_count > 0) {
		print_command_help(opts->input[0]);
	} else {
		print_general_help();
	}

	// Add a dummy output item as required
	*output = output_append_new_copy(*output, "", 0);
	ERROR_CHECK_NULL(*output, "Memory allocation error.");
	
	return 1;
}

int btk_help_requires_input(opts_p opts)
{
	assert(opts);

	if (opts->input_count > 0)
	{
		return 1;
	}

	return 0;
}

int btk_help_init(opts_p opts)
{
	assert(opts);

	opts->output_format_binary = 1;

	return 1;
}

int btk_help_cleanup(opts_p opts)
{
	assert(opts);
	
	return 1;
}
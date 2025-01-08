/*
 * Copyright (c) 2023 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#ifndef BTK_VANITY_H
#define BTK_VANITY_H 1

#include "../mods/output.h"
#include "../mods/opts.h"

int btk_vanity_main(output_item *output, opts_p opts, unsigned char *input, size_t input_len);

#endif
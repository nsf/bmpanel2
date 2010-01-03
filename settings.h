#pragma once

#include "config-parser.h"

extern struct config_format_tree g_settings;

void load_settings(const char *configfile);
void free_settings();

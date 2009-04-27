#ifndef BMPANEL2_SETTINGS_H 
#define BMPANEL2_SETTINGS_H

#include "config-parser.h"

extern struct config_format_tree g_settings;

void load_settings();
void free_settings();

#endif /* BMPANEL2_SETTINGS_H */

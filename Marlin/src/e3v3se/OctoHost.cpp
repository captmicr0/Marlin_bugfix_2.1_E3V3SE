#include "../inc/MarlinConfig.h"
#include "OctoHost.h"

bool serial_connection_active = false;

#if ENABLED(OCTOPRINT_PLUGIN)

#include "../gcode/gcode.h"
#include "../gcode/parser.h"
#include "../lcd/marlinui.h"
#include "../lcd/dwin/creality/dwin.h"

#include <string.h>

char title[50] = {};

#if ENABLED(DWIN_RENDER_THUMBNAIL)
  uint32_t MarlinUI::print_time = 0;
#endif

// Accepts N"My title", N=My title or N My title; unquoted values stop at ' ', ';' or '*'.
static void copy_title(char *dst, const size_t dstlen, const char *src) {
  dst[0] = '\0';
  if (!src) return;

  while (*src == ' ' || *src == '\t') ++src;
  if (*src == 'N' || *src == 'n') {
    ++src;
    if (*src == '=' || *src == ':') ++src;
  }

  const bool quoted = (*src == '"');
  if (quoted) ++src;

  size_t i = 0;
  for (; *src && i < dstlen - 1; ++src) {
    if (quoted) {
      if (*src == '"') break;
      if (*src == '\\' && src[1] == '"') ++src;
    }
    else if (*src == ' ' || *src == '\t' || *src == ';' || *src == '*') break;
    dst[i++] = *src;
  }
  dst[i] = '\0';
}

/**
 * M9000: Plugin connection state and print details
 *  A1        Plugin connected (A0 disconnected)
 *  L<n>      Total layers
 *  T<min>    Elapsed print time in minutes
 *  P<0-100>  Progress
 *  N<title>  Job title
 *  S1        Open the print screen
 */
void GcodeSuite::M9000() {
  if (parser.seen('A')) {
    serial_connection_active = parser.value_bool();
    if (serial_connection_active) Goto_MainMenu();
  }

  #if ENABLED(DWIN_RENDER_THUMBNAIL)
    if (parser.seen('L')) ui.set_total_layers(parser.value_int());
  #endif

  #if ENABLED(DWIN_RENDER_THUMBNAIL)
    if (parser.seen('T')) ui.set_print_time(60UL * parser.value_ulong());
  #endif

  if (parser.seen('P'))
    ui.set_progress((PROGRESS_SCALE) > 1 ? parser.value_float() * (PROGRESS_SCALE) : parser.value_byte());

  if (parser.seen('N')) copy_title(title, sizeof(title), parser.string_arg);

  if (parser.boolval('S')) Goto_ThumbPrint();
}

/**
 * M9001: Plugin thumbnail transfer (START / C <line> <offset> <hex> / END)
 *
 * Only the handshake is implemented here; the pixel data is accepted and dropped
 * until the DWIN renderer for plugin thumbnails is ported.
 */
void GcodeSuite::M9001() {
  const char *p = parser.command_ptr;
  if (!p) return;

  while (*p && *p != ' ') ++p;
  while (*p == ' ') ++p;

  if (!strncmp(p, "START", 5)) SERIAL_ECHOLNPGM("M9001 START");
  else if (!strncmp(p, "END", 3)) {
    SERIAL_ECHOLNPGM("M9001 END");
    SERIAL_ECHOLNPGM("M9001 thumbnail-rendered");
  }
}

#endif // OCTOPRINT_PLUGIN

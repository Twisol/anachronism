#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "telnet_nvt.h"

%%{
  machine telnet_nvt;
  access nvt->;
  
  action flush_text {
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
  
  action char {
    if (nvt->text_callback)
      nvt->buf[nvt->buflen++] = fc;
  }
  
  action basic_command {
    if (nvt->command_callback)
      nvt->command_callback(nvt, fc);
  }

  action option_mark {
    nvt->option_mark= fc;
  }
  action option_command {
    if (nvt->option_callback)
      nvt->option_callback(nvt, nvt->option_mark, fc);
  }

  action subneg_command {
    if (nvt->mode_callback)
      nvt->mode_callback(nvt, TELNET_SUBNEG, fc);
  }
  action subneg_command_end {
    if (nvt->mode_callback)
      nvt->mode_callback(nvt, TELNET_TEXT, 0);
  }

  action warning_cr {
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "Invalid \\r: not followed by \\n or \\0.", fpc-data);
  }
  action warning_iac {
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "IAC followed by invalid command.", fpc-data);
  }
  
  include telnet_nvt_common "telnet_common.rl";
  write data;
}%%

int telnet_nvt_init(telnet_nvt* nvt)
{
  if (!nvt)
    return 0;
  
  memset(nvt, 0, sizeof(*nvt));
  %% write init;
  
  return 1;
}

int telnet_nvt_parse(telnet_nvt* nvt, const telnet_byte* data, size_t length)
{
  if (!nvt)
    return -1;
  
  // Only bother saving text if it'll be used
  if (nvt->text_callback)
  {
    // Because of how the parser translates data, a run of text is guaranteed to
    // be at most 'length' characters long. In practice it's usually less due to
    // escaped characters (IAC IAC -> IAC), and text separated by commands.
    nvt->buf = malloc(length * sizeof(*nvt->buf));
    nvt->buflen = 0;
  }
  
  const telnet_byte* p = data;
  const telnet_byte* pe = data + length;
  const telnet_byte* eof = pe;
  
  %% write exec;
  
  free(nvt->buf);
  nvt->buf = NULL;
  
  return p-data;
}

int telnet_nvt_text(telnet_nvt* nvt, const telnet_byte* data, size_t length)
{
  if (!(nvt && nvt->send_callback))
    return 0;
  return 1;
}

int telnet_nvt_command(telnet_nvt* nvt, const telnet_command command)
{
  if (!(nvt && nvt->send_callback))
    return 0;
  return 1;
}

int telnet_nvt_option(telnet_nvt* nvt, const telnet_command command, const telnet_byte option)
{
  if (!(nvt && nvt->send_callback))
    return 0;
  return 1;
}

int telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, size_t length)
{
  if (!(nvt && nvt->send_callback))
    return 0;
  return 1;
}

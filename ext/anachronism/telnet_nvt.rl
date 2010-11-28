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

int telnet_nvt_parse(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return -1;
  
  // Only bother saving text if it'll be used
  if (nvt->text_callback)
  {
    // Because of how the parser translates data, a run of text is guaranteed to
    // be at most 'length' characters long. In practice it's usually less, due to
    // escaped characters (IAC IAC -> IAC) and text separated by commands.
    nvt->buf = malloc(length * sizeof(*nvt->buf));
    if (!nvt->buf)
      return -1; // unable to allocate a buffer
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

int telnet_nvt_text(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return 0;
  else if (!nvt->send_callback)
    return 1; // immediate success since they apparently don't want the data to go anywhere
  
  // Due to the nature of the protocol, the most any one byte can be encoded as is two bytes.
  // Hence, the smallest buffer guaranteed to contain any input is double the length of the source.
  telnet_byte* buf = malloc(length * 2 * sizeof(*buf));
  if (!buf)
    return 0; // unable to allocate a buffer
  size_t buflen = 0;
  
  size_t left = 0;
  size_t right = 0;
  for (; right < length; ++right)
  {
    switch (data[right])
    {
      case '\r':
        memcpy(buf+buflen, data+left, right-left);
        buflen += right - left;
        left = right + 1;
        
        memcpy(buf+buflen, "\r\0", 2);
        buflen += 2;
        break;
      case '\n':
        memcpy(buf+buflen, data+left, right-left);
        buflen += right - left;
        left = right + 1;
        
        memcpy(buf+buflen, "\r\n", 2);
        buflen += 2;
        break;
      case 255u: // IAC byte
        memcpy(buf+buflen, data+left, right-left);
        buflen += right - left;
        left = right + 1;
        
        memcpy(buf+buflen, "\xFF\xFF", 2);
        buflen += 2;
        break;
    }
  }
  
  if (left < right)
  {
    memcpy(buf+buflen, data+left, right-left);
    buflen += right - left;
  }
  
  nvt->send_callback(nvt, buf, buflen);
  
  free(buf);
  buf = NULL;
  
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

int telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length)
{
  if (!(nvt && nvt->send_callback))
    return 0;
  return 1;
}

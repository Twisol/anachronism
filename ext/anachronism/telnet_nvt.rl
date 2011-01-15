#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "telnet_nvt.h"

struct telnet_nvt
{
  int cs; /* current Ragel state */
  const telnet_byte* p; /* current position */
  const telnet_byte* pe; /* end of current packet */
  const telnet_byte* eof; /* end-of-file marker */
  
  telnet_byte option_mark; /* temporary storage for a command byte */
  unsigned char options[256]; /* track the state of each subnegotiation option */
  
  telnet_byte* buf; /* Buffer to build up a stretch of text in. */
  size_t buflen; /* Length so far of the buffer. */
  
  telnet_callbacks callbacks;
  void* userdata;
};

%%{
  machine telnet_nvt;
  
  access nvt->;
  variable p nvt->p;
  variable pe nvt->pe;
  variable eof nvt->eof;
  
  action flush_text {
    if (nvt->callbacks.on_text && nvt->buflen > 0)
    {
      nvt->callbacks.on_text(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
  
  action char {
    if (nvt->callbacks.on_text)
      nvt->buf[nvt->buflen++] = fc;
  }
  
  action basic_command {
    if (nvt->callbacks.on_command)
      nvt->callbacks.on_command(nvt, fc);
  }

  action option_mark {
    nvt->option_mark= fc;
  }
  action option_command {
    if (nvt->callbacks.on_option)
      nvt->callbacks.on_option(nvt, nvt->option_mark, fc);
  }

  action subneg_command {
    if (nvt->callbacks.on_mode)
      nvt->callbacks.on_mode(nvt, TELNET_SUBNEG, fc);
  }
  action subneg_command_end {
    if (nvt->callbacks.on_mode)
      nvt->callbacks.on_mode(nvt, TELNET_TEXT, 0);
  }

  action warning_cr {
    if (nvt->callbacks.on_error)
      nvt->callbacks.on_error(nvt, TELNET_WARNING, "Invalid \\r: not followed by \\n or \\0.", fpc-data);
  }
  action warning_iac {
    if (nvt->callbacks.on_error)
      nvt->callbacks.on_error(nvt, TELNET_WARNING, "IAC followed by invalid command.", fpc-data);
  }
  
  include telnet_nvt_common "telnet_common.rl";
  write data;
}%%

telnet_nvt* telnet_nvt_new()
{
  telnet_nvt* nvt = malloc(sizeof(telnet_nvt));
  memset(nvt, 0, sizeof(*nvt));
  
  %% write init;
  
  return nvt;
}

void telnet_nvt_delete(telnet_nvt* nvt)
{
  free(nvt);
}

int telnet_nvt_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks)
{
  if (!nvt)
    return 0;
  
  *callbacks = &nvt->callbacks;
}

int telnet_nvt_set_userdata(telnet_nvt* nvt, void* userdata)
{
  if (!nvt)
    return 0;
  
  nvt->userdata = userdata;
  return 1;
}

int telnet_nvt_get_userdata(telnet_nvt* nvt, void** userdata)
{
  if (!nvt)
    return 0;
  
  *userdata = nvt->userdata;
  return 1;
}

int telnet_nvt_parse(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return -1;
  
  // Only bother saving text if it'll be used
  if (nvt->callbacks.on_text)
  {
    // Because of how the parser translates data, a run of text is guaranteed to
    // be at most 'length' characters long. In practice it's usually less, due to
    // escaped characters (IAC IAC -> IAC) and text separated by commands.
    nvt->buf = malloc(length * sizeof(*nvt->buf));
    if (!nvt->buf)
      return -1; // unable to allocate a buffer
    nvt->buflen = 0;
  }
  
  nvt->p = data;
  nvt->pe = data + length;
  nvt->eof = nvt->pe;
  
  %% write exec;
  
  size_t bytes_used = nvt->p - data;
  
  free(nvt->buf);
  nvt->buf = NULL;
  nvt->p = nvt->pe = nvt->eof = NULL;
  
  return bytes_used;
}

int telnet_nvt_text(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return 0;
  else if (!nvt->callbacks.on_send)
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
  
  nvt->callbacks.on_send(nvt, buf, buflen);
  
  free(buf);
  buf = NULL;
  
  return 1;
}

int telnet_nvt_command(telnet_nvt* nvt, const telnet_command command)
{
  static telnet_byte buf[] = {'\xFF', 0};
  
  if (!nvt)
    return 0;
  else if (!nvt->callbacks.on_send)
    return 1; // immediate success since they apparently don't want the data to go anywhere
  
  buf[1] = command;
  nvt->callbacks.on_send(nvt, (const telnet_byte*)buf, 2);
  
  return 1;
}

int telnet_nvt_option(telnet_nvt* nvt, const telnet_command command, const telnet_byte option)
{
  if (!(nvt && nvt->callbacks.on_send))
    return 0;
  return 1;
}

int telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length)
{
  if (!(nvt && nvt->callbacks.on_send))
    return 0;
  return 1;
}

int telnet_nvt_halt(telnet_nvt* nvt)
{
  if (!nvt)
    return 0;
  
  // Force the parser to stop where it's at.
  if (nvt->p)
    nvt->eof = nvt->pe = nvt->p + 1;
  
  return 1;
}

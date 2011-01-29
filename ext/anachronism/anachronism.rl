#include <stdlib.h>
#include <string.h>
#include "anachronism.h"

#define BASE_EV(ev, t) \
  (ev).type = TELNET_EV_##t

#define EV_DATA(ev, text, len) {\
  BASE_EV(ev, DATA);\
  (ev).text_event.data = (text);\
  (ev).text_event.length = (len);\
}

#define EV_COMMAND(ev, cmd) {\
  BASE_EV(ev, COMMAND);\
  (ev).command_event.command = (cmd);\
}

#define EV_OPTION(ev, cmd, opt) {\
  BASE_EV(ev, OPTION);\
  (ev).option_event.command = (cmd);\
  (ev).option_event.option = (opt);\
}

#define EV_SUBNEGOTIATION(ev, act, opt) {\
  BASE_EV(ev, SUBNEGOTIATION);\
  (ev).subnegotiation_event.active = (act);\
  (ev).subnegotiation_event.option = (opt);\
}

#define EV_WARNING(ev, msg, pos) {\
  BASE_EV(ev, WARNING);\
  (ev).warning_event.message = (msg);\
  (ev).warning_event.position = (pos);\
}


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
  int subnegotiating;
};

%%{
  machine telnet_parser;
  
  access nvt->;
  variable p nvt->p;
  variable pe nvt->pe;
  variable eof nvt->eof;
  
  action flush_text {
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
  
  action char {
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
      nvt->buf[nvt->buflen++] = fc;
  }
  
  action basic_command {
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_event ev;
      EV_COMMAND(ev, fc);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }

  action option_mark {
    nvt->option_mark= fc;
  }
  action option_command {
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_event ev;
      EV_OPTION(ev, nvt->option_mark, fc);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }

  action subneg_command {
    nvt->option_mark = fc;
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_event ev;
      EV_SUBNEGOTIATION(ev, 1, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
  action subneg_command_end {
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_event ev;
      EV_SUBNEGOTIATION(ev, 0, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }

  action warning_cr {
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", fpc-data);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
  action warning_iac {
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_event ev;
      EV_WARNING(ev, "IAC followed by invalid command.", fpc-data);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
  
  include telnet_parser_common "parser_common.rl";
  write data;
}%%

telnet_nvt* telnet_new_nvt()
{
  telnet_nvt* nvt = malloc(sizeof(telnet_nvt));
  if (nvt != NULL)
  {
    memset(nvt, 0, sizeof(*nvt));
    %% write init;
  }
  return nvt;
}

void telnet_free_nvt(telnet_nvt* nvt)
{
  free(nvt);
}

telnet_error telnet_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  *callbacks = &nvt->callbacks;
  return TELNET_E_OK;
}

telnet_error telnet_set_userdata(telnet_nvt* nvt, void* userdata)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  nvt->userdata = userdata;
  return TELNET_E_OK;
}

telnet_error telnet_get_userdata(telnet_nvt* nvt, void** userdata)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  *userdata = nvt->userdata;
  return TELNET_E_OK;
}

telnet_error telnet_recv(telnet_nvt* nvt, const telnet_byte* data, const size_t length, size_t* bytes_used)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  // Only bother saving text if it'll be used
  if (nvt->callbacks.on_recv)
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
  
  if (bytes_used != NULL)
    *bytes_used = nvt->p - data;
  
  free(nvt->buf);
  nvt->buf = NULL;
  nvt->p = nvt->pe = nvt->eof = NULL;
  
  return TELNET_E_OK;
}

telnet_error telnet_halt(telnet_nvt* nvt)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  // Force the parser to stop where it's at.
  if (nvt->p)
    nvt->eof = nvt->pe = nvt->p + 1;
  
  return TELNET_E_OK;
}


static int safe_concat(const telnet_byte* in, size_t inlen, telnet_byte* out, size_t outlen)
{
  // Copy as much as possible into the buffer.
  memcpy(out, in, (outlen < inlen) ? outlen : inlen);
  
  // true if everything could be copied, false otherwise
  return outlen >= inlen;
}

// Escapes any special characters in data, writing the result data to out.
// Returns -1 if not everything could be copied (and out is full).
// Otherwise returns the length of the data in out.
//
// To avoid potential -1 return values, pass in an out buffer double the length of the data buffer.
static size_t telnet_escape(const telnet_byte* data, size_t length, telnet_byte* out, size_t outsize)
{
  if (data == NULL || out == NULL)
    return 0;
  
  size_t outlen = 0;
  size_t left = 0;
  size_t right = 0;
  const char* seq = NULL;
  for (; right < length; ++right)
  {
    switch (data[right])
    {
      case '\r':
        seq = "\r\0";
        break;
      case '\n':
        seq = "\r\n";
        break;
      case IAC_IAC:
        seq = "\xFF\xFF";
        break;
      default:
        continue; // Move to the next character
    }
    
    // Add any normal data that hasn't been added yet.
    if (safe_concat(data+left, right-left, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += right - left;
    left = right + 1;
    
    // Add the escape sequence.
    if (safe_concat(seq, 2, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += 2;
  }
  
  // Add any leftover normal data.
  if (left < right)
  {
    if (safe_concat(data+left, right-left, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += right - left;
  }
  
  return outlen;
}

telnet_error telnet_send_data(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (!nvt->callbacks.on_send)
    return TELNET_E_OK; // immediate success since they apparently don't want the data to go anywhere
  
  // Due to the nature of the protocol, the most any one byte can be encoded as is two bytes.
  // Hence, the smallest buffer guaranteed to contain any input is double the length of the source.
  size_t bufsize = sizeof(telnet_byte) * length * 2;
  telnet_byte* buf = malloc(bufsize);
  if (!buf)
    return TELNET_E_ALLOC;
  
  bufsize = telnet_escape(data, length, buf, bufsize);
  nvt->callbacks.on_send(nvt, buf, bufsize);
  
  free(buf);
  buf = NULL;
  
  return TELNET_E_OK;
}

telnet_error telnet_send_command(telnet_nvt* nvt, const telnet_byte command)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (command >= IAC_SB || command == IAC_SE) 
    return TELNET_E_BAD_COMMAND; // Invalid command
  
  if (nvt->callbacks.on_send)
  {
    const telnet_byte buf[] = {IAC_IAC, command};
    nvt->callbacks.on_send(nvt, buf, 2);
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_send_option(telnet_nvt* nvt, const telnet_byte command, const telnet_byte option)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (command < IAC_WILL || command > IAC_DONT)
    return TELNET_E_BAD_COMMAND; // Invalid option command
  
  if (nvt->callbacks.on_send)
  {
    const telnet_byte buf[] = {IAC_IAC, command, option};
    nvt->callbacks.on_send(nvt, buf, 3);
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_send_subnegotiation_start(telnet_nvt* nvt, const telnet_byte option)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  
  if (nvt->callbacks.on_send)
  {
    const telnet_byte buf[] = {IAC_IAC, IAC_SB, option};
    nvt->callbacks.on_send(nvt, buf, 3);
  }

  nvt->subnegotiating = 1;
  return TELNET_E_OK;
}

telnet_error telnet_send_subnegotiation_end(telnet_nvt* nvt)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (!nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  
  if (nvt->callbacks.on_send)
  {
    static const telnet_byte buf[] = {IAC_IAC, IAC_SE};
    nvt->callbacks.on_send(nvt, buf, 2);
  }
  
  nvt->subnegotiating = 0;
  return TELNET_E_OK;
}

telnet_error telnet_send_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (!nvt->callbacks.on_send)
    return TELNET_E_OK;
  
  // length*2 is the maximum buffer size needed for an escaped string.
  // The extra five bytes are for the IAC, SB, <option>, IAC, and SE frame around the data.
  size_t bufsize = (sizeof(telnet_byte) * length * 2) + 5;
  telnet_byte* buf = malloc(bufsize);
  if (!buf)
    return TELNET_E_ALLOC;
  
  // Begin with IAC SB <option>
  telnet_byte iac[] = {IAC_IAC, IAC_SB, option};
  memcpy(buf, iac, 3);
  
  // Add the subnegotiation body
  size_t escaped_length = telnet_escape(data, length, buf+3, bufsize-3) + 3;
  
  // End with IAC SE
  iac[1] = IAC_SE;
  memcpy(buf+escaped_length, iac, 2);
  
  nvt->callbacks.on_send(nvt, buf, escaped_length + 2);
  
  return TELNET_E_OK;
}

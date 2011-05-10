
#line 1 "src/anachronism.rl"
#include <stdlib.h>
#include <string.h>
#include "anachronism.h"

#define BASE_EV(ev, t) \
  (ev).SUPER_.type = TELNET_EV_##t

#define EV_DATA(ev, text, len) {\
  BASE_EV(ev, DATA);\
  (ev).data = (text);\
  (ev).length = (len);\
}

#define EV_COMMAND(ev, cmd) {\
  BASE_EV(ev, COMMAND);\
  (ev).command = (cmd);\
}

#define EV_OPTION(ev, cmd, opt) {\
  BASE_EV(ev, OPTION);\
  (ev).command = (cmd);\
  (ev).option = (opt);\
}

#define EV_SUBNEGOTIATION(ev, act, opt) {\
  BASE_EV(ev, SUBNEGOTIATION);\
  (ev).active = (act);\
  (ev).option = (opt);\
}

#define EV_WARNING(ev, msg, pos) {\
  BASE_EV(ev, WARNING);\
  (ev).message = (msg);\
  (ev).position = (pos);\
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


#line 60 "src/anachronism.c"
static const int telnet_parser_start = 7;
static const int telnet_parser_first_final = 7;
static const int telnet_parser_error = -1;

static const int telnet_parser_en_main = 7;


#line 137 "src/anachronism.rl"


telnet_nvt* telnet_new_nvt()
{
  telnet_nvt* nvt = malloc(sizeof(telnet_nvt));
  if (nvt != NULL)
  {
    memset(nvt, 0, sizeof(*nvt));
    
#line 78 "src/anachronism.c"
	{
	 nvt->cs = telnet_parser_start;
	}

#line 146 "src/anachronism.rl"
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
  
  
#line 142 "src/anachronism.c"
	{
	if ( ( nvt->p) == ( nvt->pe) )
		goto _test_eof;
	switch (  nvt->cs )
	{
tr1:
#line 6 "src/parser_common.rl"
	{( nvt->p)--;}
#line 118 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
	goto st7;
tr2:
#line 74 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st7;
tr3:
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
#line 79 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_command_event ev;
      EV_COMMAND(ev, (*( nvt->p)));
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
	goto st7;
tr12:
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
#line 6 "src/parser_common.rl"
	{( nvt->p)--;}
#line 126 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "IAC followed by invalid command.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
#line 109 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 0, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
	goto st7;
tr13:
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
#line 109 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 0, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
	goto st7;
tr14:
#line 91 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_option_event ev;
      EV_OPTION(ev, nvt->option_mark, (*( nvt->p)));
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
	goto st7;
st7:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof7;
case 7:
#line 267 "src/anachronism.c"
	switch( (*( nvt->p)) ) {
		case 13u: goto tr15;
		case 255u: goto st1;
	}
	goto tr2;
tr15:
#line 74 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st0;
st0:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof0;
case 0:
#line 284 "src/anachronism.c"
	switch( (*( nvt->p)) ) {
		case 0u: goto st7;
		case 10u: goto tr2;
	}
	goto tr1;
st1:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof1;
case 1:
	switch( (*( nvt->p)) ) {
		case 250u: goto tr4;
		case 255u: goto tr2;
	}
	if ( 251u <= (*( nvt->p)) && (*( nvt->p)) <= 254u )
		goto tr5;
	goto tr3;
tr4:
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
	goto st2;
st2:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof2;
case 2:
#line 317 "src/anachronism.c"
	goto tr6;
tr11:
#line 6 "src/parser_common.rl"
	{( nvt->p)--;}
#line 118 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
	goto st3;
tr7:
#line 74 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st3;
tr6:
#line 100 "src/anachronism.rl"
	{
    nvt->option_mark = (*( nvt->p));
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 1, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
    }
  }
	goto st3;
st3:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof3;
case 3:
#line 365 "src/anachronism.c"
	switch( (*( nvt->p)) ) {
		case 13u: goto tr8;
		case 255u: goto st5;
	}
	goto tr7;
tr8:
#line 74 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buf != NULL)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st4;
st4:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof4;
case 4:
#line 382 "src/anachronism.c"
	switch( (*( nvt->p)) ) {
		case 0u: goto st3;
		case 10u: goto tr7;
	}
	goto tr11;
st5:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof5;
case 5:
	switch( (*( nvt->p)) ) {
		case 240u: goto tr13;
		case 255u: goto tr7;
	}
	goto tr12;
tr5:
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
#line 88 "src/anachronism.rl"
	{
    nvt->option_mark= (*( nvt->p));
  }
	goto st6;
st6:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof6;
case 6:
#line 417 "src/anachronism.c"
	goto tr14;
	}
	_test_eof7:  nvt->cs = 7; goto _test_eof; 
	_test_eof0:  nvt->cs = 0; goto _test_eof; 
	_test_eof1:  nvt->cs = 1; goto _test_eof; 
	_test_eof2:  nvt->cs = 2; goto _test_eof; 
	_test_eof3:  nvt->cs = 3; goto _test_eof; 
	_test_eof4:  nvt->cs = 4; goto _test_eof; 
	_test_eof5:  nvt->cs = 5; goto _test_eof; 
	_test_eof6:  nvt->cs = 6; goto _test_eof; 

	_test_eof: {}
	if ( ( nvt->p) == ( nvt->eof) )
	{
	switch (  nvt->cs ) {
	case 7: 
#line 64 "src/anachronism.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_text_event ev;
      EV_DATA(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, (telnet_event*)&ev);
      nvt->buflen = 0;
    }
  }
	break;
#line 445 "src/anachronism.c"
	}
	}

	}

#line 204 "src/anachronism.rl"
  
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
      case IAC_IAC:
        seq = "\xFF\xFF";
        break;
      case '\r':
        // Only escape \r if it doesn't immediately precede \n.
        if (right + 1 >= length || data[right+1] != '\n')
        {
          seq = "\r\0";
          break;
        }
        // !!FALLTHROUGH!!
      default:
        continue; // Move to the next character
    }
    
    // Add any normal data that hasn't been added yet.
    if (safe_concat(data+left, right-left, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += right - left;
    left = right + 1;
    
    // Add the escape sequence.
    if (safe_concat((const telnet_byte*)seq, 2, out+outlen, outsize-outlen) == 0)
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

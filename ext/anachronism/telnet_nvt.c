
#line 1 "ext/anachronism/telnet_nvt.rl"
#include <stdlib.h>
#include <string.h>
#include "telnet_nvt.h"

#define BASE_EV(ev, t) \
  (ev).type = TELNET_EV_##t

#define EV_TEXT(ev, text, len) {\
  BASE_EV(ev, TEXT);\
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


#line 60 "ext/anachronism/telnet_nvt.c"
static const int telnet_nvt_start = 7;
static const int telnet_nvt_first_final = 7;
static const int telnet_nvt_error = -1;

static const int telnet_nvt_en_main = 7;


#line 137 "ext/anachronism/telnet_nvt.rl"


telnet_nvt* telnet_nvt_new()
{
  telnet_nvt* nvt = malloc(sizeof(telnet_nvt));
  memset(nvt, 0, sizeof(*nvt));
  
  
#line 77 "ext/anachronism/telnet_nvt.c"
	{
	 nvt->cs = telnet_nvt_start;
	}

#line 145 "ext/anachronism/telnet_nvt.rl"
  
  return nvt;
}

void telnet_nvt_delete(telnet_nvt* nvt)
{
  free(nvt);
}

telnet_error telnet_nvt_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  *callbacks = &nvt->callbacks;
  return TELNET_E_OK;
}

telnet_error telnet_nvt_set_userdata(telnet_nvt* nvt, void* userdata)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  nvt->userdata = userdata;
  return TELNET_E_OK;
}

telnet_error telnet_nvt_get_userdata(telnet_nvt* nvt, void** userdata)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  *userdata = nvt->userdata;
  return TELNET_E_OK;
}

telnet_error telnet_nvt_recv(telnet_nvt* nvt, const telnet_byte* data, const size_t length, size_t* bytes_used)
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
  
  
#line 141 "ext/anachronism/telnet_nvt.c"
	{
	if ( ( nvt->p) == ( nvt->pe) )
		goto _test_eof;
	switch (  nvt->cs )
	{
tr1:
#line 6 "ext/anachronism/telnet_nvt.rl"
	{( nvt->p)--;}
#line 118 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
	goto st7;
tr2:
#line 74 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st7;
tr3:
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
#line 126 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_WARNING(ev, "IAC followed by invalid command.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
#line 79 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_COMMAND(ev, (*( nvt->p)));
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
	goto st7;
tr4:
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
#line 79 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_COMMAND(ev, (*( nvt->p)));
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
	goto st7;
tr13:
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
#line 6 "ext/anachronism/telnet_nvt.rl"
	{( nvt->p)--;}
#line 126 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_WARNING(ev, "IAC followed by invalid command.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
#line 109 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_SUBNEGOTIATION(ev, 0, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
	goto st7;
tr14:
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
#line 109 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_SUBNEGOTIATION(ev, 0, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
	goto st7;
tr15:
#line 91 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_OPTION(ev, nvt->option_mark, (*( nvt->p)));
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
	goto st7;
st7:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof7;
case 7:
#line 296 "ext/anachronism/telnet_nvt.c"
	switch( (*( nvt->p)) ) {
		case 13u: goto tr16;
		case 255u: goto st1;
	}
	goto tr2;
tr16:
#line 74 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st0;
st0:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof0;
case 0:
#line 313 "ext/anachronism/telnet_nvt.c"
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
		case 250u: goto tr5;
		case 255u: goto tr2;
	}
	if ( (*( nvt->p)) > 249u ) {
		if ( 251u <= (*( nvt->p)) && (*( nvt->p)) <= 254u )
			goto tr6;
	} else if ( (*( nvt->p)) >= 241u )
		goto tr4;
	goto tr3;
tr5:
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
	goto st2;
st2:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof2;
case 2:
#line 349 "ext/anachronism/telnet_nvt.c"
	goto tr7;
tr12:
#line 6 "ext/anachronism/telnet_nvt.rl"
	{( nvt->p)--;}
#line 118 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", ( nvt->p)-data);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
	goto st3;
tr8:
#line 74 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st3;
tr7:
#line 100 "ext/anachronism/telnet_nvt.rl"
	{
    nvt->option_mark = (*( nvt->p));
    if (nvt->callbacks.on_recv)
    {
      telnet_event ev;
      EV_SUBNEGOTIATION(ev, 1, nvt->option_mark);
      nvt->callbacks.on_recv(nvt, &ev);
    }
  }
	goto st3;
st3:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof3;
case 3:
#line 397 "ext/anachronism/telnet_nvt.c"
	switch( (*( nvt->p)) ) {
		case 13u: goto tr9;
		case 255u: goto st5;
	}
	goto tr8;
tr9:
#line 74 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv)
      nvt->buf[nvt->buflen++] = (*( nvt->p));
  }
	goto st4;
st4:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof4;
case 4:
#line 414 "ext/anachronism/telnet_nvt.c"
	switch( (*( nvt->p)) ) {
		case 0u: goto st3;
		case 10u: goto tr8;
	}
	goto tr12;
st5:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof5;
case 5:
	switch( (*( nvt->p)) ) {
		case 240u: goto tr14;
		case 255u: goto tr8;
	}
	goto tr13;
tr6:
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
#line 88 "ext/anachronism/telnet_nvt.rl"
	{
    nvt->option_mark= (*( nvt->p));
  }
	goto st6;
st6:
	if ( ++( nvt->p) == ( nvt->pe) )
		goto _test_eof6;
case 6:
#line 449 "ext/anachronism/telnet_nvt.c"
	goto tr15;
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
#line 64 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->callbacks.on_recv && nvt->buflen > 0)
    {
      telnet_event ev;
      EV_TEXT(ev, nvt->buf, nvt->buflen);
      nvt->callbacks.on_recv(nvt, &ev);
      nvt->buflen = 0;
    }
  }
	break;
#line 477 "ext/anachronism/telnet_nvt.c"
	}
	}

	}

#line 203 "ext/anachronism/telnet_nvt.rl"
  
  if (bytes_used != NULL)
    *bytes_used = nvt->p - data;
  
  free(nvt->buf);
  nvt->buf = NULL;
  nvt->p = nvt->pe = nvt->eof = NULL;
  
  return TELNET_E_OK;
}

telnet_error telnet_nvt_halt(telnet_nvt* nvt)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  // Force the parser to stop where it's at.
  if (nvt->p)
    nvt->eof = nvt->pe = nvt->p + 1;
  
  return TELNET_E_OK;
}

telnet_error telnet_nvt_text(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (!nvt->callbacks.on_send)
    return TELNET_E_OK; // immediate success since they apparently don't want the data to go anywhere
  
  // Due to the nature of the protocol, the most any one byte can be encoded as is two bytes.
  // Hence, the smallest buffer guaranteed to contain any input is double the length of the source.
  telnet_byte* buf = malloc(length * 2 * sizeof(*buf));
  if (!buf)
    return TELNET_E_ALLOC;
  size_t buflen = 0;
  
  // Find the 'special' characters and escape them properly
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
    
    memcpy(buf+buflen, data+left, right-left);
    buflen += right - left;
    left = right + 1;
    
    memcpy(buf+buflen, seq, 2);
    buflen += 2;
  }
  
  if (left < right)
  {
    memcpy(buf+buflen, data+left, right-left);
    buflen += right - left;
  }
  
  nvt->callbacks.on_send(nvt, buf, buflen);
  
  free(buf);
  buf = NULL;
  
  return TELNET_E_OK;
}

telnet_error telnet_nvt_command(telnet_nvt* nvt, const telnet_command command)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if ((command >= IAC_SB && command <= IAC_IAC) || command == IAC_SE) 
    return TELNET_E_BAD_COMMAND; // Invalid command
  
  if (nvt->callbacks.on_send)
  {
    const telnet_byte buf[] = {IAC_IAC, (telnet_byte)command};
    nvt->callbacks.on_send(nvt, buf, 2);
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_nvt_option(telnet_nvt* nvt, const telnet_command command, const telnet_byte option)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (command < IAC_WILL || command > IAC_DONT)
    return TELNET_E_BAD_COMMAND; // Invalid option command
  
  if (nvt->callbacks.on_send)
  {
    const telnet_byte buf[] = {IAC_IAC, (telnet_byte)command, option};
    nvt->callbacks.on_send(nvt, buf, 3);
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_nvt_begin_subnegotiation(telnet_nvt* nvt, const telnet_byte option)
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

telnet_error telnet_nvt_end_subnegotiation(telnet_nvt* nvt)
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

telnet_error telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  
  if (nvt->callbacks.on_send)
  {
    telnet_nvt_start_subnegotiation(nvt, option);
    if (telnet_nvt_text(nvt, data, length) == TELNET_E_ALLOC)
      return TELNET_E_ALLOC;
    telnet_nvt_end_subnegotiation(nvt);
  }
  
  return TELNET_E_OK;
}

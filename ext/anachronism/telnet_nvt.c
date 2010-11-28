
#line 1 "ext/anachronism/telnet_nvt.rl"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "telnet_nvt.h"


#line 10 "ext/anachronism/telnet_nvt.c"
static const int telnet_nvt_start = 7;
static const int telnet_nvt_first_final = 7;
static const int telnet_nvt_error = -1;

static const int telnet_nvt_en_main = 7;


#line 56 "ext/anachronism/telnet_nvt.rl"


int telnet_nvt_init(telnet_nvt* nvt)
{
  if (!nvt)
    return 0;
  
  memset(nvt, 0, sizeof(*nvt));
  
#line 28 "ext/anachronism/telnet_nvt.c"
	{
	 nvt->cs = telnet_nvt_start;
	}

#line 65 "ext/anachronism/telnet_nvt.rl"
  
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
  
  
#line 60 "ext/anachronism/telnet_nvt.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  nvt->cs )
	{
tr1:
#line 6 "ext/anachronism/telnet_nvt.rl"
	{p--;}
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
#line 45 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "Invalid \\r: not followed by \\n or \\0.", p-data);
  }
	goto st7;
tr2:
#line 18 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->buf[nvt->buflen++] = (*p);
  }
	goto st7;
tr3:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
#line 49 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "IAC followed by invalid command.", p-data);
  }
#line 23 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->command_callback)
      nvt->command_callback(nvt, (*p));
  }
	goto st7;
tr4:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
#line 23 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->command_callback)
      nvt->command_callback(nvt, (*p));
  }
	goto st7;
tr14:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
	goto st7;
tr13:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
#line 6 "ext/anachronism/telnet_nvt.rl"
	{p--;}
#line 49 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "IAC followed by invalid command.", p-data);
  }
	goto st7;
tr15:
#line 31 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->option_callback)
      nvt->option_callback(nvt, nvt->option_mark, (*p));
  }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 163 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 13u: goto tr16;
		case 255u: goto st1;
	}
	goto tr2;
tr16:
#line 18 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->buf[nvt->buflen++] = (*p);
  }
	goto st0;
st0:
	if ( ++p == pe )
		goto _test_eof0;
case 0:
#line 180 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 0u: goto st7;
		case 10u: goto tr2;
	}
	goto tr1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 250u: goto tr5;
		case 255u: goto tr2;
	}
	if ( (*p) > 249u ) {
		if ( 251u <= (*p) && (*p) <= 254u )
			goto tr6;
	} else if ( (*p) >= 241u )
		goto tr4;
	goto tr3;
tr5:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 214 "ext/anachronism/telnet_nvt.c"
	goto tr7;
tr12:
#line 6 "ext/anachronism/telnet_nvt.rl"
	{p--;}
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
#line 45 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "Invalid \\r: not followed by \\n or \\0.", p-data);
  }
	goto st3;
tr8:
#line 18 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->buf[nvt->buflen++] = (*p);
  }
	goto st3;
tr7:
#line 36 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->mode_callback)
      nvt->mode_callback(nvt, TELNET_SUBNEG, (*p));
  }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 251 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 13u: goto tr9;
		case 255u: goto st5;
	}
	goto tr8;
tr9:
#line 18 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->buf[nvt->buflen++] = (*p);
  }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 268 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 0u: goto st3;
		case 10u: goto tr8;
	}
	goto tr12;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	switch( (*p) ) {
		case 240u: goto tr14;
		case 255u: goto tr8;
	}
	goto tr13;
tr6:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
#line 28 "ext/anachronism/telnet_nvt.rl"
	{
    nvt->option_mark= (*p);
  }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 301 "ext/anachronism/telnet_nvt.c"
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
	if ( p == eof )
	{
	switch (  nvt->cs ) {
	case 7: 
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->buflen > 0)
    {
      nvt->text_callback(nvt, nvt->buf, nvt->buflen);
      nvt->buflen = 0;
    }
  }
	break;
#line 327 "ext/anachronism/telnet_nvt.c"
	}
	}

	}

#line 91 "ext/anachronism/telnet_nvt.rl"
  
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
  static telnet_byte buf[] = {'\xFF', 0};
  
  if (!nvt)
    return 0;
  else if (!nvt->send_callback)
    return 1; // immediate success since they apparently don't want the data to go anywhere
  
  buf[1] = command;
  nvt->send_callback(nvt, (const telnet_byte*)buf, 2);
  
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

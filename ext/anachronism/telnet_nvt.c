
#line 1 "ext/anachronism/telnet_nvt.rl"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "telnet_nvt.h"


#line 10 "ext/anachronism/telnet_nvt.c"
static const int telnet_nvt_start = 9;
static const int telnet_nvt_first_final = 9;
static const int telnet_nvt_error = 0;

static const int telnet_nvt_en_main = 9;


#line 57 "ext/anachronism/telnet_nvt.rl"


int telnet_nvt_init(telnet_nvt* nvt)
{
  if (!nvt)
    return 0;
  
  memset(nvt, 0, sizeof(*nvt));
  
#line 28 "ext/anachronism/telnet_nvt.c"
	{
	 nvt->cs = telnet_nvt_start;
	}

#line 66 "ext/anachronism/telnet_nvt.rl"
  
  return 1;
}

int telnet_nvt_parse(telnet_nvt* nvt, const telnet_byte* data, size_t length)
{
  if (!nvt)
    return -1;
  
  nvt->left = data;
  
  const telnet_byte* p = data;
  const telnet_byte* pe = data + length;
  const telnet_byte* eof = pe;
  
  
#line 50 "ext/anachronism/telnet_nvt.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  nvt->cs )
	{
tr1:
#line 53 "ext/anachronism/telnet_nvt.rl"
	{p--;}
#line 46 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "Invalid \\r: not followed by \\n or \\0.", p-data);
  }
	goto st9;
tr2:
#line 19 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->text_callback(nvt, p, 1);
  }
	goto st9;
tr3:
#line 50 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "IAC followed by invalid command.", p-data);
  }
#line 24 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->command_callback)
      nvt->command_callback(nvt, (*p));
  }
	goto st9;
tr4:
#line 24 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->command_callback)
      nvt->command_callback(nvt, (*p));
  }
	goto st9;
tr18:
#line 41 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->mode_callback)
      nvt->mode_callback(nvt, TELNET_TEXT, 0);
  }
	goto st9;
tr20:
#line 32 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->option_callback)
      nvt->option_callback(nvt, nvt->option_mark, (*p));
  }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 109 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 13u: goto tr22;
		case 255u: goto st2;
	}
	goto tr21;
tr21:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    nvt->left = p;
  }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 125 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 13u: goto tr25;
		case 255u: goto tr26;
	}
	goto st10;
tr22:
#line 19 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->text_callback(nvt, p, 1);
  }
	goto st1;
tr25:
#line 14 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->left != p)
      nvt->text_callback(nvt, nvt->left, p - nvt->left);
  }
#line 19 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->text_callback(nvt, p, 1);
  }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 154 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 0u: goto st9;
		case 10u: goto tr2;
	}
	goto tr1;
tr26:
#line 14 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->left != p)
      nvt->text_callback(nvt, nvt->left, p - nvt->left);
  }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 171 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 250u: goto st3;
		case 255u: goto tr2;
	}
	if ( (*p) > 249u ) {
		if ( 251u <= (*p) && (*p) <= 254u )
			goto tr6;
	} else if ( (*p) >= 241u )
		goto tr4;
	goto tr3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	goto tr7;
tr15:
#line 53 "ext/anachronism/telnet_nvt.rl"
	{p--;}
#line 46 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "Invalid \\r: not followed by \\n or \\0.", p-data);
  }
	goto st4;
tr16:
#line 19 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->text_callback(nvt, p, 1);
  }
	goto st4;
tr7:
#line 37 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->mode_callback)
      nvt->mode_callback(nvt, TELNET_SUBNEG, (*p));
  }
	goto st4;
tr17:
#line 50 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->error_callback)
      nvt->error_callback(nvt, TELNET_WARNING, "IAC followed by invalid command.", p-data);
  }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 221 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 13u: goto tr9;
		case 255u: goto st7;
	}
	goto tr8;
tr8:
#line 10 "ext/anachronism/telnet_nvt.rl"
	{
    nvt->left = p;
  }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 237 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 13u: goto tr12;
		case 255u: goto tr13;
	}
	goto st5;
tr9:
#line 19 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->text_callback(nvt, p, 1);
  }
	goto st6;
tr12:
#line 14 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->left != p)
      nvt->text_callback(nvt, nvt->left, p - nvt->left);
  }
#line 19 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback)
      nvt->text_callback(nvt, p, 1);
  }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 266 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 0u: goto st4;
		case 10u: goto tr16;
	}
	goto tr15;
tr13:
#line 14 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->left != p)
      nvt->text_callback(nvt, nvt->left, p - nvt->left);
  }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 283 "ext/anachronism/telnet_nvt.c"
	switch( (*p) ) {
		case 240u: goto tr18;
		case 255u: goto st0;
	}
	goto tr17;
st0:
 nvt->cs = 0;
	goto _out;
tr6:
#line 29 "ext/anachronism/telnet_nvt.rl"
	{
    nvt->option_mark= (*p);
  }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 302 "ext/anachronism/telnet_nvt.c"
	goto tr20;
	}
	_test_eof9:  nvt->cs = 9; goto _test_eof; 
	_test_eof10:  nvt->cs = 10; goto _test_eof; 
	_test_eof1:  nvt->cs = 1; goto _test_eof; 
	_test_eof2:  nvt->cs = 2; goto _test_eof; 
	_test_eof3:  nvt->cs = 3; goto _test_eof; 
	_test_eof4:  nvt->cs = 4; goto _test_eof; 
	_test_eof5:  nvt->cs = 5; goto _test_eof; 
	_test_eof6:  nvt->cs = 6; goto _test_eof; 
	_test_eof7:  nvt->cs = 7; goto _test_eof; 
	_test_eof8:  nvt->cs = 8; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch (  nvt->cs ) {
	case 10: 
#line 14 "ext/anachronism/telnet_nvt.rl"
	{
    if (nvt->text_callback && nvt->left != p)
      nvt->text_callback(nvt, nvt->left, p - nvt->left);
  }
	break;
#line 327 "ext/anachronism/telnet_nvt.c"
	}
	}

	_out: {}
	}

#line 82 "ext/anachronism/telnet_nvt.rl"
  
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

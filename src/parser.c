
#line 1 "src/parser.rl"
#include <string.h>
#include <anachronism/parser.h>

#define BASE_EV(ev, t) \
  (ev).SUPER_.type = TELNET_EV_##t

#define EV_DATA(ev, text, len) do {\
  BASE_EV(ev, DATA);\
  (ev).data = (text);\
  (ev).length = (len);\
} while (0)

#define EV_COMMAND(ev, cmd) do {\
  BASE_EV(ev, COMMAND);\
  (ev).command = (cmd);\
} while (0)

#define EV_OPTION(ev, cmd, opt) do {\
  BASE_EV(ev, OPTION);\
  (ev).command = (cmd);\
  (ev).option = (opt);\
} while (0)

#define EV_SUBNEGOTIATION(ev, act, opt) do {\
  BASE_EV(ev, SUBNEGOTIATION);\
  (ev).active = (act);\
  (ev).option = (opt);\
} while (0)

#define EV_WARNING(ev, msg, pos) do {\
  BASE_EV(ev, WARNING);\
  (ev).message = (msg);\
  (ev).position = (pos);\
} while (0)

struct telnet_parser {
  int cs; /* current Ragel state */
  const telnet_byte* p; /* current position */
  const telnet_byte* pe; /* end of current packet */
  const telnet_byte* eof; /* end-of-file marker */
  
  telnet_byte option_mark; /* temporary storage for a command byte */
  telnet_byte* buf; /* Buffer to build up a stretch of text in. */
  size_t buflen; /* Length so far of the buffer. */
  unsigned char interrupted; /* Flag for interrupts */
  
  telnet_parser_callback callback; /* Receiver of Telnet events*/
  void* userdata; /* Context for parser callback */
};


#line 55 "src/parser.c"
static const int telnet_parser_start = 8;
static const int telnet_parser_first_final = 8;
static const int telnet_parser_error = 0;

static const int telnet_parser_en_main = 8;


#line 132 "src/parser.rl"


telnet_parser* telnet_parser_new(telnet_parser_callback callback,
                                 void* userdata)
{
  telnet_parser* parser = malloc(sizeof(telnet_parser));
  if (parser)
  {
    memset(parser, 0, sizeof(*parser));
    
#line 74 "src/parser.c"
	{
	 parser->cs = telnet_parser_start;
	}

#line 142 "src/parser.rl"
    parser->callback = callback;
    parser->userdata = userdata;
  }
  return parser;
}

void telnet_parser_free(telnet_parser* parser)
{
  free(parser);
}

telnet_error telnet_parser_get_userdata(telnet_parser* parser, void** userdata)
{
  if (!parser)
    return TELNET_E_BAD_PARSER;
  
  *userdata = parser->userdata;
  return TELNET_E_OK;
}

telnet_error telnet_parser_parse(telnet_parser* parser,
                                 const telnet_byte* data,
                                 size_t length,
                                 size_t* bytes_used)
{
  if (!parser)
    return TELNET_E_BAD_PARSER;
  
  // Reset the interrupt flag
  parser->interrupted = 0;
  
  // Only bother saving text if it'll be used
  if (parser->callback)
  {
    // Because of how the parser translates data, a run of text is guaranteed to
    // be at most 'length' characters long. In practice it's usually less, due to
    // escaped characters (IAC IAC -> IAC) and text separated by commands.
    parser->buf = malloc(length * sizeof(*parser->buf));
    if (!parser->buf)
      return TELNET_E_ALLOC;
    parser->buflen = 0;
  }
  
  parser->p = data;
  parser->pe = data + length;
  parser->eof = parser->pe;
  
  
#line 128 "src/parser.c"
	{
	if ( ( parser->p) == ( parser->pe) )
		goto _test_eof;
	switch (  parser->cs )
	{
tr1:
#line 6 "src/parser_common.rl"
	{( parser->p)--;}
#line 113 "src/parser.rl"
	{
    if (parser->callback && parser->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", ( parser->p)-data);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
	goto st8;
tr2:
#line 69 "src/parser.rl"
	{
    if (parser->callback && parser->buf)
      parser->buf[parser->buflen++] = (*( parser->p));
  }
	goto st8;
tr3:
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
#line 74 "src/parser.rl"
	{
    if (parser->callback && parser->buf)
    {
      telnet_command_event ev;
      EV_COMMAND(ev, (*( parser->p)));
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
	goto st8;
tr13:
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
#line 6 "src/parser_common.rl"
	{( parser->p)--;}
#line 121 "src/parser.rl"
	{
    if (parser->callback && parser->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "IAC followed by invalid command.", ( parser->p)-data);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
#line 104 "src/parser.rl"
	{
    if (parser->callback && parser->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 0, parser->option_mark);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
	goto st8;
tr14:
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
#line 104 "src/parser.rl"
	{
    if (parser->callback && parser->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 0, parser->option_mark);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
	goto st8;
tr15:
#line 86 "src/parser.rl"
	{
    if (parser->callback && parser->buf)
    {
      telnet_option_event ev;
      EV_OPTION(ev, parser->option_mark, (*( parser->p)));
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
	goto st8;
st8:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof8;
case 8:
#line 253 "src/parser.c"
	switch( (*( parser->p)) ) {
		case 13u: goto tr16;
		case 255u: goto st2;
	}
	goto tr2;
tr16:
#line 69 "src/parser.rl"
	{
    if (parser->callback && parser->buf)
      parser->buf[parser->buflen++] = (*( parser->p));
  }
	goto st1;
st1:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof1;
case 1:
#line 270 "src/parser.c"
	switch( (*( parser->p)) ) {
		case 0u: goto st8;
		case 10u: goto tr2;
	}
	goto tr1;
st2:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof2;
case 2:
	switch( (*( parser->p)) ) {
		case 240u: goto st0;
		case 250u: goto tr5;
		case 255u: goto tr2;
	}
	if ( 251u <= (*( parser->p)) && (*( parser->p)) <= 254u )
		goto tr6;
	goto tr3;
st0:
 parser->cs = 0;
	goto _out;
tr5:
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
	goto st3;
st3:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof3;
case 3:
#line 307 "src/parser.c"
	goto tr7;
tr12:
#line 6 "src/parser_common.rl"
	{( parser->p)--;}
#line 113 "src/parser.rl"
	{
    if (parser->callback && parser->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", ( parser->p)-data);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
	goto st4;
tr8:
#line 69 "src/parser.rl"
	{
    if (parser->callback && parser->buf)
      parser->buf[parser->buflen++] = (*( parser->p));
  }
	goto st4;
tr7:
#line 95 "src/parser.rl"
	{
    parser->option_mark = (*( parser->p));
    if (parser->callback && parser->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 1, parser->option_mark);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
	goto st4;
st4:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof4;
case 4:
#line 355 "src/parser.c"
	switch( (*( parser->p)) ) {
		case 13u: goto tr9;
		case 255u: goto st6;
	}
	goto tr8;
tr9:
#line 69 "src/parser.rl"
	{
    if (parser->callback && parser->buf)
      parser->buf[parser->buflen++] = (*( parser->p));
  }
	goto st5;
st5:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof5;
case 5:
#line 372 "src/parser.c"
	switch( (*( parser->p)) ) {
		case 0u: goto st4;
		case 10u: goto tr8;
	}
	goto tr12;
st6:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof6;
case 6:
	switch( (*( parser->p)) ) {
		case 240u: goto tr14;
		case 255u: goto tr8;
	}
	goto tr13;
tr6:
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
#line 83 "src/parser.rl"
	{
    parser->option_mark = (*( parser->p));
  }
	goto st7;
st7:
	if ( ++( parser->p) == ( parser->pe) )
		goto _test_eof7;
case 7:
#line 407 "src/parser.c"
	goto tr15;
	}
	_test_eof8:  parser->cs = 8; goto _test_eof; 
	_test_eof1:  parser->cs = 1; goto _test_eof; 
	_test_eof2:  parser->cs = 2; goto _test_eof; 
	_test_eof3:  parser->cs = 3; goto _test_eof; 
	_test_eof4:  parser->cs = 4; goto _test_eof; 
	_test_eof5:  parser->cs = 5; goto _test_eof; 
	_test_eof6:  parser->cs = 6; goto _test_eof; 
	_test_eof7:  parser->cs = 7; goto _test_eof; 

	_test_eof: {}
	if ( ( parser->p) == ( parser->eof) )
	{
	switch (  parser->cs ) {
	case 8: 
#line 59 "src/parser.rl"
	{
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
	break;
#line 435 "src/parser.c"
	}
	}

	_out: {}
	}

#line 190 "src/parser.rl"
  
  if (bytes_used != NULL)
    *bytes_used = parser->p - data;
  
  free(parser->buf);
  parser->buf = NULL;
  parser->p = parser->pe = parser->eof = NULL;
  
  return (parser->interrupted) ? TELNET_E_INTERRUPT : TELNET_E_OK;
}

telnet_error telnet_parser_interrupt(telnet_parser* parser)
{
  if (!parser)
    return TELNET_E_BAD_PARSER;
  
  // Force the parser to stop where it's at.
  if (parser->p)
    parser->eof = parser->pe = parser->p + 1;
  
  parser->interrupted = 1;
  return TELNET_E_OK;
}

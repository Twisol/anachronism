#include <string.h>
#include <anachronism/parser.h>

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

struct telnet_parser {
  int cs; /* current Ragel state */
  const telnet_byte* p; /* current position */
  const telnet_byte* pe; /* end of current packet */
  const telnet_byte* eof; /* end-of-file marker */
  
  telnet_byte option_mark; /* temporary storage for a command byte */
  telnet_byte* buf; /* Buffer to build up a stretch of text in. */
  size_t buflen; /* Length so far of the buffer. */
  
  telnet_parser_callback callback; /* Receiver of Telnet events*/
  void* userdata; /* Context for parser callback */
};

%%{
  machine telnet_parser;
  
  access parser->;
  variable p parser->p;
  variable pe parser->pe;
  variable eof parser->eof;
  
  action flush_text {
    if (parser->callback && parser->buflen > 0)
    {
      telnet_data_event ev;
      EV_DATA(ev, parser->buf, parser->buflen);
      parser->callback(parser, (telnet_event*)&ev);
      parser->buflen = 0;
    }
  }
  
  action char {
    if (parser->callback && parser->buf)
      parser->buf[parser->buflen++] = fc;
  }
  
  action basic_command {
    if (parser->callback && parser->buf)
    {
      telnet_command_event ev;
      EV_COMMAND(ev, fc);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }

  action option_mark {
    parser->option_mark = fc;
  }
  action option_command {
    if (parser->callback && parser->buf)
    {
      telnet_option_event ev;
      EV_OPTION(ev, parser->option_mark, fc);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }

  action subneg_command {
    parser->option_mark = fc;
    if (parser->callback && parser->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 1, parser->option_mark);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
  action subneg_command_end {
    if (parser->callback && parser->buf != NULL)
    {
      telnet_subnegotiation_event ev;
      EV_SUBNEGOTIATION(ev, 0, parser->option_mark);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }

  action warning_cr {
    if (parser->callback && parser->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "Invalid \\r: not followed by \\n or \\0.", fpc-data);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
  action warning_iac {
    if (parser->callback && parser->buf != NULL)
    {
      telnet_warning_event ev;
      EV_WARNING(ev, "IAC followed by invalid command.", fpc-data);
      parser->callback(parser, (telnet_event*)&ev);
    }
  }
  
  include telnet_parser_common "parser_common.rl";
  write data;
}%%

telnet_parser* telnet_parser_new(telnet_parser_callback callback,
                                 void* userdata)
{
  telnet_parser* parser = malloc(sizeof(telnet_parser));
  if (parser)
  {
    memset(parser, 0, sizeof(*parser));
    %% write init;
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
  
  %% write exec;
  
  if (bytes_used != NULL)
    *bytes_used = parser->p - data;
  
  free(parser->buf);
  parser->buf = NULL;
  parser->p = parser->pe = parser->eof = NULL;
  
  return TELNET_E_OK;
}

telnet_error telnet_parser_interrupt(telnet_parser* parser)
{
  if (!parser)
    return TELNET_E_BAD_PARSER;
  
  // Force the parser to stop where it's at.
  if (parser->p)
    parser->eof = parser->pe = parser->p + 1;
  
  return TELNET_E_OK;
}

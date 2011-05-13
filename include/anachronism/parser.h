#ifndef ANACHRONISM_PARSER_H
#define ANACHRONISM_PARSER_H

#include <anachronism/common.h>

typedef struct telnet_parser telnet_parser;

typedef void (*telnet_parser_callback)(telnet_parser* parser,
                                       telnet_event* event);

telnet_parser* telnet_parser_new(telnet_parser_callback callback,
                                 void* userdata);

void telnet_parser_free(telnet_parser* parser);

telnet_error telnet_parser_get_userdata(telnet_parser* parser, void** userdata);

telnet_error telnet_parser_parse(telnet_parser* parser,
                                 const telnet_byte* data,
                                 size_t length,
                                 size_t* bytes_used);

telnet_error telnet_parser_interrupt(telnet_parser* parser);

#endif // ANACHRONISM_PARSER_H

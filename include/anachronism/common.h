#ifndef ANACHRONISM_ERROR_H
#define ANACHRONISM_ERROR_H

#include <stdlib.h> /* for size_t */

// Telnet bytes must be unsigned
typedef unsigned char telnet_byte;

// Error codes returned from API functions
// Positive codes are success/notice codes.
// Nonpositive codes are errors.
// ALLOC is 0 for parity with the NULL result from malloc().
typedef enum telnet_error
{
  TELNET_E_REGISTERED      = -8, // Issue with current channel registration
  TELNET_E_NOT_OPEN        = -7, // Channel isn't open
  TELNET_E_BAD_CHANNEL     = -6, // The telnet_channel* passed is NULL
  TELNET_E_BAD_PARSER      = -5, // The telnet_parser* passed is NULL
  TELNET_E_BAD_NVT         = -4, // The telnet_nvt* passed is NULL
  TELNET_E_INVALID_OPTION  = -3, // An option outside the valid range was provided.
  TELNET_E_INVALID_COMMAND = -2, // The telnet_byte passed is not an allowed command in this API method
  TELNET_E_SUBNEGOTIATING  = -1, // This operation is not permitted while subnegotiating (or only while subnegotiating)
  TELNET_E_ALLOC           =  0, // Not enough memory to allocate essential library structures
  TELNET_E_OK              =  1, // Huge Success!
  TELNET_E_INTERRUPT       =  2, // Parser interrupted by user code.
} telnet_error;

/* Events:
 * TEXT: A stretch of plain data was received. (data, length)
 * COMMAND: A simple IAC comamnd was recevied. (command)
 * OPTION: An option request was received. (command, option)
 * SUBNEGOTIATION: A subnegotiation sequence has been initiated/terminated. (active, option)
 * WARNING: A non-fatal invalid sequence was received. (message, position)
 * SEND: Outgoing data to be sent. (data, length)
 */
typedef enum telnet_event_type
{
  TELNET_EV_DATA,
  TELNET_EV_COMMAND,
  TELNET_EV_OPTION,
  TELNET_EV_SUBNEGOTIATION,
  TELNET_EV_WARNING,
  TELNET_EV_SEND,
} telnet_event_type;

typedef struct telnet_event
{
  telnet_event_type type;
} telnet_event;

typedef struct telnet_data_event
{
  telnet_event SUPER_;
  const telnet_byte* data;
  size_t length;
} telnet_data_event;

typedef struct telnet_command_event
{
  telnet_event SUPER_;
  telnet_byte command;
} telnet_command_event;

typedef struct telnet_option_event
{
  telnet_event SUPER_;
  telnet_byte command;
  telnet_byte option;
} telnet_option_event;

typedef struct telnet_subnegotiation_event
{
  telnet_event SUPER_;
  int active;
  telnet_byte option;
} telnet_subnegotiation_event;

typedef struct telnet_warning_event
{
  telnet_event SUPER_;
  const char* message;
  size_t position;
} telnet_warning_event;

typedef struct telnet_send_event
{
  telnet_event SUPER_;
  const telnet_byte* data;
  size_t length;
} telnet_send_event;

#endif // ANACHRONISM_ERROR_H

#include <stdlib.h> /* for size_t */

// Telnet bytes must be unsigned
typedef unsigned char telnet_byte;

// Error codes returned from API functions
typedef enum telnet_error {
  TELNET_E_BAD_NVT = -3,        // The telnet_nvt* passed is NULL
  TELNET_E_BAD_COMMAND = -2,    // The telnet_byte passed is not an allowed command in this API method
  TELNET_E_SUBNEGOTIATING = -1, // This operation is not permitted while subnegotiating (or only while subnegotiating)
  TELNET_E_ALLOC = 0,           // Not enough memory to allocate essential library structures
  TELNET_E_OK = 1,              // Huge Success!
} telnet_error;

enum
{
  IAC_SE = 240,
  IAC_NOP,
  IAC_DM,
  IAC_BRK,
  IAC_IP,
  IAC_AO,
  IAC_AYT,
  IAC_EC,
  IAC_EL,
  IAC_GA,
  IAC_SB,
  IAC_WILL,
  IAC_WONT,
  IAC_DO,
  IAC_DONT,
  IAC_IAC,
};

/* Events:
 * TEXT: A stretch of plain data was received. (data, length)
 * COMMAND: A simple IAC comamnd was recevied. (command)
 * OPTION: An option request was received. (command, option)
 * SUBNEGOTIATION: A subnegotiation sequence has been initiated/terminated. (active, option)
 * WARNING: A non-fatal invalid sequence was received. (message, position)
 */
typedef enum telnet_event_type
{
  TELNET_EV_DATA,
  TELNET_EV_COMMAND,
  TELNET_EV_OPTION,
  TELNET_EV_SUBNEGOTIATION,
  TELNET_EV_WARNING,
} telnet_event_type;

typedef struct telnet_text_event
{
  const telnet_byte* data;
  size_t length;
} telnet_text_event;

typedef struct telnet_command_event
{
  telnet_byte command;
} telnet_command_event;

typedef struct telnet_option_event
{
  telnet_byte command;
  telnet_byte option;
} telnet_option_event;

typedef struct telnet_subnegotiation_event
{
  int active;
  telnet_byte option;
} telnet_subnegotiation_event;

typedef struct telnet_warning_event
{
  const char* message;
  size_t position;
} telnet_warning_event;

// The tagged union type passed to the on_recv callback.
typedef struct telnet_event
{
  telnet_event_type type;
  
  union
  {
    telnet_text_event           text_event;
    telnet_command_event        command_event;
    telnet_option_event         option_event;
    telnet_subnegotiation_event subnegotiation_event;
    telnet_warning_event        warning_event;
  };
} telnet_event;


typedef struct telnet_nvt telnet_nvt;

typedef void (*telnet_recv_callback)(telnet_nvt* nvt, telnet_event* event);
typedef void (*telnet_send_callback)(telnet_nvt* nvt, const telnet_byte* data, size_t length);

typedef struct telnet_callbacks
{
  telnet_recv_callback on_recv;
  telnet_send_callback on_send;
} telnet_callbacks;

telnet_nvt* telnet_nvt_new();
void telnet_nvt_delete(telnet_nvt* nvt);

telnet_error telnet_nvt_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks);

telnet_error telnet_nvt_set_userdata(telnet_nvt* nvt, void* udata);
telnet_error telnet_nvt_get_userdata(telnet_nvt* nvt, void** udata);

telnet_error telnet_nvt_recv(telnet_nvt* nvt, const telnet_byte* data, const size_t length, size_t* bytes_used);

telnet_error telnet_nvt_data(telnet_nvt* nvt, const telnet_byte* data, const size_t length);
telnet_error telnet_nvt_command(telnet_nvt* nvt, const telnet_byte command);
telnet_error telnet_nvt_option(telnet_nvt* nvt, const telnet_byte command, const telnet_byte option);
telnet_error telnet_nvt_begin_subnegotiation(telnet_nvt* nvt, const telnet_byte option);
telnet_error telnet_nvt_end_subnegotiation(telnet_nvt* nvt);

// Shorthand for begin_subneg, text, end_subneg
telnet_error telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length);

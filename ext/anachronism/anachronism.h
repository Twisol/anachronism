#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h> /* for size_t */

// Telnet bytes must be unsigned
typedef unsigned char telnet_byte;

// Error codes returned from API functions
typedef enum telnet_error {
  TELNET_E_BAD_NVT        = -3, // The telnet_nvt* passed is NULL
  TELNET_E_BAD_COMMAND    = -2, // The telnet_byte passed is not an allowed command in this API method
  TELNET_E_SUBNEGOTIATING = -1, // This operation is not permitted while subnegotiating (or only while subnegotiating)
  TELNET_E_ALLOC          =  0, // Not enough memory to allocate essential library structures
  TELNET_E_OK             =  1, // Huge Success!
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
  telnet_recv_callback on_recv; // Called when a Telnet event is received
  telnet_send_callback on_send; // Called when outgoing data should be sent.
} telnet_callbacks;

/**
  Creates a new Telnet NVT.
  
  Errors:
    TELNET_E_ALLOC - Unable to allocate enough memory for the NVT.
 */
telnet_nvt* telnet_new_nvt();
 
void telnet_free_nvt(telnet_nvt* nvt);

/**
  Provides a pointer to the structure that contains the event callback pointers.
  This is used to register your own event callbacks.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
  
  Example:
    telnet_callbacks* callbacks;
    telnet_get_callbacks(nvt, &callbacks);
    callbacks.on_recv = &my_recv_callback;
 */
telnet_error telnet_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks);

/**
  Every NVT can have some user-specific data attached, such as a user-defined struct.
  This can be accessed (primarily by event callbacks) to differentiate between NVTs.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
  
  Example:
    FILE* out = ...;
    telnet_set_userdata(nvt, (void*)out);
 
    // (later, in the on_send callback)
    FILE* out = NULL;
    telnet_get_userdata(nvt, (void**)&out);
 */
telnet_error telnet_set_userdata(telnet_nvt* nvt, void* udata);
telnet_error telnet_get_userdata(telnet_nvt* nvt, void** udata);

/**
  Processes incoming data. The on_recv callback, if set, may be invoked during processing.
  If `bytes_used` is set, it contains the length of the string that was read. This is generally
  only useful if you use telnet_halt() in a callback.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
    TELNET_E_ALLOC   - Unable to allocate destination buffer for incoming text.
 */
telnet_error telnet_recv(telnet_nvt* nvt, const telnet_byte* data, const size_t length, size_t* bytes_used);

/**
  If currently parsing (i.e. telnet_recv() is running), halts the parser.
  This is useful for things such as MCCP, where a Telnet sequence hails the start of
  data that must be decompressed before being parsed.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
 */
telnet_error telnet_halt(telnet_nvt* nvt);


/**
  Sends a string as a stream of escaped Telnet data.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
    TELNET_E_ALLOC   - Unable to allocate destination buffer for outgoing text.
 */
telnet_error telnet_send_data(telnet_nvt* nvt, const telnet_byte* data, const size_t length);

/**
  Sends a Telnet command.
  
  Errors:
    TELNET_E_BAD_NVT        - Invalid telnet_nvt* parameter.
    TELNET_E_SUBNEGOTIATING - Unable to send a command while subnegotiating.
    TELNET_E_BAD_COMMAND    - The command cannot be WILL, WONT, DO, DONT, SB, or SE.
 */
telnet_error telnet_send_command(telnet_nvt* nvt, const telnet_byte command);

/**
  Sends a Telnet option negotiation.
  
  Errors:
    TELNET_E_BAD_NVT        - Invalid telnet_nvt* parameter.
    TELNET_E_SUBNEGOTIATING - Unable to send an option while subnegotiating.
    TELNET_E_BAD_COMMAND    - The command must be WILL, WONT, DO, or DONT.
 */
telnet_error telnet_send_option(telnet_nvt* nvt, const telnet_byte command, const telnet_byte option);

/**
  Sends a subnegotiation entry sequence
  
  Errors:
    TELNET_E_BAD_NVT        - Invalid telnet_nvt* parameter.
    TELNET_E_SUBNEGOTIATING - Unable to begin a subnegotiation while already subnegotiating.
 */
telnet_error telnet_send_subnegotiation_start(telnet_nvt* nvt, const telnet_byte option);

/**
  Ends an open subnegotiation.
  
  Errors:
    TELNET_E_BAD_NVT        - Invalid telnet_nvt* parameter.
    TELNET_E_SUBNEGOTIATING - There is no open subnegotiation to close.
 */
telnet_error telnet_send_subnegotiation_end(telnet_nvt* nvt);

/**
  Sends a string as the whole body of a subnegotiation. Equivalent to send_subnegotiation_start, send_data, and send_subnegotiation_end in sequence.
  
  Errors:
    TELNET_E_BAD_NVT        - Invalid telnet_nvt* parameter.
    TELNET_E_SUBNEGOTIATING - Unable to begin a subnegotiation while already subnegotiating.
    TELNET_E_ALLOC          - Unable to allocate destination buffer for outgoing text.
 */
telnet_error telnet_send_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length);

#ifdef __cplusplus
}
#endif

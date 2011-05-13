#ifndef ANACHRONISM_ANACHRONISM_H
#define ANACHRONISM_ANACHRONISM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <anachronism/common.h>

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

typedef enum telnet_channel_mode
{
  TELNET_CHANNEL_OFF,
  TELNET_CHANNEL_ON,
  TELNET_CHANNEL_LAZY,
} telnet_channel_mode;

typedef enum telnet_channel_event
{
  TELNET_CHANNEL_EV_BEGIN,
  TELNET_CHANNEL_EV_END,
  TELNET_CHANNEL_EV_DATA,
} telnet_channel_event;

typedef enum telnet_channel_provider
{
  TELNET_CHANNEL_LOCAL,
  TELNET_CHANNEL_REMOTE,
} telnet_channel_provider;

enum
{
  TELNET_MAIN_CHANNEL,
  TELNET_INVALID_CHANNEL,
};

/*
typedef struct telnet_interrupt_code {
  // [0, 255] are channels, -1 is main, anything else is illegal
  int option : 9;
  
  // option-specific error code
  unsigned int code : 7;
} telnet_interrupt_code;
*/


typedef struct telnet_nvt telnet_nvt;
typedef struct telnet_channel telnet_channel;

typedef void (*telnet_event_callback)(telnet_nvt* nvt, telnet_event* event);
                                     
typedef void (*telnet_channel_toggle_callback)(telnet_channel* channel,
                                               char open,
                                               telnet_channel_provider who);

typedef void (*telnet_channel_data_callback)(telnet_channel* channel,
                                             telnet_channel_event type,
                                             const telnet_byte* data,
                                             size_t length);

/**
  Creates a new Telnet NVT.
  
  Errors:
    TELNET_E_ALLOC - Unable to allocate enough memory for the NVT.
 */
telnet_nvt* telnet_nvt_new(telnet_event_callback callback, void* userdata);
 
void telnet_nvt_free(telnet_nvt* nvt);

/**
  Every NVT can have some user-specific data attached, such as a user-defined struct.
  This can be accessed (primarily by event callbacks) to differentiate between NVTs.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
  
  Example:
    // assuming a FILE was passed to telnet_nvt_new():
    FILE out = NULL;
    telnet_get_userdata(nvt, (void**)&out);
 */
telnet_error telnet_get_userdata(telnet_nvt* nvt, void** udata);

/**
  Processes incoming data. The on_recv callback, if set, may be invoked during processing.
  If `bytes_used` is set, it contains the length of the string that was read. This is generally
  only useful if you use telnet_halt() in a callback.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
    TELNET_E_ALLOC   - Unable to allocate destination buffer for incoming text.
 */
telnet_error telnet_recv(telnet_nvt* nvt, const telnet_byte* data, size_t length, size_t* bytes_used);

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


telnet_error telnet_channel_register(telnet_channel* channel,
                                     short option,
                                     telnet_channel_mode local,
                                     telnet_channel_mode remote);

telnet_channel* telnet_channel_new(telnet_nvt* nvt,
                                   telnet_channel_toggle_callback on_toggle,
                                   telnet_channel_data_callback on_data,
                                   void* userdata);

telnet_error telnet_channel_get_userdata(telnet_channel* channel,
                                         void** userdata);

telnet_error telnet_channel_get_nvt(telnet_channel* channel, telnet_nvt** nvt);

telnet_error telnet_channel_send(telnet_channel* channel,
                                 const telnet_byte* data,
                                 size_t length);

#ifdef __cplusplus
}
#endif

#endif // ANACHRONISM_ANACHRONISM_H

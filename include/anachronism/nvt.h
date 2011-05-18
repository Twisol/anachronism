#ifndef ANACHRONISM_ANACHRONISM_H
#define ANACHRONISM_ANACHRONISM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <anachronism/common.h>

// predefined Telnet commands from 240-255
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


// All channels take up 0-255.
// The MAIN channel is defined to be -1.
// INVALID is a sentinel value for an invalid/unregistered channel.
enum
{
  TELNET_INVALID_CHANNEL = -2,
  TELNET_MAIN_CHANNEL = -1,
};

typedef struct telnet_interrupt_code
{
  short source;
  char code;
} telnet_interrupt_code;


typedef struct telnet_nvt telnet_nvt;
typedef struct telnet_channel telnet_channel;

typedef void (*telnet_event_callback)(telnet_nvt* nvt, telnet_event* event);
                                     
typedef void (*telnet_channel_toggle_callback)(telnet_channel* channel,
                                               telnet_channel_mode on,
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
    TELNET_E_BAD_NVT   - Invalid telnet_nvt* parameter.
    TELNET_E_ALLOC     - Unable to allocate destination buffer for incoming text.
    TELNET_E_INTERRUPT - User code interrupted the parser.
 */
telnet_error telnet_recv(telnet_nvt* nvt, const telnet_byte* data, size_t length, size_t* bytes_used);

/**
  If currently parsing (i.e. telnet_recv() is running), interrupts the parser.
  This is useful for things such as MCCP, where a Telnet sequence hails the start of
  data that must be decompressed before being parsed.
  
  Errors:
    TELNET_E_BAD_NVT - Invalid telnet_nvt* parameter.
 */
telnet_error telnet_interrupt(telnet_nvt* nvt, telnet_interrupt_code code);

telnet_error telnet_get_last_interrupt(telnet_nvt* nvt,
                                       telnet_interrupt_code* code);


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


/**
  Registers a channel with a Telnet option.
  
  If `local` and/or `remote` are TELNET_CHANNEL_ON, immediately negotiates
  to enable the option.
  If `local` and/or `remote` are TELNET_CHANNEL_LAZY, negotiations will be
  accepted if the remote end asks.
  
  Errors:
    TELNET_E_BAD_CHANNEL    - Invalid telnet_channel* parameter.
    TELNET_E_REGISTERED     - Either the option or the channel is already registered.
    TELNET_E_INVALID_OPTION - The option is out of range (-1 to 255)
 */
telnet_error telnet_channel_register(telnet_channel* channel,
                                     telnet_nvt* nvt,
                                     short option,
                                     telnet_channel_mode local,
                                     telnet_channel_mode remote);

telnet_error telnet_channel_unregister(telnet_channel* channel);

/**
  Creates a new channel with the supplied event callbacks.
  
  Errors:
    TELNET_E_ALLOC - Unable to allocate the telnet_channel.
 */
telnet_channel* telnet_channel_new(telnet_channel_toggle_callback on_toggle,
                                   telnet_channel_data_callback on_data,
                                   void* userdata);

void telnet_channel_free(telnet_channel* channel);

/**
  Retrieves the userdata stored with the channel.
  
  Errors:
    TELNET_E_BAD_CHANNEL - Invalid telnet_channel* parameter.
 */
telnet_error telnet_channel_get_userdata(telnet_channel* channel,
                                         void** userdata);

/**
  Retrieves the NVT that services this channel.
  
  Errors:
    TELNET_E_BAD_CHANNEL - Invalid telnet_channel* parameter.
    TELNET_E_REGSITERED  - The channel is not registered with an NVT.
 */
telnet_error telnet_channel_get_nvt(telnet_channel* channel, telnet_nvt** nvt);

/**
  Retreives the option that this channel is bound to.
  
  Errors:
    TELNET_E_BAD_CHANNEL - Invalid telnet_channel* parameter.
    TELNET_E_REGISTERED  - The channel is not registered with an NVT.
 */
telnet_error telnet_channel_get_option(telnet_channel* channel, short* option);

/**
  Retrieves the on/off status of the channel for a given host.
  
  Errors:
    TELNET_E_BAD_CHANNEL - Invalid telnet_channel* parameter.
 */
telnet_error telnet_channel_get_status(telnet_channel* channel,
                                       telnet_channel_provider where,
                                       telnet_channel_mode* on);

/**
  Sends a message through this channel to the remote host.
  
  Errors:
    TELNET_E_BAD_CHANNEL    - Invalid telnet_channel* parameter.
    TELNET_E_BAD_NVT        - The NVT the channel was created with is invalid.
    TELNET_E_NOT_OPEN       - The channel has not been negotiated open.
    TELNET_E_SUBNEGOTIATING - Unable to begin a subnegotiation while already subnegotiating.
    TELNET_E_ALLOC          - Unable to allocate destination buffer for outgoing text.
 */
telnet_error telnet_channel_send(telnet_channel* channel,
                                 const telnet_byte* data,
                                 size_t length);

/**
  Negotiates to enable or disable a channel.
  
  Errors:
    TELNET_E_BAD_CHANNEL - Invalid telnet_channel* parameter.
    TELNET_E_REGISTERED  - The channel is unregistered or is the main channel.
 */
telnet_error telnet_channel_toggle(telnet_channel* channel,
                                   telnet_channel_provider where,
                                   telnet_channel_mode what);

#ifdef __cplusplus
}
#endif

#endif // ANACHRONISM_ANACHRONISM_H

#include <stdlib.h> /* for size_t */

typedef unsigned char telnet_byte;

typedef enum telnet_command
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
} telnet_command;

typedef enum telnet_event_type
{
  TELNET_EV_TEXT,
  TELNET_EV_COMMAND,
  TELNET_EV_OPTION,
  TELNET_EV_SUBNEGOTIATION,
  TELNET_EV_WARNING,
} telnet_event_type;

typedef struct telnet_event
{
  telnet_event_type type;
  
  union
  {
    struct
    {
      const telnet_byte* data;
      size_t length;
    } text_event;
    
    struct
    {
      telnet_command command;
    } command_event;
    
    struct
    {
      telnet_command command;
      telnet_byte option;
    } option_event;
    
    struct
    {
      int active;
      telnet_byte option;
    } subnegotiation_event;
    
    struct
    {
      const char* message;
      size_t position;
    } warning_event;
  };
} telnet_event;

typedef struct telnet_nvt telnet_nvt;

typedef void (*telnet_recv_callback)(telnet_nvt* nvt, telnet_event* event);
typedef void (*telnet_send_callback)(telnet_nvt* nvt, const telnet_byte* data, size_t length);

typedef struct telnet_callbacks
{
  telnet_recv_callback    on_recv;
  telnet_send_callback    on_send;
} telnet_callbacks;


telnet_nvt* telnet_nvt_new();
void telnet_nvt_delete(telnet_nvt* nvt);

int telnet_nvt_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks);

int telnet_nvt_set_userdata(telnet_nvt* nvt, void* udata);
int telnet_nvt_get_userdata(telnet_nvt* nvt, void** udata);

int telnet_nvt_recv(telnet_nvt* nvt, const telnet_byte* data, const size_t length);

int telnet_nvt_text(telnet_nvt* nvt, const telnet_byte* data, const size_t length);
int telnet_nvt_command(telnet_nvt* nvt, const telnet_command command);
int telnet_nvt_option(telnet_nvt* nvt, const telnet_command command, const telnet_byte option);
int telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length);

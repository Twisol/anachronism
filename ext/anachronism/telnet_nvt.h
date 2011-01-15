#include <stdlib.h> /* for size_t */

typedef unsigned char telnet_byte;

typedef enum telnet_mode
{
  TELNET_TEXT,
  TELNET_SUBNEG,
} telnet_mode;

typedef enum telnet_severity
{
  TELNET_WARNING,
  TELNET_FATAL,
} telnet_severity;

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
} telnet_command;

typedef struct telnet_nvt telnet_nvt;

typedef void (*telnet_text_callback)(telnet_nvt* nvt, const telnet_byte* data, size_t length);
typedef void (*telnet_eol_callback)(telnet_nvt* nvt);
typedef void (*telnet_command_callback)(telnet_nvt* nvt, telnet_byte command);
typedef void (*telnet_option_callback)(telnet_nvt* nvt, telnet_byte command, telnet_byte option);

typedef void (*telnet_mode_callback)(telnet_nvt* nvt, telnet_mode mode, telnet_byte extra);
typedef void (*telnet_error_callback)(telnet_nvt* nvt, int fatal, const char* message, size_t position);

typedef void (*telnet_send_callback)(telnet_nvt* nvt, const telnet_byte* data, size_t length);

typedef struct telnet_callbacks
{
  telnet_text_callback    on_text;
  telnet_command_callback on_command;
  telnet_option_callback  on_option;
  
  telnet_mode_callback    on_mode;
  telnet_error_callback   on_error;

  telnet_send_callback    on_send;
} telnet_callbacks;


telnet_nvt* telnet_nvt_new();
void telnet_nvt_delete(telnet_nvt* nvt);

int telnet_nvt_get_callbacks(telnet_nvt* nvt, telnet_callbacks** callbacks);

int telnet_nvt_set_userdata(telnet_nvt* nvt, void* udata);
int telnet_nvt_get_userdata(telnet_nvt* nvt, void** udata);

int telnet_nvt_parse(telnet_nvt* nvt, const telnet_byte* data, const size_t length);

int telnet_nvt_text(telnet_nvt* nvt, const telnet_byte* data, const size_t length);
int telnet_nvt_command(telnet_nvt* nvt, const telnet_command command);
int telnet_nvt_option(telnet_nvt* nvt, const telnet_command command, const telnet_byte option);
int telnet_nvt_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length);

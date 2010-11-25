#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "ruby.h"
#include "telnet_nvt.h"
#include "chains.h"

#define DISABLED       000
#define ENABLING       001
#define BEING_ENABLED  010
#define ENABLED        100
#define DISABLING      101
#define BEING_DISABLED 110

typedef struct mud_item
{
  short type; /* -1 if text, 0-255 if subnegotiation */
  CHAIN* data;
} mud_item;

typedef struct mud_nvt
{
  telnet_nvt base;

  int options[256];
  mud_item items[];
} mud_nvt;

void got_text(telnet_nvt* nvt, telnet_byte data)
{
  printf("Text: ");
  if (isprint(data))
    printf("%.*s", 1, &data);
  else
    printf("<%i>", data);
  printf("\n");
}

void got_command(telnet_nvt* nvt, telnet_byte command)
{
  printf("Command: ");
  switch (command)
  {
    case IAC_NOP: printf("NOP"); break;
    case IAC_DM:  printf("DM");  break;
    case IAC_BRK: printf("BRK"); break;
    case IAC_IP:  printf("IP");  break;
    case IAC_AO:  printf("AO");  break;
    case IAC_AYT: printf("AYT"); break;
    case IAC_EC:  printf("EC");  break;
    case IAC_EL:  printf("EL");  break;
    case IAC_GA:  printf("GA");  break;
    default:
      printf("<%i>", command);
  }
  printf("\n");
}

void got_option(telnet_nvt* nvt, telnet_byte command, telnet_byte option)
{
  printf("Option: ");
  switch (command)
  {
    case IAC_WILL: printf("WILL"); break;
    case IAC_WONT: printf("WONT"); break;
    case IAC_DO:   printf("DO  "); break;
    case IAC_DONT: printf("DONT"); break;
    default:
      assert("Got something other than WILL/WONT/DO/DONT!");
  }
  printf(" <%i>\n", option);
}

void got_mode(telnet_nvt* nvt, telnet_mode mode, telnet_byte extra)
{
  printf("Mode: ");
  switch (mode)
  {
    case TELNET_TEXT:   printf("Text");   break;
    case TELNET_SUBNEG: printf("Subneg"); break;
    default:
      assert("Got something other than TEXT or SUBNEG!");
  }
  printf("\n");
}

void got_error(telnet_nvt* nvt, int fatal, const char* message, size_t position)
{
  printf("[%s:%i] %s\n", (fatal) ? "FATAL" : "WARNING", position, message);
}

int main()
{
  mud_nvt* mud = (mud_nvt*)malloc(sizeof(mud_nvt));
  telnet_nvt* nvt = (telnet_nvt*)mud;
  
  if (!telnet_nvt_init(nvt))
    return 1;

  nvt->text_callback = &got_text;
  nvt->command_callback = &got_command;
  nvt->option_callback = &got_option;
  nvt->mode_callback = &got_mode;
  nvt->error_callback = &got_error;

  const char* data = "\xFF\xFA\xFF\r\n\r\xFF\xF0";
  size_t length = strlen(data);

  int count = telnet_nvt_recv(nvt, (const telnet_byte*)data, length);
  printf("%i\n", count);

  free(nvt);
  return 0;
}

#include <stdlib.h>
#include <string.h>
#include <anachronism/nvt.h>
#include <anachronism/parser.h>

#define CLOSE_CALLBACK(channel, who) do { \
  channel->open -= 1; \
  if ((channel)->on_toggle) \
    (channel)->on_toggle((channel), TELNET_CHANNEL_OFF, (who)); \
} while (0)

#define OPEN_CALLBACK(channel, who) do { \
  channel->open += 1; \
  if ((channel)->on_toggle) \
    (channel)->on_toggle((channel), TELNET_CHANNEL_ON, (who)); \
} while (0)

#define DATA_CALLBACK(channel, msg, data, length) do { \
  if ((channel)->open > 0 && (channel)->on_data) \
    (channel)->on_data((channel), (msg), (data), (length)); \
} while (0)


#define EV_SEND(ev, text, len) do { \
  (ev).SUPER_.type = TELNET_EV_SEND; \
  (ev).data = (text); \
  (ev).length = (len); \
} while (0)

struct telnet_channel
{
  telnet_nvt* nvt;
  short option;
  void* userdata;
  unsigned char open;
  
  telnet_channel_toggle_callback on_toggle;
  telnet_channel_data_callback on_data;
};

// Q Method of Implementing TELNET Option Negotiation
// ftp://ftp.rfc-editor.org/in-notes/rfc1143.txt
typedef enum qstate {
  Q_NO = 0,  Q_WANTYES, Q_WANTYESNO,
  Q_YES,     Q_WANTNO, Q_WANTNOYES,
} qstate;

typedef struct telnet_qstate
{
  unsigned int r_current : 3;
  unsigned int r_lazy : 1;
  
  unsigned int l_current : 3;
  unsigned int l_lazy : 1;
} telnet_qstate;

struct telnet_nvt
{
  telnet_parser* parser;
  int subnegotiating;
  
  telnet_qstate options[256]; // track the state of each subnegotiation option
  telnet_channel* channels[256]; // track all registered channels
  telnet_channel* primary; // the main channel
  
  short current_remote;
  
  telnet_interrupt_code interrupt_code;
  telnet_event_callback callback;
  void* userdata;
};

static telnet_channel* get_current_remote(telnet_nvt* nvt)
{
  if (nvt->current_remote == TELNET_MAIN_CHANNEL)
    return nvt->primary;
  else
    return nvt->channels[nvt->current_remote];
}

static void set_current_remote(telnet_nvt* nvt, short option)
{
  nvt->current_remote = option;
}

static void process_option_event(telnet_nvt* nvt,
                                 telnet_byte command,
                                 telnet_byte option)
{
  telnet_channel* channel = nvt->channels[option];
  telnet_qstate* q = &nvt->options[option];
  // If the channel isn't registered, that's okay!
  // Every qstate begins zeroed-out, and Q_NO is 0.
  
  switch (command)
  {
    case IAC_WILL:
      switch (q->r_current)
      {
        case Q_NO:
          telnet_send_option(nvt, IAC_DONT, option);
          break;
        case Q_WANTNO:
          // error
          q->r_current = Q_NO;
          break;
        case Q_WANTNOYES:
          // error
          q->r_current = Q_YES;
          OPEN_CALLBACK(channel, TELNET_CHANNEL_REMOTE);
          break;
        case Q_WANTYES:
          if (q->r_lazy)
            telnet_send_option(nvt, IAC_DO, option);
          q->r_current = Q_YES;
          OPEN_CALLBACK(channel, TELNET_CHANNEL_REMOTE);
          break;
        case Q_WANTYESNO:
          telnet_send_option(nvt, IAC_DONT, option);
          q->r_current = Q_WANTNO;
          OPEN_CALLBACK(channel, TELNET_CHANNEL_REMOTE);
          break;
      }
      break;
    case IAC_WONT:
      switch (q->r_current)
      {
        case Q_YES:
          telnet_send_option(nvt, IAC_DONT, option);
          q->r_current = Q_NO;
          CLOSE_CALLBACK(channel, TELNET_CHANNEL_REMOTE);
          break;
        case Q_WANTNO:
          q->r_current = Q_NO;
          break;
        case Q_WANTNOYES:
          if (!q->r_lazy)
            telnet_send_option(nvt, IAC_DO, option);
          q->r_current = Q_WANTYES;
          break;
        case Q_WANTYES:
          q->r_current = Q_NO;
          break;
        case Q_WANTYESNO:
          q->r_current = Q_NO;
          break;
      }
      break;
    case IAC_DO:
      switch (q->l_current)
      {
        case Q_NO:
          telnet_send_option(nvt, IAC_WONT, option);
          break;
        case Q_WANTNO:
          // error
          q->l_current = Q_NO;
          break;
        case Q_WANTNOYES:
          // error
          q->l_current = Q_YES;
          OPEN_CALLBACK(channel, TELNET_CHANNEL_LOCAL);
          break;
        case Q_WANTYES:
          if (q->l_lazy)
            telnet_send_option(nvt, IAC_WILL, option);
          q->l_current = Q_YES;
          OPEN_CALLBACK(channel, TELNET_CHANNEL_LOCAL);
          break;
        case Q_WANTYESNO:
          q->l_current = Q_WANTNO;
          telnet_send_option(nvt, IAC_WONT, option);
          OPEN_CALLBACK(channel, TELNET_CHANNEL_LOCAL);
          break;
      }
      break;
    case IAC_DONT:
      switch (q->l_current)
      {
        case Q_YES:
          telnet_send_option(nvt, IAC_DONT, option);
          q->l_current = Q_NO;
          CLOSE_CALLBACK(channel, TELNET_CHANNEL_LOCAL);
          break;
        case Q_WANTNO:
          q->l_current = Q_NO;
          break;
        case Q_WANTNOYES:
          if (!q->l_lazy)
            telnet_send_option(nvt, IAC_WILL, option);
          q->l_current = Q_WANTYES;
          break;
        case Q_WANTYES:
          q->l_current = Q_NO;
          break;
        case Q_WANTYESNO:
          q->l_current = Q_NO;
          break;
      }
      break;
  }
}

static void process_data_event(telnet_nvt* nvt,
                               const telnet_byte* data,
                               size_t length)
{
  telnet_channel* channel = get_current_remote(nvt);
  if (channel)
    DATA_CALLBACK(channel, TELNET_CHANNEL_EV_DATA, data, length);
}

static void process_subnegotiation_event(telnet_nvt* nvt,
                                         int active,
                                         telnet_byte option)
{
  if (active)
  {
    set_current_remote(nvt, option);
    telnet_channel* channel = get_current_remote(nvt);
    if (channel)
      DATA_CALLBACK(channel, TELNET_CHANNEL_EV_BEGIN, NULL, 0);
  }
  else
  {
    telnet_channel* channel = get_current_remote(nvt);
    if (channel)
      DATA_CALLBACK(channel, TELNET_CHANNEL_EV_END, NULL, 0);
    set_current_remote(nvt, TELNET_MAIN_CHANNEL);
  }
}

static void process_event(telnet_parser* parser, telnet_event* event)
{
  telnet_nvt* nvt = NULL;
  telnet_parser_get_userdata(parser, (void*)&nvt);
  
  if (nvt->callback)
    nvt->callback(nvt, event);
  
  switch (event->type)
  {
    case TELNET_EV_DATA:
    {
      telnet_data_event* ev = (telnet_data_event*)event;
      process_data_event(nvt, ev->data, ev->length);
      break;
    }
    
    case TELNET_EV_OPTION:
    {
      telnet_option_event* ev = (telnet_option_event*)event;
      process_option_event(nvt, ev->command, ev->option);
      break;
    }
    
    case TELNET_EV_SUBNEGOTIATION:
    {
      telnet_subnegotiation_event* ev = (telnet_subnegotiation_event*)event;
      process_subnegotiation_event(nvt, ev->active, ev->option);
      break;
    }
    
    default:
      break; // let the user callback handle anything else
  }
}


telnet_nvt* telnet_nvt_new(telnet_event_callback callback, void* userdata)
{
  telnet_nvt* nvt = malloc(sizeof(telnet_nvt));
  if (nvt)
  {
    telnet_parser* parser = telnet_parser_new(&process_event, (void*)nvt);
    if (parser)
    {
      memset(nvt, 0, sizeof(*nvt));
      nvt->parser = parser;
      nvt->callback = callback;
      nvt->userdata = userdata;
      
      set_current_remote(nvt, TELNET_MAIN_CHANNEL);
    }
    else
    {
      free(nvt);
      nvt = NULL;
    }
  }
  return nvt;
}

void telnet_nvt_free(telnet_nvt* nvt)
{
  if (nvt)
  {
    telnet_parser_free(nvt->parser);
    free(nvt);
  }
}

telnet_error telnet_get_userdata(telnet_nvt* nvt, void** userdata)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  *userdata = nvt->userdata;
  return TELNET_E_OK;
}

telnet_error telnet_recv(telnet_nvt* nvt, const telnet_byte* data, size_t length, size_t* bytes_used)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  return telnet_parser_parse(nvt->parser, data, length, bytes_used);
}

telnet_error telnet_interrupt(telnet_nvt* nvt, telnet_interrupt_code code)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  nvt->interrupt_code = code;
  return telnet_parser_interrupt(nvt->parser);
}

telnet_error telnet_get_last_interrupt(telnet_nvt* nvt,
                                       telnet_interrupt_code* code)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  
  *code = nvt->interrupt_code;
  return TELNET_E_OK;
}

static int safe_concat(const telnet_byte* in, size_t inlen, telnet_byte* out, size_t outlen)
{
  // Copy as much as possible into the buffer.
  memcpy(out, in, (outlen < inlen) ? outlen : inlen);
  
  // true if everything could be copied, false otherwise
  return outlen >= inlen;
}

// Escapes any special characters in data, writing the result data to out.
// Returns -1 if not everything could be copied (and out is full).
// Otherwise returns the length of the data in out.
//
// To avoid potential -1 return values, pass in an out buffer double the length of the data buffer.
static size_t telnet_escape(const telnet_byte* data, size_t length, telnet_byte* out, size_t outsize)
{
  if (data == NULL || out == NULL)
    return 0;
  
  size_t outlen = 0;
  size_t left = 0;
  size_t right = 0;
  const char* seq = NULL;
  for (; right < length; ++right)
  {
    switch (data[right])
    {
      case IAC_IAC:
        seq = "\xFF\xFF";
        break;
      case '\r':
        // Only escape \r if it doesn't immediately precede \n.
        if (right + 1 >= length || data[right+1] != '\n')
        {
          seq = "\r\0";
          break;
        }
        // !!FALLTHROUGH!!
      default:
        continue; // Move to the next character
    }
    
    // Add any normal data that hasn't been added yet.
    if (safe_concat(data+left, right-left, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += right - left;
    left = right + 1;
    
    // Add the escape sequence.
    if (safe_concat((const telnet_byte*)seq, 2, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += 2;
  }
  
  // Add any leftover normal data.
  if (left < right)
  {
    if (safe_concat(data+left, right-left, out+outlen, outsize-outlen) == 0)
      return -1;
    outlen += right - left;
  }
  
  return outlen;
}

telnet_error telnet_send_data(telnet_nvt* nvt, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (!nvt->callback)
    return TELNET_E_OK; // immediate success since they apparently don't want the data to go anywhere
  
  // Due to the nature of the protocol, the most any one byte can be encoded as is two bytes.
  // Hence, the smallest buffer guaranteed to contain any input is double the length of the source.
  size_t bufsize = sizeof(telnet_byte) * length * 2;
  telnet_byte* buf = malloc(bufsize);
  if (!buf)
    return TELNET_E_ALLOC;
  
  bufsize = telnet_escape(data, length, buf, bufsize);
  
  telnet_send_event event;
  EV_SEND(event, buf, bufsize);
  nvt->callback(nvt, (telnet_event*)&event);
  
  free(buf);
  buf = NULL;
  
  return TELNET_E_OK;
}

telnet_error telnet_send_command(telnet_nvt* nvt, const telnet_byte command)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (command >= IAC_SB || command == IAC_SE) 
    return TELNET_E_INVALID_COMMAND; // Invalid command
  
  if (nvt->callback)
  {
    const telnet_byte buf[] = {IAC_IAC, command};
    telnet_send_event event;
    EV_SEND(event, buf, 2);
    nvt->callback(nvt, (telnet_event*)&event);
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_send_option(telnet_nvt* nvt, const telnet_byte command, const telnet_byte option)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (command < IAC_WILL || command > IAC_DONT)
    return TELNET_E_INVALID_COMMAND; // Invalid option command
  
  if (nvt->callback)
  {
    const telnet_byte buf[] = {IAC_IAC, command, option};
    telnet_send_event event;
    EV_SEND(event, buf, 3);
    nvt->callback(nvt, (telnet_event*)&event);
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_send_subnegotiation_start(telnet_nvt* nvt, const telnet_byte option)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  
  if (nvt->callback)
  {
    const telnet_byte buf[] = {IAC_IAC, IAC_SB, option};
    telnet_send_event event;
    EV_SEND(event, buf, 3);
    nvt->callback(nvt, (telnet_event*)&event);
  }

  nvt->subnegotiating = 1;
  return TELNET_E_OK;
}

telnet_error telnet_send_subnegotiation_end(telnet_nvt* nvt)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (!nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  
  if (nvt->callback)
  {
    static const telnet_byte buf[] = {IAC_IAC, IAC_SE};
    telnet_send_event event;
    EV_SEND(event, buf, 2);
    nvt->callback(nvt, (telnet_event*)&event);
  }
  
  nvt->subnegotiating = 0;
  return TELNET_E_OK;
}

telnet_error telnet_send_subnegotiation(telnet_nvt* nvt, const telnet_byte option, const telnet_byte* data, const size_t length)
{
  if (!nvt)
    return TELNET_E_BAD_NVT;
  else if (nvt->subnegotiating)
    return TELNET_E_SUBNEGOTIATING;
  else if (!nvt->callback)
    return TELNET_E_OK;
  
  // length*2 is the maximum buffer size needed for an escaped string.
  // The extra five bytes are for the IAC, SB, <option>, IAC, and SE frame around the data.
  size_t bufsize = (sizeof(telnet_byte) * length * 2) + 5;
  telnet_byte* buf = malloc(bufsize);
  if (!buf)
    return TELNET_E_ALLOC;
  
  // Begin with IAC SB <option>
  telnet_byte iac[] = {IAC_IAC, IAC_SB, option};
  memcpy(buf, iac, 3);
  
  // Add the subnegotiation body
  size_t escaped_length = telnet_escape(data, length, buf+3, bufsize-3) + 3;
  
  // End with IAC SE
  iac[1] = IAC_SE;
  memcpy(buf+escaped_length, iac, 2);
  
  telnet_send_event event;
  EV_SEND(event, buf, escaped_length+2);
  nvt->callback(nvt, (telnet_event*)&event);
  
  free(buf);
  buf = NULL;
  
  return TELNET_E_OK;
}


telnet_channel* telnet_channel_new(telnet_channel_toggle_callback on_toggle,
                                   telnet_channel_data_callback on_data,
                                   void* userdata)
{
  telnet_channel* channel = malloc(sizeof(telnet_channel));
  if (channel)
  {
    memset(channel, 0, sizeof(*channel));
    
    channel->option = TELNET_INVALID_CHANNEL;
    channel->userdata = userdata;
    channel->open = 0;
    
    channel->on_toggle = on_toggle;
    channel->on_data = on_data;
  }
  return channel;
}

void telnet_channel_free(telnet_channel* channel)
{
  if (channel)
  {
    telnet_channel_unregister(channel);
    free(channel);
  }
}

telnet_error telnet_channel_register(telnet_channel* channel,
                                     telnet_nvt* nvt,
                                     short option,
                                     telnet_channel_mode local,
                                     telnet_channel_mode remote)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  else if (channel->nvt)
    return TELNET_E_REGISTERED;
  
  channel->nvt = nvt;
  
  if (option == TELNET_MAIN_CHANNEL)
  {
    if (nvt->primary)
      return TELNET_E_REGISTERED;
    
    nvt->primary = channel;
    channel->open = 1;
  }
  else if (0 <= option && option <= 255)
  {
    if (nvt->channels[option])
      return TELNET_E_REGISTERED;
    
    telnet_qstate* q = &nvt->options[option];
    
    if (local != TELNET_CHANNEL_OFF)
    {
      q->l_current = Q_WANTYES;
      if (local == TELNET_CHANNEL_LAZY)
        q->l_lazy = 1;
      else
        telnet_send_option(nvt, IAC_WILL, option);
    }
    if (remote != TELNET_CHANNEL_OFF)
    {
      q->r_current = Q_WANTYES;
      if (remote == TELNET_CHANNEL_LAZY)
        q->r_lazy = 1;
      else
        telnet_send_option(nvt, IAC_DO, option);
    }
    
    nvt->channels[option] = channel;
    channel->option = option;
  }
  else
  {
    return TELNET_E_INVALID_OPTION;
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_channel_unregister(telnet_channel* channel)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  else if (!channel->nvt)
    return TELNET_E_OK;
  
  telnet_nvt* nvt = channel->nvt;
  
  short option = channel->option;
  if (option != TELNET_MAIN_CHANNEL)
  {
    telnet_channel_toggle(channel, TELNET_CHANNEL_LOCAL, TELNET_CHANNEL_OFF);
    telnet_channel_toggle(channel, TELNET_CHANNEL_REMOTE, TELNET_CHANNEL_OFF);
    
    memset(&nvt->options[option], 0, sizeof(telnet_qstate));
    nvt->channels[option] = NULL;
  }
  else
  {
    nvt->primary = NULL;
    channel->open = 0;
  }
  
  channel->nvt = NULL;
  channel->option = TELNET_INVALID_CHANNEL;
  
  return TELNET_E_OK;
}

telnet_error telnet_channel_get_userdata(telnet_channel* channel,
                                         void** userdata)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  
  *userdata = channel->userdata;
  return TELNET_E_OK;
}

telnet_error telnet_channel_get_nvt(telnet_channel* channel, telnet_nvt** nvt)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  else if (!channel->nvt)
    return TELNET_E_REGISTERED;
  
  *nvt = channel->nvt;
  return TELNET_E_OK;
}

telnet_error telnet_channel_get_option(telnet_channel* channel, short* option)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  else if (!channel->nvt)
    return TELNET_E_REGISTERED;
  
  *option = channel->option;
  return TELNET_E_OK;
}

telnet_error telnet_channel_get_status(telnet_channel* channel,
                                       telnet_channel_provider where,
                                       telnet_channel_mode* on)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  
  if (channel->option == TELNET_MAIN_CHANNEL)
  {
    *on = TELNET_CHANNEL_ON;
  }
  else
  {
    telnet_qstate* q = &channel->nvt->options[channel->option];
    if (where == TELNET_CHANNEL_REMOTE)
    {
      switch (q->l_current)
      {
        case Q_YES: case Q_WANTNO: case Q_WANTNOYES:
          *on = TELNET_CHANNEL_ON;
          break;
        default:
          *on = TELNET_CHANNEL_OFF;
          break;
      }
    }
    else if (where == TELNET_CHANNEL_LOCAL)
    {
      switch (q->r_current)
      {
        case Q_YES: case Q_WANTNO: case Q_WANTNOYES:
          *on = TELNET_CHANNEL_ON;
          break;
        default:
          *on = TELNET_CHANNEL_OFF;
          break;
      }
    }
  }
  
  return TELNET_E_OK;
}

telnet_error telnet_channel_send(telnet_channel* channel,
                                 const telnet_byte* data,
                                 size_t length)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  else if (!channel->open)
    return TELNET_E_NOT_OPEN;
  
  telnet_send_subnegotiation(channel->nvt, channel->option, data, length);
  
  return TELNET_E_OK;
}

telnet_error telnet_channel_toggle(telnet_channel* channel,
                                   telnet_channel_provider where,
                                   telnet_channel_mode what)
{
  if (!channel)
    return TELNET_E_BAD_CHANNEL;
  else if (!channel->nvt || channel->option == TELNET_MAIN_CHANNEL)
    return TELNET_E_REGISTERED;
  
  telnet_nvt* nvt = channel->nvt;
  telnet_byte option = (telnet_byte)channel->option;
  telnet_qstate* q =  &nvt->options[option];
  
  if (what == TELNET_CHANNEL_OFF)
  { // Disabling
    if (where == TELNET_CHANNEL_REMOTE)
    {
      switch (q->r_current)
      {
        case Q_YES:
          telnet_send_option(nvt, IAC_DONT, option);
          CLOSE_CALLBACK(channel, TELNET_CHANNEL_REMOTE);
          q->r_current = Q_WANTNO;
          break;
        case Q_WANTNOYES:
          q->r_current = Q_WANTNO;
          break;
        case Q_WANTYES:
          q->r_current = (q->r_lazy) ? Q_NO : Q_WANTYESNO;
          break;
      }
      q->r_lazy = 0;
    }
    else if (where == TELNET_CHANNEL_LOCAL)
    {
      switch (q->l_current)
      {
        case Q_YES:
          telnet_send_option(nvt, IAC_WONT, option);
          CLOSE_CALLBACK(channel, TELNET_CHANNEL_LOCAL);
          q->l_current = Q_WANTNO;
          break;
        case Q_WANTNOYES:
          q->l_current = Q_WANTNO;
          break;
        case Q_WANTYES:
          q->l_current = (q->l_lazy) ? Q_NO : Q_WANTYESNO;
          break;
      }
      q->l_lazy = 0;
    }
  }
  else
  { // Enabling (maybe lazy)
    if (where == TELNET_CHANNEL_REMOTE)
    {
      switch (q->r_current)
      {
        case Q_NO:
          if (what != TELNET_CHANNEL_LAZY)
            telnet_send_option(nvt, IAC_DO, option);
          q->r_current = Q_WANTYES;
          break;
        case Q_WANTNO:
          q->r_current = Q_WANTNOYES;
          break;
        case Q_WANTYES:
          if (q->r_lazy && what != TELNET_CHANNEL_LAZY)
            telnet_send_option(nvt, IAC_DO, option);
          break;
        case Q_WANTYESNO:
          q->r_current = Q_WANTYES;
          break;
      }
      q->r_lazy = (what == TELNET_CHANNEL_LAZY);
    }
    else if (where == TELNET_CHANNEL_LOCAL)
    {
      switch (q->l_current)
      {
        case Q_NO:
          if (what != TELNET_CHANNEL_LAZY)
            telnet_send_option(nvt, IAC_WILL, option);
          q->l_current = Q_WANTYES;
          break;
        case Q_WANTNO:
          q->l_current = Q_WANTNOYES;
          break;
        case Q_WANTYES:
          if (q->l_lazy && what != TELNET_CHANNEL_LAZY)
            telnet_send_option(nvt, IAC_WILL, option);
          break;
        case Q_WANTYESNO:
          q->l_current = Q_WANTYES;
          break;
      }
      q->l_lazy = (what == TELNET_CHANNEL_LAZY);
    }
  }
  
  return TELNET_E_OK;
}

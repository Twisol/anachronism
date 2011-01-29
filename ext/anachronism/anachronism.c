#include <string.h>
#include "ruby.h"
#include "telnet_nvt.h"

#define STR2SYM(str) ID2SYM(rb_intern((str)))

static VALUE g_iac2sym = Qnil;
static VALUE g_sym2iac = Qnil;

// 'symbol' must be a symbol or a number
static telnet_byte symbol2iac(const VALUE symbol)
{
  VALUE result = rb_hash_lookup(g_sym2iac, symbol);
  if (result == Qnil)
    result = symbol;
  return FIX2INT(result);
}

static VALUE iac2symbol(const telnet_byte command)
{
  VALUE iac = INT2FIX(command);
  return rb_hash_lookup2(g_iac2sym, iac, iac);
}

static void on_recv(telnet_nvt* nvt, telnet_event* event)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  VALUE handler = rb_iv_get(self, "@handler");
  
  VALUE hash = rb_hash_new();
  
  switch (event->type)
  {
    case TELNET_EV_DATA:
      rb_hash_aset(hash, STR2SYM("type"), STR2SYM("data"));
      rb_hash_aset(hash, STR2SYM("data"), rb_str_new(event->text_event.data, event->text_event.length));
      break;
    case TELNET_EV_COMMAND:
      rb_hash_aset(hash, STR2SYM("type"), STR2SYM("command"));
      rb_hash_aset(hash, STR2SYM("command"), iac2symbol(event->command_event.command));
      break;
    case TELNET_EV_OPTION:
      rb_hash_aset(hash, STR2SYM("type"), STR2SYM("option"));
      rb_hash_aset(hash, STR2SYM("command"), iac2symbol(event->option_event.command));
      rb_hash_aset(hash, STR2SYM("option"), INT2FIX(event->option_event.option));
      break;
    case TELNET_EV_SUBNEGOTIATION:
      rb_hash_aset(hash, STR2SYM("type"), STR2SYM("subnegotiation"));
      rb_hash_aset(hash, STR2SYM("active"), event->subnegotiation_event.active ? Qtrue : Qfalse);
      rb_hash_aset(hash, STR2SYM("option"), INT2FIX(event->subnegotiation_event.option));
      break;
    case TELNET_EV_WARNING:
      rb_hash_aset(hash, STR2SYM("type"), STR2SYM("warning"));
      rb_hash_aset(hash, STR2SYM("message"), rb_str_new_cstr(event->warning_event.message));
      rb_hash_aset(hash, STR2SYM("position"), INT2NUM(event->warning_event.position));
      break;
  }
  
  rb_funcall(handler, rb_intern("on_recv"), 1, hash);
}

static void on_send(telnet_nvt* nvt, const telnet_byte* data, size_t length)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  
  VALUE handler = rb_iv_get(self, "@handler");
  rb_funcall(handler, rb_intern("on_send"), 1, rb_str_new(data, length));
}


static VALUE parser_mark(telnet_nvt* nvt)
{
}

static VALUE parser_free(telnet_nvt* nvt)
{
  telnet_nvt_delete(nvt);
}

static VALUE parser_allocate(VALUE klass)
{
  telnet_nvt* nvt = telnet_nvt_new();
  
  VALUE object = Data_Wrap_Struct(klass, NULL, parser_free, nvt);
  telnet_nvt_set_userdata(nvt, (void*)object);
  
  telnet_callbacks* callbacks = NULL;
  telnet_nvt_get_callbacks(nvt, &callbacks);
  callbacks->on_recv = &on_recv;
  callbacks->on_send = &on_send;
  
  return object;
}

static VALUE parser_initialize(VALUE self, VALUE callbacks)
{
  rb_iv_set(self, "@handler", callbacks);
  return Qnil;
}

static VALUE parser_recv(VALUE self, VALUE data)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  size_t bytes_used;
  telnet_nvt_recv(nvt, str, len, &bytes_used);
  return rb_str_substr(rb_str, bytes_used, len-bytes_used);
}

static VALUE parser_send_data(VALUE self, VALUE data)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  // TODO: Handle TELNET_E_ALLOC error case
  if (telnet_nvt_data(nvt, str, len) == TELNET_E_ALLOC)
    rb_raise(rb_eNoMemError, "Unable to allocate output buffer for outgoing Telnet data.");
  
  return Qnil;
}

static VALUE parser_send_command(VALUE self, VALUE command)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_command(nvt, symbol2iac(command));
  return Qnil;
}

static VALUE parser_send_option(VALUE self, VALUE command, VALUE option)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_option(nvt, symbol2iac(command), NUM2INT(option));
  return Qnil;
}

static VALUE parser_send_subnegotiation(VALUE self, VALUE option, VALUE data)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  telnet_nvt_subnegotiation(nvt, NUM2INT(option), str, len);
  return Qnil;
}

static VALUE parser_halt(VALUE self)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_halt(nvt);
  return Qnil;
}

static void setup_iac_hash()
{
  const char* codes[] = {"SE",   "NOP", "DM",   "BRK",
                         "IP",   "AO",  "AYT",  "EC",
                         "EL",   "GA",  "SB",   "WILL",
                         "WONT", "DO",  "DONT", "IAC",
                        };
  g_sym2iac = rb_hash_new();
  g_iac2sym = rb_hash_new();
  
  VALUE sym = Qnil;
  VALUE val = Qnil;
  int i;
  for (i = 0; i < 16; ++i)
  {
    sym = STR2SYM(codes[i]);
    val = INT2FIX(i+240);
    
    rb_hash_aset(g_sym2iac, sym, val);
    rb_hash_aset(g_iac2sym, val, sym);
  }
}

void Init_anachronism()
{
  setup_iac_hash();
  
  VALUE mAnachronism = rb_define_module("Anachronism");
  VALUE cNVT = rb_define_class_under(mAnachronism, "NVT", rb_cObject);
  rb_define_alloc_func(cNVT, parser_allocate);
  rb_define_method(cNVT, "initialize", parser_initialize, 1);
  rb_define_method(cNVT, "recv", parser_recv, 1);
  rb_define_method(cNVT, "send_data", parser_send_data, 1);
  rb_define_method(cNVT, "send_command", parser_send_command, 1);
  rb_define_method(cNVT, "send_option", parser_send_option, 2);
  rb_define_method(cNVT, "send_subnegotiation", parser_send_subnegotiation, 2);
  rb_define_method(cNVT, "halt", parser_halt, 0);
}

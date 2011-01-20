#include <string.h>
#include "ruby.h"
#include "telnet_nvt.h"

#define STR2SYM(str) ID2SYM(rb_intern((str)))

VALUE mAnachronism = Qnil;

static telnet_byte symbol2iac(const VALUE symbol)
{
  telnet_byte iac = 0;
  
  if (symbol == STR2SYM("SE"))        iac = IAC_SE;
  else if (symbol == STR2SYM("NOP"))  iac = IAC_NOP;
  else if (symbol == STR2SYM("DM"))   iac = IAC_DM;
  else if (symbol == STR2SYM("BRK"))  iac = IAC_BRK;
  else if (symbol == STR2SYM("IP"))   iac = IAC_IP;
  else if (symbol == STR2SYM("AO"))   iac = IAC_AO;
  else if (symbol == STR2SYM("AYT"))  iac = IAC_AYT;
  else if (symbol == STR2SYM("EC"))   iac = IAC_EC;
  else if (symbol == STR2SYM("EL"))   iac = IAC_EL;
  else if (symbol == STR2SYM("GA"))   iac = IAC_GA;
  else if (symbol == STR2SYM("SB"))   iac = IAC_SB;
  else if (symbol == STR2SYM("WILL")) iac = IAC_WILL;
  else if (symbol == STR2SYM("WONT")) iac = IAC_WONT;
  else if (symbol == STR2SYM("DO"))   iac = IAC_DO;
  else if (symbol == STR2SYM("DONT")) iac = IAC_DONT;
  else if (symbol == STR2SYM("IAC"))  iac = IAC_IAC;
  else                                iac = FIX2INT(symbol);
  
  return iac;
}

static VALUE iac2symbol(const telnet_byte command)
{
  VALUE symbol = Qnil;
  
  switch (command)
  {
    case IAC_SE:   symbol = STR2SYM("SE");    break;
    case IAC_NOP:  symbol = STR2SYM("NOP");   break;
    case IAC_DM:   symbol = STR2SYM("DM");    break;
    case IAC_BRK:  symbol = STR2SYM("BRK");   break;
    case IAC_IP:   symbol = STR2SYM("IP");    break;
    case IAC_AO:   symbol = STR2SYM("AO");    break;
    case IAC_AYT:  symbol = STR2SYM("AYT");   break;
    case IAC_EC:   symbol = STR2SYM("EC");    break;
    case IAC_EL:   symbol = STR2SYM("EL");    break;
    case IAC_GA:   symbol = STR2SYM("GA");    break;
    case IAC_SB:   symbol = STR2SYM("SB");    break;
    case IAC_WILL: symbol = STR2SYM("WILL");  break;
    case IAC_WONT: symbol = STR2SYM("WONT");  break;
    case IAC_DO:   symbol = STR2SYM("DO");    break;
    case IAC_DONT: symbol = STR2SYM("DONT");  break;
    case IAC_IAC:  symbol = STR2SYM("IAC");   break;
    default:       symbol = INT2FIX(command); break;
  }
  
  return symbol;
}

static void on_recv(telnet_nvt* nvt, telnet_event* event)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  VALUE handler = rb_iv_get(self, "@handler");
  
  VALUE hash = rb_hash_new();
  
  switch (event->type)
  {
    case TELNET_EV_TEXT:
      rb_hash_aset(hash, STR2SYM("type"), STR2SYM("text"));
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
  rb_funcall(handler, rb_intern("on_send"), 1,
      rb_str_new(data, length));
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
  
  VALUE object = Data_Wrap_Struct(klass, parser_mark, parser_free, nvt);
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

static VALUE parser_send_text(VALUE self, VALUE data)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  // TODO: Handle TELNET_E_ALLOC error case
  telnet_nvt_text(nvt, str, len);
  
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

void Init_anachronism()
{
  mAnachronism = rb_define_module("Anachronism");
  
  VALUE cNVT = rb_define_class_under(mAnachronism, "NVT", rb_cObject);
  rb_define_alloc_func(cNVT, parser_allocate);
  rb_define_method(cNVT, "initialize", parser_initialize, 1);
  rb_define_method(cNVT, "recv", parser_recv, 1);
  rb_define_method(cNVT, "send_text", parser_send_text, 1);
  rb_define_method(cNVT, "send_command", parser_send_command, 1);
  rb_define_method(cNVT, "send_option", parser_send_option, 2);
  rb_define_method(cNVT, "send_subnegotiation", parser_send_subnegotiation, 2);
  rb_define_method(cNVT, "halt", parser_halt, 0);
}

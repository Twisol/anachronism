#include <string.h>
#include "ruby.h"
#include "telnet_nvt.h"

static void on_recv(telnet_nvt* nvt, telnet_event* event)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  VALUE handler = rb_iv_get(self, "@handler");
  
  switch (event->type)
  {
    case TELNET_EV_TEXT:
      rb_funcall(handler, rb_intern("on_text"), 1,
          rb_str_new(event->text_event.data, event->text_event.length));
      break;
    case TELNET_EV_COMMAND:
      rb_funcall(handler, rb_intern("on_command"), 1,
          INT2FIX(event->command_event.command));
      break;
    case TELNET_EV_OPTION:
      rb_funcall(handler, rb_intern("on_option"), 2,
          INT2FIX(event->option_event.command),
          INT2FIX(event->option_event.option));
      break;
    case TELNET_EV_SUBNEGOTIATION:
      rb_funcall(handler, rb_intern("on_subnegotiation"), 2,
          event->subnegotiation_event.active ? Qtrue : Qfalse,
          INT2FIX(event->subnegotiation_event.option));
      break;
    case TELNET_EV_WARNING:
      rb_funcall(handler, rb_intern("on_warning"), 2,
          rb_str_new_cstr(event->warning_event.message),
          INT2NUM(event->warning_event.position));
      break;
  }
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
  
  size_t bytes_used = telnet_nvt_recv(nvt, str, len);
  return INT2NUM(bytes_used);
}

static VALUE parser_send_text(VALUE self, VALUE data)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  telnet_nvt_text(nvt, str, len);
  return Qnil;
}

static VALUE parser_send_command(VALUE self, VALUE command)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_command(nvt, NUM2INT(command));
  return Qnil;
}

static VALUE parser_send_option(VALUE self, VALUE command, VALUE option)
{
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_option(nvt, NUM2INT(command), NUM2INT(option));
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
  VALUE mAnachronism = rb_define_module("Anachronism");
  
  VALUE cNative = rb_define_class_under(mAnachronism, "Native", rb_cObject);
  rb_define_alloc_func(cNative, parser_allocate);
  rb_define_method(cNative, "initialize", parser_initialize, 1);
  rb_define_method(cNative, "recv", parser_recv, 1);
  rb_define_method(cNative, "send_text", parser_send_text, 1);
  rb_define_method(cNative, "send_command", parser_send_command, 1);
  rb_define_method(cNative, "send_option", parser_send_option, 2);
  rb_define_method(cNative, "send_subnegotiation", parser_send_subnegotiation, 2);
  rb_define_method(cNative, "halt", parser_halt, 0);
}

#include <string.h>
#include "ruby.h"
#include "telnet_nvt.h"

#define STR2SYM(str) ID2SYM(rb_intern(str))

VALUE mAnachronism = Qnil;


static void got_text(telnet_nvt* nvt, const telnet_byte* text, size_t length)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  
  VALUE handler = rb_iv_get(self, "@handler");
  rb_funcall(handler, rb_intern("on_text"), 1,
      rb_str_new(text, length));
}

static void got_command(telnet_nvt* nvt, telnet_byte command)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  
  VALUE handler = rb_iv_get(self, "@handler");
  rb_funcall(handler, rb_intern("on_command"), 1,
      INT2FIX(command));
}

static void got_option(telnet_nvt* nvt, telnet_byte command, telnet_byte option)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  
  VALUE handler = rb_iv_get(self, "@handler");
  rb_funcall(handler, rb_intern("on_option"), 2,
      INT2FIX(option),
      INT2FIX(command));
}

static void got_mode(telnet_nvt* nvt, telnet_mode mode, telnet_byte extra)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  
  VALUE handler = rb_iv_get(self, "@handler");
  rb_funcall(handler, rb_intern("on_mode"), 2,
      INT2FIX(mode),
      INT2FIX(extra));
}

static void got_error(telnet_nvt* nvt, int fatal, const char* message, size_t position)
{
  VALUE self = Qnil;
  telnet_nvt_get_userdata(nvt, (void**)&self);
  
  VALUE handler = rb_iv_get(self, "@handler");
  rb_funcall(handler, rb_intern("on_error"), 3,
      INT2FIX(fatal),
      rb_str_new_cstr(message),
      INT2NUM(position));
}

static void send_data(telnet_nvt* nvt, const telnet_byte* data, size_t length)
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
  callbacks->on_text    = &got_text;
  callbacks->on_command = &got_command;
  callbacks->on_option  = &got_option;
  callbacks->on_mode    = &got_mode;
  callbacks->on_error   = &got_error;
  callbacks->on_send    = &send_data;
  
  return object;
}

static VALUE parser_initialize(VALUE self, VALUE callbacks)
{
  rb_iv_set(self, "@handler", callbacks);
  return Qnil;
}

static VALUE parser_process(VALUE self, VALUE data)
{
  telnet_nvt* nvt = Qnil;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  size_t bytes_used = telnet_nvt_parse(nvt, str, len);
  return INT2NUM(bytes_used);
}

static VALUE parser_send_text(VALUE self, VALUE data)
{
  telnet_nvt* nvt = Qnil;
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
  telnet_nvt* nvt = Qnil;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_command(nvt, NUM2INT(command));
  return Qnil;
}

static VALUE parser_halt(VALUE self)
{
  telnet_nvt* nvt = Qnil;
  Data_Get_Struct(self, telnet_nvt, nvt);
  
  telnet_nvt_halt(nvt);
  return Qnil;
}

void Init_anachronism()
{
  mAnachronism = rb_define_module("Anachronism");
  
  VALUE cNative = rb_define_class_under(mAnachronism, "Native", rb_cObject);
  rb_define_alloc_func(cNative, parser_allocate);
  rb_define_method(cNative, "initialize", parser_initialize, 1);
  rb_define_method(cNative, "process", parser_process, 1);
  rb_define_method(cNative, "send_text", parser_send_text, 1);
  rb_define_method(cNative, "send_command", parser_send_command, 1);
  rb_define_method(cNative, "halt", parser_halt, 0);
}

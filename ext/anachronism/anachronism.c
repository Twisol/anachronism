#include <string.h>
#include "ruby.h"
#include "telnet_nvt.h"

#define NVT(nvt) ((telnet_nvt*)(nvt))
#define RB_NVT(nvt) ((rb_telnet_nvt*)(nvt))

#define STR2SYM(str) ID2SYM(rb_intern(str))

typedef struct rb_telnet_nvt
{
  telnet_nvt base;
  unsigned char subneg; // boolean
  VALUE object;
} rb_telnet_nvt;

static mAnachronism = Qnil;


static void emit_event(const char* type, VALUE data)
{
  rb_yield_values(2, STR2SYM(type), data);
}

static void got_text(telnet_nvt* nvt, const telnet_byte* text, size_t length)
{
  emit_event("text", rb_str_new(text, length));
}

static void got_command(telnet_nvt* nvt, telnet_byte command)
{
  VALUE data = INT2FIX(command);
  emit_event("command", data);
}

static void got_option(telnet_nvt* nvt, telnet_byte command, telnet_byte option)
{
  const char* strCommand = NULL;
  switch (command)
  {
    case IAC_WILL: strCommand = "will"; break;
    case IAC_WONT: strCommand = "wont"; break;
    case IAC_DO:   strCommand = "do";   break;
    case IAC_DONT: strCommand = "dont"; break;
    default:
      assert("Got something other than WILL/WONT/DO/DONT!");
  }
  
  VALUE data = rb_ary_new3(2, INT2FIX(option), STR2SYM(strCommand));
  emit_event("option", data);
}

static void got_mode(telnet_nvt* nvt, telnet_mode mode, telnet_byte extra)
{
  RB_NVT(nvt)->subneg = (mode == TELNET_SUBNEG);
  
  VALUE data = Qnil;
  if (RB_NVT(nvt)->subneg)
    data = rb_ary_new3(2, STR2SYM("begin"), INT2FIX(extra));
  else
    data = rb_ary_new3(1, STR2SYM("end"));
  
  emit_event("subnegotiation", data);
}

static void got_error(telnet_nvt* nvt, int fatal, const char* message, size_t position)
{
  VALUE type = STR2SYM(fatal ? "fatal" : "warning");
  VALUE data = rb_ary_new3(2, type, rb_str_new_cstr(message));
  emit_event("error", data);
}

static void send_data(telnet_nvt* nvt, const telnet_byte* data, size_t length)
{
  VALUE out = rb_iv_get(RB_NVT(nvt)->object, "@out");
  if (out != Qnil)
    rb_funcall(out, rb_intern("call"), 1, rb_str_new(data, length));
}


static VALUE parser_mark(telnet_nvt* nvt)
{
}

static VALUE parser_free(telnet_nvt* nvt)
{
  free(RB_NVT(nvt));
}

static VALUE parser_allocate(VALUE klass)
{
  rb_telnet_nvt* nvt = malloc(sizeof(rb_telnet_nvt));
  telnet_nvt_init(NVT(nvt)); // Don't forget this, dummy...
  VALUE object = Data_Wrap_Struct(klass, parser_mark, parser_free, nvt);
  
  NVT(nvt)->text_callback    = &got_text;
  NVT(nvt)->command_callback = &got_command;
  NVT(nvt)->option_callback  = &got_option;
  NVT(nvt)->mode_callback    = &got_mode;
  NVT(nvt)->error_callback   = &got_error;
  NVT(nvt)->send_callback    = &send_data;
  
  nvt->subneg = 0;
  nvt->object = object;
  
  return object;
}

static VALUE parser_process(VALUE self, VALUE data)
{
  rb_telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, rb_telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  // Pass the string to the parser.
  telnet_nvt_parse(NVT(nvt), str, len);
  
  return Qnil;
}

static VALUE parser_send_text(VALUE self, VALUE data)
{
  rb_telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, rb_telnet_nvt, nvt);
  
  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  telnet_nvt_text(NVT(nvt), str, len);
  
  return Qnil;
}

static VALUE parser_send_command(VALUE self, VALUE command)
{
  rb_telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, rb_telnet_nvt, nvt);
  
  if (TYPE(command) == T_SYMBOL)
  {
    VALUE command_list = rb_const_get(mAnachronism, rb_intern("COMMANDS"));
    command = rb_hash_aref(command_list, command);
  }
  
  telnet_nvt_command(NVT(nvt), NUM2INT(command));
}

void Init_anachronism()
{
  mAnachronism = rb_define_module("Anachronism");
  
  // The Parser class processes a stream of data into discrete Telnet events
  VALUE cNVT = rb_define_class_under(mAnachronism, "NVT", rb_cObject);
  rb_define_alloc_func(cNVT, parser_allocate);
  rb_define_method(cNVT, "process", parser_process, 1);
  rb_define_method(cNVT, "send_text", parser_send_text, 1);
  rb_define_method(cNVT, "send_command", parser_send_command, 1);
}

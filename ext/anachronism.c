#include "ruby.h"
#include "telnet_nvt.h"

#define NVT(nvt) ((telnet_nvt*)(nvt))
#define RB_NVT(nvt) ((rb_telnet_nvt*)(nvt))

#define STR2SYM(str) ID2SYM(rb_intern(str))

static VALUE cEvent;

typedef struct rb_telnet_nvt
{
  telnet_nvt base;
  unsigned char subneg; // boolean
  VALUE object;
} rb_telnet_nvt;

static VALUE make_telnet_data(const char* type, VALUE data)
{
  VALUE hash = rb_hash_new();
  rb_hash_aset(hash, ID2SYM(rb_intern(type)), data);
  return hash;
}

static void got_text(telnet_nvt* nvt, telnet_byte chr)
{
  const char* type = (RB_NVT(nvt)->subneg) ? "subnegotiation" : "text";
  VALUE data = rb_ary_new3(1, rb_str_new(&chr, 1));
  rb_yield(make_telnet_data(type, data));
}

static void got_command(telnet_nvt* nvt, telnet_byte command)
{
  VALUE data = rb_ary_new3(1, INT2FIX(command));
  rb_yield(make_telnet_data("command", data));
}

static void got_option(telnet_nvt* nvt, telnet_byte command, telnet_byte option)
{
  const char* strCommand = NULL;
  switch (command)
  {
    case IAC_WILL: strCommand = "WILL"; break;
    case IAC_WONT: strCommand = "WONT"; break;
    case IAC_DO:   strCommand = "DO";   break;
    case IAC_DONT: strCommand = "DONT"; break;
    default:
      assert("Got something other than WILL/WONT/DO/DONT!");
  }
  
  VALUE data = rb_ary_new3(2, INT2FIX(option), STR2SYM(strCommand));
  rb_yield(make_telnet_data("option", data));
}

static void got_mode(telnet_nvt* nvt, telnet_mode mode, telnet_byte extra)
{
  RB_NVT(nvt)->subneg = (mode == TELNET_SUBNEG);
}

static void got_error(telnet_nvt* nvt, int fatal, const char* message, size_t position)
{
  VALUE type = STR2SYM(fatal ? "fatal" : "warning");
  VALUE data = rb_ary_new3(2, type, rb_str_new_cstr(message));
  rb_yield(make_telnet_data("error", data));
}


static VALUE parser_mark(telnet_nvt* nvt)
{
}

static VALUE parser_free(telnet_nvt* nvt)
{
  free(nvt);
}

static VALUE parser_allocate(VALUE klass)
{
  rb_telnet_nvt* nvt = malloc(sizeof(rb_telnet_nvt));
  VALUE object = Data_Wrap_Struct(klass, parser_mark, parser_free, nvt);
  
  NVT(nvt)->text_callback = &got_text;
  NVT(nvt)->command_callback = &got_command;
  NVT(nvt)->option_callback = &got_option;
  NVT(nvt)->mode_callback = &got_mode;
  NVT(nvt)->error_callback = &got_error;
  
  nvt->subneg = 0;
  nvt->object = object;
  
  return object;
}

static VALUE parser_process(VALUE self, VALUE data)
{
  return rb_ary_new3(1, rb_struct_new(cEvent, STR2SYM("text"), data));
  /*
  telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, telnet_nvt, nvt);
  return data;
  */
}

void Init_anachronism()
{
  VALUE mAnachronism = rb_define_module("Anachronism");
  
  VALUE cParser = rb_define_class_under(mAnachronism, "Parser", rb_cObject);
  rb_define_alloc_func(cParser, parser_allocate);
  rb_define_method(cParser, "process", parser_process, 1);
  
  cEvent = rb_struct_define("Event", "type", "data", NULL);
  rb_define_const(mAnachronism, "Event", cEvent);
}

/*
grammar Telnet
  rule protocol
    (text | iac_sequence)*
  end
  
  rule text
    ~(iac)
  end
  
  rule iac
    "\255"
  end
  
  rule iac_sequence
    iac (command|option|subnegotiation)
  end
  
  rule command
    "\241" | "\242" | "\243" |
    "\244" | "\245" | "\246" |
    "\247" | "\248" | "\249"
  end
end
*/

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
  VALUE events;
} rb_telnet_nvt;

static VALUE cEvent = Qnil;

static VALUE new_event(const char* type, VALUE data)
{
  return rb_struct_new(cEvent, STR2SYM(type), data);
}

static void got_text(telnet_nvt* nvt, const telnet_byte* text, size_t length)
{
  const char* type = (RB_NVT(nvt)->subneg) ? "subnegotiation" : "text";
  VALUE data = rb_str_new(text, length);
  
  // Concatenate it with the previous event if it was a text event.
  size_t ary_len = RARRAY_LEN(RB_NVT(nvt)->events);
  VALUE event = rb_ary_entry(RB_NVT(nvt)->events, ary_len-1);
  if (event != Qnil && rb_struct_aref(event, STR2SYM("type")) == STR2SYM("text"))
    rb_str_cat(rb_struct_aref(event, STR2SYM("data")), text, length);
  else
    rb_ary_push(RB_NVT(nvt)->events, new_event(type, data));
}

static void got_command(telnet_nvt* nvt, telnet_byte command)
{
  VALUE data = INT2FIX(command);
  rb_ary_push(RB_NVT(nvt)->events, new_event("command", data));
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
  rb_ary_push(RB_NVT(nvt)->events, new_event("option", data));
}

static void got_mode(telnet_nvt* nvt, telnet_mode mode, telnet_byte extra)
{
  RB_NVT(nvt)->subneg = (mode == TELNET_SUBNEG);
}

static void got_error(telnet_nvt* nvt, int fatal, const char* message, size_t position)
{
  VALUE type = STR2SYM(fatal ? "fatal" : "warning");
  VALUE data = rb_ary_new3(2, type, rb_str_new_cstr(message));
  rb_ary_push(RB_NVT(nvt)->events, new_event("error", data));
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
  
  nvt->subneg = 0;
  nvt->object = object;
  nvt->events = Qnil;
  
  return object;
}

static VALUE parser_process(VALUE self, VALUE data)
{
  rb_telnet_nvt* nvt = NULL;
  Data_Get_Struct(self, rb_telnet_nvt, nvt);
  
  // Create and store an array to collect the events in
  VALUE events = rb_ary_new();
  nvt->events = events;

  // Prepare the data as a string
  VALUE rb_str = StringValue(data);
  const telnet_byte* str = (const telnet_byte*)RSTRING_PTR(rb_str);
  size_t len = RSTRING_LEN(rb_str);
  
  // Pass the string to the parser.
  telnet_nvt_recv(NVT(nvt), str, len);

  nvt->events = Qnil;
  return events;
}

void Init_anachronism()
{
  VALUE mAnachronism = rb_define_module("Anachronism");
  
  // The Parser class processes a stream of data into discrete Telnet events
  VALUE cParser = rb_define_class_under(mAnachronism, "Parser", rb_cObject);
  rb_define_alloc_func(cParser, parser_allocate);
  rb_define_method(cParser, "process", parser_process, 1);
  
  // Event embodies a single datum from the telnet stream, such as text or a command
  cEvent = rb_struct_define("Event", "type", "data", NULL);
  rb_define_const(mAnachronism, "Event", cEvent);
}

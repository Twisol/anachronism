%%{
  machine telnet_nvt_common;
  alphtype unsigned char;
  
  # Special bytes that must be handled differently from normal text:
  CR = "\r"; # Only \0 or \n may follow
  IAC = 255; # Telnet command marker
  special_byte = CR | IAC;
  
  # The only bytes that may follow a CR:
  NL = "\n";
  NUL = "\0";
  
  # The only command bytes that may follow an IAC:
  SE   = 240;
  NOP  = 241;
  DM   = 242;
  BRK  = 243;
  IP   = 244;
  AO   = 245;
  AYT  = 246;
  EC   = 247;
  EL   = 248;
  GA   = 249;
  SB   = 250;
  WILL = 251;
  WONT = 252;
  DO   = 253;
  DONT = 254;
  # IAC IAC is interpreted as a plain_text IAC byte.
  
  # Sorting the above IAC commands by type:
  basic_command_type   = NOP | DM | BRK | IP | AO | AYT | EC | EL | GA;
  arg_command_type     = WILL | WONT | DO | DONT;
  subneg_command_type  = SB;
  unknown_command_type = ^( basic_command_type
                          | arg_command_type
                          | subneg_command_type
                          | IAC
                          );

  ###
  # Plain text
  ###
  plain_text = (^special_byte)+ >start_text %text;

  ###
  # CR sequence
  ###
  cr_sequence = CR @char
                ( NUL
                | NL @char
                | ^(NUL|NL) @{fhold;} @warning_cr
                );
  
  ###
  # IAC sequence
  ###
  escaped_iac = IAC @char;
  
  basic_command = basic_command_type @basic_command;

  arg_command = arg_command_type @option_mark
                any @option_command;

  subneg_command = subneg_command_type
                   any @subneg_command
                   ( cr_sequence
                   | plain_text
                   | IAC ^(IAC|SE) @warning_iac
                   )**
                   IAC SE @subneg_command_end;

  unknown_command = unknown_command_type @warning_iac @basic_command;

  iac_sequence = IAC ( escaped_iac
                     | basic_command
                     | arg_command
                     | subneg_command
                     | unknown_command
                     );

  ###
  # Telnet stream
  ###

  # These are the three basic data formats that will be accepted
  # by the telnet parser.
  telnet_stream = ( plain_text
                  | cr_sequence
                  | iac_sequence
                  )**;

  main := telnet_stream;
}%%

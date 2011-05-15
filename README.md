# Anachronism
Anachronism is a fully-compliant implementation of [the Telnet protocol][wiki-telnet]. Fallen
out of favor in this day and age, most people only know it as a command-line
tool for debugging HTTP. Today, Telnet is most commonly used in the realm of
[MUDs][wiki-muds], though there are still a few other niches filled by Telnet.

Anachronism offers a simple API for translating between streams of data and
events, and is completely network-agnostic. Anachronism also offers **channels**, an
abstraction layer which treats Telnet as a data multiplexer. Channels make it
extremely easy to build reusable modules for Telnet sub-protocols such
as MCCP (MUD Client Compression Protocol), which can be written once and plugged
into any application that wants to include support.

[wiki-telnet]: http://en.wikipedia.org/wiki/Telnet (Telnet at Wikipedia)
[wiki-muds]: http://en.wikipedia.org/wiki/MUD (MUDs at Wikipedia)

## Installation
While Anachronism has no dependencies and is theoretically cross-platform, I've
only written a Makefile for Linux. Help would be appreciated for making this
work across more platforms.

    make
    sudo make install

This will install Anachronism's shared and static libraries to /usr/local/lib,
and its header files to /usr/local/include/anachronism/. You may also need to
run `ldconfig` to make Anachronism available to your project's compiler/linker.

## Usage
The anachronism/nvt.h header can be consulted for more complete documentation.

### Basic usage
The core type exposed by Anachronism is the telnet\_nvt, which represents the
Telnet RFC's "Network Virtual Terminal". An NVT is created using
telnet\_nvt\_new(). When creating an NVT, you must provide it with a callback to
send events to, and an optional void\* to store as the event handler's context.
You can use telnet\_recv() to process incoming data, and the telnet\_send\_\*() set
of functions to emit outgoing data.

    #include <stdio.h>
    #include <anachronism/nvt.h>
    
    void on_event(telnet_nvt* nvt, telnet_event* event)
    {
      switch (event->type)
      {
        // A data event (normal text received)
        case TELNET_EV_DATA:
        {
          telnet_data_event* ev = (telnet_data_event*)event;
          printf("[IN]: %.*s\n", ev->length, ev->data);
          break;
        }
        
        // Outgoing data emitted by the NVT
        case TELNET_EV_SEND:
        {
          telnet_send_event* ev = (telnet_send_event*)event;
          printf("[OUT]: %.*s\n", ev->length, ev->data);
          break;
        }
      }
    }
    
    int main()
    {
      // Create an NVT
      telnet_nvt* nvt = telnet_nvt_new(&on_event, NULL);
      
      // Process some incoming data
      const char* data = "foo bar baz";
      telnet_recv(nvt, (const telnet_byte*)data, strlen(data), NULL);
      
      // Free the NVT
      telnet_nvt_free(nvt);
      return 0;
    }

### Channels
Anachronism provides an abstraction of "channels" on top of the Telnet protocol,
treating Telnet as a data multiplexer. There is one "main" channel that is
always present, and 256 other channels that must be negotiated open. You can
register event handlers to these channels using telnet\_channel\_new() and
telnet\_channel\_register().

As channels may be negotiated two ways (once with the local side "serving" the
underlying Telnet option, and once with the remote side "serving"), you must
specify which directions you will allow to be opened. TELNET\_CHANNEL\_ON will
immediately attempt to open the channel, and TELNET\_CHANNEL\_LAZY will wait until
the remote side asks to open it. TELNET\_CHANNEL\_OFF will keep the channel
closed.

    #include <stdio.h>
    #include <anachronism/nvt.h>
    
    void on_event(telnet_nvt* nvt, telnet_event* event)
    {
      switch (event->type)
      {
        // Outgoing data emitted by the NVT
        case TELNET_EV_SEND:
        {
          telnet_send_event* ev = (telnet_send_event*)event;
          printf("[OUT]: %.*s\n", ev->length, ev->data);
          break;
        }
      }
    }
    
    void on_channel_toggle(telnet_channel* channel,
                           telnet_channel_mode on,
                           telnet_channel_provider who)
    {
      // `on` says whether the channel has been opened or closed
      // `who` is TELNET_CHANNEL_LOCAL if you are serving, _REMOTE otherwise
    }
    
    void on_channel_data(telnet_channel* channel,
                         telnet_channel_event type,
                         const telnet_byte* data,
                         size_t length)
    {
      // `type` is TELNET_CHANNEL_EV_BEGIN, _END, or _DATA
      // _BEGIN and _END mark where the channel is switched to or from,
      // effectively marking the boundaries of a message.
      
      // `data` and `length` are only valid when `type` is
      // TELNET_CHANNEL_EV_DATA
    }
    
    int main()
    {
      // Create an NVT
      telnet_nvt* nvt = telnet_nvt_new(&on_event, NULL);
      
      // Create a channel
      telnet_channel* channel = telnet_channel_new(&on_channel_toggle,
                                           &on_channel_data,
                                           NULL);
      
      // Register the channel
      telnet_channel_register(channel,
                              nvt,
                              230, // which channel to bind to
                                   // can be TELNET_MAIN_CHANNEL also
                              TELNET_CHANNEL_ON, // we will serve
                              TELNET_CHANNEL_OFF); // they don't serve
      
      // Process some incoming data
      const char* data = "\xFF\xFD\xE6" // IAC DO 230  (turn channel on)
                         "\xFF\xFA\xE6" // IAC SB E6   (switch to channel)
                         "foo bar baz"                 (send data)
                         "\xFF\xF0";    // IAC SE      (switch to main)
      telnet_recv(nvt, (const telnet_byte*)data, strlen(data), NULL);
      
      // Free the NVT
      telnet_nvt_free(nvt);
      return 0;
    }

### Interrupting
    TODO: Explain how to interrupt the parser.

## Alternatives
* [libtelnet][github-libtelnet], by Elanthis<br>
  It incorporates a number of (rather MUD-specific) protocols by default,
  and/but lacks the channels paradigm.

[github-libtelnet]: https://github.com/elanthis/libtelnet (libtelnet on GitHub)

## Credits
Someone from #startups on Freenode IRC suggested the name (I'm sure as a joke).
If you read this, remind me who you are so I can credit you properly!

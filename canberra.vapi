/***
  This file is part of libcanberra.

  Copyright (C) 2009 Michael 'Mickey' Lauer <mlauer vanille-media de>

  libcanberra is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 2.1 of the
  License, or (at your option) any later version.

  libcanberra is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with libcanberra. If not, see
  <http://www.gnu.org/licenses/>.
***/

using Posix;

[CCode (cprefix = "CA_", lower_case_cprefix = "ca_", cheader_filename = "canberra.h")]
namespace Canberra {

    //
    // properties
    //
    public const string PROP_MEDIA_NAME;
    public const string PROP_MEDIA_TITLE;
    public const string PROP_MEDIA_ARTIST;
    public const string PROP_MEDIA_LANGUAGE;
    public const string PROP_MEDIA_FILENAME;
    public const string PROP_MEDIA_ICON;
    public const string PROP_MEDIA_ICON_NAME;
    public const string PROP_MEDIA_ROLE;
    public const string PROP_EVENT_ID;
    public const string PROP_EVENT_DESCRIPTION;
    public const string PROP_EVENT_MOUSE_X;
    public const string PROP_EVENT_MOUSE_Y;
    public const string PROP_EVENT_MOUSE_HPOS;
    public const string PROP_EVENT_MOUSE_VPOS;
    public const string PROP_EVENT_MOUSE_BUTTON;
    public const string PROP_WINDOW_NAME;
    public const string PROP_WINDOW_ID;
    public const string PROP_WINDOW_ICON;
    public const string PROP_WINDOW_ICON_NAME;
    public const string PROP_WINDOW_X11_DISPLAY;
    public const string PROP_WINDOW_X11_SCREEN;
    public const string PROP_WINDOW_X11_MONITOR;
    public const string PROP_WINDOW_X11_XID;
    public const string PROP_APPLICATION_NAME;
    public const string PROP_APPLICATION_ID;
    public const string PROP_APPLICATION_VERSION;
    public const string PROP_APPLICATION_ICON;
    public const string PROP_APPLICATION_ICON_NAME;
    public const string PROP_APPLICATION_LANGUAGE;
    public const string PROP_APPLICATION_PROCESS_ID;
    public const string PROP_APPLICATION_PROCESS_BINARY;
    public const string PROP_APPLICATION_PROCESS_USER;
    public const string PROP_APPLICATION_PROCESS_HOST;
    public const string PROP_CANBERRA_CACHE_CONTROL;
    public const string PROP_CANBERRA_VOLUME;
    public const string PROP_CANBERRA_XDG_THEME_NAME;
    public const string PROP_CANBERRA_XDG_THEME_OUTPUT_PROFILE;
    public const string PROP_CANBERRA_ENABLE;

    //
    // errors
    //
    [CCode (cname = "CA_SUCCESS")]
    public const int SUCCESS;

    [CCode (cname = "int", cprefix = "CA_ERROR_")]
    public enum Error {
        NOTSUPPORTED,
        INVALID,
        STATE,
        OOM,
        NODRIVER,
        SYSTEM,
        CORRUPT,
        TOOBIG,
        NOTFOUND,
        DESTROYED,
        CANCELED,
        NOTAVAILABLE,
        ACCESS,
        IO,
        INTERNAL,
        DISABLED,
        FORKED
    }

    public unowned string strerror( Error code );

    //
    // callback
    //
    public delegate void FinishCallback( Context context, uint32 id, Error code );

    //
    // property list
    //
    [Compact]
    [CCode (cname = "ca_proplist", free_function = "")]
    public class Proplist {

        public static int create( Proplist* p );
        public Error destroy();
        public Error sets( string key, string value );
        [PrintfFormat]
        public Error setf( string key, string format, ... );
        public Error set( string key, void* data, size_t nbytes );
    }

    [Compact]
    [CCode (cname = "ca_context", free_function = "")]
    public class Context {

        public static Error create( Context* context );
        public Error destroy();
        public Error set_driver( string driver );
        public Error change_device( string device );
        public Error open();
        [CCode (sentinel = "")]
        public Error change_props( ... );
        public Error change_props_full( Proplist p );
        [CCode (instance_pos = 0)]
        public Error play_full( uint32 id, Proplist p, FinishCallback cb );
        [CCode (sentinel = "")]
        public Error play( uint32 id, ... );
        public Error cache_full( Proplist p );
        [CCode (sentinel = "")]
        public Error cache( ... );
        public Error cancel( uint32 id );
    }
}

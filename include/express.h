/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_EXPRESS
#define NODEPP_EXPRESS

/*────────────────────────────────────────────────────────────────────────────*/

#define MIDDL function_t<void,express_cli_t,function_t<void>>
#define CALBK function_t<void,express_cli_t>

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>
#include <nodepp/optional.h>
#include <nodepp/stream.h>
#include <nodepp/http.h>
#include <nodepp/path.h>
#include <nodepp/json.h>
#include <nodepp/url.h>
#include <nodepp/fs.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class express_cli_t : public http_t { 
protected:

    struct NODE {
        uint _status= 200;
        header_t _headers; 
    }; ptr_t<NODE> obj;

public: query_t params;

     express_cli_t ( http_t& cli ) noexcept : http_t( cli ), obj( new NODE() ) {}

     express_cli_t& sendFile( string_t dir ) { 
          if( fs::exists_file( dir ) == false )
            { process::error("file does not exist"); } file_t file ( dir, "r" );
          header( "content-length", string::to_string(file.size()) );
          header( "content-type", path::mimetype(dir) );
          write_header( obj->_status, obj->_headers );
          stream::pipe( file, *this ); return (*this);
     }

     express_cli_t& sendJSON( object_t obj ) {
          auto data = json::stringify( obj );
          header( "content-length", string::to_string(data.size()) );
          header( "content-type", path::mimetype(".json") );
          send( data ); return (*this);
     }

     express_cli_t& send( string_t msg ) { 
          header( "content-length", string::to_string(msg.size()) );
          write_header( obj->_status, obj->_headers );
          write( msg ); close(); return (*this); 
     }

     express_cli_t& header( string_t name, string_t value ) {
          obj->_headers[ name ] = value; return (*this);
     }

     express_cli_t& redirect( uint value, string_t url ) {
          header( "location", url ); status( value ); 
          send( "" ); close(); return (*this);
     }

     express_cli_t& send() {
          write_header( obj->_status, obj->_headers ); 
          return (*this);
     }

     express_cli_t& status( uint value ) {
          obj->_status = value; return (*this);
     }

     express_cli_t& redirect( string_t url ) {
          return redirect( 301, url );
     }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { struct express_item_t {
    optional_t<MIDDL> middleware;
    optional_t<CALBK> callback;
    optional_t<any_t> router;
    string_t          method;
    string_t          path;
};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class express_t {
protected:

     struct NODE {
          queue_t<express_item_t> list;
          agent_t* agent= nullptr;
          string_t path = nullptr;
          tcp_t    http;
     };   ptr_t<NODE> obj;

     bool path_match( express_cli_t& cli, string_t base, string_t path ) const noexcept {
          string_t pathname = path == nullptr ? base : path::join( base, path );

          array_t<string_t> _path[2] = {
               string::split( cli.path, '/' ), 
               string::split( pathname, '/' )
          };

          if( _path[0].size() != _path[1].size() ){ return false; }

          for ( ulong x=0; x<_path[0].size(); x++ ){ if( _path[1][x]==nullptr ){ return false; }
          elif( _path[1][x][0] == ':' ){ cli.params[_path[1][x].slice(1)]= _path[0][x]; }
          elif( _path[1][x]    == "*"         ){ continue;     }
          elif( _path[1][x]    != _path[0][x] ){ return false; }
          }

          return true;
     }

     void execute( express_item_t& data, express_cli_t& cli, function_t<void>& next ) const noexcept {
            if( data.middleware.has_value() ){ data.middleware.value()( cli, next ); }
          elif( data.callback.has_value()   ){ data.callback.value()( cli ); next(); }
          elif( data.router.has_value()     ){ 
                auto self = type::bind( data.router.value().as<express_t>() );
                     self->run( self, cli ); next();
          }
     }

     template<class T> void run( T& self, express_cli_t& cli ) const noexcept {
          auto n = self->obj->list.first(); function_t<void> next = [&](){ n = n->next; };
          while( n!=nullptr ){ if( !cli.is_available() ){ break; } 
               if(( n->data.path == nullptr && self->obj->path == nullptr )|| 
                   self->path_match( cli, self->obj->path, n->data.path ) ){
               if( n->data.method== nullptr || n->data.method == cli.method )
                 { self->execute( n->data, cli, next ); } else { next(); }
               } else { next(); }
          }
     }

public:

    express_t( agent_t* agent ) noexcept : obj( new NODE() ){ agent = obj->agent; }

    express_t() noexcept : obj( new NODE() ) {}

   ~express_t() noexcept {}

    /*.........................................................................*/

    void     set_path( string_t path ) const noexcept { obj->path = path; }

    string_t get_path() const noexcept { return obj->path; }

    /*.........................................................................*/

    bool is_closed() const noexcept { return obj->http.is_closed(); }

    void close() const noexcept { obj->http.close(); }

    /*.........................................................................*/

    const express_t& USE( string_t _path, express_t cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         cb.set_path( path::join( obj->path, _path ) );
         item.path       = nullptr;
         item.method     = nullptr;
         item.router     = optional_t<any_t>(cb);
         obj->list.push( item ); return (*this);
    }

    const express_t& USE( express_t cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         cb.set_path( path::join( obj->path, "" ) );
         item.path       = nullptr;
         item.method     = nullptr;
         item.router     = optional_t<any_t>(cb);
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& USE( string_t _path, MIDDL cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.middleware = optional_t<MIDDL>(cb);
         item.method     = nullptr;
         item.path       = _path;
         obj->list.push( item ); return (*this);
    }

    const express_t& USE( MIDDL cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.middleware = optional_t<MIDDL>(cb);
         item.method     = nullptr;
         item.path       = nullptr;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& GET( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "GET";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& GET( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "GET";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& POST( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "POST";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& POST( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "POST";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& DELETE( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "DELETE";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& DELETE( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "DELETE";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& PUT( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "PUT";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& PUT( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "PUT";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& HEAD( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "HEAD";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& HEAD( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "HEAD";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& TRACE( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "TRACE";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& TRACE( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "TRACE";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& PATCH( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "PATCH";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& PATCH( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "PATCH";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& OPTIONS( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "OPTIONS";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& OPTIONS( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "OPTIONS";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_t& CONNECT( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "CONNECT";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_t& CONNECT( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "CONNECT";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    template<class... T>void listen( T... args ) const noexcept {
          auto self = type::bind( this );

          function_t<void,http_t> cb = [=]( http_t cli ){
               express_cli_t res (cli); run( self, res );
          };

          obj->http = http::server( cb, obj->agent );
          obj->http.listen( args... );
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace express {  

     express_t create() { return express_t(); }

     express_t file( string_t base ) { express_t app;

          app.GET([=]( express_cli_t cli ){

               string_t pth = regex::replace( cli.path, "/[^/]+", base ); 
               string_t dir = pth=="/" ? path::join( base,"index" ): pth;

               if( fs::exists_file(dir+".html") == true ){ dir += ".html"; }
               
               if( fs::exists_file(dir) == false ){
               if( fs::exists_file( path::join( base, "404.html" ) ) ){
                   dir = path::join( base, "404.html" );
               } else {
                   cli.status(404); cli.send("Oops 404 Error"); return;
               }}

               auto str = fs::readable( dir );

               if( cli.headers["Range"].empty() ){

                    cli.header( "Content-Length", string::to_string(str.size()) );
                    cli.header( "Cache-Control", "public, max-age=3600" );
                    cli.header( "Content-Type",   path::mimetype(dir) );
                    cli.send();

                    if( !regex::test(path::mimetype(dir),"audio|video",true) ) 
                         stream::pipe( str, cli );

               } elif ( !cli.headers["Range"].empty() ) {

                    array_t<string_t> range = regex::match_all(cli.headers["Range"],"\\d+",true);
                     ulong rang[2]; rang[0] = string::to_ulong( range[0] );
                           rang[1]= min( rang[0]+CHUNK_MB(10), str.size()-1 );

                    cli.header( "Content-Range", string::format("bytes %lu-%lu/%lu",rang[0],rang[1],str.size()) );
                    cli.header( "Content-Type",  path::mimetype(dir) );
                    cli.header( "Accept-Range", "bytes" ); 
                    cli.status(206); cli.send();

                    str.set_range( rang[0], rang[1] ); stream::pipe( str, cli );

               }
          });

          return app;
     }

}}

/*────────────────────────────────────────────────────────────────────────────*/

#undef CALBK
#undef MIDDL
#endif
/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_EXPRESS_HTTP
#define NODEPP_EXPRESS_HTTP

/*────────────────────────────────────────────────────────────────────────────*/

#define MIDDL function_t<void,express_http_t,function_t<void>>
#define CALBK function_t<void,express_http_t>

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>

#include <nodepp/optional.h>
#include <nodepp/cookie.h>
#include <nodepp/stream.h>
#include <nodepp/http.h>
#include <nodepp/path.h>
#include <nodepp/json.h>
#include <nodepp/zlib.h>
#include <nodepp/url.h>
#include <nodepp/fs.h>


/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class express_http_t : public http_t { 
protected:

    struct NODE {
        header_t _headers;
        cookie_t _cookies;
        uint  status= 200;
        char  state = 1;
    };  ptr_t<NODE> exp;

public: query_t params;

     express_http_t ( http_t& cli ) noexcept : http_t( cli ), exp( new NODE() ) { exp->state = 1; }

    ~express_http_t () noexcept { if( exp.count() > 1 ){ return; } free(); exp->state = 0; }

     express_http_t () noexcept : exp( new NODE() ) { exp->state = 0; } 

    /*.........................................................................*/

    bool is_express_available() const noexcept { return exp->state > 0; }

    bool is_express_closed() const noexcept { return exp->state <= 0; }

    /*.........................................................................*/

     express_http_t& sendFile( string_t dir ) { 
          if( exp->state == 0 ){ return (*this); } if( fs::exists_file( dir ) == false )
            { process::error("file does not exist"); } file_t file ( dir, "r" );
              header( "content-length", string::to_string(file.size()) );
              header( "content-type", path::mimetype(dir) );
          if( headers["Accept-Encoding"].empty() == false ){
              header( "Content-Encoding", "gzip" ); send();
              zlib::gzip::pipe( file, *this );
          } else {
              send(); stream::pipe( file,*this );
          }   exp->state = 0; return (*this);
     }

     express_http_t& sendJSON( object_t json ) {
          if( exp->state == 0 ){ return (*this); } auto data = json::stringify(json);
          header( "content-length", string::to_string(data.size()) );
          header( "content-type", path::mimetype(".json") );
          send( data ); exp->state = 0; return (*this);
     }

     express_http_t& send( string_t msg ) { 
          if( exp->state == 0 ){ return (*this); }
          header( "content-length", string::to_string(msg.size()) );
          write_header( exp->status, exp->_headers );
          write( msg ); close(); exp->state = 0;
          return (*this); 
     }

     express_http_t& cookie( string_t name, string_t value ) {
          if( exp->state == 0 ){ return (*this); } 
              exp->_cookies[ name ] = value;
          header( "Set-Cookie", cookie::format( exp->_cookies ) );
          return (*this);
     }

     express_http_t& header( string_t name, string_t value ) {
          if( exp->state == 0 )    { return (*this); }
          exp->_headers[name]=value; return (*this);
     }

     express_http_t& render( string_t msg ) {
          if( exp->state == 0 )    { return (*this); }
          header( "Content-Type", path::mimetype(".html") );
          send( msg ); return (*this);
     }

     express_http_t& redirect( uint value, string_t url ) {
          if( exp->state == 0 ){ return (*this); }
          header( "location",url );status( value ); 
          send(); exp->state = 0; return (*this);
     }

     express_http_t& clear_cookies() {
          if( exp->state == 0 ){ return (*this); } 
          header( "Clear-Site-Data", "\"cookies\"" );
          return (*this);
     }

     express_http_t& status( uint value ) {
          if( exp->state == 0 ){ return (*this); }
              exp->status=value; return (*this);
     }

     express_http_t& send() {
          if( exp->state == 0 ){ return (*this); }
          write_header(exp->status,exp->_headers); 
          exp->state = 0; return (*this);
     }

     express_http_t& redirect( string_t url ) {
          if( exp->state == 0 ){ return (*this); }
          return redirect( 302, url );
     }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class express_tcp_t {
protected:

     struct express_item_t {
          optional_t<MIDDL> middleware;
          optional_t<CALBK> callback;
          optional_t<any_t> router;
          string_t          method;
          string_t          path;
     };

     struct NODE {
          queue_t<express_item_t> list;
          agent_t* agent= nullptr;
          string_t path = nullptr;
          tcp_t    fd;
     };   ptr_t<NODE> obj;

     bool path_match( express_http_t& cli, string_t base, string_t path ) const noexcept {
          string_t pathname = path.empty() ? base : path::join( base, path );

          array_t<string_t> _path[2] = {
               string::split( cli.path, '/' ), 
               string::split( pathname, '/' )
          };

          if( cli.path == pathname || ( base.empty() && cli.path == path ) ){ return true; }
          if( _path[0].size() != _path[1].size() ){ return false; }

          for ( ulong x=0; x<_path[0].size(); x++ ){ if( _path[1][x]==nullptr ){ return false; }
          elif( _path[1][x][0] == ':' ){ cli.params[_path[1][x].slice(1)]= _path[0][x]; }
          elif( _path[1][x]    == "*"         ){ continue;     }
          elif( _path[1][x]    == nullptr     ){ continue;     }
          elif( _path[1][x]    != _path[0][x] ){ return false; }
          }

          return true;
     }

     void execute( express_item_t& data, express_http_t& cli, function_t<void>& next ) const noexcept {
            if( cli.is_express_closed()     ){ next(); }
          elif( data.middleware.has_value() ){ data.middleware.value()( cli, next ); }
          elif( data.callback.has_value()   ){ data.callback.value()( cli ); next(); }
          elif( data.router.has_value()     ){ 
                auto self = type::bind( data.router.value().as<express_tcp_t>() );
                     self->run( self, cli ); next();
          }
     }

     template<class T> void run( T& self, express_http_t& cli ) const noexcept {
          auto n = self->obj->list.first(); function_t<void> next = [&](){ n = n->next; };
          while( n!=nullptr ){ if( !cli.is_available() ){ break; }
               if(( n->data.path == nullptr && regex::test( cli.path, self->obj->path )) 
               || ( n->data.path == nullptr && self->obj->path == nullptr ) 
               || self->path_match( cli, self->obj->path, n->data.path )){
               if( n->data.method== nullptr || n->data.method== cli.method )
                 { self->execute( n->data, cli, next ); } else { next(); }
               } else { next(); }
          }
     }

public:

    express_tcp_t( agent_t* agent ) noexcept : obj( new NODE() ){ obj->agent = agent; }

    express_tcp_t() noexcept : obj( new NODE() ) {}

   ~express_tcp_t() noexcept {}

    /*.........................................................................*/

    void     set_path( string_t path ) const noexcept { obj->path = path; }

    string_t get_path() const noexcept { return obj->path; }

    /*.........................................................................*/

    bool is_closed() const noexcept { return obj->fd.is_closed(); }

    void close() const noexcept { obj->fd.close(); }

    /*.........................................................................*/

    const express_tcp_t& USE( string_t _path, express_tcp_t cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         cb.set_path( path::join( obj->path, _path ) );
         item.path       = nullptr;
         item.method     = nullptr;
         item.router     = optional_t<any_t>(cb);
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& USE( express_tcp_t cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         cb.set_path( path::join( obj->path, "" ) );
         item.path       = nullptr;
         item.method     = nullptr;
         item.router     = optional_t<any_t>(cb);
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& USE( string_t _path, MIDDL cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.middleware = optional_t<MIDDL>(cb);
         item.method     = nullptr;
         item.path       = _path;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& USE( MIDDL cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.middleware = optional_t<MIDDL>(cb);
         item.method     = nullptr;
         item.path       = nullptr;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& ALL( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = nullptr;
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& ALL( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = nullptr;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& GET( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "GET";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& GET( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "GET";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& POST( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "POST";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& POST( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "POST";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& DELETE( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "DELETE";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& DELETE( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "DELETE";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& PUT( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "PUT";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& PUT( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "PUT";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& HEAD( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "HEAD";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& HEAD( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "HEAD";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& TRACE( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "TRACE";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& TRACE( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "TRACE";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& PATCH( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "PATCH";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& PATCH( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "PATCH";
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& OPTIONS( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "OPTIONS";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& OPTIONS( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "OPTIONS";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    /*.........................................................................*/

    const express_tcp_t& CONNECT( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "CONNECT";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item ); return (*this);
    }

    const express_tcp_t& CONNECT( CALBK cb ) const noexcept {
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
               express_http_t res(cli); 
               self->run( self, res );
          };

          obj->fd=http::server( cb, obj->agent );
          obj->fd.listen( args... );
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace express { namespace http {  

     template< class... T > express_tcp_t add( T... args ) { 
        return express_tcp_t( args... ); 
     }

     express_tcp_t file( string_t base ) { 
          
          express_tcp_t app;

          app.GET([=]( express_http_t cli ){

               auto pth = regex::replace( cli.path, app.get_path(), "/" );
               string_t dir = pth.empty() ? path::join( base, "" ) :
                                            path::join( base,pth ) ;

               if ( dir.empty() ){ dir = path::join( base, "index.html" ); }
               if ( dir[dir.last()] == '/' ){ dir += "index.html"; }

               if( fs::exists_file(dir+".html") == true ){ dir += ".html"; }
               if( fs::exists_file(dir) == false || dir == base ){
               if( fs::exists_file( path::join( base, "404.html" ) )){
                   dir = path::join( base, "404.html" );
               } else { 
                   cli.status(404).send("Oops 404 Error"); 
                   return; 
               }}

               auto str = fs::readable( dir );

               if ( cli.headers["Range"].empty() == true ){
               if ( cli.headers["Accept-Encoding"].empty() == false ){
                    cli.header( "Content-Encoding", "gzip" );
               }    cli.header( "Content-Length", string::to_string(str.size()) );
                    cli.header( "Cache-Control", "public, max-age=3600" );
                    cli.header( "Content-Type",   path::mimetype(dir) );
                    cli.send();

                    if( !regex::test(path::mimetype(dir),"audio|video",true) ) {
                    if(  cli.headers["Accept-Encoding"].empty() == false ){
                         zlib::gzip::pipe( str, cli );
                    } else { stream::pipe( str, cli ); }}

               } else {

                    array_t<string_t> range = regex::match_all(cli.headers["Range"],"\\d+",true);
                     ulong rang[2]; rang[0] = string::to_ulong( range[0] );
                           rang[1]= min(rang[0]+CHUNK_MB(10),str.size()-1);

                    cli.header( "Content-Range", string::format("bytes %lu-%lu/%lu",rang[0],rang[1],str.size()) );
                    cli.header( "Content-Type",  path::mimetype(dir) ); cli.header( "Accept-Range", "bytes" ); 
                    cli.status(206); cli.send();

                    str.set_range( rang[0], rang[1] ); stream::pipe( str, cli );

               }
          });

          return app;
     }

}}}

/*────────────────────────────────────────────────────────────────────────────*/

#undef CALBK
#undef MIDDL
#endif
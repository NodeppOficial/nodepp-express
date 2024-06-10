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
#include <nodepp/regex.h>
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

    const express_cli_t& sendFile( string_t dir ) { 
        if( fs::exists_file( dir ) == false )
          { process::error("file does not exist"); } file_t file ( dir, "r" );
        header( "content-length", string::to_string(file.size()) );
        header( "content-type", path::mimetype(dir) );
        write_header( obj->_status, obj->_headers );
        stream::pipe( file, *this ); return (*this);
    }

    const express_cli_t& header( string_t name, string_t value ) {
        obj->_headers[ name ] = value; return (*this);
    }

    const express_cli_t& redirect( uint value, string_t url ) {
        header( "location", url ); status( value ); 
        send( "" ); close(); return (*this);
    }

    const express_cli_t& sendJSON( object_t obj ) {
        auto data = json::stringify( obj );
        header( "content-length", string::to_string(data.size()) );
        header( "content-type", path::mimetype(".json") );
        send( data ); return (*this);
    }

    const express_cli_t& send( string_t msg ) { 
        header( "content-length", string::to_string(msg.size()) );
        write_header( obj->_status, obj->_headers );
        write( msg ); close(); return (*this); 
    }

    const express_cli_t& status( uint value ) {
        obj->_status = value; return (*this);
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
        tcp_t    http;
    };  ptr_t<NODE> obj;

    bool path_match( http_t& path, string_t paths ) const noexcept {
         return true;
    }

    void execute( express_item_t& data, express_cli_t& cli, function_t<void>& next ) const noexcept {
          if( data.middleware.has_value() ){ data.middleware.value()( cli, next ); }
        elif( data.callback.has_value()   ){ data.callback.value()( cli ); next(); }
    }

public:

    /*.........................................................................*/

    express_t( agent_t* agent ) noexcept : obj( new NODE() ){ agent = obj->agent; }

    express_t() noexcept : obj( new NODE() ) {}

   ~express_t() noexcept {}

    /*.........................................................................*/

    const express_t& USE( string_t _path, MIDDL cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.middleware = optional_t<MIDDL>(cb);
         item.method     = nullptr;
         item.path       = _path;
         obj->list.push( item );
         return (*this);
    }

    const express_t& USE( MIDDL cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.middleware = optional_t<MIDDL>(cb);
         item.path       = nullptr;
         item.method     = nullptr;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& USE( string_t path, express_t cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.router     = optional_t<any_t>(cb);
         item.method     = nullptr;
         item.path       = path;
         obj->list.push( item );
         return (*this);
    }

    const express_t& USE( express_t cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.router     = optional_t<any_t>(cb);
         item.method     = nullptr;
         item.path       = nullptr;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& GET( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = _path;
         item.method   = "GET";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& GET( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "GET";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& POST( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "POST";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& POST( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "POST";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& DELETE( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "DELETE";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& DELETE( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "DELETE";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& PUT( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "PUT";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& PUT( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "PUT";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& HEAD( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "HEAD";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& HEAD( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "HEAD";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& TRACE( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "TRACE";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& TRACE( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "TRACE";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& PATCH( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "PATCH";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& PATCH( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.path     = nullptr;
         item.method   = "PATCH";
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    const express_t& OPTIONS( string_t _path, CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "OPTIONS";
         item.path     = _path;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    const express_t& OPTIONS( CALBK cb ) const noexcept {
         express_item_t item; memset( &item, sizeof(item), 0 );
         item.method   = "OPTIONS";
         item.path     = nullptr;
         item.callback = cb;
         obj->list.push( item );
         return (*this);
    }

    /*.........................................................................*/

    bool is_closed() const noexcept { return obj->http.is_closed(); }

    void close() const noexcept { obj->http.close(); }

    /*.........................................................................*/

    template<class... T>void listen( T... args ) const {
        auto self = type::bind( this );

        function_t<void,http_t> cb = [=]( http_t cli ){
            express_cli_t res (cli); auto n = self->obj->list.first();
            function_t<void> next = [&](){ n= n->next; };
            
            while( n!=nullptr ){ if( !cli.is_available() ){ break; } if( n->data.path == nullptr ){

                 if( n->data.method == nullptr    ){ self->execute( n->data, res, next ); } 
               elif( n->data.method == cli.method ){ self->execute( n->data, res, next ); } 
               else{ next(); }

               } elif( self->path_match( cli, n->data.path ) ) {

                 if( n->data.method == nullptr    ){ self->execute( n->data, res, next ); } 
               elif( n->data.method == cli.method ){ self->execute( n->data, res, next ); } 
               else{ next(); }

               } else { next(); }

            }

        };

        obj->http = http::server( cb, obj->agent );
        obj->http.listen( args... );

    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace express {  }}

/*────────────────────────────────────────────────────────────────────────────*/

#undef CALBK
#undef MIDDL
#endif
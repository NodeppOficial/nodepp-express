/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_COOKIE
#define NODEPP_COOKIE

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/regex.h>
#include <nodepp/map.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp {

    using cookie_t = map_t< string_t, string_t >;

    namespace cookie {

        cookie_t parse( string_t data ){
            if ( data.empty() ){ return cookie_t(); } cookie_t res;
                 auto args = string::split( data, ';' );
            for( auto x : args ){ 
                 auto y = regex::match_all( x, "[^=]+" );
                 if ( y.size() != 2 ){ continue; }
                 res[ y[0].to_slugify() ] = y[1].to_slugify();
            }    return res;
        }
        
        /*─······································································─*/
        
        string_t format( const cookie_t& data ){ 
            array_t<string_t> result; for( auto x:data.data() ) 
                   result.push( x.first + "=" + x.second );
            return string::format("%s",result.join(";").c_str());
        }

    }

}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
#include <nodepp/nodepp.h>
#include <express/https.h>

using namespace nodepp;

void onMain() {

    ssl_t ssl; // ( "ssl/cert.key", "ssl/cert.crt" );

    auto app = express::https::add( &ssl );

    app.USE([]( express_https_t cli, function_t<void> next ){
        console::log( "this is a middleware" );
        next();
    });

    app.GET("/test",[]( express_https_t cli ){
        cli.status(200)
           .header( "content-type", "text/plain" )
           .send("this is a test");
    });

    app.GET([]( express_https_t cli ){
        cli.status(200)
           .header( "content-type", "text/plain" )
           .send("Hello World!");
    });

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at:" );
        console::log( "https://localhost:8000" );
    });

}

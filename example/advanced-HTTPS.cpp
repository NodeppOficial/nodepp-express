#include <nodepp/nodepp.h>
#include <express/https.h>

using namespace nodepp;

express_tls_t adminHandler(){

    ssl_t ssl; // ( "ssl/cert.key", "ssl/cert.crt" );

    auto app = express::https::add( &ssl );

    app.USE([]( express_https_t cli, function_t<void> next ){
        if( cli.headers["Auth"].empty() )
          { cli.send("you are not an admin"); return; }
        console::log( "hello world admin" ); next();
    });

    app.GET("/:id",[]( express_https_t cli ){
        string_t message;
        message += "id:" + cli.params["id"] + "\n";
        message += "normal method";
        cli.send( message );
    });

    app.GET("/:id/:di",[]( express_https_t cli ){
        string_t message;
        message += "id:" + cli.params["id"] + "\n";
        message += "di:" + cli.params["di"] + "\n";
        message += "normal method";
        cli.send( message );
    });

    app.GET("/:id/pupu/:di",[]( express_https_t cli ){
        string_t message;
        message += "id:" + cli.params["id"] + "\n";
        message += "di:" + cli.params["di"] + "\n";
        message += "pupu method";
        cli.send( message );
    });

    app.GET([]( express_https_t cli ){
        cli.send("hello admin!");
    });

    return app;

}

express_tls_t normalHandler() {

    auto app = express::https::add();

    app.GET("/json",[]( express_https_t cli ){
        object_t object ({
            { "var1", array_t<uchar>({ 10, 20, 30, 40, 50 }) },
            { "var2", "Hello World! 🫠" },
            { "var3", true },
            { "var4", 1999 }
        }); cli.sendJSON( object );
    });

    app.GET("/redirect",[]( express_https_t cli ){
        cli.redirect("https://www.google.com");
    });

    app.GET("/send",[]( express_https_t cli ){
        cli.send("Hello World!");
    });

    app.GET([]( express_https_t cli ){
        cli.send("hello user");
    });

    return app;

}

express_tls_t restFull() {
    auto app = express::https::add();

    app.GET([]( express_https_t cli ){
        cli.send( "Hi! I'm a GET Request" );
    });

    app.PUT([]( express_https_t cli ){
        cli.send( "Hi! I'm a PUT Request" );
    });

    app.POST([]( express_https_t cli ){
        cli.send( "Hi! I'm a POST Request" );
    });

    app.REMOVE([]( express_https_t cli ){
        cli.send( "Hi! I'm a DELETE Request" );
    });

    return app;
}

void onMain() {

    auto app = express::https::add();

    app.USE( "/api", restFull() );

    app.USE( "/admin", adminHandler() );

    app.USE( "/user", normalHandler() );

    app.USE( "/*", express::https::file( "www" ) );

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at https://localhost:8000" );
    });

}

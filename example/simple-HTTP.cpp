#include <nodepp/nodepp.h>
#include <express/http.h>

using namespace nodepp;

void onMain() {

    auto app = express::http::add();

    app.USE([]( express_http_t cli, function_t<void> next ){
        console::log( "this is a middleware" );
        next();
    });

    app.GET("/test",[]( express_http_t cli ){
        cli.status(200)
           .header( "content-type", "text/plain" )
           .send("this is a test");
    });

    app.GET([]( express_http_t cli ){
        cli.status(200)
           .header( "content-type", "text/plain" )
           .send("Hello World!");
    });

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at:" );
        console::log( "http://localhost:8000" );
    });

}

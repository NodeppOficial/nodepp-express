#include <nodepp/nodepp.h>
#include <nodepp/fs.h>
#include <express.h>

using namespace nodepp;

void onMain() {

    express_t app;

    app.GET([]( express_cli_t cli ){
        cli.status(200);
        cli.header( "content-type", "text/plain" );
        cli.send("Hello World!");
    });

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at:" );
        console::log( "http://localhost:8000" );
    });

}
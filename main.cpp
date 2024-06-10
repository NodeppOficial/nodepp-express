#include <nodepp/nodepp.h>
#include <nodepp/fs.h>
#include <express.h>

using namespace nodepp;

void onMain() {

    express_t app;

    app.GET([]( express_cli_t cli ){
        cli.sendJSON(object_t({
            { "arrg", array_t<int>({ 10, 20, 30, 40, 50, 60 }) },
            { "var2", "adios mundo" },
            { "var1", "hola mundo" },
            { "emoji", "🫠" },
            { "bool", true },
            { "int", 50000 }
        }));
    });

    app.listen( "localhost", 8000, []( ... ){
        console::log( "hola mundo" );
    });

}
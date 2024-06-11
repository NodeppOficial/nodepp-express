# NODEPP-EXPRESS

A Simple ExpressJS Implementation in NodePP

# Dependencies
- Zlib
    - 🪟: `pacman -S mingw-w64-ucrt-x86_64-zlib`
    - 🐧: `sudo apt install zlib1g-dev`

## Build & Run
- 🪟: `g++ -o main main.cpp -I ./include -lz -lws2_32 ; ./main`
- 🐧: `g++ -o main main.cpp -I ./include -lz ; ./main`

## Usage

```cpp
#include <nodepp/nodepp.h>
#include <express.h>

using namespace nodepp;

void onMain() {

    express_t app;

    app.USE([]( express_cli_t cli, function_t<void> next ){
        console::log( "this is a middleware" );
        next();
    });

    app.GET("/test",[]( express_cli_t cli ){
        cli.status(200)
           .header( "content-type", "text/plain" )
           .send("this is a test");
    });

    app.GET([]( express_cli_t cli ){
        cli.status(200)
           .header( "content-type", "text/plain" )
           .send("Hello World!");
    });

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at:" );
        console::log( "http://localhost:8000" );
    });

}
```

## License

**Nodepp** is distributed under the MIT License. See the LICENSE file for more details.
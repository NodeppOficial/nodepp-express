# NODEPP-EXPRESS

A Simple ExpressJS Implementation in [C++/NodePP]([NodePP](https://github.com/NodeppOficial/nodepp)) that supports server side rendering

## Dependencies
```bash
# Openssl
🪟: pacman -S mingw-w64-ucrt-x86_64-openssl
🐧: sudo apt install libssl-dev

# Zlib
🪟: pacman -S mingw-w64-ucrt-x86_64-zlib
🐧: sudo apt install zlib1g-dev

# Nodepp
💻: https://github.com/NodeppOficial/nodepp
```

## Build & Run
```bash
🪟: g++ -o main main.cpp -I ./include -lz -lws2_32 -lssl -lcrypto ; ./main
🐧: g++ -o main main.cpp -I ./include -lz -lssl -lcrypto ; ./main
```

## Usage

```cpp
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
           .send("this is an ExpressPP test");
    });

    app.GET([]( express_http_t cli ){
        cli.status(200)
           .header( "content-type", "text/html" )
           .render( R"(
                <h1> 
                    <° http://localhost:8000/test °> 
                </h1>
            )" );
    });

    app.listen( "localhost", 8000, []( ... ){
        console::log( "server started at:" );
        console::log( "http://localhost:8000" );
    });

}
```

## License

**Nodepp** is distributed under the MIT License. See the LICENSE file for more details.

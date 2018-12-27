# tiny-MQTT-c

A small incomplete undocumented and poorly tested MQTT 3.1.1 client library written in <500 lines of C99.
The library uses no dynamic allocation and compiles down to ~1K on ARM thumb.

NOTE: This is very much a work in progress still. It should be fairly easy to hack on though.


See [mqtt.h](https://github.com/kokke/tiny-MQTT-c/blob/master/mqtt.h) and [mqtt.c](https://github.com/kokke/tiny-MQTT-c/blob/master/mqtt.c) for the implementation of the MQTT protocol.

[client.c](https://github.com/kokke/tiny-MQTT-c/blob/master/client.c), [client.h](https://github.com/kokke/tiny-MQTT-c/blob/master/client.h) and [client_test.c](https://github.com/kokke/tiny-MQTT-c/blob/master/client_test.c).c are just TCP drivers to test the MQTT library. The test is performed by connecting to a public MQTT broker and publishing some gibberish.

Compile and try by running 

    gcc client.c mqtt.c client_test.c -Wall -Wextra 
    ./a.out &
    ./a.out pub

.... and sit back and watch the horrors unfold







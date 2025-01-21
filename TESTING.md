Pripojenie (CONNACK OK):

``` bash
mqttx conn -h '127.0.0.1' -p 1883 -V '3.1.1'
```

Pripojenie (CONNACK invalid version):

``` bash
mqttx conn -h '127.0.0.1' -p 1883 -V '5'
```

Sub:

``` bash
mqttx sub -h '127.0.0.1' -p 1883 -V '3.1.1' -t asdf -t qwer
```

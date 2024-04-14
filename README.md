The goal of this project is to enable remote control over two main functionalities: the swimming pool cover and the outdoor lighting.


The code used to program the Esp8266 micro-controllers, which act as controllable relays, can be found in this repository. Each Esp8266 opens a web server, allowing for control via HTTP requests.

Moreover, a Raspberry Pi connected to the same Wi-Fi network as the microcontrollers hosts a website. You can access a demo of this website is available [here](comtecteddemo.floriancomte.ch). Interacting with this web application triggers requests to the appropriate Esp8266 for remote control.

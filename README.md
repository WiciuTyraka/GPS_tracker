# Rocket Localization System

## About

This is a project of a research rocket localization system created for the [PUT Rocket Lab](https://www.facebook.com/putrocketlab) science club.

One of the biggest problems for engineers who build rockets is their recovery. Most teams decide on very expensive commercial solutions, which are not free of limitations. Our proposed system is a cheap alternative to the products available on the market, and thanks to the use of the chirp spread spectrum modulation technique, it offers more possibilities.

The whole system consists of a tracker, receiver, and a mobile application that acts as a graphical user interface. The tracker and the receiver are equipped with a LoRa radio module, which is considered to be the first low-cost implementation of CSS modulation. To operate the radio module, our team decided to implement a library, which allowed us to use all of LoRa features. Microcontroller STM32F411 is used as a central processing unit, while the receiver is equipped with a microcontroller with integrated Bluetooth and Wi-Fi modules - ESP32. In order to display the location of the object we implemented a mobile application. The application offers real-time capture of location data and displays them on the map, which significantly speeds up and facilitates the process of finding the rocket.

- [Rocket Localization System](#rocket-localization-system)
  - [About](#about)
  - [GPS Tracker](#gps-tracker)
    - [Hardware](#hardware)
      - [Sensors and components](#sensors-and-components)
      - [Schema](#schema)
      - [Board](#board)
    - [Software](#software)
  - [Receiver](#receiver)
    - [Hardware](#hardware)
      - [Sensors](#sensors)
      - [Schema](#schema)
      - [board](#board)
    - [Software](#software)
  - [LoRa Library](#lora-library)
  - [Mobile App](#mobile-app)
    - [Software](#software)
  - [Web App](#web-app)
    - [Software](#software)

## GPS Tracker

Pierwszą częścia systemu lokalizacji jest GPS Tracker. Urządzenie to, przy pomocy fal radiowych rozsyła informację na temat swojego aktualnego połorzenia w przestrzeni.

Do komunikacji radiowej wykrozystana została warstwa fizyczna systemu LoRa, protokuł łączności został zaimplementowany według potrzeb naszego zespołu. Długość i szerokość geograficzna pozyskiwane są za pomooca systemu zanwigacji satelitarnej. Wysokość obliczana jest na podstawie odczytu ciśnienia atmosferycznego mierzonego przez barometr. Z uwagi na niską przepustowość łącza radiowego wszystkie dane zapisywane sa w trwałej pamięci flash, Na ich podstawie uzytkownik w post procesingu może dokładnie wyznaczyć tor lotu rakiety.

Urządzenie jest także wyposażone w akumulator litowo-polimerowy i moduł umożliwiający jego ładowanie przez złącze mini-USB. Cztery diody led pełnią rolę prostego interfejsu, który informuje uzytkownika o bierzących ustawieniach. Ostatnim elementem urządzenia jest 4 pinowe złącze które umożliwia konfigurowanie ustwień trackera, takich jak kanał radiowy, moc transmitowanego sygnału i data rate.

### Hardware

![datasheet](pictures/tracker_description.png)

#### Sensors and components

| Sensor          |          Device |                                           Datasheet |
| --------------- | --------------: | --------------------------------------------------: |
| uC              |      STM32 F411 |              [datasheet](datasheet/stm32f411ce.pdf) |
| GPS             |     Quectel L80 | [datasheet](datasheet/L80_Hardware_Design_V1.1.pdf) |
| Radio           | LoRa E32-ttl-1W |   [datasheet](datasheet/E32-433T30D_Usermanual.pdf) |
| Barometer       |          BMP280 |               [datasheet](datasheet/BST-BMP280.pdf) |
| Flash memory    |         W25Q128 |                [datasheet](datasheet/w25q128fv.pdf) |
| Battery charger | MCP73833-AMI/UN |             [datasheet](datasheet/22005a-76648.pdf) |

#### Schema

#### Board

# Rocket Localization System

## About

[PUT Rocket Lab](https://www.facebook.com/putrocketlab) jest Kołem Naukowym z Politechniki Poznańskiej zajmującym się projektowaniem, budowaniem oraz testowaniem technologii
rakietowych. Flagowym projektem zespołu jest budowa rakiety sondażowej
Hexa 2 napędzanej hybrydowym silnikiem rakietowym. Jednym z większych problemów, z jakimi przychodzi zmagać się inżynierom budującym
rakiety, jest ich odzysk. Podczas testów rakieta zostaje wystrzelona w
przestrzeń. Wysokość oraz uzyskiwana prędkość obiektu jest oszacowana z
dużą dokładnością. Komplikacje pojawiają się przy estymowaniu miejsca
spadku rakiety, gdyż jest ono zależne w głównej mierze od powodzenia się
wszystkich procedur lądowania oraz w dużym stopniu od czynników meteorologicznych. Istotą odzysku jest zlokalizowanie rakiety w celu znalezienia
części, które są zdatne do ponownego użycia oraz tych, które już nie zadziałają, ale by zanieczyszczały środowisko. Większość zespołów decyduje
się na bardzo drogie komercyjne rozwiązania, które mimo wszystko nie są
wolne od ograniczeń. W tym repozytorium przedstawiony jest autorski projekt systemu lokalizacji rakiet stratosferycznych.

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

Pierwszą częścia systemu lokalizacji jest GPS Tracker. Urządzenie to umieszczone wewnątrz rakiety, przy pomocy fal radiowych rozsyła informację na temat swojego aktualnego połorzenia w przestrzeni.

### Hardware

Do komunikacji radiowej wykrozystana została warstwa fizyczna systemu dalekiej komunikacji - LoRa, protokuł łączności został zaimplementowany według potrzeb naszego zespołu. Długość i szerokość geograficzna pozyskiwane są za pomooca systemu nawigacji satelitarnej. Wysokość obliczana jest na podstawie odczytu ciśnienia atmosferycznego mierzonego przez barometr. Z uwagi na stosunkowo niską przepustowość łącza radiowego wszystkie dane zapisywane sa w trwałej pamięci flash, Na ich podstawie użytkownik w post procesingu może dokładnie wyznaczyć tor lotu rakiety.

Urządzenie jest także wyposażone w akumulator litowo-polimerowy i moduł umożliwiający jego ładowanie przez złącze mini-USB. Cztery diody led pełnią rolę prostego interfejsu, który informuje użytkownika o bierzących ustawieniach. Ustawienia te mogą być konfigurowane poprzez deydkowaną aplikację, która pozwala na ustawienie takich parametrów jak kanał radiowy, moc transmitowanego sygnału oraz data rate. Komunikacja z aplikacją odbywa się poprzez uniwersalny port asynchroniczny - UART. Urządzenie posiada 4 pinowe złącze któe przy pomocy konwertera UART-USB umożliwia podłączenie do komputera. Ostatnim ważnym elementem urządzenia jest włącznik zasilania o logice bistabilnej. Urządzenie zostaje uruchomione poprzez wyciągnięcie specjalnej zworki, tzw "Remove before fly".

![datasheet](pictures/tracker_description.png)
_Figure 1. Render of GPS Tracker Board_

#### Sensors and components

<details>

Poniższa tabela przedstawia wszytkie moduły cyforwe wykorzystane w projekcie GPS tracker'a wraz z odnośnikiem do poszczególnych dokumentacji.

| Sensor          |          Device |                                           Datasheet |
| --------------- | --------------: | --------------------------------------------------: |
| uC              |      STM32 F411 |              [datasheet](datasheet/stm32f411ce.pdf) |
| GPS             |     Quectel L80 | [datasheet](datasheet/L80_Hardware_Design_V1.1.pdf) |
| Radio           | LoRa E32-ttl-1W |   [datasheet](datasheet/E32-433T30D_Usermanual.pdf) |
| Barometer       |          BMP280 |               [datasheet](datasheet/BST-BMP280.pdf) |
| Flash memory    |         W25Q128 |                [datasheet](datasheet/w25q128fv.pdf) |
| Battery charger | MCP73833-AMI/UN |             [datasheet](datasheet/22005a-76648.pdf) |

</details>

#### Schema

<details>

Poniżej przedstawiony został dokładny schemat GPS tracker'a.
Plik z schematem dostępny jest pod tym [linkiem](Hardware/gps_tracker2.0/gps_tracker2.0.sch)

![datasheet](pictures/GPS_Tracker_schema.png)
_Figure 2. Schema of GPS Tracker_

</details>

#### Board

<details>

Na poniższym obrazku przedstawione zostały obie strony dwustronnej płytki PCB, która łączy wszytskie elementy. Lewa strona (niebieskie ścieżki) przedstawia dolną warstwę natomiast prawa (czerwone ścieżki) górną warstwę.
Plik z płytką dostępny jest pod tym [linkiem](Hardware/gps_tracker2.0/gps_tracker2.0.sch)

![datasheet](pictures/gps_tracker_board.png)
_Figure 3. GPS Tracker board_

</details>

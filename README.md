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
      - [Block Diagram](#block-diagram)
  - [Receiver](#receiver)
    - [Hardware](#hardware-1)
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

Do komunikacji radiowej ziemia-powietrze posłużyła warstwa fizyczna systemu dalekiego zasięgu - LoRa, protokuł łączności został zaimplementowany według potrzeb naszego zespołu. LoRa używa techniki modulacji z widmem rozproszonym CSS (ang. chirp spread spectrum), która jest uznawana za pierwszą niskokosztową implementację tego rodzaju modulacji do zastosowań komercyjnych. Długość i szerokość geograficzna pozyskiwane są za pomooca systemu nawigacji satelitarnej. Wysokość obliczana jest na podstawie odczytu ciśnienia atmosferycznego mierzonego przez barometr. Z uwagi na stosunkowo niską przepustowość łącza radiowego wszystkie nadmiarowe dane zapisywane sa w trwałej pamięci flash.

Urządzenie jest także wyposażone w akumulator litowo-polimerowy i moduł umożliwiający jego ładowanie przez złącze mini-USB. Cztery diody led pełnią rolę prostego interfejsu, który informuje użytkownika o bierzących ustawieniach. Ustawienia te mogą być konfigurowane poprzez deydkowaną aplikację, która pozwala na ustawienie takich parametrów jak kanał radiowy, moc transmitowanego sygnału oraz data rate. Komunikacja z aplikacją odbywa się poprzez uniwersalny port asynchroniczny - UART. Urządzenie posiada 4 pinowe złącze któe przy pomocy konwertera UART-USB umożliwia podłączenie do komputera. Ostatnim ważnym elementem urządzenia jest włącznik zasilania o logice bistabilnej. Urządzenie zostaje uruchomione poprzez wyciągnięcie specjalnej zworki, tzw "Remove before fly".

![datasheet](pictures/tracker_description.png)

<div align="center"><font size="2">_Figure 1. Render of GPS Tracker Board_</font></div>

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

- [link do pliku z schematem](Hardware/gps_tracker2.0/gps_tracker2.0.sch)

![schema](pictures/GPS_Tracker_schema.png)

<div align="center"><font size="2">_Figure 2. Schema of GPS Tracker_</font></div>

</details>

#### Board

<details>

Na poniższym obrazku przedstawione zostały obie strony dwustronnej płytki PCB, która łączy wszytskie elementy. Lewa strona (niebieskie ścieżki) przedstawia dolną warstwę natomiast prawa (czerwone ścieżki) górną warstwę.

- [link do piku z płytką PCB](Hardware/gps_tracker2.0/gps_tracker2.0.sch)

![board](pictures/gps_tracker_board.png)

<div align="center"><font size="2">_Figure 3. GPS Tracker board_</font></div>

</details>

### Software

Napisanie oprogramowania obsługującego moduły urządzenia oraz utworzenie protokołu komunikacyjnego jest niezbędne, by obsłużyć hardware i spełnić wymagania,
które stawiane są przed GPS Tracker'em. W tym celu wykorzystany został popularny framework STM32duino wraz, który jest wygodną i przyjemną platformą do tworzenia aplikacji dla
systemów wbudowanych.
GPS Tracker działa w oparciu o system czasu rzeczywistego FreeRTOS, aby dawać gwarancję wykonania zadań w określonym czasie. Symulacja wielowątkowego działania procesora pozwala
na odczytywanie danych z wszytskich sensorów z maksymalną częstotliwością czujników. Wszystkie dane zapisywane są w trwałej pamięci flash, dzięki czemu po znaleieniu rakiety mogą zostać one odczytane, a na ich podstawie możliwe jest wykreślenie na mapie dokładnego toru lotu rakiety.

Na potrzeby obsługi nowoczesnego modułu komunikacji dalekiego zasięgu -
LoRa - została przez nas napisana dedykowana biblioteka. Biblioteka implementuje protokuł komunkacyjny, który znacząco ułatwia komunikację radiową natomiast obsługa modułu jest przyjamna dla użytkownika.Natomiast wydajność łącza radiowego uzyskana dzięki oprogramowaniu naszego zespołu jest większa niż w przypadku użytkowania biblioteki afirmowanej przez producenta modułu.

#### Block Diagram

<details>

Na pozniższym obrazku przedstawiony został schemat blokowy procesów wykonywanych w ramach działania systemu czasu rzeczywistego zaimplementowanego na potrzeby obługi GPS Tracker'a.

- [link do pliku z kodem źródłowym GPS Tracker'a]()
- [link do pliku nagłówkowego GPS Tracker'a]()

![block_diagram]()

<div align="center"><font size="2"> _Figure 4. GPS Tracker code diagram_</font></div>

</details>

## Receiver

Drugą częścią systemu jest Receiver, który wraz z smartfonem tworzy stację odbiorczą dla sygnału emitowanego z rakiety. Urządzenie wyposażone w antenę kierunkową pozwala na łączność z rakietami odbywającymi loty stratosferyczne. Odebrane dane wizualizowane są w czasie rzeczywistym na mapie satelitarnej na ekranie połączonego smartfona.

### Hardware

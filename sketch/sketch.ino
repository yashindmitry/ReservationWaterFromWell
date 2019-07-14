#include <SPI.h>
#include <Ethernet.h>
byte mac[] = {
  0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD // MAC адрес веб сервера
};
IPAddress ip(192, 168, 0, 15); // IP адрес веб сервера
EthernetServer server(80); // Порт веб сервера
const int RelayWell = 5; // Реле управление соленоидным клапаном на колодце
const int RelayBarrelOut = 6; // Реле управление соленоидным клапаном забора воды из бочки
const int RelayBarrelIn = 7; // Реле управление соленоидным клапаном пополнения бочки
boolean RelayWellIsOpen = true; // Признак того, что клапан колодца открыт
boolean RelayBarrelIsOpen = false; // Признак того, что клапан бочки открыт
boolean AutomaticControl = true; // Включено автоматическое управление клапанами
boolean ReserveAutomaticControl = true; // Включено автоматическое наполнение резервной бочки из колодца
boolean RelayBarrelInIsOpen = false; // Признак того, что клапан наполнения бочки открыт

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RelayWell, OUTPUT);
  pinMode(RelayBarrelOut, OUTPUT);
  pinMode(RelayBarrelIn, OUTPUT);
  Ethernet.begin(mac, ip);
  server.begin();
}

unsigned long lastCheckMillis = 0; // Время последней проверки уровня воды в колодце
unsigned long checkInterval = 1000; // Через сколько осуществить следующую проверку урвня воды в колодце
unsigned long ReserveLastCheckMillis = 0; // Время последней проверки необходимости пополнения резервной емкости
unsigned long checkIntervalReserve = 1000; // Через сколько осуществить следующую проверку необходимости пополнения резервной емкости

void loop() {

  unsigned long currentMillis = millis();

  // Проверим не пришло ли время проверять уровень воды в колодце
  long differenceValues = currentMillis - lastCheckMillis;
  if((differenceValues > checkInterval) || (differenceValues < 0)) {
    lastCheckMillis = currentMillis;
    int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
    digitalWrite(LED_BUILTIN, IsHighWaterLevel); // Выставляем диод

    if (IsHighWaterLevel == HIGH) {
      if (AutomaticControl) {
        // Открываем кран колодца
        digitalWrite(RelayWell, LOW);
        RelayWellIsOpen = true;
        // Закрываем кран из бочки
        digitalWrite(RelayBarrelOut, LOW);
        RelayBarrelIsOpen = false;
      }
      checkInterval = 60000;
    } else {
      if (AutomaticControl) {
        // Открываем кран из бочки
        digitalWrite(RelayBarrelOut, HIGH);
        RelayBarrelIsOpen = true;
        // Закрываем кран колодца
        digitalWrite(RelayWell, HIGH);
        RelayWellIsOpen = false;
      }
      checkInterval = 600000;
    }
  }

  // Автоматическое поддержание уровня воды резервной емкости
  long differenceValues2 = currentMillis - ReserveLastCheckMillis;
  if((differenceValues2 > checkIntervalReserve) || (differenceValues2 < 0)) {
    ReserveLastCheckMillis = currentMillis;
    if (ReserveAutomaticControl) {
      int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
      int IsHighWaterLevelInBarrel = digitalRead(A1); // Читаем данные с датчика уровня воды в бочке
      if (IsHighWaterLevelInBarrel == LOW && IsHighWaterLevel == HIGH) {
        // Открываем подачу воды из колодца
        digitalWrite(RelayBarrelIn, HIGH);
        RelayBarrelInIsOpen = true;
        checkIntervalReserve = 1000;
      } else {
        // Закрываем подачу воды из колодца
        digitalWrite(RelayBarrelIn, LOW);
        RelayBarrelInIsOpen = false;
        checkIntervalReserve = 600000; // Следующую проверку уровня воды в резервной емкости проведем через 10 минут
      }
    } else {
      // Закрываем подачу воды из колодца
      digitalWrite(RelayBarrelIn, LOW);
      RelayBarrelInIsOpen = false;
    }
  }

  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    String header = "";
    byte requestIndex = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          if (requestIndex == 0) {
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-Type: text/html; charset=utf-8"));
            client.println(F("Connection: close"));
            client.println();
            client.println(F("<!DOCTYPE HTML>"));
            client.println(F("<html>"));
            int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
            if (IsHighWaterLevel == HIGH) {
              client.println(F("<p>Уровень воды в колодце <b>высокий</b></p>"));
            } else {
              client.println(F("<p>Уровень воды в колодце <b>низкий</b></p>"));
            }
            client.println(F("<hr>"));
            if (AutomaticControl) {
              client.println(F("<p>Автоматическое управление клапанами <b>on</b> <i><a href=\"/1\">off</a></i></p>"));
            } else {
              client.println(F("<p>Автоматическое управление клапанами <b>off</b> <i><a href=\"/2\">on</a></i></p>"));
            }
            if (RelayWellIsOpen) {
              client.println(F("<p>Клапан колодца <b>on</b> <i><a href=\"/3\">off</a></i></p>"));
            } else {
              client.println(F("<p>Клапан колодца <b>off</b> <i><a href=\"/4\">on</a></i></p>"));
            }
            if (RelayBarrelIsOpen) {
              client.println(F("<p>Клапан бочки <b>on</b> <i><a href=\"/5\">off</a></i></p>"));
            } else {
              client.println(F("<p>Клапан бочки <b>off</b> <i><a href=\"/6\">on</a></i></p>"));
            }
            client.println(F("<hr>"));
            int IsHighWaterLevelInBarrel = digitalRead(A1); // Читаем данные с датчика уровня воды в резервной емкости
            if (IsHighWaterLevelInBarrel == HIGH) {
              client.println(F("<p>Уровень воды в резервной емкости <b>высокий</b></p>"));
            } else {
              client.println(F("<p>Уровень воды в резервной емкости <b>низкий</b></p>"));
            }
            client.println(F("<hr>"));
            if (ReserveAutomaticControl) {
              client.println(F("<p>Автоматика резервной емкости <b>on</b> <i><a href=\"/7\">off</a></i></p>"));
            } else {
              client.println(F("<p>Автоматика резервной емкости <b>off</b> <i><a href=\"/8\">on</a></i></p>"));
            }
            if (RelayBarrelInIsOpen) {
              client.println(F("<p>Клапан наполнения резервной емкости <b>on</b> <i><a href=\"/9\">off</a></i></p>"));
            } else {
              client.println(F("<p>Клапан наполнения резервной емкости <b>off</b> <i><a href=\"/10\">on</a></i></p>"));
            }
            client.println(F("</html>"));
          } else {
            client.println(F("HTTP/1.1 307 temporary redirect"));
            client.println(F("Location: /"));
            client.println(F("Connection: close"));
            if (requestIndex == 1) {
              AutomaticControl = false;
            } else if (requestIndex == 2) {
              AutomaticControl = true;
            } else if (requestIndex == 3) {
              // Закрываем кран колодца
              digitalWrite(RelayWell, HIGH);
              RelayWellIsOpen = false;
              AutomaticControl = false;
            } else if (requestIndex == 4) {
              // Открываем кран колодца
              digitalWrite(RelayWell, LOW);
              RelayWellIsOpen = true;
              AutomaticControl = false;
            } else if (requestIndex == 5) {
              // Закрываем кран бочки
              digitalWrite(RelayBarrelOut, LOW);
              RelayBarrelIsOpen = false;
              AutomaticControl = false;
            } else if (requestIndex == 6) {
              // Открываем кран бочки
              digitalWrite(RelayBarrelOut, HIGH);
              RelayBarrelIsOpen = true;
              AutomaticControl = false;
            } else if (requestIndex == 7) {
              ReserveAutomaticControl = false;
            } else if (requestIndex == 8) {
              ReserveAutomaticControl = true;
            } else if (requestIndex == 9) {
              // Закрываем кран наполнения бочки
              digitalWrite(RelayBarrelIn, LOW);
              RelayBarrelInIsOpen = false;
              ReserveAutomaticControl = false;
            } else if (requestIndex == 10) {
              // Открываем кран наполнения бочки
              digitalWrite(RelayBarrelIn, HIGH);
              RelayBarrelInIsOpen = true;
              ReserveAutomaticControl = false;
            }
          }
          break;
        }
        if (c == '\n') {
          if (header.substring(0, 4) == "GET ") {
            requestIndex = header.substring(5, header.indexOf(" ", 4)).toInt();
          }
          header = "";
          currentLineIsBlank = true;
        } else if (c != '\r') {
          header = header + c;
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }

}

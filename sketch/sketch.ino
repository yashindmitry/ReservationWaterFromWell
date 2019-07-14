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
    String requestUrl = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          if (requestUrl == "") {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
            if (IsHighWaterLevel == HIGH) {
              client.println("<p>Уровень воды в колодце <b>высокий</b></p>");
            } else {
              client.println("<p>Уровень воды в колодце <b>низкий</b></p>");
            }
            client.println("<hr>");
            if (AutomaticControl) {
              client.println("<p>Автоматическое управление клапанами <b>on</b> <i><a href=\"/acoff\">off</a></i></p>");
            } else {
              client.println("<p>Автоматическое управление клапанами <b>off</b> <i><a href=\"/acon\">on</a></i></p>");
            }
            if (RelayWellIsOpen) {
              client.println("<p>Клапан колодца <b>on</b> <i><a href=\"/rwoff\">off</a></i></p>");
            } else {
              client.println("<p>Клапан колодца <b>off</b> <i><a href=\"/rwon\">on</a></i></p>");
            }
            if (RelayBarrelIsOpen) {
              client.println("<p>Клапан бочки <b>on</b> <i><a href=\"/rboff\">off</a></i></p>");
            } else {
              client.println("<p>Клапан бочки <b>off</b> <i><a href=\"/rbon\">on</a></i></p>");
            }
            client.println("<hr>");
            if (ReserveAutomaticControl) {
              client.println("<p>Автоматика резервной емкости <b>on</b> <i><a href=\"/aroff\">off</a></i></p>");
            } else {
              client.println("<p>Автоматика резервной емкости <b>off</b> <i><a href=\"/aron\">on</a></i></p>");
            }
            if (RelayBarrelInIsOpen) {
              client.println("<p>Клапан наполнения бочки <b>on</b> <i><a href=\"/rbioff\">off</a></i></p>");
            } else {
              client.println("<p>Клапан наполнения бочки <b>off</b> <i><a href=\"/rbion\">on</a></i></p>");
            }
            client.println("</html>");
          } else {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            if (requestUrl == "acoff") {
              AutomaticControl = false;
            } else if (requestUrl == "acon") {
              AutomaticControl = true;
            } else if (requestUrl == "rwoff") {
              // Закрываем кран колодца
              digitalWrite(RelayWell, HIGH);
              RelayWellIsOpen = false;
            } else if (requestUrl == "rwon") {
              // Открываем кран колодца
              digitalWrite(RelayWell, LOW);
              RelayWellIsOpen = true;
            } else if (requestUrl == "rboff") {
              // Закрываем кран бочки
              digitalWrite(RelayBarrelOut, LOW);
              RelayBarrelIsOpen = false;
            } else if (requestUrl == "rbon") {
              // Открываем кран бочки
              digitalWrite(RelayBarrelOut, HIGH);
              RelayBarrelIsOpen = true;
            } else if (requestUrl == "aroff") {
              ReserveAutomaticControl = false;
            } else if (requestUrl == "aron") {
              ReserveAutomaticControl = true;
            } else if (requestUrl == "rbioff") {
              // Закрываем кран наполнения бочки
              digitalWrite(RelayBarrelIn, LOW);
              RelayBarrelInIsOpen = false;
            } else if (requestUrl == "rbion") {
              // Открываем кран rbioff бочки
              digitalWrite(RelayBarrelIn, HIGH);
              RelayBarrelInIsOpen = true;
            }
          }
          break;
        }
        if (c == '\n') {
          if (header.substring(0, 4) == "GET ") {
            requestUrl = header.substring(5, header.indexOf(" ", 4));
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

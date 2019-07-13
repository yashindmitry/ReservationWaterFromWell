#include <SPI.h>
#include <Ethernet.h>
byte mac[] = {
  0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD // MAC адрес веб сервера
};
IPAddress ip(192, 168, 0, 15); // IP адрес веб сервера
EthernetServer server(80); // Порт веб сервера
const int RelayWell = 5; // Реле управление соленоидным клапаном на колодце
const int RelayBarrel = 6; // Реле управление соленоидным клапаном на бочке
byte RelayWellIsOpen = 1; // Признак того, что клапан колодца открыт
byte RelayBarrelIsOpen = 0; // Признак того, что клапан бочки открыт
byte AutomaticControl = 1; // Включено автоматическое управление клапанами

void setup() {
//  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RelayWell, OUTPUT);
  pinMode(RelayBarrel, OUTPUT);
  Ethernet.begin(mac, ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
//    Serial.println("Не присоединена плата ethernet W5100");
    while (true) {
      delay(1);
    }
  }
  server.begin();
//  Serial.print("Поднял веб сервер по адресу ");
//  Serial.println(Ethernet.localIP());
}

unsigned long lastCheckMillis = 0; // Время последней проверки уровня воды в колодце
unsigned long checkInterval = 1000; // Через сколько осуществить следующую проверку

void loop() {

  unsigned long currentMillis = millis();

  // Проверим не пришло ли время проверять уровень воды в колодце
  int differenceValues = currentMillis - lastCheckMillis;
  if((differenceValues > checkInterval) || (differenceValues < 0)) {
//    Serial.print("Проверяю уровень воды в колодце: ");
    lastCheckMillis = currentMillis;
    int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
    digitalWrite(LED_BUILTIN, IsHighWaterLevel); // Выставляем диод
    if (IsHighWaterLevel == HIGH) {
//      Serial.println("высокий");
      if (AutomaticControl == 1) {
        // Открываем кран колодца
        digitalWrite(RelayWell, LOW);
        RelayWellIsOpen = 1;
        // Закрываем кран из бочки
        digitalWrite(RelayBarrel, LOW);
        RelayBarrelIsOpen = 0;
      }
//      Serial.println("Следующую проверку уровня воды в колодце сделаем через 1 минуту");
      checkInterval = 60000;
    } else {
//      Serial.println("низкий");
      if (AutomaticControl == 1) {
        // Открываем кран из бочки
        digitalWrite(RelayBarrel, HIGH);
        RelayBarrelIsOpen = 1;
        // Закрываем кран колодца
        digitalWrite(RelayWell, HIGH);
        RelayWellIsOpen = 0;
      }
//      Serial.println("Следующую проверку уровня воды в колодце сделаем через 10 минут");
      checkInterval = 600000;
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
        //Serial.print(c);
        if (c == '\n' && currentLineIsBlank) {
          if (requestUrl == "/") {
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
            if (AutomaticControl == 1) {
              client.println("<p>Автоматическое управление клапанами <b>включено</b> <i><a href=\"/automaticcontroloff\">выключить</a></i></p>");
            } else {
              client.println("<p>Автоматическое управление клапанами <b>выключено</b> <i><a href=\"/automaticcontrolon\">включить</a></i></p>");
            }
            if (RelayWellIsOpen == 1) {
              client.println("<p>Клапан колодца <b>открыт</b> <i><a href=\"/relaywelloff\">закрыть</a></i></p>");
            } else {
              client.println("<p>Клапан колодца <b>закрыт</b> <i><a href=\"/relaywellon\">открыть</a></i></p>");
            }
            if (RelayBarrelIsOpen == 1) {
              client.println("<p>Клапан бочки <b>открыт</b> <i><a href=\"/relaybarreloff\">закрыть</a></i></p>");
            } else {
              client.println("<p>Клапан бочки <b>закрыт</b> <i><a href=\"/relaybarrelon\">открыть</a></i></p>");
            }
            client.println("</html>");
          } else if (requestUrl == "/automaticcontroloff") {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            AutomaticControl = 0;
          } else if (requestUrl == "/automaticcontrolon") {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            AutomaticControl = 1;
          } else if (requestUrl == "/relaywelloff") {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            // Закрываем кран колодца
            digitalWrite(RelayWell, HIGH);
            RelayWellIsOpen = 0;
          } else if (requestUrl == "/relaywellon") {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            // Открываем кран колодца
            digitalWrite(RelayWell, LOW);
            RelayWellIsOpen = 1;
          } else if (requestUrl == "/relaybarreloff") {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            // Закрываем кран бочки
            digitalWrite(RelayBarrel, LOW);
            RelayBarrelIsOpen = 0;
          } else if (requestUrl == "/relaybarrelon") {
            client.println("HTTP/1.1 307 temporary redirect");
            client.println("Location: /");
            client.println("Connection: close");
            // Открываем кран бочки
            digitalWrite(RelayBarrel, HIGH);
            RelayBarrelIsOpen = 1;
          }
          break;
        }
        if (c == '\n') {
          if (header.substring(0, 4) == "GET ") {
            requestUrl = header.substring(4, header.indexOf(" ", 4));
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

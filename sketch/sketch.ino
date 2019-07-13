
const int RelayWell = 4; // Реле управление соленоидным клапаном на колодце
const int RelayBarrel = 6; // Реле управление соленоидным клапаном на бочке


void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RelayWell, OUTPUT);
  pinMode(RelayBarrel, OUTPUT);
}

long lastCheckMillis = 0; // Время последней проверки уровня воды в колодце
long checkInterval = 1000; // Через сколько осуществить следующую проверку

void loop() {

  unsigned long currentMillis = millis();

  // Проверим не пришло ли время проверять уровень воды в колодце
  if(currentMillis - lastCheckMillis > checkInterval) {
    Serial.print("Проверяю уровень воды в колодце: ");
    lastCheckMillis = currentMillis;
    int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
    digitalWrite(LED_BUILTIN, IsHighWaterLevel); // Выставляем диод
    if (IsHighWaterLevel == HIGH) {
      Serial.println("высокий");
      // Открываем кран колодца
      digitalWrite(RelayWell, LOW);
      // Закрываем кран из бочки
      digitalWrite(RelayBarrel, LOW);
      Serial.println("Следующую проверку уровня воды в колодце сделаем через 1 минуту");
      checkInterval = 60000;
    } else {
      Serial.println("низкий");
      // Открываем кран из бочки
      digitalWrite(RelayBarrel, HIGH);
      // Закрываем кран колодца
      digitalWrite(RelayWell, HIGH);
      Serial.println("Следующую проверку уровня воды в колодце сделаем через 10 минут");
      checkInterval = 600000;
    }
  }

}

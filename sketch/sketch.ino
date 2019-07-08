
int RelayWell = 4; // Реле управление соленоидным клапаном на колодце
int RelayBarrel = 6; // Реле управление соленоидным клапаном на бочке


void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RelayWell, OUTPUT);
  pinMode(RelayBarrel, OUTPUT);
}

void loop() {
  int IsHighWaterLevel = digitalRead(A0); // Читаем данные с датчика уровня воды
  Serial.println(IsHighWaterLevel); // Печатаем в консоль
  digitalWrite(LED_BUILTIN, IsHighWaterLevel); // Выставляем диод
  if (IsHighWaterLevel == HIGH) {
    // Открываем кран колодца
    digitalWrite(RelayWell, LOW);
    // Закрываем кран из бочки
    digitalWrite(RelayBarrel, LOW);
    // Следующую проверку уровня воды в колодце сделаем через 1 минуту
    delay(60000);
  } else {
    // Открываем кран из бочки
    digitalWrite(RelayBarrel, HIGH);
    // Закрываем кран колодца
    digitalWrite(RelayWell, HIGH);
    // Следующую проверку уровня воды в колодце сделаем через 10 минут
    delay(600000);
  }
  delay(1000);
}

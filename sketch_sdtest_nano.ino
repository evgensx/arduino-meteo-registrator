/*  SD card attached to SPI bus as follows:  _____________
 ** MOSI - pin 11                           |8            |
 ** MISO - pin 12                           |...          |
 ** CLK - pin 13                            |1            |
 ** CS - pin 10 (SD: SDCARD_SS_PIN)          \9___________|
9 NC, 1 SS/CS, 2 MOSI, 3 GND, 4 VCC (3.3v), 5 CLK/SCK, 6 GND, 7 MISO, 8 NC */

#include <SPI.h>      // Подключаем библиотеку для работы с шиной SPI
#include <SD.h>       //библиотека для работы с SD картой
#include <Arduino.h>  // бибилотека ардуино
#include <Wire.h>     //библиотека контактов
#include <DS3231.h>           // часовая библиотека обязательна wire
#include <Adafruit_Sensor.h>  // Подключаем библиотеку Adafruit_Sensor
#include <Adafruit_BME280.h>  // Подключаем библиотеку Adafruit_BME280

#define SEALEVELPRESSURE_HPA (1013.25)  // Задаем высоту

#define TORR (1.33322) // float torr = 1.33322 1гПа = 101325 / 76000 mmHg

unsigned long timing; // Переменная для хранения точки отсчета

Adafruit_BME280 bme;

DS3231 Clock;
boolean Century = false;
bool h12;
bool PM;

File myFile;
void delFile();
void readFile();
void writingBmeDataToFile();

void getTimetoSerial() {
  Serial.print((char *) "2");
  if (Century) {      // Won't need this for 89 years.
    Serial.print((char *) "1");
  } else {
    Serial.print((char *) "0");
  }
  Serial.print(Clock.getYear(), DEC);
  Serial.print(' ');
  // then the month
  Serial.print(Clock.getMonth(Century), DEC);
  Serial.print(' ');
  // then the date
  Serial.print(Clock.getDate(), DEC);
  Serial.print(' ');
  // and the day of the week
  Serial.print(Clock.getDoW(), DEC);
  Serial.print(' ');
  // Finally the hour, minute, and second
  Serial.print(Clock.getHour(h12, PM), DEC);
  Serial.print(' ');
  Serial.print(Clock.getMinute(), DEC);
  Serial.print(' ');
  Serial.print(Clock.getSecond(), DEC);
  // Add AM/PM indicator
  if (h12) {
    if (PM) {
      Serial.print((char *) " PM ");
    } else {
      Serial.print((char *) " AM ");
    }
  } else {
    Serial.println((char *) " 24h");
  }
//  // Display the temperature
//  Serial.print((char *) "T=");
//  Serial.print(Clock.getTemperature(), 2);
//  // Tell whether the time is (likely to be) valid
//  if (Clock.oscillatorCheck()) {
//    Serial.println((char *) " O+");
//  } else {
//    Serial.println((char *) " O-");
//  }
}


void setupClock(String& InString) {
  // Call this if you notice something coming in on 
  // the serial port. The stuff coming in should be in 
  // the order YYMMDDwHHMMSS, with an 'X' at the end.
  byte Year;
  byte Month;
  byte Date;
  byte DoW;
  byte Hour;
  byte Minute;
  byte Second;
  
  byte Temp1, Temp2;

  // Read Year first
  Temp1 = (byte)InString[0] -48;
  Temp2 = (byte)InString[1] -48;
  Year = Temp1*10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] -48;
  Temp2 = (byte)InString[3] -48;
  Month = Temp1*10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] -48;
  Temp2 = (byte)InString[5] -48;
  Date = Temp1*10 + Temp2;
  // now Day of Week
  DoW = (byte)InString[6] - 48;   
  // now Hour
  Temp1 = (byte)InString[7] -48;
  Temp2 = (byte)InString[8] -48;
  Hour = Temp1*10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[9] -48;
  Temp2 = (byte)InString[10] -48;
  Minute = Temp1*10 + Temp2;
  // now Second
  Temp1 = (byte)InString[11] -48;
  Temp2 = (byte)InString[12] -48;
  Second = Temp1*10 + Temp2;

  Clock.setClockMode(false);  // set to 24h; set to 12h (true)
  Clock.setYear(Year);
  Clock.setMonth(Month);
  Clock.setDate(Date);
  Clock.setDoW(DoW);
  Clock.setHour(Hour);
  Clock.setMinute(Minute);
  Clock.setSecond(Second);
}


void readSerialStream() {
  String InSerialString;
//  String InString;
  if (Serial.available()) {
    InSerialString = Serial.readString();
    if (InSerialString == "time") {
      Serial.println(InSerialString);
      getTimetoSerial();  //Вывод в последовательный порт данных с датчика DS3231
      Serial.println();
    }
    else if (InSerialString[13] == 'X') {
      Serial.println(InSerialString);
      setupClock(InSerialString);
      Serial.println();
    }
    else if (InSerialString == "bme") {
      Serial.println(InSerialString);
      bmeSerial();  //Вывод в последовательный порт данных с датчика BME280
      Serial.println();
    }
    else if (InSerialString == "h" || InSerialString == "help") {
      Serial.println((char *) "Set time YYMMDDwHHMMSS with 'X' at the end");
      Serial.println((char *) "Commands: time, bme, readfile, writebme, delfile");
      Serial.println();
    }
    else if (InSerialString == "readfile") {
      Serial.println(InSerialString);
      readFile();  //Выводим содержимое файла в последовательный порт
      Serial.println();
    }
    else if (InSerialString == "writebme") {
      Serial.println(InSerialString);
      writingBmeDataToFile();  //Выводим содержимое файла в последовательный порт
      Serial.println((char *) "Write successfully\n");
    }
    else if (InSerialString == "delfile") {
      Serial.println(InSerialString);
      delFile();  //Удаление файла file.txt
    }
  }
}
  
void readFile() {
  if (SD.begin()) {
    myFile = SD.open("file.txt");  // Открываем файл для чтения
    if (!myFile) {                                  // если файл file.txt не найден
      Serial.println((char *) "file not found");             // выводим текст в монитор последовательного порта,
    } else {
      while(myFile.available()){
        Serial.write(myFile.read());  // выводим содержимое файла в последовательный порт
      }
      myFile.close();
    }
  }
}

void bmeSerial() {
  Serial.print((char *) "Temperature = ");                       // Печать текста
  Serial.print(bme.readTemperature());                  // Печать температуры
  Serial.print((char *) " °C\nPressure = ");                                 // Печать текста
//  Serial.print((char *) "Pressure = ");                            // Печать текста       
  Serial.print((bme.readPressure() / 100.0F) / TORR); // Печать атмосферное давление
  Serial.print((char *) " mmHg\nApprox. Altitude = ");                               // Печать текста
//  Serial.print((char *) "Approx. Altitude = ");                    // Печать текста
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA)); // Вычисление высоты
  Serial.print((char *) " m\nHumidity = ");                                  // Печать текста
//  Serial.print((char *) "Humidity = ");                           // Печать текста
  Serial.print(bme.readHumidity());                     // Печать влажности
  Serial.println((char *) " %");                                  // Печать текста
}

void writingBmeDataToFile() {
  myFile = SD.open("file.txt", FILE_WRITE);
  if (myFile) {
    myFile.print(Clock.getDate(), DEC);  // записываем переменную в файл
    myFile.print('.');
    myFile.print(Clock.getMonth(Century), DEC);
    myFile.print(".2");
    if (Century) {      // Won't need this for 89 years.
      myFile.print("1");
    } else {
      myFile.print("0");
    }
    myFile.print(Clock.getYear(), DEC);
    myFile.print(' ');
    // Finally the hour, minute, and second
    myFile.print(Clock.getHour(h12, PM), DEC);
    myFile.print(':');
    myFile.print(Clock.getMinute(), DEC);
    myFile.print(':');
    myFile.print(Clock.getSecond(), DEC);
    myFile.print(';');
    myFile.print(bme.readTemperature());
    myFile.print(';');
    myFile.print(bme.readHumidity());
    myFile.print(';');
    myFile.print((bme.readPressure() / 100.0F) / TORR);
    myFile.print(';');
    myFile.println(bme.readAltitude(SEALEVELPRESSURE_HPA));
    myFile.close();  // close the file
  }
}

void delFile() {
  if (SD.exists("file.txt")) {
    SD.remove("file.txt");
    Serial.println((char *) "file.txt deleted successfully\n");
  }
}

void miTimer() {
  if (millis() - timing > 600000) {  // Запись каждые 5 минут = 300000
    timing = millis();
    writingBmeDataToFile();
    delay(1000);
  }
}


void setup() {
  Serial.begin(9600);  // инициализируем порт 9600 бод
  Wire.begin();        // Start the I2C interface
  Serial.println((char *) "Waiting for the card to be inserted");  // выводим текст в последовательный порт.
  delay(1000);               // ждем секунду, чтобы убедиться что карта вставлена полностью
  if (SD.begin(10)) {  // Проверяем, есть ли связь с картой и, если нет, то (CS_pin)
    Serial.println((char *) "Card\t[OK]");  // выводим текст в последовательный порт.
  } else {
    Serial.println((char *) "Card\t[FAIL]");
  }
  if (bme.begin(0x76)) {  // Инициализация датчика BME280
    Serial.println((char *) "BME280\t[OK]");
  } else {
    Serial.println((char *) "BME280\t[FAIL]");  // Печать сообщения об ошибки
  }
  if (Clock.getYear() == 0) {
    Serial.println((char *) "Set time, see 'h'");
  }
}


void loop() {
  readSerialStream();  // Для отладки, можно отключить для финального проекта
  miTimer();
  delay(100);
}

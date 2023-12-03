#include <DHT.h>
#include <LiquidCrystal.h>
#include <Thread.h>
#include <ThreadController.h>

// Pines
const int pinLED1 = A2;
const int pinLED2 = 3;
const int pinJoyY = A0;
const int pinJoyX = A1;
const int pinJoyButton = 10;
const int pinBoton = 11;
const int pinTrigger = 12;
const int pinEcho = 13;

// Definiciones
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define COLS 16
#define ROWS 2
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Variables globales para productos y precios
String productos[] = {"Cafe Solo", "Cafe Cort", "Cafe Dobl", "Cafe Prem", "Chocolate"};
float precios[] = {1.0, 1.1, 1.25, 1.5, 2.0};

// Declarar el controlador de hilos
ThreadController controll = ThreadController();


// Objeto del hilo para actualizar constantemente el valor de los sensores
Thread sensors_thread = Thread();

// Hilo para reiniciar el estado
Thread reinicio_thread = Thread();

// Hilo para el acceso a la interfaz de administración
Thread admin_thread = Thread();

// Variables globales de sensores
float distancia_us = 0;
float temperatura = 0;
float humedad = 0;

// TIempo de inicio
unsigned long startTime;

// Seleccion del admin
int seleccion_admin = 0;

// Bool para saber si estamos en servicio b) y poder reiniciar
static boolean on_b = false;

// Funciones

// Esperar tiempo pero permitir la ejecucion de los hilos
void esperar(int tiempo) {
  unsigned long inicio = millis();
  while (millis() - inicio < tiempo) {
    controll.run();
  }
}

// Actualizar valores de los sensores
void sensors_thread_func() {
  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();
  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);
  distancia_us = pulseIn(pinEcho, HIGH) / 59.0;
}

// Reiniciar a servicio si se pulsa el boton de 2 a 3 segundos si estamos en b)
void reinicio_thread_func() {
  static unsigned long tiempoInicio = 0;
  static boolean enEspera = false;

  if (digitalRead(pinBoton) == HIGH && !enEspera) {
    tiempoInicio = millis();
    enEspera = true;
  } else if (digitalRead(pinBoton) == LOW && enEspera) {
    enEspera = false;

    if (millis() - tiempoInicio > 2000 && millis() - tiempoInicio < 3000 && on_b) {
      on_b = false;
      Serial.println("Reiniciando estado...");
      servicio();
    }
  }
}


// Cambiar etre modo admin y not admin
void admin_thread_func() {
  static unsigned long tiempoInicio = 0;
  static boolean enEspera = false;
  static boolean modoAdmin = false;

  if (digitalRead(pinBoton) == HIGH) {;
    if (!enEspera) {
      tiempoInicio = millis();
      enEspera = true;
      Serial.println("Comenzando a contar tiempo");
    } else {
      if (millis() - tiempoInicio > 5000) {
        // Cambiar al modo admin si aún no está en modo admin
        if (!modoAdmin) {
          Serial.println("Entrando al modo Admin...");
          modoAdmin = true;
          enEspera = false;
          adminMenu();
          esperar(200);
        } else {
          Serial.println("Saliendo del modo Admin...");
          modoAdmin = false;
          enEspera = false;
          servicio();
          esperar(200);
        }
      }
    }
  } else {
    if (enEspera) {
    }
    Serial.println("Tiempo parado");
    enEspera = false;
    tiempoInicio = 0;  // Restablecer tiempoInicio cuando el botón se suelta
    
  }
}



void parpadearLED1() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(pinLED1, HIGH);
    esperar(1000);
    digitalWrite(pinLED1, LOW);
    esperar(1000);
  }
}

void mostrarMensajeLCD(String mensaje, int tiempo) {
  lcd.clear();
  lcd.setCursor(0, 0);

  int pos = 0;
  int len = mensaje.length();

  // Si la longitud del mensaje es mayor que la pantalla, dar error
  if (len > COLS * ROWS) {
    // Dar error si la longitud del mensaje es mayor que la pantalla
    Serial.println("Error: El mensaje es demasiado largo para la pantalla\n");
    lcd.setCursor(0, 0);
    lcd.print("Error:)");
    lcd.setCursor(0, 1);
    lcd.print("MSG MUY LARGO");
    return;
  }

  for (int i = 0; i < len; ++i) {
    if (mensaje[i] == '\n') {
      // Si se encuentra un salto de línea, pasa a la siguiente fila
      lcd.setCursor(0, 1);
      pos = 0;
    } else {
      // Si no, imprime el carácter actual
      lcd.print(mensaje[i]);
      ++pos;
    }

    // Espera un breve periodo para que se vea bien
    esperar(20);
  }

  // Espera el tiempo indicado
  esperar(tiempo);
}

void mostrarTemperaturaHumedad(int tiempo) {
  if (!isnan(humedad) && !isnan(temperatura)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatura);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humedad);
    lcd.print(" %");
    esperar(tiempo);
  } else {
    mostrarMensajeLCD("Error DHT11", 5000);
  }
}

void mostrarProductos() {
  int seleccion = 0;
  int posicionSuperior = 0;
  int posicionInferior = min(4, posicionSuperior + 1);

  // Mientras no se pulse el joystick
  while (digitalRead(pinJoyButton) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);

    // Parte gráfica
    for (int i = posicionSuperior; i <= posicionInferior; i++) {
      if (i == seleccion) {
        lcd.print(">");
      } else {
        lcd.print(" ");
      }
      lcd.print(productos[i] + " ");
      lcd.print(precios[i], 2);
      lcd.print("E");

      lcd.setCursor(0, 1 + (i - posicionSuperior));
    }

    // Aumentar o disminuir la selección dependiendo del joystick Y
    int joyY = analogRead(pinJoyY);
    if (joyY > 900 && seleccion > 0) {
      seleccion--;
      esperar(300);
    } else if (joyY < 200 && seleccion < 4) {
      seleccion++;
      esperar(300);
    }

    // Establecer límites
    if (seleccion < posicionSuperior) {
      posicionSuperior = max(0, seleccion);
      posicionInferior = min(4, posicionSuperior + 1);
    } else if (seleccion > posicionInferior) {
      posicionInferior = min(4, seleccion);
      posicionSuperior = max(0, posicionInferior - 1);
    }

    esperar(100);
  }

  // Se ha pulsado el joystick, mostrar preparación
  String productoSeleccionado = productos[seleccion];
  float precioSeleccionado = precios[seleccion];

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Preparando...");
  lcd.setCursor(0, 1);
  lcd.print(productoSeleccionado);

  incrementarLED2Aleatorio();

  mostrarMensajeLCD("RETIRE BEBIDA", 3000);
  servicio();
}

void incrementarLED2Aleatorio() {
  int tiempoAleatorio = random(4000, 8000);  // Generar tiempo aleatorio entre 4 y 8 segundos

  for (int i = 0; i <= 255; i += 5) {
    analogWrite(pinLED2, i);
    // Ajustar la velocidad de espera proporcionalmente al tiempo aleatorio
    esperar(tiempoAleatorio/(255/5));
  }

  analogWrite(pinLED2, 0);
}


void adminMenu() {

  // Mientras no se pulse el joystick
  while (digitalRead(pinJoyButton) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Admin Menu");
    lcd.setCursor(0, 1);

    // Aumentar o disminuir la selección dependiendo del joystick Y
    int joyY = analogRead(pinJoyY);
    if (joyY < 200) {
      seleccion_admin = (seleccion_admin + 1) % 4;
      esperar(300);
    } else if (joyY > 900 && seleccion_admin > 0) {
      seleccion_admin = (seleccion_admin - 1 + 4) % 4;
      esperar(300);
    }

    // Parte gráfica
    lcd.setCursor(0, 1);
    switch (seleccion_admin) {
      case 0:
        lcd.print("> Ver temperatura");
        break;
      case 1:
        lcd.print("> Ver dist. sensor");
        break;
      case 2:
        lcd.print("> Ver contador");
        break;
      case 3:
        lcd.print("> Modificar precios");
        break;
    }

    esperar(100);
  }

  int joyX = analogRead(pinJoyX);
  // Selección elegida, mostrar cada menú mientras el joystick no se gire a la izquierda
  switch (seleccion_admin) {
    case 0:
      while (joyX > 100) {
        joyX = analogRead(pinJoyX);
        esperar(100);
        mostrarTemperaturaHumedad(100);
      }
      esperar(100);
      break;

    case 1:
      while (joyX > 100) {
        joyX = analogRead(pinJoyX);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Distancia: ");
        lcd.print(distancia_us);
        lcd.print("cm");
        esperar(100);
      }
      break;

    case 2:
      while (joyX > 100) {
        mostrarContador();
        joyX = analogRead(pinJoyX);
      }
      break;

    case 3:
      esperar(400);
      modificarPrecios();
      break;
  }

  esperar(100);
  adminMenu();
}

// Contador desde el inicio
void mostrarContador() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Contador: ");
  lcd.print((millis() - startTime) / 1000);
  lcd.print("s");
  esperar(100);
}

void modificarPrecios() {
  int seleccion = 0;
  float tempPrecio = precios[seleccion]; // Variable temporal para evitar cambios inmediatos

  while (true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Precio: " + String(tempPrecio, 2) + "EUR");

    int joyX = analogRead(pinJoyX);
    if (joyX > 900 && seleccion >= 0) {
      seleccion = (seleccion + 1) % 5;
      tempPrecio = precios[seleccion]; // Variable temporal (no sabemos si vamos a confirmar o no)
      esperar(300);
    }

    int joyY = analogRead(pinJoyY);
    if (joyY < 200) {
      // Decrementar precio en 5 centavos
      tempPrecio -= 0.05;
      esperar(200);
    } else if (joyY > 900) {
      // Incrementar precio en 5 centavos
      tempPrecio += 0.05;
      esperar(200);
    }

    lcd.setCursor(0, 1);
    switch (seleccion) {
      case 0:
        lcd.print("Cafe Solo");
        break;
      case 1:
        lcd.print("Cafe Cortado");
        break;
      case 2:
        lcd.print("Cafe Doble");
        break;
      case 3:
        lcd.print("Cafe Premium");
        break;
      case 4:
        lcd.print("Chocolate");
        break;
    }

    // Confirmar precio al presionar el joystick
    if (digitalRead(pinJoyButton) == LOW) {
      precios[seleccion] = tempPrecio; // Confirmar el precio
      esperar(1000); // Esperar para no salir del bucle
    }

    // Cancelar y volver a la lista de precios al mover el joystick a la izquierda
    joyX = analogRead(pinJoyX);
    if (joyX < 200) {
      esperar(300);
      break; // Salir del bucle (no se confirma el precio)
    }

    esperar(100);
  }

  esperar(300);
}


void servicio() {
  if (distancia_us > 100) {
    mostrarMensajeLCD("ESPERANDO\nCLIENTE", 400);
  } else { // b)
    on_b = true;
    Serial.println("on_b");
    Serial.println(on_b);
    mostrarTemperaturaHumedad(5000); // 5 segundos
    mostrarProductos(); // mostrar productos
  }
}

void setup() {
  pinMode(pinLED1, OUTPUT);
  pinMode(pinLED2, OUTPUT);
  pinMode(pinJoyButton, INPUT_PULLUP);
  pinMode(pinBoton, INPUT);
  pinMode(pinTrigger, OUTPUT);
  pinMode(pinEcho, INPUT);
  Serial.begin(9600);

  // Inicializar el sensor DHT11
  dht.begin();

  // Inicializar el LCD
  lcd.begin(COLS, ROWS);

  startTime = millis();

  // Configurar thread de los sensores
  sensors_thread.onRun(sensors_thread_func);
  sensors_thread.setInterval(500); // Intervalo de verificación de 500 ms

  // Configurar thread de reinicio
  reinicio_thread.onRun(reinicio_thread_func);
  reinicio_thread.setInterval(100);  // Intervalo de verificación de 100 ms

  // Configurar thread de acceso a la interfaz de administración
  admin_thread.onRun(admin_thread_func);
  admin_thread.setInterval(100);  // Intervalo de verificación de 100 ms

  // Añadir threads al controlador
  controll.add(&sensors_thread);
  controll.add(&reinicio_thread);
  controll.add(&admin_thread);

  // Mostrar cargando y parpadear 3 veces
  lcd.print("CARGANDOO...");
  parpadearLED1();
  lcd.clear();
}

void loop() {
  controll.run(); // Lanzar controlador de los threads
  servicio(); // Lanzar servicio inicialmente
}

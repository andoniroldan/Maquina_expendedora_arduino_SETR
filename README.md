
# Maquina expendedora en arduino

¡Bienvenido a la documentación completa de la máquina expendedora de bebidas!

Este proyecto utiliza Arduino y diversas bibliotecas para proporcionar un servicio de dispensación de bebidas. La siguiente documentación explora detalladamente cada aspecto del código y profundiza en la implementación de hilos para un rendimiento eficiente.

## Contenido

1. [**Introducción**](#introducción)
2. [**Pines y Definiciones**](#pines-y-definiciones)
3. [**Inicialización y Configuración**](#inicialización-y-configuración)
4. [**Hilos en Arduino**](#hilos-en-arduino)
   - [**Hilo de Sensores**](#hilo-de-sensores)
   - [**Hilo de Reinicio**](#hilo-de-reinicio)
   - [**Hilo de Administración**](#hilo-de-administración)
5. [**Funciones Principales**](#funciones-principales)
   - [**Esperar con Hilos en Ejecución**](#esperar-con-hilos-en-ejecución)
   - [**Menú de Administración**](#menú-de-administración)
   - [**Modificar Precios**](#modificar-precios)
   - [**Servicio Principal**](#servicio-principal)
5. [**Conclusión**](#conclusión)

## Introducción

La máquina expendedora de bebidas es un proyecto basado en Arduino que utiliza diversos sensores y actuadores para proporcionar un servicio de dispensación de bebidas. Este README se centrará en el código que impulsa la máquina, ofreciendo una explicación detallada y snippets de código relevantes.

## Pines y Definiciones

En esta sección, se detallan los pines utilizados en el proyecto y se establecen definiciones clave. La asignación de pines es crucial para conectar y controlar eficientemente los componentes físicos de la máquina. Aquí, cada elemento, desde LEDs hasta sensores, está claramente definido, facilitando el mantenimiento y la comprensión del código.

### Pines Utilizados:

*pinLED1* y *pinLED2*: Controlan LEDs para indicar estados y procesos. 

*pinJoyY*, *pinJoyX*, y *pinJoyButton*: Conectados al joystick para la interacción del usuario.  

*pinBoton*: Botón adicional utilizado para acciones específicas.

*pinTrigger* y *pinEcho*: Pines del sensor de ultrasonido para medir la distancia.

Definiciones:  
*DHTPIN* y *DHTTYPE*: Configuran el sensor de temperatura y humedad DHT11.

*COLS* y *ROWS*: Especifican las columnas y filas de la pantalla LCD.

productos[] y precios[]: Almacenan nombres y precios de productos respectivamente.

### Hilos y Controlador:

Se utiliza un controlador de hilos (ThreadController) para gestionar eficientemente múltiples tareas concurrentes. 

Tres hilos importantes se crean y utilizan:

*sensors_thread*: Actualiza constantemente los valores de los sensores.

*reinicio_thread*: Monitorea el botón de reinicio y gestiona la transición de estados.

*admin_thread*: Facilita el acceso al modo de administración y realiza funciones relacionadas.

Estas decisiones de diseño aseguran un control eficaz de los componentes físicos, mejorando la modularidad y la mantenibilidad del código.

```cpp
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

// Tiempo de inicio
unsigned long startTime;

// Seleccion del admin
int seleccion_admin = 0;

// Bool para saber si estamos en servicio b) y poder reiniciar
static boolean on_b = false;

```

## Inicialización y Configuración

La función setup() es esencial para la configuración inicial de la máquina expendedora. Aquí, se establecen las configuraciones de los pines, se inicia la comunicación serial, se inicializan componentes como el sensor DHT11 y la pantalla LCD, y se configuran los hilos con sus funciones asociadas y frecuencias de ejecución. Además, se realiza un proceso de inicio visual mediante el parpadeo de LEDs y se muestra un mensaje de carga en la pantalla LCD.

Esta sección asegura que la máquina esté preparada para operar de manera óptima desde el momento de su encendido, estableciendo la base para un funcionamiento suave y eficiente.

``` cpp
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

```

## Hilos en Arduino

En este proyecto, se implementan hilos en Arduino para gestionar tareas concurrentes de manera eficiente. A continuación, se describen los hilos específicos utilizados:

### Hilo de Sensores

Este hilo se encarga de actualizar periódicamente los valores de los sensores, incluyendo la temperatura, humedad y la distancia medida por el sensor de ultrasonido. La frecuencia de actualización se establece en 500 ms para mantener información actualizada.

```cpp
// Objeto del hilo para actualizar constantemente el valor de los sensores
Thread sensors_thread = Thread();

// Función del hilo de sensores
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
```

### Hilo de Reinicio

El hilo de reinicio se encarga de monitorear el botón de reinicio y realizar acciones correspondientes en función de su estado. Si se pulsa el botón de 2 a 3 segundos y el estado es servicio b),
se reinicia al estado servicio.

```cpp
// Hilo para reiniciar el estado
Thread reinicio_thread = Thread();

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

```


### Hilo de Administración

Este hilo gestiona el acceso al modo de administración, permitiendo la visualización de información específica y la ejecución de acciones administrativas cuando se activa.  
Si el botón se pulsa más de 5 segundos en cualquier estado, se accede al modo admin si no se está y se sale del modo admin si se está en él.

```cpp
// Hilo para el acceso a la interfaz de administración
Thread admin_thread = Thread();

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

```

Estos hilos se integran en el controlador de hilos (controll), lo que permite una ejecución simultánea y eficiente de las diferentes tareas dentro de la máquina expendedora.

## Funciones Principales

En esta sección, se describen las funciones principales que impulsan el funcionamiento de la máquina expendedora. Estas funciones abordan aspectos clave del proyecto, desde la espera controlada hasta la administración de precios y la prestación de servicios.

### Esperar con Hilos en Ejecución

La función esperar se encarga de pausar la ejecución del programa durante un período específico, pero con la particularidad de permitir que los hilos continúen su ejecución. Esto es fundamental para evitar bloqueos y asegurar que los procesos concurrentes, como la actualización de sensores o la gestión de administración, puedan continuar su ejecución durante los tiempos de espera.

```cpp
// Esperar tiempo pero permitir la ejecución de los hilos
void esperar(int tiempo) {
  unsigned long inicio = millis();
  while (millis() - inicio < tiempo) {
    controll.run();
  }
}

```

### Menú de Administración

La función adminMenu representa la interfaz principal para la administración de la máquina. Permite al usuario navegar por un menú interactivo, proporcionando acceso a diversas funcionalidades administrativas. Esto incluye la visualización de información relevante, como temperatura, distancia de sensor y contador, así como la capacidad de modificar precios.

La navegación de hace completamente con el joystick, en las cuatro direcciones y con su propio switch

```cpp
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

```

### Modificar Precios

La función modificarPrecios, a la que se accede desde adminMenu, posibilita la alteración dinámica de los precios de los productos. Esto ofrece flexibilidad para ajustar los costos de los productos sin la necesidad de modificar directamente el código. Durante la ejecución de esta función, el usuario puede seleccionar un producto y ajustar su precio de manera intuitiva mediante el joystick.

```cpp

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

```

### Servicio Principal

La función servicio coordina el flujo principal de la máquina dispensadora.  

Dependiendo de la distancia medida por el sensor de ultrasonido, determina si la máquina está en espera de un cliente o si se encuentra en el estado b) 

En el estado b) se muestra la temperatura y humedad durante 5 segundos para despúes mostrar el menú de eleccion de bebida y su preparación (mostrarProductos()).  
La preparación se simula mostrando el mensaje "Preparando $bebida" e incrementado la intensidad del led de manera gradual durante un tiempo aleatorio entre 4 y 8 segundos. Después se muestra el mensaje "RETIRE BEBIDA" durante 3 segundos y se reinicia el estado de servicio.

```cpp
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

```

Esta función encapsula la lógica principal del servicio y garantiza una transición suave entre diferentes estados.

Estas funciones constituyen la columna vertebral del código, proporcionando una estructura modular que facilita la comprensión y el mantenimiento del sistema.


## Conclusión

La documentación detallada de la máquina expendedora de bebidas basada en Arduino ofrece una visión exhaustiva del proyecto, abordando aspectos cruciales de su implementación y funcionamiento.  
A lo largo de esta documentación, hemos explorado desde la asignación de pines y definiciones hasta las funciones principales que definen el comportamiento de la máquina.

El uso de hilos en Arduino destaca como un elemento clave en la estructura del código, permitiendo la ejecución concurrente de tareas y mejorando la eficiencia del sistema. Los hilos de sensores, reinicio y administración se combinan para garantizar la actualización continua de datos, el manejo adecuado de reinicios y la administración intuitiva de la máquina.

La función adminMenu proporciona una interfaz clara y accesible para la administración, permitiendo al usuario visualizar información relevante y realizar acciones administrativas de manera sencilla. La capacidad de modificar precios dinámicamente mediante modificarPrecios añade flexibilidad y personalización al sistema, adaptándose a cambios en los costos de los productos.

En el núcleo de la máquina, la función servicio coordina el flujo principal, gestionando desde la espera de clientes hasta la dispensación de bebidas. 

La función esperar asegura pausas controladas, permitiendo que los hilos mantengan su ejecución durante estos períodos.

En resumen, la máquina expendedora de bebidas demuestra un diseño modular, eficiencia en la ejecución de tareas y una interfaz de administración intuitiva. 

Esta documentación sirve como recurso valioso para comprender y expandir el proyecto, ofreciendo una guía detallada para cualquier persona interesada en explorar y mejorar la funcionalidad de la máquina.

Se dispone de un jpg de el montaje de la máquina hecho con Fritzing y un video explicativo del uso de la máquina.

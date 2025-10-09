#include <Dynamixel2Arduino.h>

// Define el baudrate para la comunicación
#define DYNAMIXEL_BAUDRATE 9600

// Instancia un objeto para controlar el motor
Dynamixel2Arduino dxl(Serial, 2);  // Usamos Serial para el puerto y 2 como pin de dirección

// ID del motor (en este caso, 1)
#define MOTOR_ID 47
void setup() {
  // Inicia la comunicación con los motores Dynamixel
  dxl.begin(DYNAMIXEL_BAUDRATE);

  // Establece el modo de operación del motor en "Position Control"
  dxl.setOperatingMode(MOTOR_ID, 3);  // Usamos 3 para "Position Control"

  // Mueve el motor a una posición específica (valor entre 0 y 1023)
  int position = 512;  // El valor central del rango de movimiento
  dxl.writeControlTableItem(MOTOR_ID, 30, position);  // 30 es el registro para "Goal Position"
}

void loop() {
  // El motor se mueve a la posición indicada solo una vez durante el inicio
  delay(1000);  // Pausa para observar el movimiento del motor
}

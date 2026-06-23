# V.2 del projecte

### 1. Gestió de Tasques (Tasks)
* **`TaskServo` (Asignada al Nucli 0):** Controla de forma exclusiva la màquina d'estats del servomotor. Executa el gir en sentit horari (CW) i antihorari (CCW). En arrancar o activar-se la bandera de la interrupció, força una subrutina `homeServo()` per buscar l'orientació física inicial.
* **`TaskRCWL` (Asignada al Nucli 1):** Realitza peticions cícliques a través del bus I2C per obtenir les distàncies mesurades en mil·límetres (`distanciaMM`). Calcula teòricament l'angle actual basant-se en la velocitat de rotació i desa els registres dins d'un array de memòria volàtil compartida `distancies[181]`.
* **`TaskWeb` (Asignada al Nucli 1):** Escolta les peticions de xarxa local entrants (`server.handleClient()`). Té un impacte de CPU mínim gràcies a la seva baixa prioritat i temps de retard (*delay*) no bloquejants.

### 2. Rutina de Interrupció per Maquinari (ISR)
* **`limitISR()`:** Rutina lleugera carregada directamen a la **IRAM** per obtenir una velocitat de resposta instantània. S'activa per flanc de baixada (`FALLING`) quan el servo col·lideix suaument amb el final de cursa. Modifica de forma atòmica les banderes globals `origenTrobat = true` i `netejarRadar = true` perquè les tasques principals reajustin les seves variables d'estat en el seu proper cicle d'execució.

---

### Taula de connexions

> 
> | Component | Color de cable | Senyal | Pin ESP32-S3 | 
> | :--: | :---: | :---: | :---: | 
> | Servomotor | Marró | GND | GND | 
> |  | Vermell | VCC (3.3V) | 3V3 | 
> |  | Groc | PMW | GPIO 5 | 
> | Sensor | Negre | GND | GND | 
> |  | Vermell | VCC (3.3V) | 3V3 | 
> |  | Verd | SDA | GPIO 9 |
> |  | Blau | SCL | GPIO 8 |
> | Polsador | Groc | Senyal | GPIO 4 | 
> |  | Groc | GND | GND | 
>

---

## 🌐 API Endpoints del Servidor Web

O microcontrolador aixeca una infraestructura HTTP lleugera que exposa dues rutes accessibles des de navegadors web tradicionals:

1.  **`GET /` (Arrel):** Retorna el document HTML, els estils CSS i la lògica de control JavaScript encarregada d'inicialitzar un element `<canvas>` on es renderitza el radar de forma gràfica i fluida.
2.  **`GET /data` (Dades JSON):** Retorna l'estat en viu del microcontrolador en format JSON estructurat, emetent l'angle actual i el llistat de distàncies processades perquè la interfície s'actualitzi dinàmicament sense recarregar la pàgina.

---

## 📦 Dependències de Biblioteques

L'entorn descarrega i gestiona automàticament les llibreries necessàries especificades a la secció `lib_deps` de la configuració de l'entorn:
-   `madhephaestus/ESP32Servo @ ^3.2.0` (Control per PWM avançat i estable de servomotors en arquitectures ESP32).
-   Biblioteques natives del nucli d'Arduino per a ESP32: `Wire`, `WiFi`, `WebServer`.
"""

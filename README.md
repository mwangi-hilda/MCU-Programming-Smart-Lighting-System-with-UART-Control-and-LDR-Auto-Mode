# MCU-Programming-Smart-Lighting-System-with-UART-Control-and-LDR-Auto-Mode
This project implements an intelligent lighting system using an AVR microcontroller that supports **automatic lighting based on ambient light** (LDR) and **manual override via UART commands**. It combines **interrupt-driven ADC** and **UART communication**, using a flexible control structure to manage up to 3 LEDs.

---

## Project Overview

- **Microcontroller**: AVR (ATmega2560)
- **Clock Frequency**: 8 MHz
- **Inputs**:
  - Light-Dependent Resistor (LDR) on ADC0
  - UART serial commands from (virtual) terminal
- **Outputs**:
  - 3 LEDs (PF1, PF2, PF3)
- **Features**:
  - Automatic lighting based on ambient light (<30% threshold)
  - UART-controlled manual override (`Light 1 on`, `All lights off`, etc.)
  - Command to return to **Auto Mode** (`ldr on`)
  - Buffered UART input and interrupt-driven ADC sampling

---

## UART Command Set

| Command            | Action                          |
|--------------------|---------------------------------|
| `light 1 on`       | Turn on LED 1                   |
| `light 1 off`      | Turn off LED 1                  |
| `light 2 on`       | Turn on LED 2                   |
| `light 2 off`      | Turn off LED 2                  |
| `light 3 on`       | Turn on LED 3                   |
| `light 3 off`      | Turn off LED 3                  |
| `all lights on`    | Turn on all LEDs                |
| `all lights off`   | Turn off all LEDs               |
| `ldr on`           | Switch back to light-sensing mode |
| _Unknown command_  | Returns: `"Unknown Command.\r\n"` |

Commands are **case-insensitive** and sent via UART (ends with `\n` or `\r`).

---

## How It Works

- **ADC**:
  - Samples all 16 channels using interrupt-driven logic.
  - LDR is connected to ADC0.
  - Light intensity is converted to % using:  
    ```c
    float light_value = (adc_results[0] / 1024.0) * 100.0;
    ```
  - Lights turn on **automatically** when ambient light < 30%.

- **UART**:
  - Incoming characters are captured via `USART0_RX_vect`.
  - Once a complete message is received (newline or carriage return), it is parsed and handled.
  - Commands can override the automatic LDR behavior until `ldr on` is received.

- **LED Control**:
  - Uses a struct array `led_states[3]` for each LED:
    ```c
    typedef struct {
      uint8_t current_state;
      uint8_t mannual_override_active;
    } led_control_t;
    ```
---

## Example Use Case

1. In a dark room (LDR < 30%), all lights turn on automatically.
2. You type `light 1 off` → LED 1 turns off, others remain on.
3. You type `ldr on` → System resumes auto control (LED 1 comes back on if LDR < 30%).

---

## Circuit Setup in Proteus
<img width="919" height="667" alt="image" src="https://github.com/user-attachments/assets/660b5cb2-bfc9-46da-a445-61c83aee85dc" />


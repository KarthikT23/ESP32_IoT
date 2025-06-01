/**
 * @file bmp180.h
 * ESP-IDF driver for BMP180 digital pressure sensor
 * Enhanced version with altitude, sea level pressure, dew point, and air density calculations
 */
#ifndef __BMP180_H__
#define __BMP180_H__

#include <stdint.h>
#include <stdbool.h>
#include "hal/i2c_types.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMP180_DEVICE_ADDRESS 0x77

// Standard atmosphere constants (moved from .c)
#define SEA_LEVEL_PRESSURE_PA 101325.0f
#define TEMPERATURE_LAPSE_RATE 0.0065f
#define MOLAR_MASS_DRY_AIR 0.0289644f
#define UNIVERSAL_GAS_CONSTANT 8.31432f
#define GRAVITY_ACCEL 9.80665f

/**
 * Hardware accuracy mode
 */
typedef enum
{
    BMP180_MODE_ULTRA_LOW_POWER = 0,  // 1 sample, 4.5 ms
    BMP180_MODE_STANDARD,             // 2 samples, 7.5 ms
    BMP180_MODE_HIGH_RESOLUTION,      // 4 samples, 13.5 ms
    BMP180_MODE_ULTRA_HIGH_RESOLUTION // 8 samples, 25.5 ms
} bmp180_mode_t;

typedef struct i2c_lowlevel_s
{
   i2c_master_bus_handle_t *bus;  // If NULL, will create new bus using params below
   i2c_port_t port;               // I2C port number
   int pin_sda;                   // SDA pin
   int pin_scl;                   // SCL pin
} i2c_lowlevel_config;

/**
 * BMP180 sensor readings
 */
typedef struct {
    float temperature;        // Temperature in Celsius
    uint32_t pressure;        // Pressure in Pa
    float pressure_hPa;       // Pressure in hPa
    float altitude;           // Altitude in meters
    float sea_level_pressure; // Sea level pressure in Pa
    float dew_point;          // Dew point in Celsius
    float air_density;        // Air density in kg/m³
    bool valid;               // True if readings are valid
} bmp180_readings_t;

typedef void *bmp180_t;

// Initialization and cleanup
bmp180_t bmp180_init(i2c_lowlevel_config *config, uint8_t i2c_address, bmp180_mode_t mode);
bool bmp180_free(bmp180_t bmp);

// Basic measurement
bool bmp180_measure(bmp180_t bmp, float *temperature, uint32_t *pressure);

// Calculation functions
float bmp180_calculate_altitude(uint32_t pressure_pa, float sea_level_pressure_pa);
float bmp180_calculate_sea_level_pressure(uint32_t pressure_pa, float altitude_m, float temperature_c);
float bmp180_calculate_dew_point(float temperature_c, float humidity_percent);
float bmp180_calculate_air_density(uint32_t pressure_pa, float temperature_c, float humidity_percent);

// Task management
bool BMP180_task_start(int sda_pin, int scl_pin, float altitude);
bool BMP180_task_start_advanced(i2c_lowlevel_config *config, uint8_t i2c_address, bmp180_mode_t mode, float altitude);

// Data access functions
bmp180_readings_t BMP180_get_readings(void);
float BMP180_get_temperature(void);
uint32_t BMP180_get_pressure(void);
float BMP180_get_pressure_hPa(void);
float BMP180_get_altitude(void);
float BMP180_get_sea_level_pressure(void);
float BMP180_get_dew_point(void);
float BMP180_get_air_density(void);

#ifdef __cplusplus
}
#endif

#endif /* __BMP180_H__ */
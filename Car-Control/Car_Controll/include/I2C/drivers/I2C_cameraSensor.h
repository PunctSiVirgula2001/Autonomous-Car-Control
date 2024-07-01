/*
 * I2C_cameraSensor.h
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_I2C_I2C_CAMERASENSOR_H_
#define INCLUDE_I2C_I2C_CAMERASENSOR_H_

#include "I2C_common.h"

#define WIDTH 78
#define HEIGHT 51
#define QUEUE_SIZE_PIXY 20
#define QUEUE_SIZE_DATATYPE_PIXY sizeof(uint8_t)

#define ON 1U
#define OFF 0U

#define PIXY_DETECTION ON

typedef enum {
    LINE_VECTOR = 1,
    LINE_INTERSECTION = 2,
    LINE_BARCODE = 4
} Pixy2FeatureType;

typedef struct {
    uint8_t x0;
    uint8_t y0;
    uint8_t x1;
    uint8_t y1;
    uint8_t index;
} Vector;

typedef struct {
    uint8_t numVectors;
    Vector vectors[100]; // Adjust the size as per your needs
} Pixy2Lines;

typedef struct{
	uint8_t computedSpeedSetpoint;
	uint8_t computedSteerSetpoint;
}pixy2Commands;

typedef enum {
    PIXY2_REQUEST_GET_VERSION = 14,
    PIXY2_REQUEST_GET_RESOLUTION = 12,
    PIXY2_REQUEST_SET_CAMERA_BRIGHTNESS = 16,
    PIXY2_REQUEST_SET_LED = 20,
    PIXY2_REQUEST_SET_LAMP = 22,
    PIXY2_REQUEST_GET_BLOCKS = 32,
    PIXY2_REQUEST_GET_MAIN_FEATURES = 48,
    PIXY2_REQUEST_SET_MODE = 54
} Pixy2RequestTypes;

typedef enum {
    PIXY2_RESPONSE_VERSION = 15,
    PIXY2_RESPONSE_RESOLUTION = 13,
    PIXY2_RESPONSE_ACK = 1,
    PIXY2_RESPONSE_BLOCKS = 33,
    PIXY2_RESPONSE_MAIN_FEATURES = 49
} Pixy2ResponseTypes;




void requestPixy2Version(I2C_dev_handles pixy2_handle);
void setPixy2Lamp(I2C_dev_handles pixy2_handle, uint8_t upper, uint8_t lower);
void setPixy2LED(I2C_dev_handles pixy2_handle, uint8_t r, uint8_t g, uint8_t b);
void getPixy2Lines(I2C_dev_handles pixy2_handle, bool wait, Pixy2Lines *lines);
bool getBestVectorLeft(Pixy2Lines *lines, Vector *vec);
bool getBestVectorRight(Pixy2Lines *lines, Vector *vec);
void computeSpeedAndSteer(Vector vecLeft, Vector vecRight, bool existsGoodVecLeft, bool existsGoodVecRight, pixy2Commands *commands);

#endif /* INCLUDE_I2C_I2C_CAMERASENSOR_H_ */

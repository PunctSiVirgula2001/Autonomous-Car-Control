/*
 * I2C_cameraSensor.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#include "I2C_cameraSensor.h"
#include "MotorAndServoControl.h"
#include "PID.h"
#include "esp_log.h"

#define TAG "PIXY2"

QueueHandle_t autonomousModeControlPixyQueue;

// Function to send a request packet to Pixy2 camera
void requestPixy2Version(I2C_dev_handles pixy2_handle) {
    uint8_t requestPacket[4] = {0xae, 0xc1, 0x0e, 0x00};

    // Transmit the request packet to Pixy2
    I2C_transmit(pixy2_handle, requestPacket, sizeof(requestPacket));
    ESP_LOGI(TAG, "Sent request packet to Pixy2");

    // Prepare to receive the response
    uint8_t responsePacket[20]; // Adjust size based on expected response length

    // Receive the response packet from Pixy2
    I2C_receive(pixy2_handle, responsePacket, sizeof(responsePacket));

    // Print the response for debugging
    ESP_LOGI(TAG, "Received response packet from Pixy2:");
    for (int i = 0; i < sizeof(responsePacket); i++) {
        ESP_LOGI(TAG, "0x%02x ", responsePacket[i]);
    }

    // Parse and print the version information from the response packet
    if (responsePacket[0] == 0xaf && responsePacket[1] == 0xc1) {
        ESP_LOGI(TAG, "Checksum sync bytes matched");
        if (responsePacket[2] == 0x0f) {
            ESP_LOGI(TAG, "This is a different response type: 0x0f");
            // Add specific handling for 0x0f response type if known
        } else if (responsePacket[2] == 0x0e) {
            ESP_LOGI(TAG, "Version response type matched");
            ESP_LOGI(TAG, "Hardware Version: %d.%d", responsePacket[6], responsePacket[7]);
            ESP_LOGI(TAG, "Firmware Version: %d.%d", responsePacket[8], responsePacket[9]);
            ESP_LOGI(TAG, "Firmware Build Number: %d", (responsePacket[10] | (responsePacket[11] << 8)));
            ESP_LOGI(TAG, "Firmware Type: ");
            for (int i = 12; i < 12 + 10; i++) {
                ESP_LOGI(TAG, "%c", responsePacket[i]);
            }
        } else {
            ESP_LOGI(TAG, "Unexpected response type: 0x%02x", responsePacket[2]);
        }
    } else {
        ESP_LOGI(TAG, "Invalid response from Pixy2");
    }
}

void setPixy2CameraBrightness(I2C_dev_handles pixy2_handle, uint8_t brightness) {
    uint8_t requestPacket[5] = {0xae, 0xc1, PIXY2_REQUEST_SET_CAMERA_BRIGHTNESS, 0x01, brightness};

    // Select the Pixy2 camera channel on the multiplexer
    I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);

    // Transmit the request packet to Pixy2
    I2C_transmit(pixy2_handle, requestPacket, sizeof(requestPacket));
    ESP_LOGI(TAG, "Sent set camera brightness request to Pixy2");
}


void setPixy2LED(I2C_dev_handles pixy2_handle, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t requestPacket[7] = {0xae, 0xc1, PIXY2_REQUEST_SET_LED, 0x03, r, g, b};

    // Select the Pixy2 camera channel on the multiplexer
    I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);

    // Transmit the request packet to Pixy2
    I2C_transmit(pixy2_handle, requestPacket, sizeof(requestPacket));
    ESP_LOGI(TAG, "Sent set LED request to Pixy2");
}

void setPixy2Lamp(I2C_dev_handles pixy2_handle, uint8_t upper, uint8_t lower) {
    uint8_t requestPacket[6] = {0xae, 0xc1, PIXY2_REQUEST_SET_LAMP, 0x02, upper, lower};

    // Select the Pixy2 camera channel on the multiplexer
    I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);

    // Transmit the request packet to Pixy2
    I2C_transmit(pixy2_handle, requestPacket, sizeof(requestPacket));
    ESP_LOGI(TAG, "Sent set lamp request to Pixy2");
}


#define MAX_VECTORS 100

void getPixy2Lines(I2C_dev_handles pixy2_handle, bool wait, Pixy2Lines *lines) {
    uint8_t requestPacket[6] = {0xae, 0xc1, 48, 2, LINE_VECTOR, wait};

    // Transmit the request packet to Pixy2
    I2C_transmit(pixy2_handle, requestPacket, sizeof(requestPacket));
    //ESP_LOGI(TAG, "Sent get main features request to Pixy2");

    // Prepare to receive the response
    uint8_t responsePacket[100]; // Adjust size based on expected response length

    // Receive the response packet from Pixy2
    I2C_receive(pixy2_handle, responsePacket, sizeof(responsePacket));

    // Initialize the number of vectors to 0
    lines->numVectors = 0;

    // Validate sync and packet type
    if (responsePacket[0] == 0xaf && responsePacket[1] == 0xc1 && responsePacket[2] == 49) {
        uint8_t payloadLength = responsePacket[3]; // payload length is 1 byte

        // Calculate the total length to read
        int totalLength = 4 + payloadLength; // Includes the headers before payload

        int index = 6; // Start after the checksum (bytes 4-5)

        while (index < totalLength) {
            uint8_t featureType = responsePacket[index];
            uint8_t featureLength = responsePacket[index + 1];
            uint8_t* featureData = &responsePacket[index + 2];

            // Loop through each vector in this feature
            if (featureType == LINE_VECTOR) {
                int numberOfVectors = featureLength / 6; // Each vector is 6 bytes
                for (int i = 0; i < numberOfVectors; i++) {
                    int offset = i * 6;
                    uint8_t x0 = featureData[offset];
                    uint8_t y0 = featureData[offset + 1];
                    uint8_t x1 = featureData[offset + 2];
                    uint8_t y1 = featureData[offset + 3];
                    uint8_t vectorIndex = featureData[offset + 4];

                    if (lines->numVectors < MAX_VECTORS) {
                        lines->vectors[lines->numVectors].x0 = x0;
                        lines->vectors[lines->numVectors].y0 = HEIGHT - y0; // The vectors have the coordinate starting in the top-left corner.
                        lines->vectors[lines->numVectors].x1 = x1;
                        lines->vectors[lines->numVectors].y1 = HEIGHT - y1;
                        lines->vectors[lines->numVectors].index = vectorIndex;
                        lines->numVectors++;
                    }
                }
            }

            index += 2 + featureLength; // Move to the next feature
        }
    } else {
        //ESP_LOGI(TAG, "Invalid response from Pixy2");
    }
}

static float slope(Vector lines)
{
	if (lines.x0 - lines.x1 == 0) {
		return 9999.0f;

	} else {
		float x0 = (float)lines.x0;
		float x1 = (float)lines.x1;
		float y0 = (float)lines.y0;
		float y1 = (float)lines.y1;
		float panta = (y1 - y0) / (x1 - x0);

		return (double)(panta);
	}
}

static float procent_val(float panta_cur, float panta_prev)
{
	if ((double)(panta_cur) > (double)(panta_prev)) {
		return (double)(panta_prev / panta_cur);
	}

	return (double)(panta_cur / panta_prev);
}

bool getBestVectorLeft(Pixy2Lines *lines, Vector *vec) {
    bool found = false;
    double maxLength = 0;
    double minSlopeThreshold = 0.20;  // Lowered threshold to include nearly horizontal lines

    for (int i = 0; i < lines->numVectors; i++) {
        Vector v = lines->vectors[i];
        double midpoint = (v.x0 + v.x1) / 2.0;
        if (midpoint <= 39) {
            double dx = (double)(v.x1 - v.x0);
            double dy = (double)(v.y1 - v.y0);
            double slope = dx == 0 ? INFINITY : dy / dx;  // Handle vertical lines safely
            double length = sqrt(dx * dx + dy * dy);
            double absSlope = fabs(slope);

            if (absSlope >= minSlopeThreshold  && length > maxLength) {  // Arbitrary length threshold
                maxLength = length;
                *vec = v;
                found = true;
            }
        }
    }
    return found;
}


bool getBestVectorRight(Pixy2Lines *lines, Vector *vec) {
    bool found = false;
    double maxLength = 0;
    double minSlopeThreshold = 0.20;  // Consistent threshold with left vector function

    for (int i = 0; i < lines->numVectors; i++) {
        Vector v = lines->vectors[i];
        double midpoint = (v.x0 + v.x1) / 2.0;
        if (midpoint > 39) {
            double dx = (double)(v.x1 - v.x0);
            double dy = (double)(v.y1 - v.y0);
            double slope = dx == 0 ? INFINITY : dy / dx;  // Handle vertical lines safely
            double length = sqrt(dx * dx + dy * dy);
            double absSlope = fabs(slope);

            if ((absSlope >= minSlopeThreshold || length > 50) && length > maxLength) {
                maxLength = length;
                *vec = v;
                found = true;
            }
        }
    }
    return found;
}


static void flipVector(Vector *vec) {
    float auxX = vec->x0;
    float auxY = vec->y0;
    vec->x0 = vec->x1;
    vec->y0 = vec->y1;
    vec->x1 = auxX;
    vec->y1 = auxY;
}
void computeSpeedAndSteer(Vector vecLeft, Vector vecRight, bool existsGoodVecLeft, bool existsGoodVecRight, pixy2Commands *commands) {
    static Vector vecLlast = {0}, vecRlast = {0};
	static uint8_t lastSteer = 50;  // Neutral position
    static uint8_t lastSpeed = 0;
    float frameWidth = 78.0;  // Frame width assumption
    float frameHeight = 52.0;
    float frame_center = 39.0; // Central position of the frame to respect
    static uint8_t last_index_left = 0;
    static uint8_t last_index_right = 0;
    // Determine number of good vectors detected
    int numVectors = (existsGoodVecLeft ? 1 : 0) + (existsGoodVecRight ? 1 : 0);

    // check if line is upside down
	if (vecLeft.y1 < vecLeft.y0)
	{
		// swap endpoints
		flipVector(&vecLeft);
	}

	if (vecRight.y1 < vecRight.y0)
	{
		// swap endpoints
		flipVector(&vecRight);
	}

    // Apply the logic from the second function
    if (existsGoodVecLeft) {
        if (vecLeft.index != last_index_left && procent_val(fabs(slope(vecLeft)),fabs(slope(vecLlast))) > 0.3) {
            vecLlast = vecLeft;
            last_index_left = vecLeft.index;
        } else if (procent_val( fabs(slope(vecLeft)), fabs(slope(vecLlast))) > 0.7) {
            vecLeft = vecLlast;
            last_index_left = 255;  // Assuming 255 means invalid index
        }
    }

    if (existsGoodVecRight) {
        if (vecRight.index != last_index_right && procent_val(fabs(slope(vecRight)),fabs(slope(vecRlast))) > 0.3) {
            vecRlast = vecRight;
            last_index_right = vecRight.index;
        } else if (procent_val(fabs(slope(vecRight)),fabs(slope(vecRlast))) > 0.7) {
            vecRight = vecRlast;
            last_index_right = 255;  // Assuming 255 means invalid index
        }
    }

    switch (numVectors) {
        case 0:
            // No vectors detected
            commands->computedSteerSetpoint = lastSteer;
            commands->computedSpeedSetpoint = lastSpeed; // Reduce speed significantly
            break;

        case 1:
        {
		   Vector single_vec;
		   if (existsGoodVecRight) {
			   single_vec = vecRight;
		   } else if (existsGoodVecLeft) {
			   single_vec = vecLeft;
		   }

		   // Calculate the slope of the detected vector
		   float single_vec_slope = (float)(single_vec.y1 - single_vec.y0) / (float)(single_vec.x1 - single_vec.x0);
		   float single_vec_midpoint_x = (float)(single_vec.x1 + single_vec.x0) / 2;

		   float estimated_lane_width = 78.0f;
		   float estimated_other_boundary_midpoint_x;

		   if (single_vec_slope > 0) {
			   estimated_other_boundary_midpoint_x = single_vec_midpoint_x + estimated_lane_width;
		   } else {
			   estimated_other_boundary_midpoint_x = single_vec_midpoint_x - estimated_lane_width;
		   }

		   float lane_center_x = (single_vec_midpoint_x + estimated_other_boundary_midpoint_x) / 2;

		   float error = (lane_center_x - frame_center) / frame_center;

		   // Implement proportional-derivative control for steering
		   float Kp = 1.0f; // Proportional gain
		   float Kd = 0.1f; // Derivative gain

		   static float last_error = 0;
		   float derivative = error - last_error;
		   float control_steer = -(Kp * error + Kd * derivative);

		   // Update last_error
		   last_error = error;

		   // Make sure the steering angle is within the range [0, 100]
		   float steer = 50 + control_steer * 50; // Map control_steer from [-1, 1] to [0, 100]
		   commands->computedSteerSetpoint = (uint8_t)fmax(0, fmin(100, steer));

		   // Adjust speed based on steer: more deviation from 50, lower the speed
		   float steer_deviation = fabs(50.0f - (float)commands->computedSteerSetpoint);
		   commands->computedSpeedSetpoint = (uint8_t)(70 - (steer_deviation / 50.0f) * (70 - 50));

		   // Ensure the speed is within the allowable range
		   if (commands->computedSpeedSetpoint > 70) commands->computedSpeedSetpoint = 70;

		   lastSpeed = commands->computedSpeedSetpoint;

		   break;
	    }

        case 2:  // Both vectors detected, calculate direct center
		{
			float MidpointX0 = (vecRight.x0 + vecLeft.x0) / 2.0;
			float MidpointX1 = (vecRight.x1 + vecLeft.x1) / 2.0;
			float MidpointY0 = (vecRight.y0 + vecLeft.y0) / 2.0;
			float MidpointY1 = (vecRight.y1 + vecLeft.y1) / 2.0;

			float new_x = (MidpointX1 - MidpointX0) / frameWidth;
			float new_y = (MidpointY1 - MidpointY0) / frameHeight;

			double slope = new_y == 0 ? INFINITY : new_x / new_y;
			// Normalize the slope to fit within the range of [-1, 1]
			slope = fmax(fmin(slope, 1), -1); // Clamping the value to [-1, 1] if out of bounds
			static float kp = 1.5f;
			// Map gradient to steering setpoint range [0, 100]
			commands->computedSteerSetpoint = (uint8_t)(50 - kp*slope * 50);

			// Adjust speed based on steer: more deviation from 50, lower the speed
			float steer_deviation = fabs(50.0f - (float)commands->computedSteerSetpoint);
			commands->computedSpeedSetpoint = (uint8_t)(70 - (steer_deviation / 50.0f) * (70 - 50));

			// Ensure the speed is within the allowable range
			if (commands->computedSpeedSetpoint > 70) commands->computedSpeedSetpoint = 70;
			lastSpeed = commands->computedSpeedSetpoint;

			break;
		}
    }
    if(commands->computedSpeedSetpoint < 40){
    	commands->computedSpeedSetpoint = 40;
    	lastSpeed = commands->computedSpeedSetpoint;
    }


lastSteer = commands->computedSteerSetpoint;

  //  lastSpeed = commands->computedSpeedSetpoint;
}

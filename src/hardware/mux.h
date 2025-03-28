/**
 * @file mux.h
 * @brief Header file for controlling the CD74HC4067 multiplexer.
 *
 * This file provides the function declaration for setting the multiplexer 
 * channel using a 4-bit control signal.
 *
 * @author [Your Name]
 * @date [Date]
 */

 #ifndef MUX_H
 #define MUX_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #include <stdio.h>
 
 /**
  * @brief Sets the multiplexer channel.
  *
  * This function sets the 4 control pins of the CD74HC4067 multiplexer 
  * to select one of the 16 available channels.
  *
  * @param channel The desired multiplexer channel (0-15).
  */
 void set_mux_channel(uint8_t mux, uint8_t channel);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* MUX_H */
 
#ifndef MAIN_VOLTAGE_H
#define MAIN_VOLTAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the ADC (Analog-to-Digital Converter) for voltage measurement.
 * 
 * This function configures the ADC hardware to allow for accurate voltage readings. 
 * It must be called before any ADC operations are performed.
 * 
 * @return int Returns 0 if initialization is successful, or a negative error code on failure.
 */
int init_adc(void);

void store_sample(void);

void attempt_send(void);

void load_samples_from_nvs(void);

void store_sample_nvs(void);

void nvs_debug(void);

void flash_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_VOLTAGE_H*/
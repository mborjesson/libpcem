#ifndef __PCEM_CONFIG_H_
#define __PCEM_CONFIG_H_

#include "pcem.h"

/**
 * \brief A callback for when the video buffer has been updated.
 * 
 * \return A handle (pointer) to the loaded config.
 */
extern void *pcem_config_load(const char *fn);
/**
 * \brief Save the configuration to the specified filename.
 */
extern void pcem_config_save(void *handle, const char *fn);
/**
 * \brief Free up the memory of the configuration. The handle can't be used after this.
 */
extern void pcem_config_free(void *handle);

/**
 * \brief Set a configuration-value.
 * 
 * \param handle Handle to the config
 * \param type Can be either PCEM_CONFIG_TYPE_INT, PCEM_CONFIG_TYPE_FLOAT or PCEM_CONFIG_TYPE_STRING
 * \param section Set to null for the default section
 */
extern void pcem_config_set(void *handle, int type, const char* section, const char* key, void *value);
/**
 * \brief Get a configuration-value.
 * 
 * \param handle Handle to the config
 * \param type Can be either PCEM_CONFIG_TYPE_INT, PCEM_CONFIG_TYPE_FLOAT or PCEM_CONFIG_TYPE_STRING
 * \param section Set to null for the default section
 * \return A non-zero value is returned if a value was found for this section and key. It is converted to the specified type.
 */
extern int pcem_config_get(void *handle, int type, const char* section, const char* key, void *value);

/**
 * \brief Initialize the simple configuration-handler with the specified global- and machine-configuration.
 * 
 * \param read_only If non-zero the saving will be disabled.
 * \return 0 if the handler was successfully initialized, 1 if the global configuration could not be loaded or 2 if the machine configuration could not be loaded.
 */
extern int pcem_config_simple_init(const char *global_config, const char *machine_config, int read_only);
/**
 * \brief Close the simple configuration-handler and free up memory.
 */
extern void pcem_config_simple_close();

#endif /* __PCEM_CONFIG_H_ */
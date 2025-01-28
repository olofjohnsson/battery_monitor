#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "application/application.h"

/**
 * @brief Main entry point for the application.
 * 
 * This function serves as the starting point for the program execution. 
 * It initializes necessary system components and then calls the `run_application()` 
 * function to start the main application logic.
 * 
 * @return int Returns 0 if the program completes successfully. 
 *         (Note: This function is expected to be the entry point for Zephyr RTOS, 
 *         which typically doesn't return.)
 */
int main(void)
{
    run_application();
}

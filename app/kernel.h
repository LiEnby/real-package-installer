int module_is_running(char* module_name);
void load_kernel_modules();

int disable_power_off();
int enable_power_off();

void lock_shell();
void unlock_shell();
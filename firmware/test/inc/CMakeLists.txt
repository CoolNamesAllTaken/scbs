target_sources(scbs_test PRIVATE
    pico.h
    hardware/address_mapped.h    
    hardware/gpio.h
    hardware/platform_defs.h
    hardware/timer.h
    hardware/uart.h
    hardware/regs/addressmap.h
    hardware/regs/dreq.h
    hardware/regs/uart.h
    hardware/structs/uart.h
    pico/stdlib.h
    pico/stdio.h
    pico/time.h
)
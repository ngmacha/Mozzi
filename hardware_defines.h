#ifndef HARDWARE_DEFINES_H_
#define HARDWARE_DEFINES_H_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

/* Macros to tell apart the supported platforms. The advantages of using these are, rather than the underlying defines
- Easier to read and write
- Compiler protection against typos
- Easy to extend for new but compatible boards */

#define IS_AVR() (defined(__AVR__))  // "Classic" Arduino boards
#define IS_SAMD21() (defined(ARDUINO_ARCH_SAMD))
#define IS_TEENSY3() (defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__MKL26Z64__) )  // 32bit arm-based Teensy
#define IS_STM32() (defined(__arm__) && !IS_TEENSY3() && !IS_SAMD21())  // STM32 boards (note that only the maple based core is supported at this time. If another cores is to be supported in the future, this define should be split.
#define IS_ESP8266() (defined(ESP8266))

#if !(IS_AVR() || IS_TEENSY3() || IS_STM32() || IS_ESP8266() || IS_SAMD21())
#error Your hardware is not supported by Mozzi or not recognized. Edit hardware_defines.h to proceed.
#endif

// Hardware detail defines
#if IS_STM32()
#define NUM_ANALOG_INPUTS 16  // probably wrong, but mostly needed to allocate an array of readings
#elif IS_ESP8266()
#define NUM_ANALOG_INPUTS 1
#endif

#if IS_ESP8266()
#define PGMSPACE_INCLUDE_H <Arduino.h>  // dummy because not needed
template<typename T> inline T FLASH_OR_RAM_READ(T* address) {
    return (T) (*address);
}
#define CONSTTABLE_STORAGE(X) const X
#else
#define PGMSPACE_INCLUDE_H <avr/pgmspace.h>
// work around missing std::is_const
template<typename T> inline bool mozzi_is_const_pointer(T* x) { return false; }
template<typename T> inline bool mozzi_is_const_pointer(const T* x) { return true; }
/** @ingroup core
 *  Helper fucntion to FLASH_OR_RAM_READ. You do not want to call this, directly. */
template<typename T> inline T mozzi_pgm_read_wrapper(const T* address) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Data type not supported");
    switch (sizeof(T)) {
        case 1: return (T) pgm_read_byte_near(address);
        case 2: return (T) pgm_read_word_near(address);
        case 4: return (T) pgm_read_dword_near(address);
    }
    // case 8: AVR-libc does not provide a read function for this, so we combine two 32-bit reads. TODO: is this MSB/LSB safe?
    return (T) (pgm_read_dword_near(address) | (uint64_t) pgm_read_dword_near(((byte*) address) + 4) << 32);
}
template<> inline float mozzi_pgm_read_wrapper(const float* address) {
    return pgm_read_float_near(address);
}
template<> inline double mozzi_pgm_read_wrapper(const double* address) {
    static_assert(sizeof(uint64_t) == sizeof(double) || sizeof(float) == sizeof(double), "Reading double from pgmspace memory not supported on this architecture");
    if (sizeof(double) == sizeof(uint64_t)) {
        union u { uint64_t i; double d; };
        return u{mozzi_pgm_read_wrapper((uint64_t*) address)}.d;
    }
    return pgm_read_float_near(address);
}
/** @ingroup core
 *  Read a value from flash or RAM. The specified address is read from flash, if T is const, _and_ const
 *  tables are stored in flash on this platform (i.e. not on ESP8266). It is read from RAM, if T is not-const
 *  or tables are always stored in RAM on this platform. @see CONSTTABLE_STORAGE . */
template<typename T> inline T FLASH_OR_RAM_READ(T* address) {
    if(mozzi_is_const_pointer(address)) {
        return mozzi_pgm_read_wrapper(address);
    }
    return (T) *address;
}
/** @ingroup core
 *  Declare a variable such that it will be stored in flash memory, instead of RAM, on platforms where this
 *  is reasonably possible (i.e. not on ESP8266, where random location flash memory access is too slow).
 *  To read the variable in a cross-platform compatible way, use FLASH_OR_RAM_READ(). */
#define CONSTTABLE_STORAGE(X) const X __attribute__((section(".progmem.data")))
#endif

#endif /* HARDWARE_DEFINES_H_ */

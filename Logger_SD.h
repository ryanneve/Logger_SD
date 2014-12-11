/* Manage LimnoTerra SD card files.
  Created by Ryan Neve October 2, 2013
  PUT LICENSE HERE
*/

#ifndef _LOGGER_SD_
#define _LOGGER_SD_
#include <arduino.h>
//#include <SD.h>
#include <SdFat.h>
#include <RTClib.h>      // RTC Clock



/*-----( Defenitions )-----*/  
//#define HS_ERROR_LOG_FILE "ERR_LOG.CSV"
// Log levels
#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3
#define CRITICAL 4

#define LOGGER_BUFFER_SIZE 200
#define SAMPLE_BUFFER_SIZE 200

//const char CONFIG_FILE[] = "config.txt";

/*-----( Declare CONSTANTS )-----*/  
#if (defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)) // SD card chip select. 10 on small boards, 53 on MEGA
const uint8_t SD_CHIP_SELECT = 53;
#else
const uint8_t SD_CHIP_SELECT = 10;
#endif
const char LOG_HEADER[] = "Log Level, Time, Message\r\n";
/*----------( Forward Declarations (?) )----------*/
void getDate(char* date_str);
void getTime(char* time_str);
/*-----( Declare function protoTypes )-----*/ 

class Logger_SD {
  public:
    static Logger_SD* Instance();
    bool        initializeSD(const uint8_t, const uint8_t disable_chip_select);
    void        setLogDT(DateTime* dt);
    void        initializeLog(const char * log_file_name, bool log_debug);
    void        initializeConfig(const char * config_file_name);
    int32_t     getConfig(char * tag);
    void        closeConfig();
    void        initializeSample(const char * header);
    void        setSampleFile(const char * sample_file_name);
    void        saveSample(char * sample_arr,uint8_t sample_len);
    void        msgL(uint8_t logLevel, const char *format, ... );
    void        msgL(uint8_t logLevel,const __FlashStringHelper *format, ... );
    //void        setLogTime(char * time_str, char * date_str);
    void        setDebug(bool debug);
  private:
    Logger_SD(){};
    Logger_SD(Logger_SD const&){};
    Logger_SD& operator=(Logger_SD const&){};
    static Logger_SD* m_pInstance;
    void        _sd_msg();
    void        _msg(bool serial_out, bool sd_out,const char *format, va_list args);
    void        _msg(bool serial_out, bool sd_out,const __FlashStringHelper *format, va_list args);
    void        _write_buffer(uint8_t buffer_length);
    
    uint8_t     _chip_select;
    uint8_t     _disable_chip_select;
    uint8_t     _log_level;
    bool        _log_debug; // log debug messages to SD
    char        _logger_buf[LOGGER_BUFFER_SIZE];
    uint8_t     _logger_buf_length;
    char        _sample_buf[SAMPLE_BUFFER_SIZE];
    uint8_t     _sample_buf_length;
    
    char        _sample_file_name[13];
    SdFile        _sample_file;
    char        _log_file_name[13]; 
    SdFile        _log_file;
    DateTime    *_dt;
    bool        _dt_set;
    bool        _debug;
    
    
    SdFile _config_file;
};

uint32_t compare_tags(char * tag,char * found, bool debug);

#endif
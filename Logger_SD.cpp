/*
Functions relating to SD and logging

Usage:


  Logger_SD::Instance()->initializeSD(SD_CHIP_SELECT); // Do once
  
  Logger_SD::Instance()->initializeLog(LOG_FILE,false); // Do once
  Logger_SD::Instance()->setSampleFile(SCHED1_FILE);  // > once only if multiple log files
  Logger_SD::Instance()->initializeSample(SCHED1_HEADER); // Once per log file.
  
  Logger_SD::Instance()->msgL(DEBUG,"Synchronizing with clock."); // Typical log message
  Line 188:   Logger_SD::Instance()->saveSample(log_output,log_idx); // Saves char * log_output of size log_idx
  
  
*/

#include <arduino.h>
#include <SdFat.h>
#include <SPI.h>
#include <Logger_SD.h>

// Global static pointer used to ensure a single instance of the class.
Logger_SD* Logger_SD::m_pInstance = NULL;
SdFat SD;

// Set DISABLE_CHIP_SELECT to disable a second SPI device.
// For example, with the RTC, set DISABLE_CHIP_SELECT
// to 49 to disable the RTC.
const int8_t DISABLE_CHIP_SELECT = 49; //RTC

// Test with reduced SPI speed for breadboards.
// Change spiSpeed to SPI_FULL_SPEED for better performance
// Use SPI_QUARTER_SPEED for even slower SPI bus speed
const uint8_t spiSpeed = SPI_HALF_SPEED;


/** This function is called to create an instance of the class.
    Calling the constructor publicly is not allowed. The constructor
    is private and is only called by this Instance function.
*/

Logger_SD* Logger_SD::Instance() {
  if (!m_pInstance)
    m_pInstance = new Logger_SD;
  return m_pInstance;
}

bool Logger_SD::initializeSD(const uint8_t chip_select, const uint8_t disable_chip_select) {
  _debug = false;
  _chip_select = chip_select;
  _disable_chip_select = disable_chip_select;
  Serial.println("Initializing SD card with SPI pins:");
  Serial.print("MOSI = "); Serial.println(MOSI);
  Serial.print("MISO = "); Serial.println(MISO);
  Serial.print("SCK = ");  Serial.println(SCK);
  pinMode(_chip_select,OUTPUT);
  pinMode(10,OUTPUT);
  if ( _disable_chip_select > 0 ) {
    pinMode(_disable_chip_select,OUTPUT);
    digitalWrite(_disable_chip_select,HIGH);
  }
  if (!SD.begin(_chip_select)) {
    msgL(CRITICAL,F("Card failed, or not present."));
    return 0;
  }
  Serial.println("SD Card initialized");
  return 1;
}

void Logger_SD::initializeSample(const char * header){
  bool new_file = !SD.exists(_sample_file_name);
  if ( new_file ) msgL(INFO,F("%s will be created."),_sample_file_name);
  else  msgL(DEBUG,F("%s opened."),_sample_file_name);
  _sample_file.open(_sample_file_name,(O_WRITE | O_APPEND | O_CREAT)); // Open file for WRITING and APPENDING
  _sample_file.print(header);
  _sample_file.close();  
}

void Logger_SD::setSampleFile(const char * sample_file_name){
  strncpy(_sample_file_name,sample_file_name,12) ; _sample_file_name[12] = '\0';
  Logger_SD::Instance()->msgL(DEBUG,F("%s is current sample file."),_sample_file_name);
}
void Logger_SD::saveSample(char * sample_arr,uint8_t sample_len){
  Serial.print(F("got array [")); Serial.print(sample_arr); Serial.print(F("] of length "));
  Serial.println(sample_len);
  //msgL(DEBUG,F("%s got array [%s] of size %d "),_sample_file_name, sample_arr,sample_len);
  if ( sample_len >= SAMPLE_BUFFER_SIZE ) {
    sample_len = SAMPLE_BUFFER_SIZE - 1;
    msgL(WARN,F("Array larger than max allowed. Truncating to "),sample_len);
  }
  strncpy(_sample_buf,sample_arr,sample_len);
  _sample_buf[sample_len] = '\0';
  //File _sample_file = SD.open(_sample_file_name,FILE_WRITE); // Open file for WRITING
  
  Serial.print(_sample_file_name) ; Serial.println(F(" opened for writing."));
  Serial.flush();
  _sample_file.open(_sample_file_name,(O_WRITE | O_APPEND | O_CREAT)); 
  //msgL(DEBUG,F("%s opened for writing."),_sample_file_name);
  //_sample_file.write(_sample_buf,sample_len);_sample_file.write(13); _sample_file.write(10);
  uint8_t write_size = _sample_file.write(sample_arr,sample_len);_sample_file.write(13); _sample_file.write(10);
  _sample_file.close();
  Serial.print(_sample_file_name) ; Serial.print(F(" closed after writing string of size "));
  Serial.println(write_size);
  //msgL(DEBUG,F("%s closed after writing string of size %d"),_sample_file_name,sample_len);
  //msgL(DEBUG,F("%s closed after writing string of size %d"),_sample_file_name,sample_len);
}
void Logger_SD::_write_buffer(uint8_t buffer_length){
  // Perhaps we should check the String to see if it has the right number of commas?

}


/*-----( Message Logging  )-----*/
/*  The log_msgL function must be called with at least two arguments
      logger.msgL(<log_level>,<message_string as in printf>[,<printf arg1>[,<printf arg2>[,...]]])
      The message_string argument can be surrounded by the F() macro to save RAM if desired.
    Typical Log Levels:
      4 - critical
      3 - error
      2 - warning
      1 - info
      0 - debug
*/
void Logger_SD::initializeLog(const char* log_file_name, bool log_debug){
  _log_debug = log_debug;
  strncpy(_log_file_name,log_file_name,12) ; _log_file_name[12] = '\0';
  bool new_file = !SD.exists(_log_file_name);
  if ( new_file )msgL(INFO,F("New log file %s will be created."),_log_file_name);
  _log_file.open(_log_file_name,(O_WRITE | O_APPEND | O_CREAT)); 
  if ( new_file ) _log_file.print(LOG_HEADER);
  else {
    msgL(DEBUG,F("Opened log file %s of size %u"),_log_file_name,_log_file.fileSize());
    for ( uint8_t i = 0 ; i <= 80 ; i++ ) { _log_file.write('_'); }
    _log_file.write(13);
    _log_file.write(10);
  }
  _log_file.close();

}
void Logger_SD::_msg(  bool serial_out, bool sd_out, const char *format, va_list args){
  vsnprintf(_logger_buf,LOGGER_BUFFER_SIZE, format, args);
  _logger_buf_length =  min(strlen(_logger_buf),LOGGER_BUFFER_SIZE);
  if ( serial_out  ) Serial.println(_logger_buf);
  if ( sd_out ) _sd_msg();
}
void Logger_SD::_msg(  bool serial_out, bool sd_out, const __FlashStringHelper *format, va_list args){
#ifdef __AVR__
  vsnprintf_P(_logger_buf, LOGGER_BUFFER_SIZE, (const char *)format, args); // progmem for AVR
#else
  vsnprintf(_logger_buf, LOGGER_BUFFER_SIZE, (const char *)format, args); // for the rest of the world
#endif
  _logger_buf_length =  min(strlen(_logger_buf),LOGGER_BUFFER_SIZE);
  if ( serial_out ) Serial.println(_logger_buf);
  if ( sd_out ) _sd_msg();
}

void Logger_SD::msgL( uint8_t log_level, const char *format, ... ){ // no F()
  va_list args;
  va_start (args, format );
  _log_level = log_level;
  bool log_sd = true;
  if ( _log_level == 0 && _log_debug == false ) log_sd = false;
  _msg(true,log_sd,format,args);
  va_end(args);
}
void Logger_SD::msgL( uint8_t log_level,const __FlashStringHelper *format, ... ){ // with F()
  // Can we call the other version instead?
  va_list args;
  va_start(args, format );
  _log_level = log_level;
  bool log_sd = true;
  if ( _log_level == 0 && _log_debug == false ) log_sd = false;
  _msg(true,log_sd,format,args);
  va_end(args);
}
void Logger_SD::_sd_msg() {
  /* Save message to _log_file_name with other data.
  */
  
  //_log_file = SD.open(_log_file_name,FILE_WRITE); // Open file for WRITING
  
  _log_file.open(_log_file_name,(O_WRITE | O_APPEND | O_CREAT)); 
  if ( _debug ) {
    Serial.print("Opened log file "); Serial.print(_log_file_name);
    Serial.print(" at "); Serial.print(_log_file.curPosition());
  }
  _log_file.write(_log_level + '0');   _log_file.write(',');
  char time_field[] = "YYYY/MM/DD HH:mm:ss,";
  sprintf(time_field,"%04u/%02u/%02u %02u:%02u:%02u,",
          (*_dt).year(), (*_dt).month(), (*_dt).day(),
          (*_dt).hour(), (*_dt).minute(), (*_dt).second());
  uint8_t bytes_written = _log_file.write(time_field,20);
  //_logger_buf[_logger_buf_length] = 0; // IS THIS NEEDED?
  bytes_written += _log_file.write(_logger_buf,_logger_buf_length);        _log_file.write(13);_log_file.write(10);
  if ( _debug) {
    Serial.print(" on ");              Serial.println(time_field);
    Serial.print("Closing log file "); Serial.print(  _log_file_name);
    Serial.print(" wrote ");           Serial.print(  _logger_buf);
    Serial.print(" at ");              Serial.print(  _log_file.curPosition());
    Serial.print(" after writing ");   Serial.print(  bytes_written);
    Serial.print(" of ");              Serial.print(  _logger_buf_length);
    Serial.println(" bytes");
  }
  _log_file.close();
  _dt_set = false;
}

void Logger_SD::setLogDT(DateTime* dt) {
  _dt = dt;
  _dt_set = true;
  //Serial.println("Logger_SD._dt set to:");
  //Serial.print("YYYY/") ;  Serial.print((*_dt).year());
  //Serial.print("MM/") ;    Serial.print((*_dt).month());
  //Serial.print("DD  ") ;   Serial.print((*_dt).day());
  //Serial.print("HH:") ;    Serial.print((*_dt).hour());
  //Serial.print("mm:") ;    Serial.print((*_dt).minute());
  //Serial.print("ss") ;     Serial.println((*_dt).second());
  msgL(INFO,F("Logger_SD._dt set to: %d/%d/%d %d:%d:%d"),(*_dt).year(),(*_dt).month(),(*_dt).day(),(*_dt).hour(),(*_dt).minute(),(*_dt).second());
}

void Logger_SD::initializeConfig(const char * config_file_name){
    _config_file.open(config_file_name,(O_READ)); // Open file 
    msgL(INFO,F("Opened config file %s of size %u"),config_file_name,_config_file.fileSize());
}

void Logger_SD:: setDebug( bool debug){
  _debug = debug;
  Serial.print("SD debugging");
  if ( _debug ) Serial.println(" on.");
  else Serial.println(" off.");
}

int32_t Logger_SD:: getConfig(char * tag){
  // return value of tag in config file
  //file is in format:
  // tag=int\r\n
  // Go to beginning of file
    _config_file.seekSet(0);
    int16_t byte_read = 0;
    bool reading_tag = true;
    bool tag_found = false;
    bool find_EOL = false;
    char tag_read[40];
    uint8_t tag_read_idx = 0;
    char val_read[40];
    uint8_t val_read_idx = 0;
    uint32_t file_size = _config_file.fileSize();
    if ( _debug ) Serial.print("Looking for TAG: "); Serial.println(tag);
    for ( uint32_t pos = _config_file.curPosition(); pos <= file_size ; pos++ ) {
      byte_read = _config_file.read();
      if ( byte_read == -1 ) return -1; // EOF. should have found something by now.
      if ( byte_read == 13 ) { // CR
        if ( find_EOL ) {
          find_EOL = false; // found it
          if (_debug) Serial.println(" DONE");
        }
        if ( tag_found ) {// Need to process and return val_read as an int32_t
          val_read[val_read_idx] = 0; // null terminate string
          // CONVERT STRING TO INTEGER but first see if it is in Hex notation.
		  if ( val_read[0] == '0' && ( val_read[1] == 'x' || val_read[1] == 'X' ) ) {
		    return (int32_t) strtol(val_read,NULL,16);
		  }
          else return (int32_t) atoi(val_read);
        }  
        else { // Start over on a new line
          tag_read_idx = 0;
          val_read_idx = 0;
          reading_tag = true;
          continue; // keep going if we're not looking for a value.
        }
      }
      if ( find_EOL ) continue; // Looking for <CR> at end of line or EOF.
      if ( byte_read == '/' ) { // Ignore anything after a slash as a comment
        if (_debug) Serial.print("COMMENT");
        reading_tag = false;
        find_EOL = true;
        continue;
      }
      if ( byte_read == ' ' ) continue; // ignore spaces
      if ( byte_read == 10 ) continue; // ignore LF
      if ( byte_read == '=' ) { // end of tag
        if (_debug) { Serial.print("Comparing "); Serial.print(tag_read); Serial.print(" of length "); Serial.println(tag_read_idx); }
        if ( compare_tags(tag,tag_read,_debug)){
          reading_tag = false; // This means we found tag and are now reading value
          tag_found = true;
        }
        else { // got a tag, but not what we were looking for.
          find_EOL = true;
          tag_read[0] = 0;
          tag_read_idx = 0;
        }
        continue;
      }
      // Should be A-Z or 0-9 from here on.
      if ( tag_found == false ) {
        if ( reading_tag ) {
          tag_read[tag_read_idx] = byte_read;
          tag_read_idx++;
          tag_read[tag_read_idx] = 0; // null terminate string
          continue;
        }
        // Should not be an else
      }
      else { // reading value
        val_read[val_read_idx] = byte_read;
        val_read_idx++;
        continue;
      }
    }
	return -1; // Shouldn't get here
  // process one line at a time looking for tag match
}

void Logger_SD::closeConfig() {
  _config_file.close(); // CLose file
}

uint32_t compare_tags(char * tag,char * found, bool debug) {
  // retunrs a 0 if tags don't match, returns tag length if they do.
  for ( uint32_t i = 0 ; i < 200 ; i++ ) {
    if (debug) {
      Serial.print("Comparing "); Serial.print(char(tag[i])); Serial.print(" to "); Serial.print(char(found[i])); Serial.print("  ");
    }
    if ( tag[i] != found[i] ){
      if (debug) Serial.println(" NO MATCH");
      return 0;
    }
    // they match
    if ( tag[i] == 0) {
      if (debug) Serial.println(" FOUND MATCH");
      return i; // Found null terminator
    }
    if (debug) Serial.println();
  }
  return 0;
}

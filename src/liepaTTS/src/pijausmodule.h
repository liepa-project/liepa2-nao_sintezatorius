#pragma once
#ifndef PIJAUS_MODULE_H
# define PIJAUS_MODULE_H

#include <qi/anyobject.hpp>
#include <qi/applicationsession.hpp>
#include "LithUSS.h"

class LiepaTTS
{
public:
  LiepaTTS(qi::SessionPtr session);
  virtual ~LiepaTTS();

  /**
   * Overloading ALModule::init().
   * This is called right after the module has been loaded
   */
  virtual void init();

  // add all your bind methods
  void printHello();
  void printWord(char *toSay);
  bool returnTrue();

  void sayHello();
  void sayTextWithSpeed(char *toSay, int speed);
  void sayTextWithSpeedToFile(char *toSay, int speed, bool sayToOutputFile, const std::string& outputFileName);  
  void sayText(char *toSay); 
  void say(char *toSay);
  void sayToFile(char *toSay, const std::string& fileName);
  int sayTextAndReturnLength(char *toSay); 
  std::vector<std::string> getAvailableVoices();
  float getParameter(const std::string& parameter);
  std::string getVoice();
  float getVolume();
  void resetSpeed();
  void setParameter(const std::string& parameter, const float& value);
  void setVoice(const std::string& voiceID);
  void setVolume(const float& value);
  void stopAll();

private:
  qi::SessionPtr _session;
};
QI_REGISTER_MT_OBJECT(LiepaTTS, printHello, printWord, returnTrue, sayHello, sayTextWithSpeed, sayTextWithSpeedToFile, sayText, say, sayToFile, sayTextAndReturnLength, getAvailableVoices, getParameter, getVoice, getVolume, resetSpeed, setParameter, setVoice, setVolume, stopAll);
#endif // PIJAUS_MODULE_H

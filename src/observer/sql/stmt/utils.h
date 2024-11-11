#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string.h>


static void match(int year, int month, int day, const std::string monthEnglish[],
            std::unordered_map<int, std::string> dayEnglish,char *substr, std::stringstream &ss)
{
    if (strcmp(substr, "%Y") == 0) {
        ss << year;
      } else if (strcmp(substr, "%y") == 0) {
        int tmp = year % 100;
        if (tmp < 10) ss << 0;
        ss << (tmp);
      } else if (strcmp(substr, "%M") == 0) {
        ss << monthEnglish[month];
      } else if (strcmp(substr, "%m") == 0) {
        if (month < 10) ss << 0;
        ss << month;
      } else if (strcmp(substr, "%D") == 0) {
        ss << dayEnglish[day];
      } else if (strcmp(substr, "%d") == 0) {
        if (day < 10) ss << 0;
        ss << day;
      } else if (strcmp(substr, "%z") == 0) {
        ss << "z";
      } else if (strcmp(substr, "%Z") == 0) {
        ss << "Z";
      } else if (strcmp(substr, "%n") == 0) {
        ss << "n";
      } else if (strcmp(substr, "%N") == 0) {
        ss << "N";
      }
}


static std::string formatDate(const char *raw_data,const char *format)
{
  int year=0, month=0, day=0;
  int i;
  for (i=0; i<10&&raw_data[i]!='-';i++) {
    year=year*10+(raw_data[i]-'0');
  }
  i++;
  for(;i<10&&raw_data[i]!='-';i++){
    month=month*10+(raw_data[i]-'0');
  }
  i++;
  for(;i<10&&raw_data[i]!='\0';i++){
    day=day*10+(raw_data[i]-'0');
  }

  std::stringstream ss;
  const std::string monthEnglish[] = {"","January", "February", "March", "April", "May", "June",
                              "July", "August", "September", "October", "November", "December"};
  std::unordered_map<int, std::string> dayEnglish;
  dayEnglish[1] = "1st";
  dayEnglish[2] = "2nd";
  dayEnglish[3] = "3rd";
  dayEnglish[4] = "4th";
  dayEnglish[5] = "5th";
  dayEnglish[6] = "6th";
  dayEnglish[7] = "7th";
  dayEnglish[8] = "8th";
  dayEnglish[9] = "9th";
  dayEnglish[10] = "10th";
  dayEnglish[11] = "11th";
  dayEnglish[12] = "12th";
  dayEnglish[13] = "13th";
  dayEnglish[14] = "14th";
  dayEnglish[15] = "15th";
  dayEnglish[16] = "16th";
  dayEnglish[17] = "17th";
  dayEnglish[18] = "18th";
  dayEnglish[19] = "19th";
  dayEnglish[20] = "20th";
  dayEnglish[21] = "21st";
  dayEnglish[22] = "22nd";
  dayEnglish[23] = "23rd";
  dayEnglish[24] = "24th";
  dayEnglish[25] = "25th";
  dayEnglish[26] = "26th";
  dayEnglish[27] = "27th";
  dayEnglish[28] = "28th";
  dayEnglish[29] = "29th";
  dayEnglish[30] = "30th";
  dayEnglish[31] = "31st";

  int length = strlen(format);
  int left = 0;
  for (int index=0; index<length; ++index) {
    if (format[index] == '/' || format[index] == ',' || format[index] == '.') {
      char substr[3];
      strncpy(substr, format+left, index-left);
      match(year, month, day, monthEnglish, dayEnglish, substr, ss);

      ss << format[index];
      left = index + 1;     
    }
  }
  if (length >= 2) {
    // 处理最后一个%
    char substr[3];
    strncpy(substr, format+left, length-left);
    match(year, month, day, monthEnglish, dayEnglish, substr, ss);
  }

  return ss.str();
}
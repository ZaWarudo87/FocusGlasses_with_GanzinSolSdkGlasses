# FocusGlasses_with_GanzinSolSdkGlasses
The eye-tracking glasses which can detect what you're gazing and supervise you study.

## Table of Contents

[Installation Instructions](#installation-instructions)<br/>
[Operating Instructions](#operating-instructions)<br/>
[Copyright and Licensing Information](#copyright-and-licensing-information)<br/>
[Contact Information for the Distributor](#contact-information-for-the-distributor)<br/>
[A List of Known Bugs](#a-list-of-known-bugs)<br/>
[Troubleshooting Instructions](#troubleshooting-instructions)<br/>
[Credits and Acknowledgments](#credits-and-acknowledgments)<br/>
[A Changelog](#a-changelog)

## Installation Instructions 
To start, you need to download all the files and keep them in the same folder.

### Sol SDK
Make sure you have a Sol SDK Glasses and its App.
Just start it and get its IP.

### Backend Build by Flask
1. Install python (recommended version: 3.10 or newer)
2. Install all the modules (flask, flask_cors, opencv-python, numpy)
```bash
pip install flask flask_cors opencv-python numpy
```
3. Open `detect_object_backend.py`, and type your Sol SDK's IP in address (#line 54). Remember to save the file
4. Click `start.bat` and memorize your PC's IP

### Glasses Module
1. How to make your own glasses module
  ![IMG_0715](https://github.com/user-attachments/assets/704c1eea-bc33-4c9d-bef5-c8b8d7cf9e2f)
2. Install all the libraries (esp32:esp32@2.0.17, WebSockets@2.5.1, ArduinoJson@7.0.4, ESP8266 and ESP32 OLED driver for SSD1306 displays@4.6.1)
```bash
.\arduino-cli_windows config set directories.user ([System.String]::Concat([Environment]::GetFolderPath("MyDocuments"), "\Arduino"))
# .\arduino-cli_windows config set directories.user "C:\Users\user\Downloads\Arduino"
.\arduino-cli_windows core install esp32:esp32@2.0.17
.\arduino-cli_windows lib install WebSockets@2.5.1 ArduinoJson@7.0.4 "ESP8266 and ESP32 OLED driver for SSD1306 displays@4.6.1"
```
3. Open focus_glassees.ino with Arduino IDE and type your wifi name and password (#line 17, 18)
4. Type your PC's IP (#line 21, 95)
5. Upload your code on the ESP32 of your glasses module

## Operating Instructions
1. Put on your glasses module on Sol SDK
2. Start Sol SDK: just start it
3. Build the Backend: open start.bat
4. Start your glasses module and make sure your it connected to your wifi
5. If you want to acquire your learning status, you can enter your glasses module's IP and connect to the website

## Features
1. Determining the object type you're looking at. If you're looking at it over 10s, it will show you the object type
2. If your eyes "succade" over 20s, the glasses will ask you question to make sure that you feel ok
3. If you're still focusing on your job, it will enter "study-mode", which encourages you and make sure you are focusing on your job
4. The website built by glasses module can supervise your learning/working status

## Copyright and Licensing Information

We use the MIT license.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Contact Information for the Distributor
* 洪翠憶 (k112700044.mg12@nycu.edu.tw)
* 黃浩誠 (jerry138138.ee13@nycu.edu.tw) - my boyfriend ><

## A List of Known Bugs
Because of technical problems, we cannot judge what the users are exactly doing. The judging algorithm is measuring user's sight to decide whether the user is focuing on something.
So please use this program with honesty XD

## Troubleshooting Instructions

### Q1: Complie failure
> *A1: Make sure all the libraries you've included and your python version is new enough. * 
\-
### Q2: Having difficulty soldering your own glasses module
> *A2: Find a lovely little cat as your girlfriend so that she will help you <3 * 
\-

## Credits and Acknowledgments
* OpenCV 
* Yolo model (to judge objects)
* Modules offered by Ganzin
### Refenrce Web
* [yolov3](https://github.com/ultralytics/yolov3)
* [OpenCV](https://opencv.org/)
* [oled](https://github.com/ThingPulse/esp8266-oled-ssd1306)

### Special Thanks
* Ganzin Tech.
* my bf :sparkling_heart:
* Our parents, teachers, people who support us to keep going...
* All the Users :sparkling_heart:


## A Changelog

* **1.0.0**<br/>
*2024.10.27*<br/>
\- Finish all files<br/>

###### tags: `Ganzin_Sol_SDK` `ESP32` `Focus_Glasses` `Arduino-C` `Python` `HTML` `Wearable_Device` `Glasses`

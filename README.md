bluemix-ti-board-starter
================================================================================

This [project](https://github.com/IBM-Bluemix/bluemix-ti-board-starter) contains an [Energia](http://energia.nu/faqs/) sketch that connects [SimpleLink Wi-Fi CC3200 LaunchPads](http://www.ti.com/tool/cc3200-launchxl) from Texas Instruments with the [Internet of Things](https://console.ng.bluemix.net/?ace_base=true#/store/serviceOfferingGuid=8e3a9040-7ce8-4022-a36b-47f836d2b83e&fromCatalog=true) service in [IBM Bluemix](https://bluemix.net). This allows receiving sensor data from the device as well as sending commands to the device.

![alt text](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/energia.png "energia")
Author: Mark VanderWiele


Setup of the Bluemix Application
================================================================================

In order to interact with a board from Bluemix a Bluemix application is used in combination with the IBM Internet of Things Foundation service. 

Log in to Bluemix and create a new application, e.g. [my-ti-app](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/bluemixcreateapp1.png), based on the [Internet of Things Foundation Starter](https://console.ng.bluemix.net/?ace_base=true#/store/appType=web&cloudOEPaneId=store&appTemplateGuid=iot-template&fromCatalog=true). Additionally add the [Internet of Things](https://console.ng.bluemix.net/?ace_base=true#/store/serviceOfferingGuid=8e3a9040-7ce8-4022-a36b-47f836d2b83e&fromCatalog=true) service to it, see [sample](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/bluemixcreateapp2.png). After this your application dashboard should look like [this](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/bluemixcreateapp3.png).

In the next step you have to register your own device. Open the dashboard of the Internet of Things service and navigate to 'Add Device'. As device type choose 'ti-board' and an unique device id - [screenshot](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/registerdevice1.png). As result you'll get an org id and credentials - [screenshot](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/registerdevice2.png).


Setup of the Energia Sketch
================================================================================

[Download](http://energia.nu/download/) and install the Energia IDE.

You also need to download and install the CC3200 drivers, for example for [Mac OS X](http://energia.nu/files/EnergiaFTDIDrivers2.2.18.zip).

In order to use the current time in the provided sketch you also need to [download](http://www.pjrc.com/teensy/td_libs_Time.html) and install the Time Library into the libraries directory - see [screenshot1](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/installtime1.png) and [screenshot1](https://raw.githubusercontent.com/IBM-Bluemix/bluemix-ti-board-starter/master/images/installtime2.png).

Open the provided sketch TICC3200LP_DevOxx.ino in Energia. Change the board to CC3200 via the menu Tools-Board.

Change the following lines with your own credentials.

- char ssid[] = "YOURSSID";
- char password[] = "YOURNETPASSWORD";
- char organization[] = "YOUR_IOT_FOUNDATON_ORG";
- char typeId[]   = "YOUR_IOT_FOUNDATION_TYPEID";
- char deviceId[] = "YOUR_IOT_FOUNDATION_DEVICEID";
- char authToken[] = "YOUR_IOT_FOUNDATION_AUTHTOKEN_FOR THIS DEVICE";

Create or add any unique Pub/sub topics if necessary, these will get you started with the IOT_foundation.

- char pubtopic[] = "iot-2/evt/status/fmt/json";
- char subTopic[] = "iot-2/cmd/+/fmt/json";

Update the timezone if necessary.


Run the Sample
================================================================================

Connect your board via USB to your machine running Energia, compile the code and upload it to your board.

When opening the Internet of Things dashboard of your application in Bluemix now you can see the received data when expanding the section under your registered device.
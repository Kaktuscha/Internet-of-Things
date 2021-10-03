# Internet-of-Things

## Coursework task
To develop an IOT prototype device using the Aston IOT trainer boards. 
To gain access to the premises, an employee must present their RFID card to the device. Upon a
successful scan of the RFID card’s unique identifier, the device will measure the user’s body
temperature. Upon measurement of the temperature, the device will then record this access attempt
and will grant access if their bodily temperature is within an acceptable range.
Each device must connect to the existing WiFi network which exists on-site.
The system must have the following features/functionality:
1. It should check to see if an RFID card has been presented every 100ms. Whilst doing this, the
LCD should display a message on the top line to let users know to scan their cards.
2. Upon being presented with an RFID card, the device should measure the temperature of the user.
For this, assume that the potentiometer output represents the analogue output of an infrared
temperature sensor pointed at the user, with the following response:
Fully anti-clockwise: 25°C.
Fully clockwise: 45°C.
After taking a measurement of the temperature, display the value on the top line of the LCD for 2
seconds.
3. After measuring the temperature, transfer the values of:
a. The RFID tag’s unique identifier.
b. Temperature.
c. The devices’ unique identifier (for localisation purposes). This is a string which can be used
 to identify where the device has been installed.
Over the local network using a webpage and over the internet using MQTT with SSL/TLS. The
webpage should display the last 10 access attempt UIDs and their corresponding temperatures.
4. If the temperature is within a user programmable range, such as 36.5 to 38°C, illuminate the LED
for one second to simulate the door solenoid being allowed to open. Otherwise, do not illuminate the
LED, and inform the user that they must return home.
5. Be able to set the upper and lower bodily temperature limits via MQTT and a HTTP server.
6. Update the bottom line the LCD on start-up with the IP address.
7. Implement a device discovery algorithm, which responds to a UDP broadcast message with the
board’s IP address OR use some form of multicast DNS system such as mDNS.
8. Use a task scheduler where possible to handle the devices various processes after initialisation.
The coursework’s deliverables consist of a technical report to be submitted through Blackboard and
a code submission.

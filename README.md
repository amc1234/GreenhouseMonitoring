# GreenhouseMonitoring
Monitoring system for plants that maintains them in a reasonable state autonomously. Three sensors (soil moisture, ambient light and temperature) are used to monitor an enclosed zone. Two push buttons are implemented to toggle between two zones and the other push button displays either temperature or soil moisture readings for the selected zone. Motors were also integrated to irrigate and ventilate each zone. Irrigation only runs at night and when the moisture is too low based on user set threshold. Ventilation only runs at daylight and when the temperature is too high based on user set threshold through UART (through USB). If the user wishes, the motors can also be controlled through UART by enabling or disabling the motors. This gives the user the option to monitor zones only and adjust irrigation and ventilation as they so please.
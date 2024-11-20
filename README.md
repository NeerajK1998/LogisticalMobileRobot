# LogisticalMobileRobot
Obstacle detection in a mobile robot

Use the same code for all ESP32s but update the CAN ID ranges for each node.
Ensure that each ESP has a unique ID range to avoid message conflicts.
The PC can process messages by their CAN IDs, identifying which ESP or sensor sent the data.


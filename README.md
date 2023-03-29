# LightsaberAntimatter

The LightsaberAntimatter is the third prototype of my special-fx lightsaber design.

The saber consists of two circuit boards, one with an esp-32 that controls leds that run the length of the blade, gathers readings from a gyroscope, manages a single cell battery, communicates with an iphone via bluetooth, and gathers readings from 4 VL53L5CX ToF sensors located on the second circuit board. These ToF sensors are used to detect if and where the lightsaber is hit along the blade, allowing the ability for the lightsaber to flash at a particular spot along the blade, bringing greater interactivity to it's environment and reducing the vfx needed in film production.

The hilt is a custom design, gathering inspiration from the starkiller lightsaber hilt.

Schematics for the PCBS are found in the schematics folder as well as the circuitboards folder and their fabrication files are found in the PCBA file. The DocsAndDiagrams folder is a simplified block diagram of the main circuit board, though it is outdated. The main code of the board and iphone are found in Code/StableSoftware. The .step files and technical drawings for the components of the lightsaber can be found in the CAD file.


![IMG_3598](https://user-images.githubusercontent.com/111181016/228496889-c88ec47f-3bdb-4b64-b0f8-2e0dd6ffbb01.JPG)=250x250

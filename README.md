# Arduino_UNO_home_thermostat
I use this sketch in the Arduino UNO to operate my furnace.  Being connected via USB to my Ubuntu headless firewall, I have remote control of it from over the Internet. Uses an old version (0.0.3) of idDHTLib making it in need of upgrading.  In the past I used a prior version of this sketch in a Leonardo, and it worked that way also, so I wouldn't hesitate to switch back to Leonardo if I ran out of UNOs.  It IS pretty hard never to damage Arduinos when you handle them and develop with them frequently, even being an experienced ET as I am.  For now, however, I'll use the UNO as my thermostat.

The reason I provide a complete copy of idDHTLib instead of just linking to the right version is because that page (https://github.com/niesteszeck/idDHTLib) does not contain the library version I am using in the sketch.  I tried using a newer version, but the compatibility was lost somewhere along the upgrade path for that library and I had too little incentive to deal with it after getting my sketch going.

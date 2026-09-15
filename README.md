# Popping Piggies

An interactive Angry Birds-themed game built as a display feature for GNCTR 2027. 

Inspired by Whack-a-Mole, players throw physical bird plushies at pigs that randomly pop in and out of holes on the vertical wall.


## Game Mechanics

1. The main control loop randomly triggers motor channels and pops out pigs at intervals
3. When a thrown bird strikes a target zone, it interrupts a break beam sensor
4. Sensor triggers send a signal back to the MCU to retract the hit pig, log the hit, and queue the next random popup

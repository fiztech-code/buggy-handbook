# Buggy Handbook
collection of code and instructions for dune buggy project


## Cluster
#### Hardware interface between honda cbr600f4i ECM and yamaha yzf-r6 cluster
cbr outputs all parameters as analog values, yzf is using k-line serial comm for speed, engine coolant temperature, fault indicator this sketch takes analog parameters processes them, then converts to serial message

#### Indicators
- neutral, turn signals, and high beam function as standard bulb indicators. 
- oil indicator on the YZF is  oil level, CBR uses oil pressure. therefore there is a 30sec delay, requires resisters
- fuel level requires resister

#### Speed Mapping
yamaha cluster accepts speed values from 0-47, internally maps these values to 0-299, it takes 6x messages to send speed value

ie.  actual speed is 21kmh, which falls between 18kmh(index:3) and 24kmh(index:4) therefore we need to send 3x messages with speed value:4, then send 3x messages with speed value:3 to get desired result 21 on cluster

#### Schematic
![cluster-interface-schematic.png](cluster-interface-schematic.png)

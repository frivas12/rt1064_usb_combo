# LNR Configurations
The names of these configurations are based on the different setups on LNR stages.

The default LNR configuration is [S\_1mm\_1-1\_500nm.xml](./S_1mm_1-1_500nm.xml).

The files follow the following naming convention.
"<motor>\_<thread>\_<gearbox>\_<encoder>.xml"
"<motor>" is the type of motor attached.

"<thread>" is the type of lead screw attached.
This affects the "CountsPerUnit" tag.

"<gearbox>" is the type of gearbox attached.
This affects the "CountsPerUnit" tag.

"<encoder>" is the type of encoder read head attached.
This affects the "NmPerCount" tag (and thus the "CountsPerUnit" tag).

.output database/schema.sql
.schema

.output database/DeviceFiles.sql
.mode insert DeviceFiles
SELECT * from DeviceFiles;

.output database/Devices.sql
.mode insert Devices
SELECT * from Devices;

.output database/PartNumbers.sql
.mode insert PartNumbers
SELECT * from PartNumbers;

.output database/SlotTypes.sql
.mode insert SlotTypes
SELECT * from SlotTypes;

.output database/StructTypes.sql
.mode insert StructTypes
SELECT * from StructTypes;

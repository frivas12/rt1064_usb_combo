CREATE TABLE IF NOT EXISTS "StructTypes" (
	"StructID"	INTEGER NOT NULL,
	"Description"	TEXT NOT NULL,
	PRIMARY KEY("StructID")
);
CREATE TABLE IF NOT EXISTS "SlotTypes" (
	"SlotType"	INTEGER NOT NULL,
	"Description"	TEXT NOT NULL,
	PRIMARY KEY("SlotType")
);
CREATE TABLE IF NOT EXISTS "Devices" (
	"SlotType"	INTEGER NOT NULL,
	"DeviceID"	INTEGER NOT NULL,
	"Description"	TEXT,
	"IsActive"	INTEGER DEFAULT 1 CHECK("IsActive" BETWEEN 0 AND 1),
	PRIMARY KEY("SlotType","DeviceID"),
	FOREIGN KEY("SlotType") REFERENCES "SlotTypes"("SlotType")
);
CREATE TABLE IF NOT EXISTS "DeviceFiles" (
	"SlotType"	INTEGER NOT NULL,
	"DeviceID"	INTEGER NOT NULL,
	"AttachedSlot"	INTEGER NOT NULL,
	"Path"	TEXT NOT NULL,
	PRIMARY KEY("AttachedSlot","SlotType","DeviceID"),
	FOREIGN KEY("AttachedSlot") REFERENCES "SlotTypes"("SlotType")
	FOREIGN KEY("SlotType","DeviceID") REFERENCES "Devices"("SlotType","DeviceID")
);
CREATE TABLE IF NOT EXISTS "PartNumbers" (
	"PartNumber"	TEXT NOT NULL,
	"UsageID"	INTEGER NOT NULL,
	"SlotType"	INTEGER NOT NULL,
	"DeviceID"	INTEGER NOT NULL,
	"IsActive"	INTEGER NOT NULL DEFAULT 1 CHECK(IsActive between 0 and 1),
	"UsageDescription"	TEXT,
	PRIMARY KEY("PartNumber","UsageID")
	FOREIGN KEY("SlotType", "DeviceID") REFERENCES "Devices"("SlotType", "DeviceID"	)
);

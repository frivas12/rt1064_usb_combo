SELECT Devices.SlotType, Devices.DeviceID, PartNumbers.PartNumber, PartNumbers.UsageID
FROM Devices INNER JOIN PartNumbers ON (Devices.DeviceID = PartNumbers.DeviceID) AND (Devices.SlotType = PartNumbers.SlotType)
WHERE Devices.IsActive = 0 AND PartNumbers.IsActive = 1;

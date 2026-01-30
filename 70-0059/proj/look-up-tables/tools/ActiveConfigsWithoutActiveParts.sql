SELECT Devices.SlotType, Devices.DeviceID
FROM Devices LEFT JOIN
( SELECT *
    FROM PartNumbers
    WHERE PartNumbers.IsActive
) AS ActivePartNumbers
ON (Devices.SlotType = ActivePartNumbers.SlotType) AND (Devices.DeviceID = ActivePartNumbers.DeviceID)
WHERE Devices.IsActive = 1 AND ActivePartNumbers.IsActive = 0;

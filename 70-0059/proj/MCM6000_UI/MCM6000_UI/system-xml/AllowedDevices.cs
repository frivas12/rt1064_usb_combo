using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using System.Windows.Forms;

namespace SystemXMLParser
{
    public class AllowedDevices : IConfigurable
    {
        internal class Device : ICollideable
        {
            internal Utils.Signature<ushort, ushort> Signature;
            internal int[] AllowedSlots;

            internal Device()
            {
                Signature = new(0xFF, 0xFF);
                AllowedSlots = Array.Empty<int>();
            }

            private Device(Utils.Signature<ushort, ushort> sig, int[] allowed)
            {
                Signature = sig;
                AllowedSlots = allowed;
            }

            internal Device(XmlElement element)
            {
                ushort slot_type = ushort.MaxValue;
                ushort device_id = ushort.MaxValue;
                List<int> allowed_slots = new();
                foreach (XmlNode subn in element.ChildNodes)
                {
                    if (subn is XmlElement sub)
                    {
                        switch (sub.Name)
                        {
                            case "Signature":
                                foreach (XmlNode subbern in sub.ChildNodes)
                                {
                                    if (subbern is XmlElement subber)
                                    {
                                        switch (subber.Name)
                                        {
                                            case "DefaultSlotType":
                                                slot_type = Utils.ParseHexableUInt16(subber.GetAttribute("value"));
                                                break;
                                            case "DeviceID":
                                                device_id = Utils.ParseHexableUInt16(subber.GetAttribute("value"));
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                }
                                break;

                            case "Allow":
                                allowed_slots.Add(Utils.ParseHexableUInt16(sub.GetAttribute("slot")));
                                break;

                            default:
                                break;
                        }
                    }
                }


                Signature = new(slot_type, device_id);
                AllowedSlots = allowed_slots.ToArray();
            }

            public bool Collides(ICollideable other)
            {
                return other.GetType() == typeof(Device) && Signature == ((Device)other).Signature;
            }

            internal static Device Resolver(ICollideable a, ICollideable b)
            {
                Device da = (Device)a;
                Device db = (Device)b;

                return new Device(da.Signature, da.AllowedSlots.Union(db.AllowedSlots).ToArray());
            }
        }

        internal class DeviceLock : ICollideable
        {
            internal ulong? SerialNumber;
            internal int TargetSlot;

            internal DeviceLock(XmlElement element)
            {
                TargetSlot = int.Parse(element.GetAttribute("slot"));
                if (element.GetAttribute("enabled").ToLowerInvariant() == "true")
                {
                    SerialNumber = ulong.Parse(element.GetAttribute("sn"));
                }
                else
                {
                    SerialNumber = null;
                }
            }

            public bool Collides(ICollideable other) =>
                other is DeviceLock dl && TargetSlot == dl.TargetSlot;
        }

        private Device[] _devices;
        private DeviceLock[] _locks;



        public AllowedDevices()
        {
            _devices = Array.Empty<Device>();
        }

        public AllowedDevices(string path_to_file)
        {
            XmlDocument doc = new XmlDocument();
            doc.Load(path_to_file);
            Utils.PreprocessingContext context = new();
            Utils.PathAsFileDir(path_to_file, context);
            Utils.PreprocessNode(doc, context);
            Setup(doc.DocumentElement);
        }

        internal AllowedDevices(XmlElement element)
        {
            Setup(element);
        }

        private void Setup(XmlElement element)
        {
            List<Device> devices = new();
            List<DeviceLock> locks = new();
            foreach (var sub in element.ChildNodes.OfType<XmlElement>())
            {
                switch (sub.Name)
                {
                    case "Device":
                        devices.Add(new Device(sub));
                        break;

                    case "DeviceLock":
                        locks.Add(new DeviceLock(sub));
                        break;
                }
            }

            Utils.ResolveCollisions(devices, Utils.ResolutionMode.UNION, Device.Resolver);
            Utils.ResolveCollisions(locks, Utils.ResolutionMode.ERROR, null);

            _devices = devices.ToArray();
            _locks = locks.ToArray();
        }

        public void Configure(ConfigurationParams args)
        {
            // Find all of the slots used.
            HashSet<int> slots_used = new();
            foreach (Device d in _devices)
            {
                foreach (int slot in d.AllowedSlots)
                {
                    slots_used.Add(slot);
                }
            }

            // Associate the device with the slot.
            Dictionary<int, List<Device>> allowed = new();
            foreach (int slot in slots_used)
            {
               foreach(Device d in _devices)
               {
                    if (d.AllowedSlots.Contains(slot))
                    {
                        if (!allowed.ContainsKey(slot))
                        {
                            allowed[slot] = new();
                        }
                        allowed[slot].Add(d);
                    }
                }
            }

            // Send the allowed devices out.
            foreach (int slot in slots_used)
            {
                int payload_size = 4 * allowed[slot].Count;

                byte[] command = BitConverter.GetBytes(APT.MGMSG_MCM_SET_ALLOWED_DEVICES);
                byte[] len = BitConverter.GetBytes(payload_size + 2);
                byte[] s = BitConverter.GetBytes(slot - 1);

                byte[] bytes_to_send = new byte[8 + payload_size];

                // Copy data to the array.
                Array.Copy(command, 0, bytes_to_send, 0, 2);
                Array.Copy(len, 0, bytes_to_send, 2, 2);
                bytes_to_send[4] = 0x91;
                bytes_to_send[5] = 0x01;
                Array.Copy(s, 0, bytes_to_send, 6, 2);

                int index = 8;
                foreach (Device d in allowed[slot])
                {
                    byte[] st = BitConverter.GetBytes(d.Signature.PrimaryKey);
                    byte[] did = BitConverter.GetBytes(d.Signature.DiscriminatorKey);

                    Array.Copy(st, 0, bytes_to_send, index, 2);
                    Array.Copy(did, 0, bytes_to_send, index + 2, 2);

                    index += 4;
                }

                // Program the allowed devices.
                args.usb_send(bytes_to_send);
            }

            // Send the serial number protection out.
            foreach (var dl in _locks)
            {
                // Set the serial number.
                byte[] bytes_to_send = new byte[16]
                {
                    0, 0, 10, 0, Thorlabs.APT.Address.MOTHERBOARD | 0x80, Thorlabs.APT.Address.HOST,
                    (byte)(dl.TargetSlot - 1), 0,                   // Slot Address
                    0, 0, 0, 0, 0, 0, 0, 0      // Serial Number
                };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_SET_DEVICE_BOARD), 0, bytes_to_send, 0, 2);
                Array.Copy(BitConverter.GetBytes(dl.SerialNumber.HasValue
                                               ? dl.SerialNumber.Value
                                               : 0xFFFF000000000000),
                   0, bytes_to_send, 8, 8);

                // Program the serial number protection.
                args.usb_send(bytes_to_send);
            }
        }
    }
}

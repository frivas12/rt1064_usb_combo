using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;

namespace SystemXMLParser
{
    public class StepperXML : IConfigurable
    {
        private readonly Dictionary<ushort, byte[]> _data = new();

        public int Slot { get; }


        public struct XmlComponents
        {
            public XmlNode config_elem = null;
            public XmlNode drive_elem = null;
            public XmlNode flags_elem = null;
            public XmlNode limits_elem = null;
            public XmlNode homing_elem = null;
            public XmlNode jog_elem = null;
            public XmlNode encoder_elem = null;
            public XmlNode pid_elem = null;
            public XmlNode store_elem = null;

            public readonly bool Completed =>
                config_elem is not null &&
                drive_elem is not null &&
                flags_elem is not null &&
                limits_elem is not null &&
                homing_elem is not null &&
                jog_elem is not null &&
                encoder_elem is not null &&
                pid_elem is not null &&
                store_elem is not null;

            public XmlComponents() { }
        }

        public StepperXML()
        { }

        public StepperXML(string[] paths_to_configs, int slot)
        {
            Slot = slot;
            var nodes = paths_to_configs.Select(Utils.OpenXml);

            Setup(GetParts(nodes.OfType<XmlElement>()));
        }

        public StepperXML(string path_to_file, int slot)
            : this((XmlElement)Utils.OpenXml(path_to_file).SelectSingleNode("/StageParameters"), slot)
        { }

        public StepperXML(XmlElement element, int slot)
        {
            Slot = slot;
            Setup(GetParts(element));
        }


        private void FlushMsg(ushort apt_command, byte[] data)
        {
            Array.Copy(BitConverter.GetBytes(apt_command), 0, data, 0, 2);
            Array.Copy(BitConverter.GetBytes(data.Length - 6), 0, data, 2, 2);
            data[4] = (byte)(0xA0 + Slot);
            data[5] = 0x01;
            Array.Copy(BitConverter.GetBytes(Slot), 0, data, 6, 2);

            _data[apt_command] = data;
        }

        private static void AddToCommand(byte[] arr, XmlNode parent, string xpath, Utils.NodeAttributeType type, uint offset, uint size, string attr = "value")
        {
            byte[] data = Utils.SerializeNodeAttribute(parent.SelectSingleNode(xpath), type, attr);
            if (data.Length < size)
            {
                byte[] tmp = new byte[size];
                for (int i = 0; i < tmp.Length; ++i) { tmp[i] = 0; }
                Array.Copy(data, tmp, data.Length);
                data = tmp;
            }
            Array.Copy(data, 0, arr, offset, size);
        }

        private static void AddScaledFloatToCommand(byte[] arr, XmlNode parent, string xpath, uint scaling_factor, uint offset, uint size, string attr = "value")
        {
            float f = BitConverter.ToSingle(Utils.SerializeNodeAttribute(parent.SelectSingleNode(xpath), Utils.NodeAttributeType.FLOAT, attr), 0);
            uint converted = (uint)(f * scaling_factor);
            byte[] data = BitConverter.GetBytes(converted);
            if (data.Length < size)
            {
                byte[] tmp = new byte[size];
                for (int i = 0; i < tmp.Length; ++i) { tmp[i] = 0; }
                Array.Copy(data, tmp, data.Length);
                data = tmp;
            }
            Array.Copy(data, 0, arr, offset, size);
        }

        private XmlComponents GetParts(XmlElement element)
        {
            return new XmlComponents()
            {
                config_elem = element.SelectSingleNode("Config"),
                drive_elem = element.SelectSingleNode("Drive"),
                flags_elem = element.SelectSingleNode("Flags"),
                limits_elem = element.SelectSingleNode("Limit"),
                homing_elem = element.SelectSingleNode("Homing"),
                jog_elem = element.SelectSingleNode("Jog"),
                encoder_elem = element.SelectSingleNode("Encoder"),
                pid_elem = element.SelectSingleNode("PID"),
                store_elem = element.SelectSingleNode("Store"),
            };
        }

        private XmlComponents GetParts(IEnumerable<XmlElement> segments)
        {
            XmlComponents rt = new();

            foreach (XmlElement element in segments)
            {
                XmlNode? signature_node = element.SelectSingleNode("/Struct/Signature");
                XmlNode? data_node = element.SelectSingleNode("/Struct/Data");
                if (signature_node is not null && data_node is not null)
                {
                    XmlNode? struct_id_node = signature_node.SelectSingleNode("StructID");
                    int struct_id;
                    if (struct_id_node is not null && int.TryParse(struct_id_node.Attributes.GetNamedItem("value").Value, out struct_id))
                    {
                        APT.StructIDs type = (APT.StructIDs)struct_id;
                        switch (type)
                        {
                            case APT.StructIDs.ENCODER:
                                rt.encoder_elem = data_node;
                                break;
                            case APT.StructIDs.LIMIT:
                                rt.limits_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_CONFIG:
                                rt.config_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_DRIVE:
                                rt.drive_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_FLAGS:
                                rt.flags_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_HOME:
                                rt.homing_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_JOG:
                                rt.jog_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_PID:
                                rt.pid_elem = data_node;
                                break;
                            case APT.StructIDs.STEPPER_STORE:
                                rt.store_elem = data_node;
                                break;
                            default:
                                break;
                        }

                    }
                }
            }

            return rt;
        }

        private void Setup(XmlComponents comps)
        {
            // Process MGMSG_MCM_SET_STAGEPARAMS.
            if (comps.config_elem is not null && comps.drive_elem is not null && comps.flags_elem is not null && comps.encoder_elem is not null)
            {
                byte[] msg = new byte[96];

                AddToCommand(msg, comps.config_elem, "AxisSerial", Utils.NodeAttributeType.UINT, 28, 4);
                AddScaledFloatToCommand(msg, comps.config_elem, "CountsPerUnit", 100000, 32, 4);
                AddToCommand(msg, comps.config_elem, "Position", Utils.NodeAttributeType.INT, 36, 4, "min");
                AddToCommand(msg, comps.config_elem, "Position", Utils.NodeAttributeType.INT, 40, 4, "max");
                AddToCommand(msg, comps.drive_elem, "Acceleration", Utils.NodeAttributeType.INT, 44, 4, "max");
                AddToCommand(msg, comps.drive_elem, "Acceleration", Utils.NodeAttributeType.INT, 48, 4, "min");
                AddToCommand(msg, comps.drive_elem, "Speed", Utils.NodeAttributeType.INT, 52, 4, "max");
                AddToCommand(msg, comps.drive_elem, "Speed", Utils.NodeAttributeType.INT, 56, 2, "min");
                AddToCommand(msg, comps.drive_elem, "FullStepSpeed", Utils.NodeAttributeType.INT, 58, 2);
                AddToCommand(msg, comps.drive_elem, "KVal/Hold", Utils.NodeAttributeType.UINT, 60, 1);
                AddToCommand(msg, comps.drive_elem, "KVal/Run", Utils.NodeAttributeType.UINT, 61, 1);
                AddToCommand(msg, comps.drive_elem, "KVal/Accel", Utils.NodeAttributeType.UINT, 62, 1);
                AddToCommand(msg, comps.drive_elem, "KVal/Decel", Utils.NodeAttributeType.UINT, 63, 1);
                AddToCommand(msg, comps.drive_elem, "IntersectSpeed", Utils.NodeAttributeType.INT, 64, 2);
                AddToCommand(msg, comps.drive_elem, "StallThreshold", Utils.NodeAttributeType.UINT, 66, 1);
                AddToCommand(msg, comps.drive_elem, "Slope/Start", Utils.NodeAttributeType.UINT, 67, 1);
                AddToCommand(msg, comps.drive_elem, "Slope/FinalAccel", Utils.NodeAttributeType.UINT, 68, 1);
                AddToCommand(msg, comps.drive_elem, "Slope/FinalDecel", Utils.NodeAttributeType.UINT, 69, 1);
                AddToCommand(msg, comps.drive_elem, "OCDThreshold", Utils.NodeAttributeType.UINT, 70, 1);
                AddToCommand(msg, comps.drive_elem, "StepMode", Utils.NodeAttributeType.UINT, 71, 1);
                AddToCommand(msg, comps.drive_elem, "ICConfig", Utils.NodeAttributeType.INT, 72, 2);
                AddToCommand(msg, comps.encoder_elem, "NmPerCount", Utils.NodeAttributeType.FLOAT, 74, 4);
                AddToCommand(msg, comps.drive_elem, "GateDriverConfig", Utils.NodeAttributeType.INT, 78, 3);
                AddToCommand(msg, comps.drive_elem, "ApproachVelocity", Utils.NodeAttributeType.INT, 81, 2);
                AddToCommand(msg, comps.drive_elem, "Deadband", Utils.NodeAttributeType.INT, 83, 2);
                AddToCommand(msg, comps.drive_elem, "Backlash", Utils.NodeAttributeType.INT, 85, 2);
                AddToCommand(msg, comps.drive_elem, "KickoutTime", Utils.NodeAttributeType.UINT, 87, 1);
                AddToCommand(msg, comps.flags_elem, "Flags", Utils.NodeAttributeType.UINT, 88, 1);
                AddToCommand(msg, comps.config_elem, "CollisionThreshold", Utils.NodeAttributeType.INT, 89, 2);
                AddToCommand(msg, comps.encoder_elem, "EncoderType", Utils.NodeAttributeType.UINT, 91, 1);
                AddToCommand(msg, comps.encoder_elem, "IndexDelta", Utils.NodeAttributeType.UINT, 92, 2, "min");
                AddToCommand(msg, comps.encoder_elem, "IndexDelta", Utils.NodeAttributeType.UINT, 94, 2, "step");

                FlushMsg(APT.MGMSG_MCM_SET_STAGEPARAMS, msg);
            }

            // Process MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS.
            if (comps.limits_elem is not null)
            {
                byte[] msg = new byte[30];

                AddToCommand(msg, comps.limits_elem, "HardLimit", Utils.NodeAttributeType.UINT, 8, 2, "cw");
                AddToCommand(msg, comps.limits_elem, "HardLimit", Utils.NodeAttributeType.UINT, 10, 2, "ccw");
                AddToCommand(msg, comps.limits_elem, "SoftLimit", Utils.NodeAttributeType.INT, 12, 4, "cw");
                AddToCommand(msg, comps.limits_elem, "SoftLimit", Utils.NodeAttributeType.INT, 16, 4, "ccw");
                AddToCommand(msg, comps.limits_elem, "AbsLimit", Utils.NodeAttributeType.INT, 20, 4, "high");
                AddToCommand(msg, comps.limits_elem, "AbsLimit", Utils.NodeAttributeType.INT, 24, 4, "low");
                AddToCommand(msg, comps.limits_elem, "Mode", Utils.NodeAttributeType.UINT, 28, 2);

                FlushMsg(APT.MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS, msg);
            }

            // Process MGMSG_MCM_SET_HOMEPARAMS.
            if (comps.homing_elem is not null)
            {
                byte[] msg = new byte[20];

                AddToCommand(msg, comps.homing_elem, "Mode", Utils.NodeAttributeType.UINT, 8, 1);
                AddToCommand(msg, comps.homing_elem, "Direction", Utils.NodeAttributeType.UINT, 9, 1);
                AddToCommand(msg, comps.homing_elem, "LimitSwitch", Utils.NodeAttributeType.UINT, 10, 2);
                AddToCommand(msg, comps.homing_elem, "Velocity", Utils.NodeAttributeType.UINT, 12, 4);
                AddToCommand(msg, comps.homing_elem, "OffsetDistance", Utils.NodeAttributeType.INT, 16, 4);

                FlushMsg(APT.MGMSG_MCM_SET_HOMEPARAMS, msg);
            }

            // Process MGMSG_MOT_SET_DCPIDPARAMS.
            if (comps.pid_elem is not null)
            {
                byte[] msg = new byte[26];

                AddToCommand(msg, comps.pid_elem, "Kp", Utils.NodeAttributeType.INT, 8, 4);
                AddToCommand(msg, comps.pid_elem, "Ki", Utils.NodeAttributeType.INT, 12, 4);
                AddToCommand(msg, comps.pid_elem, "Kd", Utils.NodeAttributeType.INT, 16, 4);
                AddToCommand(msg, comps.pid_elem, "IMax", Utils.NodeAttributeType.INT, 20, 4);
                AddToCommand(msg, comps.pid_elem, "FilterControl", Utils.NodeAttributeType.INT, 24, 2);

                FlushMsg(APT.MGMSG_MOT_SET_DCPIDPARAMS, msg);
            }
        
            //
            if (comps.store_elem is not null)
            {
                byte[] msg = new byte[10];

                AddToCommand(msg, comps.store_elem, "Deadband", Utils.NodeAttributeType.UINT, 8, 2);

                FlushMsg(APT.MGMSG_SET_STORE_POSITION_DEADBAND, msg);

                msg = new byte[48];

                int offset = 8;
                foreach (XmlNode sub in comps.store_elem.ChildNodes)
                {
                    if (sub.Name == "Position" && sub is XmlElement subby)
                    {
                        string index = subby.GetAttribute("index");
                        if (index != string.Empty)
                        {
                            offset = 4 + int.Parse(index) * 4;
                        }
                        Array.Copy(BitConverter.GetBytes(int.Parse(subby.GetAttribute("value"))), 0, msg, offset, 4);
                        offset += 4;
                    }
                }

                FlushMsg(APT.MGMSG_SET_STORE_POSITION, msg);
            }
        }

        public void Configure(ConfigurationParams args)
        {
            foreach (KeyValuePair<ushort, byte[]> kvp in _data)
            {
                args.usb_send(kvp.Value);
            }
        }

        /// <summary>
        /// If save to eeprom is true, then all the commands will also be saved to EEPROM.
        /// </summary>
        public void Configure(ConfigurationParams args, bool save_to_eeprom)
        {
            Configure(args);

            // Save to EEPROM if flagged.
            if (save_to_eeprom)
            {
                byte[] command = { 0, 0, 4, 0, (byte)(Thorlabs.APT.Address.SLOT_1 + (Slot - 1) | 0x80), Thorlabs.APT.Address.HOST, 0, 0, 0, 0 };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_EEPROMPARAMS), 0, command, 0, 2);

                foreach (KeyValuePair<ushort, byte[]> kvp in _data)
                {
                    Array.Copy(BitConverter.GetBytes(kvp.Key), 0, command, 8, 2);
                    args.usb_send(command);
                }
            }
        }
    }
}

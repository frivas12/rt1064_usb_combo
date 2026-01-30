using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;

namespace SystemXMLParser
{
    public partial class SystemXML
    {
        public class Joysticks : IConfigurable
        {
            public class Port : IConfigurable, ICollideable
            {
                private class Joystick
                {
                    private class InMap : ICollideable
                    {
                        public byte ControlNum;
                        public ushort VID;
                        public ushort PID;
                        public Utils.USBTargets Target;
                        public Utils.USBDispatcher Destination;
                        public byte Speed;
                        public byte Reverse;
                        public byte Deadband;
                        public uint ControlMode;

                        public bool Collides (ICollideable other)
                        {
                            return other.GetType() == typeof(InMap) && ControlNum == ((InMap)other).ControlNum;
                        }

                        public InMap()
                        {
                            ControlNum = 0;
                            VID = ushort.MaxValue;
                            PID = ushort.MaxValue;
                            Target = new Utils.USBTargets();
                            Destination = new Utils.USBDispatcher();
                            Speed = 0;
                            Reverse = 0;
                            Deadband = 0;
                            ControlMode = 0;
                        }

                        internal InMap(XmlElement element, ref int auto_counter)
                        {
                            string control_str = element.GetAttribute("control");

                            if (control_str == string.Empty)
                            {
                                ControlNum = (byte)auto_counter;
                                ++auto_counter;
                            }
                            else
                            {
                                ControlNum = byte.Parse(control_str);
                                auto_counter = ControlNum + 1;
                            }

                            foreach (XmlNode subn in element.ChildNodes)
                            {
                                if (subn is XmlElement sub)
                                {
                                    switch (sub.Name)
                                    {
                                        case "VID":
                                            VID = Utils.ParseHexableUInt16(sub.GetAttribute("value"));
                                            break;

                                        case "PID":
                                            PID = Utils.ParseHexableUInt16(sub.GetAttribute("value"));
                                            break;

                                        case "ModifyControl":
                                            Target = new Utils.USBTargets(sub);
                                            break;

                                        case "Destination":
                                            Destination = new Utils.USBDispatcher(sub);
                                            break;

                                        case "ModifySpeed":
                                            Speed = byte.Parse(sub.GetAttribute("value"));
                                            break;

                                        case "ReverseDir":
                                            Reverse = byte.Parse(sub.GetAttribute("value"));
                                            break;

                                        case "Deadband":
                                            Deadband = byte.Parse(sub.GetAttribute("value"));
                                            break;

                                        case "Mode":
                                            ControlMode = uint.Parse(sub.GetAttribute("value"));
                                            break;

                                        default:
                                            break;
                                    }
                                }
                            }
                        }
                    }
                    private class OutMap : ICollideable
                    {
                        public byte ControlNum;
                        public byte Usage;
                        public ushort VID;
                        public ushort PID;
                        public uint LEDMode;
                        public uint Color1;
                        public uint Color2;
                        public uint Color3;
                        public Utils.USBDispatcher Source;

                        public bool Collides (ICollideable other)
                        {
                            return other.GetType() == typeof(OutMap) && ControlNum == ((OutMap)other).ControlNum;
                        }

                        public OutMap()
                        {
                            ControlNum = byte.MaxValue;
                            Usage = byte.MaxValue;
                            VID = ushort.MaxValue;
                            PID = ushort.MaxValue;
                            LEDMode = uint.MaxValue;
                            Color1 = 0;
                            Color2 = 0;
                            Color3 = 0;
                            Source = new Utils.USBDispatcher();
                        }

                        internal OutMap(XmlElement element, ref int auto_counter)
                        {
                            string control_str = element.GetAttribute("control");

                            if (control_str == string.Empty)
                            {
                                ControlNum = (byte)auto_counter;
                                ++auto_counter;
                            }
                            else
                            {
                                ControlNum = byte.Parse(control_str);
                                auto_counter = ControlNum + 1;
                            }

                            uint?[] colors = { null, null, null };
                            int led_auto_counter = 0;
                            foreach (XmlNode subn in element.ChildNodes)
                            {
                                if (subn is XmlElement sub)
                                {
                                    switch (sub.Name)
                                    {
                                        case "VID":
                                            VID = Utils.ParseHexableUInt16(sub.GetAttribute("value"));
                                            break;

                                        case "PID":
                                            PID = Utils.ParseHexableUInt16(sub.GetAttribute("value"));
                                            break;

                                        case "Mode":
                                            LEDMode = uint.Parse(sub.GetAttribute("value"));
                                            break;

                                        case "ColorID":
                                            {
                                                string index_str = sub.GetAttribute("index");
                                                int index;
                                                if (index_str == string.Empty)
                                                {
                                                    index = led_auto_counter;
                                                    ++led_auto_counter;
                                                }
                                                else
                                                {
                                                    index = int.Parse(index_str) - 1;
                                                    led_auto_counter = index + 1;
                                                    if (colors[index] is not null)
                                                    {
                                                        throw new Exception("Color Collision");
                                                    }
                                                }

                                                colors[index] = uint.Parse(sub.GetAttribute("value"));
                                            }
                                            break;

                                        case "Source":
                                            Source = new Utils.USBDispatcher(sub);
                                            break;

                                        case "Usage":
                                            Usage = byte.Parse(sub.GetAttribute("value"));
                                            break;

                                        default:
                                            break;
                                    }
                                }
                            }

                            Color1 = (uint)colors[0];
                            Color2 = (uint)colors[1];
                            Color3 = (uint)colors[2];
                        }
                    }

                    private InMap[] _in_maps;
                    private OutMap[] _out_maps;

                    public Joystick()
                    {
                        _in_maps = Array.Empty<InMap>();
                        _out_maps = Array.Empty<OutMap>();
                    }

                    internal Joystick(XmlElement element)
                    {
                        List<InMap> in_maps = new();
                        List<OutMap> out_maps = new();

                        int in_auto_counter = 0;
                        int out_auto_counter = 0;
                        foreach (XmlNode subn in element.ChildNodes)
                        {
                            if (subn is XmlElement sub)
                            {
                                switch (sub.Name)
                                {
                                    case "InMap":
                                        in_maps.Add(new InMap(sub, ref in_auto_counter));
                                        break;
                                    case "OutMap":
                                        out_maps.Add(new OutMap(sub, ref out_auto_counter));
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }

                        Utils.ResolveCollisions(in_maps, Utils.ResolutionMode.ERROR, null);
                        Utils.ResolveCollisions(out_maps, Utils.ResolutionMode.ERROR, null);

                        _in_maps = in_maps.ToArray();
                        _out_maps = out_maps.ToArray();
                    }

                    internal void Configure (ConfigurationParams args, byte port_no, bool save_to_eeprom)
                    {
                        foreach (InMap map in _in_maps)
                        {
                            byte[] bytes_to_send = new byte[27];
                            
                            // Header
                            Array.Copy(BitConverter.GetBytes(APT.MGMSG_MOD_SET_JOYSTICK_MAP_IN), 0, bytes_to_send, 0, 2);
                            bytes_to_send[2] = 21;
                            bytes_to_send[3] = 0;
                            bytes_to_send[4] = 0x91;
                            bytes_to_send[5] = 0x01;

                            bytes_to_send[6] = port_no;
                            bytes_to_send[7] = map.ControlNum;
                            Array.Copy(BitConverter.GetBytes(map.VID), 0, bytes_to_send, 8, 2);
                            Array.Copy(BitConverter.GetBytes(map.PID), 0, bytes_to_send, 10, 2);
                            bytes_to_send[12] = map.Target.Port;
                            Array.Copy(BitConverter.GetBytes(map.Target.Control), 0, bytes_to_send, 13, 2);
                            bytes_to_send[15] = map.Destination.Slots;
                            bytes_to_send[16] = map.Destination.Bit;
                            bytes_to_send[17] = map.Destination.Slots;
                            bytes_to_send[18] = map.Destination.Virtual;
                            bytes_to_send[19] = map.Speed;
                            bytes_to_send[20] = map.Reverse;
                            bytes_to_send[21] = map.Deadband;
                            Array.Copy(BitConverter.GetBytes(map.ControlMode), 0, bytes_to_send, 22, 4);
                            bytes_to_send[26] = 0x00; // Enable Control

                            args.usb_send(bytes_to_send);
                        }

                        foreach (OutMap map in _out_maps)
                        {
                            byte[] bytes_to_send = new byte[21];

                            // Header
                            Array.Copy(BitConverter.GetBytes(APT.MGMSG_MOD_SET_JOYSTICK_MAP_OUT), 0, bytes_to_send, 0, 2);
                            bytes_to_send[2] = 15;
                            bytes_to_send[3] = 0;
                            bytes_to_send[4] = 0x91;
                            bytes_to_send[5] = 0x01;

                            // Payload
                            bytes_to_send[6] = port_no;
                            bytes_to_send[7] = map.ControlNum;
                            bytes_to_send[8] = map.Usage;
                            Array.Copy(BitConverter.GetBytes(map.VID), 0, bytes_to_send, 9, 2);
                            Array.Copy(BitConverter.GetBytes(map.PID), 0, bytes_to_send, 11, 2);
                            Array.Copy(BitConverter.GetBytes(map.LEDMode), 0, bytes_to_send, 13, 1);
                            Array.Copy(BitConverter.GetBytes(map.Color1), 0, bytes_to_send, 14, 1);
                            Array.Copy(BitConverter.GetBytes(map.Color2), 0, bytes_to_send, 15, 1);
                            Array.Copy(BitConverter.GetBytes(map.Color3), 0, bytes_to_send, 16, 1);
                            bytes_to_send[17] = map.Source.Slots;
                            bytes_to_send[18] = map.Source.Bit;
                            bytes_to_send[19] = map.Source.Port;
                            bytes_to_send[20] = map.Source.Virtual;

                            args.usb_send(bytes_to_send);
                        }

                        if (save_to_eeprom)
                        {
                            byte[] bytes_to_send = new byte[10];
                            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_EEPROMPARAMS), 0, bytes_to_send, 0, 2);
                            bytes_to_send[2] = 4;
                            bytes_to_send[3] = 0;
                            bytes_to_send[4] = Thorlabs.APT.Address.MOTHERBOARD| 0x80;
                            bytes_to_send[5] = Thorlabs.APT.Address.HOST;
                            bytes_to_send[6] = port_no;
                            bytes_to_send[7] = 0;

                            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_SET_JOYSTICK_MAP_IN), 0, bytes_to_send, 8, 2);
                            args.usb_send(bytes_to_send);

                            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_SET_JOYSTICK_MAP_OUT), 0, bytes_to_send, 8, 2);
                            args.usb_send(bytes_to_send);
                        }
                    }
                }

                private int _id;
                private Joystick _js;
                private string _name;

                public int Id {  get { return _id; } }

                public Port()
                {
                    _id = -1;
                    _js = new Joystick();
                    _name = string.Empty;
                }

                public Port(string path_to_file, ref int auto_counter)
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(path_to_file);
                    Utils.PreprocessingContext context = new();
                    Utils.PathAsFileDir(path_to_file, context);
                    Utils.PreprocessNode(doc, context);
                    Setup(doc.DocumentElement, ref auto_counter);
                }

                /**
                 * Allows a joystick file to be loaded directly.
                 */
                public Port(string path_to_file, int port_number)
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(path_to_file);
                    Utils.PreprocessNode(doc);

                    _id = port_number;
                    _js = new(doc.DocumentElement);
                }

                internal Port(XmlElement element, ref int auto_counter)
                {
                    Setup(element, ref auto_counter);
                }

                private void Setup(XmlElement element, ref int auto_counter)
                {

                    Joystick? js = null;
                    int id;
                    string name = string.Empty;

                    string id_str = element.GetAttribute("index");

                    if (id_str == string.Empty)
                    {
                        id = auto_counter;
                        ++auto_counter;
                    }
                    else
                    {
                        id = int.Parse(id_str);
                        auto_counter = id + 1;
                    }

                    foreach (XmlNode subn in element.ChildNodes)
                    {
                        if (subn is XmlElement sub)
                        {
                            switch (sub.Name)
                            {
                                case "Name":
                                    name = sub.GetAttribute("value");
                                    break;
                                case "Joystick":
                                    js = new(sub);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }


                    _id = id;
                    _js = (js is null) ? new Joystick() : js;
                    _name = name;
                }

                public bool Collides(ICollideable other)
                {
                    return other.GetType() == typeof(Port) && _id == ((Port)other)._id;
                }

                public void Configure(ConfigurationParams args, bool save_to_eeprom)
                {
                    _js.Configure(args, (byte)_id, save_to_eeprom);
                }

                public void Configure(ConfigurationParams args) => Configure(args, false);

            }

            private Port[] _ports;

            public Joysticks()
            {
                _ports = Array.Empty<Port>();
            }

            public Joysticks(string path_to_file)
            {
                XmlDocument doc = new XmlDocument();
                doc.Load(path_to_file);
                Utils.PreprocessingContext context = new();
                Utils.PathAsFileDir(path_to_file, context);
                Utils.PreprocessNode(doc, context);
                Setup(doc.DocumentElement);
            }

            internal Joysticks(XmlElement element)
            {
                Setup(element);
            }

            private void Setup(XmlElement element)
            {
                List<Port> port = new();

                int auto_counter = 0;
                foreach (XmlNode sub in element.ChildNodes)
                {
                    if (sub.Name == "Port" && sub is XmlElement subby)
                    {
                        port.Add(new Port(subby, ref auto_counter));
                    }
                }

                Utils.ResolveCollisions(port, Utils.ResolutionMode.ERROR, null);

                _ports = port.ToArray();
            }

            public void Configure(ConfigurationParams args) => Configure(args, false);

            public void Configure(ConfigurationParams args, bool save_all_ports)
            {
                foreach (Port port in _ports)
                {
                    port.Configure(args, save_all_ports);
                }
            }
        }
    }
}

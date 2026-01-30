using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.XPath;

namespace SystemXMLParser
{
    public class Utils
    {
        public static ushort ParseHexableUInt16(string str)
        {
            if (str.Length > 2 && str.Substring(0, 2) == "0x")
            {
                return ushort.Parse(str.Substring(2), System.Globalization.NumberStyles.HexNumber);
            }
            return ushort.Parse(str);
        }

        public static ulong ParseHexableUInt64(string str)
        {
            if (str.Length > 2 && str.Substring(0, 2) == "0x")
            {
                return ulong.Parse(str.Substring(2), System.Globalization.NumberStyles.HexNumber);
            }
            return ulong.Parse(str);
        }

        internal enum ResolutionMode
        {
            ERROR,
            UNION
        }

        internal class PreprocessingContext : ICloneable
        {
            public string Path { get; set; }

            public object Clone()
            {
                return new PreprocessingContext()
                {
                    Path=this.Path
                };
            }
        }

        internal static void PathAsFileDir(string path, PreprocessingContext context)
        {
            context.Path = Path.GetDirectoryName(path);
        }

        private static void PreprocessNode_Variables(XmlNode node, ref PreprocessingContext context)
        {
            if (node.Attributes is not null)
            {
                foreach (XmlAttribute attr in node.Attributes)
                {
                    if (attr.Value.Length > 0 && attr.Value[0] == '$')
                    {
                        if (attr.Value.Length > 6 && attr.Value.Substring(1, 5) == "PATH:")
                        {
                            // Path operator
                            attr.Value = Path.Combine(context.Path, attr.Value.Substring(6)).Replace('/', Path.DirectorySeparatorChar);
                        }
                        else if (attr.Value.Length > 1 && attr.Value[1] == '$')
                        {
                            // Dollarsign constant.
                            attr.Value = attr.Value.Substring(1);
                        }
                    }
                }
            }

            if (node.ChildNodes.Count == 0)
            {
                return;
            }

            var child_list = node.ChildNodes;
            for (int i = 0; i < child_list.Count; i++)
            {
                XmlNode child = child_list[i];

                PreprocessNode_Variables(child, ref context);
            }
        }

        // Recursive function to replace include and import nodes in the dom.
        public const string PREPROCESSING_SOURCE_ATTRIBUTE = "_import_src";
        private static void PreprocessNode_Includes(XmlNode node, PreprocessingContext context)
        {
            if (node.ChildNodes.Count == 0)
            {
                return;
            }

            var child_list = node.ChildNodes;
            for (int i = 0; i < child_list.Count; i++)
            {
                XmlNode child = child_list[i];
                if (child.Name == "Include" || child.Name == "Import")
                {
                    string file = Path.Combine(context.Path, child.Attributes.GetNamedItem("href").Value).Replace('/', Path.DirectorySeparatorChar);
                    string file_cwd = Path.GetDirectoryName(file);

                    XmlDocument doc = new();

                    doc.Load(file);

                    XmlNode new_node;

                    PreprocessingContext new_context = (PreprocessingContext)context.Clone();
                    new_context.Path = file_cwd;
                    PreprocessNode(doc, new_context);

                    if (child.Name == "Import")
                    {
                        var nav = doc.CreateNavigator();

                        new_node = node.OwnerDocument.ImportNode((XmlNode)nav.SelectSingleNode(child.Attributes.GetNamedItem("xpath").Value).UnderlyingObject, true);
                    }
                    else
                    {
                        new_node = node.OwnerDocument.ImportNode(doc.DocumentElement, true);
                    }

                    XmlAttribute attr = doc.CreateAttribute(PREPROCESSING_SOURCE_ATTRIBUTE);
                    attr.Value = file;

                    new_node.Attributes.SetNamedItem(attr);
                    node.ReplaceChild(new_node, child);
                }
                else
                {
                    PreprocessNode_Includes(child, context);
                }
            }
        }

        internal static void PreprocessNode(XmlNode node, PreprocessingContext context)
        {
            PreprocessNode_Variables(node, ref context);
            PreprocessNode_Includes(node, context);
        }

        internal static void PreprocessNode(XmlNode node)
        {
            PreprocessNode(node, new() { 
                Path = "."
            });
        }

        internal delegate ICollideable CollisionResolver(ICollideable node1, ICollideable node2);

        private struct ResolutionPair
        {
            public ICollideable Node1;
            public ICollideable Node2;

            public ResolutionPair(ICollideable node1, ICollideable node2)
            {
                Node1 = node1;
                Node2 = node2;
            }
        }

        // Function that will resolve collisions of child elements of an XmlNode.
        // In Error resolution mode, an exception will raise when a collision occurs.
        // In Union resolutio mode, the colliding nodes will be removed and a resolved node from the resolver will be appended.
        internal static void ResolveCollisions<T>(List<T> nodes, ResolutionMode mode, CollisionResolver? resolver) where T : ICollideable
        {
            HashSet<ICollideable> to_remove = new();
            List<ResolutionPair> to_resolve = new();
            for (int i = 0; i < nodes.Count; i++)
            {
                for (int j = i + 1; j < nodes.Count; j++)
                {
                    if (nodes[i].Collides(nodes[j]))
                    {
                        switch (mode)
                        {
                            case ResolutionMode.UNION:
                                to_remove.Add(nodes[i]);
                                to_remove.Add(nodes[j]);
                                to_resolve.Add(new ResolutionPair(nodes[i], nodes[j]));
                                break;

                            case ResolutionMode.ERROR:
                                throw new Exception("Collision occurred between index " + i.ToString() + " and " + j.ToString() + ".");

                            default:
                                throw new Exception("Unknown resolution mode.");
                        }
                    }
                }
            }

            foreach (T node in to_remove)
            {
                nodes.Remove(node);
            }
            foreach (ResolutionPair pair in to_resolve)
            {
                nodes.Add((T)resolver(pair.Node1, pair.Node2));
            }
        }

        /**
         * Function that queries the part number for a given
         * device signature.
         * Returns null if the signature was not found.
         */
        public delegate string? QueryPartNumber (Signature<ushort, ushort> signature);

        internal static string? QueryPartNumber_Default(Signature<ushort, ushort> signature)
        {
            return null;
        }

        public class Signature<A,B> : IComparable
            where A : IComparable
            where B : IComparable 
        {
            public A PrimaryKey;
            public B DiscriminatorKey;

            public Signature(A pk, B dk)
            {
                PrimaryKey = pk;
                DiscriminatorKey = dk;
            }

            public int CompareTo(object? obj)
            {
                if (obj is null)
                {
                    return 1;
                }

                Signature<A,B>? other = obj as Signature<A,B>;
                if (other is null)
                {
                    throw new ArgumentException("Comparing a non-signature or differntly-typed signature.");
                }
                int rt = PrimaryKey.CompareTo(other.PrimaryKey);
                rt = (rt == 0) ? (DiscriminatorKey.CompareTo(other.DiscriminatorKey)) : rt;
                return rt;
            }

            private bool Compare(Signature<A,B> other)
            {
                return CompareTo(other) == 0;
            }

            public static bool operator ==(Signature<A, B> a, Signature<A, B> b) => a.Compare(b);
            public static bool operator !=(Signature<A, B> a, Signature<A, B> b) => !a.Compare(b);

            public override string ToString()
            {
                return "<" + PrimaryKey.ToString() + "," + DiscriminatorKey.ToString() + ">";
            }

            public override bool Equals(object obj)
            {
                return CompareTo(obj) == 0;
            }

            public override int GetHashCode()
            {
                return PrimaryKey.GetHashCode() ^ DiscriminatorKey.GetHashCode();
            }
        }

        internal struct USBTargets
        {
            public byte Port;
            public ushort Control;

            private static bool Comp(USBTargets a, USBTargets b)
            {
                return a.Port == b.Port && a.Control == b.Control;
            }

            public static bool operator ==(USBTargets a, USBTargets b) => Comp(a, b);
            public static bool operator !=(USBTargets a, USBTargets b) => !Comp(a, b);

            internal USBTargets(XmlElement element)
            {
                Port = byte.MaxValue;
                Control = ushort.MaxValue;
                foreach (XmlNode subn in element.ChildNodes)
                {
                    if (subn is XmlElement sub)
                    {
                        switch (sub.Name)
                        {
                            case "Port":
                                Port = byte.Parse(sub.GetAttribute("value"));
                                break;

                            case "ControlNum":
                                Control = ushort.Parse(sub.GetAttribute("value"));
                                break;

                            default:
                                break;
                        }
                    }
                }
            }
        }

        internal struct USBDispatcher
        {
            public byte Slots;
            public byte Bit;
            public byte Port;
            public byte Virtual;

            private static bool Comp(USBDispatcher a, USBDispatcher b)
            {
                return a.Slots == b.Slots && a.Bit == b.Bit && a.Port == b.Port && a.Virtual == b.Virtual;
            }

            public static bool operator ==(USBDispatcher a, USBDispatcher b) => Comp(a, b);
            public static bool operator !=(USBDispatcher a, USBDispatcher b) => !Comp(a, b);

            internal USBDispatcher(XmlElement element)
            {
                Slots = 0;
                Bit = 0;
                Port = 0;
                Virtual = 0;

                foreach (XmlNode subn in element.ChildNodes)
                {
                    if (subn is XmlElement sub)
                    {
                        switch (sub.Name)
                        {
                            case "Slot":
                                Slots = byte.Parse(sub.GetAttribute("value"));
                                break;

                            case "Bit":
                                Bit = byte.Parse(sub.GetAttribute("value"));
                                break;

                            case "Port":
                                Port = byte.Parse(sub.GetAttribute("value"));
                                break;

                            case "Virtual":
                                Virtual = byte.Parse(sub.GetAttribute("value"));
                                break;

                            default:
                                break;
                        }
                    }
                }
            }
        }


        internal static void LUTProgrammingRoutine(ConfigurationParams args, byte look_up, byte[] inquiry_data, byte[] payload_data)
        {
            int last_step = 0;

            try
            {
                // Step 1:  Begin the inquiry.
                // Try to send a begin request.
                byte[] command = BitConverter.GetBytes(APT.MGMSG_LUT_SET_INQUIRY);
                byte[] bytesToSend = new byte[6] { command[0], command[1], look_up, 0x01, 0x11, 0x01 };
                args.usb_send(bytesToSend);

                byte[]? response = args.usb_timeout(args.DEFAULT_TIMEOUT);
                if (response is null)
                {
                    throw new Exception("Programming Failure: Step 1 Timeout");
                }

                if (response[1] != 0)
                {
                    throw new Exception("Programming Failure: Step 1 Bad Response (" + response[1].ToString() + ")");
                }
                last_step = 1;

                // Step 2:  Get the maximum programming size.
                command = BitConverter.GetBytes(APT.MGMSG_LUT_REQ_PROGRAMMING_SIZE);
                bytesToSend = new byte[6] { command[0], command[1], look_up, 0x01, 0x11, 0x01 };
                args.usb_send(bytesToSend);

                response = args.usb_timeout(args.DEFAULT_TIMEOUT);
                if (response is null)
                {
                    throw new Exception("Programming Failure: Step 2 Timeout");
                }
                last_step = 2;

                ushort MAX_PROGRAMMING_SIZE = (ushort)BitConverter.ToUInt32(response, 1);

                // Step 3:  Send over inquiry data.
                command = BitConverter.GetBytes(APT.MGMSG_LUT_INQUIRE);
                bytesToSend = new byte[7 + MAX_PROGRAMMING_SIZE];
                Array.Copy(command, bytesToSend, 2);
                bytesToSend[4] = 0x91;
                bytesToSend[5] = 0x01;
                bytesToSend[6] = look_up;
                for (uint i = 0; i < inquiry_data.Length; i += MAX_PROGRAMMING_SIZE)
                {
                    uint bytes_to_write = (i + MAX_PROGRAMMING_SIZE > inquiry_data.Length) ? ((uint)inquiry_data.Length - i) : MAX_PROGRAMMING_SIZE;
                    if (bytes_to_write != MAX_PROGRAMMING_SIZE)
                    {
                        bytesToSend = new byte[7 + bytes_to_write];
                        Array.Copy(command, bytesToSend, 2);
                        bytesToSend[4] = 0x91;
                        bytesToSend[5] = 0x01;
                        bytesToSend[6] = look_up;
                    }
                    byte[] len = BitConverter.GetBytes(bytes_to_write + 1);
                    Array.Copy(len, 0, bytesToSend, 2, 2);
                    Array.Copy(inquiry_data, i, bytesToSend, 7, bytes_to_write);
                    args.usb_send(bytesToSend);

                    response = args.usb_timeout(args.DEFAULT_TIMEOUT);
                    if (response is null)
                    {
                        throw new Exception("Programming Failure: Step 3 Timeout");
                    }
                    if (response[1] != 0 && response[1] != 12)
                    {
                        throw new Exception("Programming Failure: Step 3 Bad Response (" + response[1].ToString() + ")");
                    }
                }
                last_step = 3;

                // Step 4:  Begin programming.
                command = BitConverter.GetBytes(APT.MGMSG_LUT_SET_PROGRAMMING);
                bytesToSend = new byte[6] { command[0], command[1], look_up, 0x01, 0x11, 0x01 };
                args.usb_send(bytesToSend);

                response = args.usb_timeout(args.DEFAULT_TIMEOUT);
                if (response is null)
                {
                    throw new Exception("Programming Failure: Step 4 Timeout");
                }
                if (response[1] != 0)
                {
                    throw new Exception("Programming Failure: Step 4 Bad Response (" + response[1].ToString() + ")");
                }
                last_step = 4;

                // Step 5:  Forward programming data.
                byte[] programming_data = inquiry_data.Concat(payload_data).ToArray();

                command = BitConverter.GetBytes(APT.MGMSG_LUT_PROGRAM);
                bytesToSend = new byte[7 + MAX_PROGRAMMING_SIZE];
                Array.Copy(command, bytesToSend, 2);
                bytesToSend[4] = 0x91;
                bytesToSend[5] = 0x01;
                bytesToSend[6] = look_up;
                for (uint i = 0; i < programming_data.Length; i += MAX_PROGRAMMING_SIZE)
                {
                    uint bytes_to_write = (i + MAX_PROGRAMMING_SIZE > programming_data.Length) ? ((uint)programming_data.Length - i) : MAX_PROGRAMMING_SIZE;
                    if (bytes_to_write != MAX_PROGRAMMING_SIZE)
                    {
                        bytesToSend = new byte[7 + bytes_to_write];
                        Array.Copy(command, bytesToSend, 2);
                        bytesToSend[4] = 0x91;
                        bytesToSend[5] = 0x01;
                        bytesToSend[6] = look_up;
                    }
                    byte[] len = BitConverter.GetBytes(bytes_to_write + 1);
                    Array.Copy(len, 0, bytesToSend, 2, 2);
                    Array.Copy(programming_data, i, bytesToSend, 7, bytes_to_write);
                    args.usb_send(bytesToSend);

                    response = args.usb_timeout(args.DEFAULT_TIMEOUT);
                    if (response is null)
                    {
                        throw new Exception("Programming Failure: Step 5 Timeout");
                    }
                    if (response[1] != 0 && response[1] != 12)
                    {
                        throw new Exception("Programming Failure: Step 5 Bad Response (" + response[1].ToString() + ")");
                    }
                }
                last_step = 5;

                // Step 6:  Finish Programming
                command = BitConverter.GetBytes(APT.MGMSG_LUT_FINISH_PROGRAMMING);
                bytesToSend = new byte[6] { command[0], command[1], look_up, 0x01, 0x11, 0x01 };
                args.usb_send(bytesToSend);

                response = args.usb_timeout(args.DEFAULT_TIMEOUT);
                uint bad_addr = BitConverter.ToUInt32(response, 1);
                if (response is null)
                {
                    throw new Exception("Programming Failure: Step 6 Timeout");
                }
                if (bad_addr != 0xFFFFFFFF)
                {
                    throw new Exception("Programming Failure: Step 6 Invalid Address (" + bad_addr + ")");
                }
                last_step = 6;
            }
            catch
            {
                // Exception Cleanup
                byte[] command;
                byte[] bytesToSend;
                switch (last_step)
                {
                    case 1:
                    case 2:
                    case 3:
                        // Stop Inquiry.
                        command = BitConverter.GetBytes(APT.MGMSG_LUT_SET_INQUIRY);
                        bytesToSend = new byte[6] { command[0], command[1], look_up, 0x00, 0x11, 0x01 };
                        args.usb_send(bytesToSend);
                        break;

                    case 4:
                    case 5:
                    case 6:
                        command = BitConverter.GetBytes(APT.MGMSG_LUT_SET_PROGRAMMING);
                        bytesToSend = new byte[6] { command[0], command[1], look_up, 0x00, 0x11, 0x01 };
                        args.usb_send(bytesToSend);
                        break;

                    default:
                        break;
                }

                throw;
            }
        }

        internal static string? OneWireProgrammingRoutine(ConfigurationParams args, ushort slot_id, byte[] data_to_program)
        {
            string? rt = null;

            byte DESTINATION = (byte)(slot_id);
            byte[] command;
            byte[] bytesToSend;
            byte[] bytes_to_program = (byte[])data_to_program;
            byte[]? response;

            uint step = 1;

            // Step 1:  Enable programming
            command = BitConverter.GetBytes(Convert.ToInt32(Thorlabs.APT.MGMSG_OW_SET_PROGRAMMING));
            bytesToSend = new byte[9] { command[0], command[1], 0x03, 0x00, (byte)(Thorlabs.APT.Address.MOTHERBOARD | 0x80), Thorlabs.APT.Address.HOST, DESTINATION, 0x00, 1 };
            args.usb_send(bytesToSend);
            response = args.usb_timeout(args.DEFAULT_TIMEOUT);

            if (response is not null)
            {
                if (response[2] == 0 )
                {
                    step = 2;
                    // Step 2:  Get maximum programming packet size
                    command = BitConverter.GetBytes(Convert.ToInt32(Thorlabs.APT.MGMSG_OW_REQ_PROGRAMMING_SIZE));
                    bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
                    args.usb_send(bytesToSend);
                    response = args.usb_timeout(args.DEFAULT_TIMEOUT);

                    if (response is not null)
                    {
                        uint MAX_PROGRAMMING_SIZE = BitConverter.ToUInt16(response, 0);
                        command = BitConverter.GetBytes(Convert.ToInt32(Thorlabs.APT.MGMSG_OW_PROGRAM));
                        bytesToSend = new byte[8 + MAX_PROGRAMMING_SIZE];
                        bytesToSend[0] = command[0];
                        bytesToSend[1] = command[1];
                        bytesToSend[4] = (byte)(Thorlabs.APT.Address.MOTHERBOARD | 0x80);
                        bytesToSend[5] = Thorlabs.APT.Address.HOST;
                        bytesToSend[6] = DESTINATION;
                        bytesToSend[7] = 0x00;
                        step = 3;

                        // Need to wait a bit for the connection delay system to finish.
                        Thread.Sleep(400);
                        // Step 3:  Program
                        uint byte_index;
                        for (byte_index = 0; byte_index < bytes_to_program.Length; byte_index += MAX_PROGRAMMING_SIZE)
                        {
                            uint bytes_to_write =
                                (bytes_to_program.Length - byte_index > MAX_PROGRAMMING_SIZE)
                                ? MAX_PROGRAMMING_SIZE
                                : (uint)(bytes_to_program.Length - byte_index);
                            byte[] len = BitConverter.GetBytes(bytes_to_write + 2);
                            Array.Copy(len, 0, bytesToSend, 2, 2);
                            Array.Copy(bytes_to_program, byte_index, bytesToSend, 8, bytes_to_write);

                            args.usb_send_fragment(bytesToSend, 8 + (int)bytes_to_write);
                            response = args.usb_timeout(args.DEFAULT_TIMEOUT);

                            if (response is not null)
                            {
                                if (response[2] != 0)
                                {
                                    switch (response[2])
                                    {
                                            case 1:
                                            rt = "Device was disconnected.";
                                            break;
                                        case 2:
                                            rt = "Device does not support one-wire";
                                            break;
                                        case 4:
                                            rt = "Device abnormally stopped programming.";
                                            break;
                                        case 5:
                                            rt = "Device's memory has overflowed.";
                                            break;
                                        case 6:
                                            rt = "Maximum programming size violated.";
                                            break;
                                        default:
                                            rt = "Some unknown event occurred.";
                                            break;
                                    }

                                    break;
                                }
                            }
                        }

                            if (byte_index >= bytes_to_program.Length)
                            {
                                step = 4;
                                // Step 4:  Disable programming
                                command = BitConverter.GetBytes(Convert.ToInt32(Thorlabs.APT.MGMSG_OW_SET_PROGRAMMING));
                                bytesToSend = new byte[9] { command[0], command[1], 0x03, 0x00, (byte)(Thorlabs.APT.Address.MOTHERBOARD | 0x80), Thorlabs.APT.Address.HOST, DESTINATION, 0x00, 0 };
                                args.usb_send(bytesToSend);
                                response = args.usb_timeout(args.DEFAULT_TIMEOUT);

                                if (response is not null)
                                {
                                    switch (response[2])
                                    {
                                        case 0:
                                            rt = "Device was successfully programmed.";
                                            step = 5;
                                            break;
                                        case 1:
                                            rt = "No device is connected.";
                                            break;
                                        case 2:
                                            rt = "Device does not support one-wire.";
                                            break;
                                        case 3:
                                            rt = "Unable to exit programming mode.";
                                            break;
                                        case 4:
                                            rt = "Programming sucessful.";
                                            break;
                                        default:
                                            rt = "Some unknown event occurred.";
                                            break;
                                    }
                                }
                            }
                    }
                }
                else
                {
                    switch (response[2])
                    {
                        case 0:
                            rt = "Programming was cancelled.";
                            break;
                        case 1:
                            rt = "No device is connected.";
                            break;
                        case 2:
                            rt = "Device does not support one-wire.";
                            break;
                        case 3:
                            rt = "Device was already programming.";
                            break;
                        case 4:
                            rt = "Unable to enter programming mode.";
                            break;
                        default:
                            rt = "Some unknown event occurred.";
                            break;
                    }
                }
            }

            switch (step)
            {
                case 1:
                    // Do nothing.
                    break;
                case 2:
                case 3:
                    // Send stop programming until threshold, then restart device if it didn't work.
                    for (int i = 0; i < 10; ++i)
                    {
                        command = BitConverter.GetBytes(Convert.ToInt32(Thorlabs.APT.MGMSG_OW_SET_PROGRAMMING));
                        bytesToSend = new byte[6] { command[0], command[1], DESTINATION, 0, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
                        args.usb_send(bytesToSend);
                        response = args.usb_timeout(args.DEFAULT_TIMEOUT);

                        if (response is not null)
                        {
                            step = 0;
                            break;
                        }
                    }
                    if (step != 0)
                    {
                        // Restart device
                        args.restart_board();
                    }
                    break;
                case 4:
                    // Restart device
                    args.restart_board();
                    break;
                default:
                    break;
            }

            return rt;
        }

        /**
         * Takes in list of arrays and combines them into one array.
         */
        public static T[] CompressSegments<T>(List<T[]> segments)
        {
            int size = 0;
            foreach (T[] arr in segments)
            {
                size += arr.Length;
            }

            T[] rt = new T[size];

            int offset = 0;
            foreach(T[] segment in segments)
            {
                Array.Copy(segment, 0, rt, offset, segment.Length);
                offset += segment.Length;
            }

            return rt;
        }

        internal static byte CRC8(byte[] data, int len)
        {
            const byte POLY = 0x31;

            byte rt = 0;

            for (int i = 0; i < len; ++i)
            {
                rt ^= data[i];
                for (int j = 0; j < 8; ++j)
                {
                    if ((rt & 0x80) == 0)
                    {
                        rt <<= 1;
                    }
                    else
                    {
                        rt = (byte)((rt << 1) ^ POLY);
                    }
                }
            }

            return rt;
        }

        internal static byte CRC8(byte[] data)
        {
            return CRC8(data, data.Length);
        }

        internal enum NodeAttributeType
        {
            BOOL,
            INT,
            UINT,
            FLOAT,
            STRING
        }

        internal static byte[] SerializeNodeAttribute(XmlNode node, NodeAttributeType type, string attribute_name = "value")
        {
            byte[] rt;

            string attr = node.Attributes.GetNamedItem(attribute_name).Value;

            Regex whitespace_remover = new("\\s");
            switch(type)
            {
                case NodeAttributeType.BOOL:
                    attr = whitespace_remover.Replace(attr, "");
                    rt = BitConverter.GetBytes(bool.Parse(attr));
                    break;
                case NodeAttributeType.INT:
                    attr = whitespace_remover.Replace(attr, "");
                    rt = BitConverter.GetBytes(int.Parse(attr));
                    break;
                case NodeAttributeType.UINT:
                    attr = whitespace_remover.Replace(attr, "");
                    rt = BitConverter.GetBytes(uint.Parse(attr));
                    break;
                case NodeAttributeType.FLOAT:
                    attr = whitespace_remover.Replace(attr, "");
                    rt = BitConverter.GetBytes(float.Parse(attr));
                    break;
                case NodeAttributeType.STRING:
                    rt = Encoding.ASCII.GetBytes(attr);
                    break;
                default:
                    rt = Array.Empty<byte>();
                    break;
            }

            return rt;
        }

        public static XmlElement OpenXml(string path)
        {

            XmlDocument doc = new();
            doc.Load(path);
            PreprocessingContext context = new();
            PathAsFileDir(path, context);
            PreprocessNode(doc, context);

            return doc.DocumentElement;
        }
    }
}

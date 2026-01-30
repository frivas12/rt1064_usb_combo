using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO.Ports;
using System.IO;
using System.Timers;
using Microsoft.Win32;
using System.Threading;
using System.ComponentModel;
using System.Diagnostics;
using System.Xml;
using System.Data;
using System.Collections;
using Microsoft.VisualBasic.FileIO;

namespace MCM6000_UI
{

    public class DeviceSignature
    {
        CardTypes m_slot_type;
        ushort m_device_type;

        public CardTypes SlotType
        {
            get { return m_slot_type; }
            set { m_slot_type = value; }
        }

        public ushort DeviceType
        {
            get { return m_device_type; }
            set { m_device_type = value;}
        }

        public DeviceSignature(CardTypes card_type, ushort device_config)
        {
            m_slot_type = card_type;
            m_device_type = device_config;
        }

        public DeviceSignature()
        {
            m_slot_type = CardTypes.NO_CARD_IN_SLOT;
            m_device_type = 0;
        }

        public override bool Equals (object obj) 
        {
            if (obj != null && obj is DeviceSignature)
            {
                DeviceSignature other = (DeviceSignature)obj;
                return other.m_device_type == this.m_device_type
                    && other.m_slot_type == this.m_slot_type;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return Tuple.Create(m_slot_type, m_device_type).GetHashCode();
        }

    }

    public class ConfigSignature
    {
        ushort _struct_id;
        ushort _config_id;

        public ushort StructID
        {
            get { return _struct_id; }
            set { _struct_id = value; }
        }

        public ushort ConfigID
        {
            get { return _config_id; }
            set { _config_id = value; }
        }

        public ConfigSignature(ushort struct_id, ushort config_id)
        {
            _struct_id = struct_id;
            _config_id = config_id;
        }

        public ConfigSignature()
        {
            _struct_id = 0xFFFF;
            _config_id = 0;
        }

        public override bool Equals(object obj)
        {
            if (obj != null && obj is ConfigSignature)
            {
                ConfigSignature other = (ConfigSignature)obj;
                return other._struct_id == this._struct_id
                    && other._config_id == this._config_id;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return Tuple.Create(_struct_id, _config_id).GetHashCode();
        }

        public static implicit operator ConfigSignature(SystemXMLParser.Utils.Signature<ushort, ushort> sig) => new(sig.PrimaryKey, sig.DiscriminatorKey);
    }

    public class LUT : IEnumerable
    {
        public enum CollisionHandling
        {
            ABORT,
            TAKE_OLD,
            TAKE_NEW,
        }

        private Dictionary<DeviceSignature, byte[]> m_device_signatures = new Dictionary<DeviceSignature, byte[]>();

        public byte[] get_config (DeviceSignature signature)
        {
            byte[] payload = (byte[])m_device_signatures[signature];
            byte[] rt = new byte[2 + payload.Length];
            Array.Copy(payload, rt, 2);
            Array.Copy(payload, 0, rt, 2, payload.Length);
            return rt;
        }

        public bool add_config (DeviceSignature signature, Byte[] data)
        {
            try
            {
                m_device_signatures[signature] = data;
            }
            catch
            {
                return false;
            }

            return true;
        }

        public bool has_config(DeviceSignature signature)
        {
            return m_device_signatures.ContainsKey(signature);
        }

        public bool remove_config(DeviceSignature signature)
        {
            try
            {
                m_device_signatures.Remove(signature);
            } catch
            {
                return false;
            }

            return true;
        }

        public void erase_configs()
        {
            m_device_signatures.Clear();
        }

        public bool extend(LUT lut_to_add, CollisionHandling handling=CollisionHandling.ABORT)
        {
            if (handling == CollisionHandling.ABORT)
            {
                // Check for collisions.
                foreach (KeyValuePair<DeviceSignature, byte[]> entry in lut_to_add.m_device_signatures)
                {
                    DeviceSignature signature = (DeviceSignature)entry.Key;
                    if (lut_to_add.has_config(signature))
                    {
                        return false;
                    }
                }
            }

            // If collisions are resolved, continue.
            foreach (KeyValuePair<DeviceSignature, byte[]> entry in lut_to_add.m_device_signatures)
            {
                DeviceSignature signature = (DeviceSignature)entry.Key;
                try
                {
                    m_device_signatures.Add(signature, lut_to_add.m_device_signatures[signature]);
                }
                catch
                {
                    switch (handling)
                    {
                    case CollisionHandling.TAKE_OLD:
                    break;
                    case CollisionHandling.TAKE_NEW:
                        m_device_signatures[signature] = lut_to_add.m_device_signatures[signature];
                    break;
                    default:
                        return false;
                    }

                }
            }

            return true;
        }

        private class LUTEnumerator : IEnumerator
        {
            private DeviceSignature[] m_signatures;
            private int m_index = -1;

            public LUTEnumerator(LUT lut)
            {
                m_signatures = new DeviceSignature[lut.m_device_signatures.Count];
                int i = 0;
                foreach (KeyValuePair<DeviceSignature, byte[]> entry in lut.m_device_signatures)
                {
                    m_signatures[i] = (DeviceSignature)entry.Key;
                    i++;
                }
            }

            public DeviceSignature Current
            {
                get { return m_signatures[m_index]; }
            }

            object IEnumerator.Current
            {
                get { return Current; }
            }

            public bool MoveNext()
            {
                ++m_index;
                return (m_index < m_signatures.Length);
            }

            public void Reset()
            {
                m_index=-1;
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return new LUTEnumerator(this);
        }
    }

    public partial class MainWindow
    {
        private enum LUTState
        {
            UNKNOWN,
            INQUIRING,
            PROGRAMMING,
            GOOD,
            OUT_OF_MEMORY
        };

        private LUT m_loaded_lut = new LUT();
        private object m_lut_lock = new object();

        private CancellationTokenSource m_lut_cancel_source = new CancellationTokenSource();

        private const int MAX_RESPONSE_WAIT = 2000; // 2 seconds


        private Semaphore m_lut_response_ready = new Semaphore(0, 1);
        private byte[] m_lut_response_buffer = new byte[20]; /// \todo Make this value (a) make sense and (b) not a magic number.


        /**
         * Parses a LUT csv file at the path file_path.
         * A LUT csv file has the structure "slot_type_id, config_id, binary_file_location" for each line.
         * \param[in]       file_path Path to the csv file for the LUT.
         * \return A LUT structure with all of the configurations in the LUT file.
         */
        private LUT lut_parse_source_csv(string file_path)
        {
            LUT lut = new LUT();
            
            using (TextFieldParser parser = new TextFieldParser(file_path))
            {
                parser.TextFieldType = FieldType.Delimited;
                parser.SetDelimiters(",");
                while (!parser.EndOfData)
                {
                    string[] fields = parser.ReadFields();
                    if (fields.Length < 3)
                    {
                        throw new IndexOutOfRangeException();
                    }

                    DeviceSignature sig = new DeviceSignature
                    (
                        (CardTypes)Convert.ToInt32(fields[0]),
                        (ushort)Convert.ToInt16(fields[1])
                    );

                    if (! File.Exists(fields[2]))
                    {
                        throw new FileNotFoundException();
                    }

                    byte[] payload = File.ReadAllBytes(fields[2]);
                    lut.add_config(sig, payload);
                }
            }

            return lut;
        }

        private void safe_set_text(TextBlock obj, string text)
        {
            if (obj.Dispatcher.CheckAccess())
            {
                obj.Text = text;
            } else
            {
                obj.Dispatcher.BeginInvoke(new Action(() => safe_set_text(obj, text)));
            }
        }

        private void safe_set_text(TextBox obj, string text)
        {
            if (obj.Dispatcher.CheckAccess())
            {
                obj.Text = text;
            }
            else
            {
                obj.Dispatcher.BeginInvoke(new Action(() => safe_set_text(obj, text)));
            }
        }

        private void RequestLutLocked()
        {
            byte[] bytes_to_send = new byte[6];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_LUT_REQ_LOCK), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = 0;
            bytes_to_send[3] = 0;
            bytes_to_send[4] = MOTHERBOARD_ID;
            bytes_to_send[5] = HOST_ID;

            usb_write("LUT", bytes_to_send, bytes_to_send.Length);
        }

        private void LutSetLock(bool locked)
        {
            byte[] bytes_to_send = new byte[6];
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MCM_LUT_SET_LOCK), 0, bytes_to_send, 0, 2);
            bytes_to_send[2] = (byte)(locked ? 1 : 0);
            bytes_to_send[3] = 0;
            bytes_to_send[4] = MOTHERBOARD_ID;
            bytes_to_send[5] = HOST_ID;

            usb_write("LUT", bytes_to_send, bytes_to_send.Length);
        }

        private void Button_lut_locked_Clicked(object? sender, RoutedEventArgs args)
        {
            bool to_lock = true;
            Dispatcher.Invoke(() => {
                to_lock = e_chk_lut_locked.IsChecked == true;
            });

            LutSetLock(to_lock);            

            if (to_lock)
            {
                Thread.Sleep(30);
                for (uint i = 0; i < 32; ++i)
                {
                    bool EXISTS = _efs_driver.GetFile(0x60, i) is not null;
                    if (EXISTS)
                    {
                        _efs_driver.LookupFileAsync(0x60, i); // TODO:  Fix this
                    }
                }
            }
        }

        private void LUTSetUILock(bool locked) => Dispatcher.Invoke(() => {
            e_chk_lut_locked.IsChecked = locked;
        });
    }
}
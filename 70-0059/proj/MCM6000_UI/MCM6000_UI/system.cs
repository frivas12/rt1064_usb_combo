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

namespace MCM6000_UI
{
    public enum CardTypes
    {
        NO_CARD_IN_SLOT = 0,
        CARD_IN_SLOT_NOT_PROGRAMMED = 1,
        ST_Stepper_type,                // 2 
        High_Current_Stepper_Card,      // 3 42-0093 MCM6000 HC Stepper Module
        Servo_type,                     // 4 42-0098 MCM Servo Module
        Shutter_type,                   // 5 42-0098 MCM Servo Module
        OTM_Dac_type,                   // 6
        OTM_RS232_type,                 // 7
        High_Current_Stepper_Card_HD,   // 8 42-0107 MCM6000 HC Stepper Module Micro DB15
        Slider_IO_type,                 // 9
        Shutter_4_type,                 // 10 42-0108 MCM6000 4 Shutter Card PCB
        Piezo_Elliptec_type,            // 11
        ST_Invert_Stepper_BISS_type,    // 12 42-0113 MCM Stepper ABS BISS encoder Module
        ST_Invert_Stepper_SSI_type,     // 13 42-0113 MCM Stepper ABS BISS encoder Module
        Piezo_type,                     // 14 42-0123 Piezo Slot Card PCB
        MCM_Stepper_Internal_BISS_L6470,// 15 41-0128 MCM Stepper Internal Slot Card
        MCM_Stepper_Internal_SSI_L6470, // 16 41-0128 MCM Stepper Internal Slot Card
        MCM_Stepper_L6470_MicroDB15,    // 17 41-0129 MCM Stepper LC Micro DB15
        Shutter_4_type_REV6,            // 18 42-0108 MCM6000 4 Shutter Card PCB REV 6
        MCM_Stepper_LC_HD_DB15,         // 19 42-0132  MCM Stepper LC HD DB15
        MCM_Flipper_Shutter,            // 20 42-0161 MCM6000 Flipper Shutter Card
        MCM_Flipper_Shutter_REVA,       // 21 42-0161 MCM6000 Flipper Shutter Card REV A
    }

    public partial class MainWindow
    {
        // Stores the programming path and exposes attributes
        readonly ProgrammingPath _programmingPath = new();

        const byte NUM_OF_SLOTS = 8;

        const bool FLEX_CARD_SLOT = false;
        const bool STATIC_CARD_SLOT = true;

        int[] card_type = new int[NUM_OF_SLOTS];
        bool system_tab_init_done = false;
        int board_type_t = 0x8000;

        byte slot_error;

        public enum BoardTypes
        {
            NO_BOARD = 0x8000,
            BOARD_NOT_PROGRAMMED = 0x8001,
            MCM6000_REV_002,
            MCM6000_REV_003,
            OTM_BOARD_REV_001,
            HEXAPOD_BOARD_REV_001,
            MCM6000_REV_004,
            MCM_41_0117_001,
            MCM_41_0134_001,            // MCM301, Revision 1
            MCM_41_0117_RT1064,         // new rt1064 USB CDC and HID host
        }

        public enum SetSystemTypes
        {
            Mesoscope = 0,
            EMB1 = 1,   // B-scope only Z(5:1)
            EMB3 = 2,   // B-scope only X,Y,Z(5:1)
            EMB4 = 3,   // B-scope only X(5:1),Y,Z(5:1),R(15:1);E (spped adjustx,y)
            Hexapod = 4,
            OTM = 5,
            Cerna_XYR = 6,
            Inverted_Microscope = 7,
        }

        /*        public enum JoystickTypes
                {
                    Clear = 0,
                    Thor_4_knob,
                   // Thor_1_knob,
                    Thor_JS_knob//,
                 //   ShuttleExpress
                }*/


        // BoardTypes board_type = BoardTypes.BOARD_NOT_PROGRAMMED;
        private void system_tab_get_device_info_Click(object sender, RoutedEventArgs e)
        {
            byte[] bytesToSend = new byte[6];

            // send command to get each slot to get device type
            for (byte slot = 0; slot <= 7; slot++)
            {
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_DEVICE));
                bytesToSend = new byte[6] { command[0], command[1], (byte)(slot), 0x00, MOTHERBOARD_ID, HOST_ID };
                try
                {
                    schedule_usb_write("system_tab", bytesToSend, 6);
                }
                catch (TimeoutException)
                {
                    // Try again later.
                    return;
                }
                Thread.Sleep(15);

                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_DEVICE_BOARD));
                bytesToSend = new byte[6] { command[0], command[1], (byte)(slot), 0x00, MOTHERBOARD_ID, HOST_ID };
                try
                {
                    schedule_usb_write("system_tab", bytesToSend, 6);
                }
                catch (TimeoutException)
                {
                    // Try again later.
                    return;
                }
                Thread.Sleep(15);
            }
        }
        private void system_tab_refresh_page_Click(object sender, RoutedEventArgs e)
        {
            system_tab_init_done = false;
            system_tab_init();
        }

        // Response from MGMSG_REQ_DEVICE_TYPE
        public void system_tab_get_devices()
        {
            byte slot = ext_data[0];
            int device_type = ext_data[2] + (ext_data[3] << 8);
            int device_serial_num = ext_data[4] + (ext_data[5] << 8) + (ext_data[6] << 16) +
                (ext_data[7] << 24) + (ext_data[8] << 32) + (ext_data[9] << 40);

            string device_serial_num_t = device_serial_num.ToString();

// Code has been made decreciated since v5.0.0.0
#if false
            this.Dispatcher.Invoke(new Action(() =>
                {
                    switch (slot)
                    {
                        case SLOT1:
                            //tbdevice_serial_num_1.Text = device_serial_num_t;
                            //cb_device_id_d1.SelectedIndex = device_type;
                            break;
                        case SLOT2:
                            tbdevice_serial_num_2.Text = device_serial_num_t;
                            cb_device_id_d2.SelectedIndex = device_type;
                            break;
                        case SLOT3:
                            tbdevice_serial_num_3.Text = device_serial_num_t;
                            cb_device_id_d3.SelectedIndex = device_type;
                            break;
                        case SLOT4:
                            tbdevice_serial_num_4.Text = device_serial_num_t;
                            cb_device_id_d4.SelectedIndex = device_type;
                            break;
                        case SLOT5:
                            tbdevice_serial_num_5.Text = device_serial_num_t;
                            cb_device_id_d5.SelectedIndex = device_type;
                            break;
                        case SLOT6:
                            tbdevice_serial_num_6.Text = device_serial_num_t;
                            cb_device_id_d6.SelectedIndex = device_type;
                            break;
                        case SLOT7:
                            tbdevice_serial_num_7.Text = device_serial_num_t;
                            cb_device_id_d7.SelectedIndex = device_type;
                            break;
                        case SLOT8:
                            tbdevice_serial_num_8.Text = device_serial_num_t;
                            cb_device_id_d8.SelectedIndex = device_type;
                            break;
                        default:
                            break;
                    }
                }));
#endif
        }

        public void system_tab_init()
        {
            if (!system_tab_init_done)
            {
                Thread.Sleep(15);

                if (cbBoardType.Items.Count == 0)
                {
                    foreach (var board_item in Enum.GetValues(typeof(BoardTypes)))
                    {
                        cbBoardType.Items.Add(board_item);
                    }
                }

                if (cbcardType_1.Items.Count == 0)
                {
                    foreach (var card_item in Enum.GetValues(typeof(CardTypes)))
                    {
                        cbcardType_1.Items.Add(card_item);
                        cbcardType_2.Items.Add(card_item);
                        cbcardType_3.Items.Add(card_item);
                        cbcardType_4.Items.Add(card_item);
                        cbcardType_5.Items.Add(card_item);
                        cbcardType_6.Items.Add(card_item);
                        cbcardType_7.Items.Add(card_item);
                        cbcardType_8.Items.Add(card_item);
                    }
                }

                system_tab_get_device_info_Click(null, null);

                // load the Enable loggin info
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_ENABLE_LOG));
                byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, MOTHERBOARD_ID, HOST_ID };
                try
                {
                    usb_write("system_tab", bytesToSend, 6);
                }
                catch (TimeoutException)
                {
                    // This can occur when the CPLD is not programmed.
                }
                Thread.Sleep(15);

                system_tab_init_done = true;
            }
        }

        bool Test_bit(Int16 b, byte pos)
        {
            return ((b >> pos) & 1) != 0;
        }

        private string _serialNumber = "";
        private Version _firmwareVersion = null;
        private string _cpldVersion = "";
        private string _lutVersion = "None";
        private string LUTVersion
        {
            get
            {
                if (IsConnected)
                {
                    return _lutVersion;
                }
                else
                {
                    return "None";
                }
            }
        }

        private void render_version_info()
        {
            string version_info
                = "Serial No.: " + _serialNumber + "    " +
                  "Firmware Rev: " + (_firmwareVersion is null ? "" : _firmwareVersion.ToString()) + "    " +
                  "CPLD Rev: " + _cpldVersion + "    " +
                  "LUT Rev: " + LUTVersion + "    " +
                  "UI Rev: " + software_version;

            safe_set_content(lbl_hardware_info, version_info);
        }

        public void hardware_info(byte[] hwinfo)
        {
            _serialNumber = Encoding.ASCII.GetString(hwinfo, 25, 18);
            string textboxText = _serialNumber.Take(_serialNumber.Length - 1).Any(c => char.IsControl(c))
                                ? string.Empty
                                : _serialNumber;
            safe_set_text(e_tbx_system_serial_number, textboxText);
            /*
                        // serial number                
                        serial_number[0] = (char)hwinfo[6];
                        serial_number[1] = (char)hwinfo[7];
                        serial_number[2] = (char)hwinfo[8];
                        serial_number[3] = (char)hwinfo[9];
            */

            _firmwareVersion = new Version(hwinfo[22], hwinfo[21], hwinfo[20]);

            _cpldVersion = Convert.ToString(hwinfo[23]) + "."
                         + Convert.ToString(hwinfo[24]);

            render_version_info();

            card_type[0] = (Int16)(hwinfo[68] + (hwinfo[69] << 8));
            card_type[1] = (Int16)(hwinfo[70] + (hwinfo[71] << 8));
            card_type[2] = (Int16)(hwinfo[72] + (hwinfo[73] << 8));
            card_type[3] = (Int16)(hwinfo[74] + (hwinfo[75] << 8));
            card_type[4] = (Int16)(hwinfo[76] + (hwinfo[77] << 8));
            card_type[5] = (Int16)(hwinfo[78] + (hwinfo[79] << 8));
            card_type[6] = (Int16)(hwinfo[80] + (hwinfo[81] << 8));
            card_type[7] = (Int16)(hwinfo[82] + (hwinfo[83] << 8));

            _efs_driver.UpdateHwInfo(BitConverter.ToUInt32(hwinfo, 43));

            board_type_t = (hwinfo[84] + (hwinfo[85] << 8));

            string[] card_types = Enum.GetNames(typeof(CardTypes));
            string[] board_types = Enum.GetNames(typeof(BoardTypes));

            byte j = 0;
            Int32 board_type_i = (Int32)BoardTypes.NO_BOARD;

            foreach (int i in Enum.GetValues(typeof(BoardTypes)))
            {
                board_type_i = i;

                if (board_type_t == board_type_i)
                {
                    break;
                }
                j++;
            }

            this.Dispatcher.Invoke(new Action(() =>
            {
                cbBoardType.SelectedIndex = j;

                if (Test_bit((Int16)card_type[0], 15))
                {
                    cb_slot_static_1.IsChecked = true;
                    card_type[0] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_1.IsChecked = false;
                }
                cbcardType_1.SelectedIndex = card_type[0];


                if (Test_bit((Int16)card_type[1], 15))
                {
                    cb_slot_static_2.IsChecked = true;
                    card_type[1] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_2.IsChecked = false;
                }
                cbcardType_2.SelectedIndex = card_type[1];

                if (Test_bit((Int16)card_type[2], 15))
                {
                    cb_slot_static_3.IsChecked = true;
                    card_type[2] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_3.IsChecked = false;
                }
                cbcardType_3.SelectedIndex = card_type[2];

                if (Test_bit((Int16)card_type[3], 15))
                {
                    cb_slot_static_4.IsChecked = true;
                    card_type[3] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_4.IsChecked = false;
                }
                cbcardType_4.SelectedIndex = card_type[3];

                if (Test_bit((Int16)card_type[4], 15))
                {
                    cb_slot_static_5.IsChecked = true;
                    card_type[4] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_5.IsChecked = false;
                }
                cbcardType_5.SelectedIndex = card_type[4];

                if (Test_bit((Int16)card_type[5], 15))
                {
                    cb_slot_static_6.IsChecked = true;
                    card_type[5] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_6.IsChecked = false;
                }
                cbcardType_6.SelectedIndex = card_type[5];

                if (Test_bit((Int16)card_type[6], 15))
                {
                    cb_slot_static_7.IsChecked = true;
                    card_type[6] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_7.IsChecked = false;
                }
                cbcardType_7.SelectedIndex = card_type[6];

                if (Test_bit((Int16)card_type[7], 15))
                {
                    cb_slot_static_8.IsChecked = true;
                    card_type[7] &= 0x7fff;
                }
                else
                {
                    cb_slot_static_8.IsChecked = false;
                }
                cbcardType_8.SelectedIndex = card_type[7];
            }));

        }

        private void Set_board_type_Click(object sender, RoutedEventArgs e)
        {
            byte j = 0;

            byte board_type_index = Convert.ToByte(cbBoardType.SelectedIndex);
            Int32 board_type_i = (Int32)BoardTypes.NO_BOARD;
            string[] board_types = Enum.GetNames(typeof(BoardTypes));

            foreach (int i in Enum.GetValues(typeof(BoardTypes)))
            {
                if (j == board_type_index)
                {
                    board_type_i = i;
                    break;
                }
                j++;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_HW_REV));
            byte[] b_type = BitConverter.GetBytes(board_type_i);

            byte[] bytesToSend = new byte[8] { command[0], command[1], 2, 0x00, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID,
            b_type[0], b_type[1] };
            usb_write("system_tab", bytesToSend, 8);
            Thread.Sleep(100);
        }

        private void OnSerialNumberAck(byte ack_value)
        {
            if (ack_value == 1)
            {
                Restart_processor(false);
            }
        }

        private void SystemSetSerialNumber()
        {
            string sn = e_tbx_system_serial_number.Text;
            try
            {
                byte[] ascii = Encoding.ASCII.GetBytes(sn);

                if (ascii.Length > 17)
                {
                    // Error
                    return;
                }

                byte[] bytes_to_send = new byte[28];
                Array.Copy(BitConverter.GetBytes(MGMSG_SET_SER_TO_EEPROM), 0, bytes_to_send, 0, 2);
                bytes_to_send[2] = 22;
                bytes_to_send[3] = 0;
                bytes_to_send[4] = (byte)(MOTHERBOARD_ID | 0x80);
                bytes_to_send[5] = HOST_ID;
                Array.Copy(Encoding.ASCII.GetBytes("THOR"), 0, bytes_to_send, 6, 4);
                Array.Copy(ascii, 0, bytes_to_send, 10, ascii.Length);
                for (int i = 10 + ascii.Length; i < 28; ++i)
                {
                    bytes_to_send[i] = 0;
                }

                usb_write("system_tab", bytes_to_send, 28);
            }
            catch { }
        }

        private void e_btn_system_serial_number_Click(object sender, RoutedEventArgs args) => SystemSetSerialNumber();

        private void Set_card_type_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;
            bool static_slot = FLEX_CARD_SLOT;

            byte card_type_index = 0;

            switch (slot_number)
            {
                case SLOT1:
                    card_type_index = Convert.ToByte(cbcardType_1.SelectedIndex);
                    static_slot = (bool)cb_slot_static_1.IsChecked;
                    break;
                case SLOT2:
                    card_type_index = Convert.ToByte(cbcardType_2.SelectedIndex);
                    static_slot = (bool)cb_slot_static_2.IsChecked;
                    break;
                case SLOT3:
                    card_type_index = Convert.ToByte(cbcardType_3.SelectedIndex);
                    static_slot = (bool)cb_slot_static_3.IsChecked;
                    break;
                case SLOT4:
                    card_type_index = Convert.ToByte(cbcardType_4.SelectedIndex);
                    static_slot = (bool)cb_slot_static_4.IsChecked;
                    break;
                case SLOT5:
                    card_type_index = Convert.ToByte(cbcardType_5.SelectedIndex);
                    static_slot = (bool)cb_slot_static_5.IsChecked;
                    break;
                case SLOT6:
                    card_type_index = Convert.ToByte(cbcardType_6.SelectedIndex);
                    static_slot = (bool)cb_slot_static_6.IsChecked;
                    break;
                case SLOT7:
                    card_type_index = Convert.ToByte(cbcardType_7.SelectedIndex);
                    static_slot = (bool)cb_slot_static_7.IsChecked;
                    break;
                case SLOT8:
                    card_type_index = Convert.ToByte(cbcardType_8.SelectedIndex);
                    static_slot = (bool)cb_slot_static_8.IsChecked;
                    break;
            }

            byte j = 0;
            Int32 card_type_i = (Int32)BoardTypes.NO_BOARD;
            string[] card_types = Enum.GetNames(typeof(CardTypes));

            foreach (int i in Enum.GetValues(typeof(CardTypes)))
            {
                if (j == card_type_index)
                {
                    card_type_i = i;
                    break;
                }
                j++;
            }

            if (static_slot)
            {
                card_type_i = card_type_i | 0x8000;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_CARD_TYPE));
            byte[] b_type = BitConverter.GetBytes(card_type_i);

            byte[] bytesToSend = new byte[10] { command[0], command[1], 4, 0x00, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID,
            slot_number, 0,
            b_type[0], b_type[1] };
            usb_write("system_tab", bytesToSend, 10);
            Thread.Sleep(100);
        }

        private const string SYSTEM_SN_PROTECT_DISABLE_TEXT = "- Disabled -";
        private void Button_system_disable_sn_protect_Clicked(object sender, RoutedEventArgs args)
        {

            // Sent to disable the slot protection.
            Button? btn = sender as Button;
            if (btn is not null)
            {
                string slot_index = btn.Name.Substring(btn.Name.Length - 1);
                byte index = byte.Parse(slot_index);
                system_set_device_board((byte)(index - 1), 0xFFFF000000000000);
                string textbox_name = "e_tbx_system_protected_sn_" + index.ToString();
                Dispatcher.Invoke(() => {
                    object? textbox_obj = system_tab.FindName(textbox_name);
                    if (textbox_obj is not null)
                    {
                        TextBox? textbox = textbox_obj as TextBox;
                        if (textbox is not null)
                        {
                            textbox.Text = SYSTEM_SN_PROTECT_DISABLE_TEXT;
                        }
                    }
                });
            }
        }

        private ManualResetEventSlim _system_sn_waiter = new(false);
        private byte[] _system_last_read_devices;
        private void system_set_sn_to_current_device(byte slot)
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_DEVICE));
            byte[] bytesToSend = new byte[6] { command[0], command[1], slot, 0x00, MOTHERBOARD_ID, HOST_ID };

            _system_sn_waiter.Reset();
            usb_write("system_tab", bytesToSend, 6);
            Task.Run(() =>
            {
                _system_sn_waiter.Wait();
                ulong sn = BitConverter.ToUInt64(_system_last_read_devices, 4);
                _system_last_read_devices = Array.Empty<byte>();
                system_set_device_board(slot, sn);
            });
        }

        private ManualResetEventSlim _system_allowed_devices_waiter = new(false);
        private byte[] _system_last_read_allowed;
        private void system_allow_currently_connected_device(byte slot)
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_ALLOWED_DEVICES));
            byte[] bytesToSend = new byte[6] { command[0], command[1], slot, 0x00, MOTHERBOARD_ID, HOST_ID };

            _system_allowed_devices_waiter.Reset();
            usb_write("system_tab", bytesToSend, 6);
            Task.Run(() =>
            {
                _system_allowed_devices_waiter.Wait();
                byte[] ad = _system_last_read_allowed.Skip(2).ToArray();
                _system_last_read_allowed = Array.Empty<byte>();
                byte[] command = BitConverter.GetBytes(MGMSG_REQ_DEVICE);
                byte[] bytesToSend = new byte[6] { command[0], command[1], slot, 0x00, MOTHERBOARD_ID, HOST_ID };
                _system_sn_waiter.Reset();
                usb_write("system_tab", bytesToSend, 6);
                Task.Run(() =>
                {
                    _system_sn_waiter.Wait();
                    byte[] device_info = _system_last_read_devices;
                    _system_last_read_devices = Array.Empty<byte>();
                    ushort C_SID = BitConverter.ToUInt16(device_info, 12);
                    ushort C_DID = BitConverter.ToUInt16(device_info, 2);
                    bool set = true;
                    for (int i = 0; i < ad.Length; i += 4)
                    {
                        ushort sid = BitConverter.ToUInt16(ad, i);
                        ushort did = BitConverter.ToUInt16(ad, i + 2);

                        if (sid == C_SID && (did == C_DID || did == 0xFFFF))
                        {
                            set = false;
                            break;
                        }
                    }
                    if (set)
                    {
                        byte[] bytes_to_send = new byte[12 + ad.Length];
                        Array.Copy(BitConverter.GetBytes(MGMSG_MCM_SET_ALLOWED_DEVICES), 0, bytes_to_send, 0, 2);
                        Array.Copy(BitConverter.GetBytes(bytes_to_send.Length - 6), 0, bytes_to_send, 2, 2);
                        bytes_to_send[4] = (byte)(MOTHERBOARD_ID | 0x80);
                        bytes_to_send[5] = HOST_ID;
                        bytes_to_send[6] = slot;
                        bytes_to_send[7] = 0;
                        Array.Copy(ad, 0, bytes_to_send, 8, ad.Length);
                        Array.Copy(BitConverter.GetBytes(C_SID), 0, bytes_to_send, 8 + ad.Length, 2);
                        Array.Copy(BitConverter.GetBytes(C_DID), 0, bytes_to_send, 10 + ad.Length, 2);

                        usb_write("system_tab", bytes_to_send, bytes_to_send.Length);
                    }
                });
            });
        }

        private void system_set_sn(object sender, RoutedEventArgs args)
        {
            // Set the textbox's serial number to the device.
            Button? btn = sender as Button;
            if (btn is not null)
            {
                string slot_index = btn.Name.Substring(btn.Name.Length - 1);
                byte index = byte.Parse(slot_index);
                string textbox_name = "e_tbx_system_protected_sn_" + index.ToString();
                ulong? serial_number = null;
                Dispatcher.Invoke(() => {
                    object? textbox_obj = system_tab.FindName(textbox_name);
                    if (textbox_obj is not null)
                    {
                        TextBox? textbox = textbox_obj as TextBox;
                        if (textbox is not null)
                        {
                            try
                            {
                                serial_number = SystemXMLParser.Utils.ParseHexableUInt64(textbox.Text);
                                serial_number &= 0x0000FFFFFFFFFFFF; // Masking the upper 16 bits.
                            }
                            catch { }
                        }
                    }
                });
                if (serial_number is not null)
                {
                    system_set_device_board((byte)(index - 1), (ulong)serial_number);
                }
            }
        }

        /**
         * Sets the system-protected serial number.
         * Set to 0xFFFF000000000000 to disable serial-number protection
         */
        private void system_set_device_board(byte index, ulong serial_number)
        {
            byte[] bytes_to_send = new byte[16]
            {
                0, 0, 10, 0, Thorlabs.APT.Address.MOTHERBOARD | 0x80, Thorlabs.APT.Address.HOST,
                index, 0,                   // Slot Address
                0, 0, 0, 0, 0, 0, 0, 0      // Serial Number
            };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_SET_DEVICE_BOARD), 0, bytes_to_send, 0, 2);
            Array.Copy(BitConverter.GetBytes(serial_number), 0, bytes_to_send, 8, 8);

            usb_write("system_tab", bytes_to_send, bytes_to_send.Length);
        }
        private void system_req_device_board(byte index)
        {
            byte[] command = BitConverter.GetBytes(Thorlabs.APT.MGMSG_REQ_DEVICE_BOARD);
            byte[] bytesToSend = new byte[6] { command[0], command[1], index, 0, Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.Address.HOST };
            usb_write("system_tab", bytesToSend, bytesToSend.Length);
        }

        private void system_get_device_board(byte index, ulong serial_number)
        {
            string textbox_name = "e_tbx_system_protected_sn_" + (index + 1).ToString();
            string text_to_set = serial_number switch
            {
                0xFFFFFFFFFFFFFFFF => "- Not Set -",
                0xFFFF000000000000 => SYSTEM_SN_PROTECT_DISABLE_TEXT,
                _                  => "0x" + (serial_number & 0x0000FFFFFFFFFFFF).ToString("X06"),
            };
            Dispatcher.Invoke(() =>
            {
                object? textbox_obj = system_tab.FindName(textbox_name);
                if (textbox_obj is not null)
                {
                    TextBox? textbox = textbox_obj as TextBox;
                    if (textbox is not null)
                    {

                        textbox.Text = text_to_set;
                    }
                }
            });
        }

        private bool system_requested_allowed_devices = false;
        private void View_allowed_devices_Callback(byte slot_number, byte[] extraData)
        {
            List<AllowedDevices.TableEntry> allowed_devs = new List<AllowedDevices.TableEntry>();

            for (int i = 2; i < extraData.Length; i += 4)
            {
                DeviceSignature sig = new DeviceSignature(
                    (CardTypes)BitConverter.ToUInt16(extraData, i),
                    BitConverter.ToUInt16(extraData, i + 2)
                );

                allowed_devs.Add(new AllowedDevices.TableEntry(sig.SlotType.ToString(), sig.DeviceType));
            }

            AllowedDevicesWindow ad = new AllowedDevicesWindow(slot_number + 1, allowed_devs);
            if (ad.ShowDialog() == true)
            {
                // Set currently allowed devices.
                int d_size = 4 * allowed_devs.Count;
                byte[] command = BitConverter.GetBytes(Convert.ToUInt16(MGMSG_MCM_SET_ALLOWED_DEVICES));
                byte[] ad_size = BitConverter.GetBytes(d_size + 2);

                byte[] bytesToSend = new byte[8 + d_size];
                bytesToSend[0] = command[0];
                bytesToSend[1] = command[1];
                bytesToSend[2] = ad_size[0];
                bytesToSend[3] = ad_size[1];
                bytesToSend[4] = (byte)(MOTHERBOARD_ID | 0x80);
                bytesToSend[5] = HOST_ID;
                bytesToSend[6] = slot_number;
                bytesToSend[7] = 0x00;

                for (int i = 0; i < allowed_devs.Count; i++)
                {
                    CardTypes ct = (CardTypes)Enum.Parse(typeof(CardTypes), allowed_devs[i].SlotType);
                    byte[] st = BitConverter.GetBytes((ushort)ct);
                    byte[] dt = BitConverter.GetBytes(allowed_devs[i].DeviceID);

                    Array.Copy(st, 0, bytesToSend, 8 + 4 * i, 2);
                    Array.Copy(dt, 0, bytesToSend, 10 + 4 * i, 2);
                }

                usb_write("system_tab", bytesToSend, 8 + d_size);
            }
        }

        private void View_allowed_devices(object sender, RoutedEventArgs e)
        {
            system_requested_allowed_devices = true;
            Button clicked_button = (Button)sender;
            string button_name = clicked_button.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;

            // \todo USB Request currently allowed devices.
            byte[] command = BitConverter.GetBytes(Convert.ToUInt16(MGMSG_MCM_REQ_ALLOWED_DEVICES));
            byte[] bytesToSend = new byte[6] {
                command[0],
                command[1],
                slot_number,
                0x00,
                MOTHERBOARD_ID,
                HOST_ID
            };

            usb_write("system_tab", bytesToSend, 6);
        }

        private void DisableDD_Clicked(object sender, RoutedEventArgs e)
        {
            CheckBox clicked_button = (CheckBox)sender;
            string button_name = clicked_button.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;

            if ((bool)clicked_button.IsChecked)
            {
                MessageBoxResult rt = MessageBox.Show("Disabling device-detection will force the system to drive the output, no matter what is or is not on it.  This can cause damage to the system and/or stage.  Are you sure you want to disable device detection?", "WARNING", MessageBoxButton.YesNo);

                if (rt == MessageBoxResult.Yes)
                {
                    byte[] command = BitConverter.GetBytes(Convert.ToUInt16(MGMSG_MCM_SET_DEVICE_DETECTION));
                    byte[] bytesToSend = new byte[9] {
                        command[0],
                        command[1],
                        0x03,
                        0x00,
                        (byte)(MOTHERBOARD_ID | 0x80),
                        HOST_ID,
                        slot_number,
                        0x00,
                        0x00
                    };

                    usb_write("system_tab", bytesToSend, 9);
                }
                else
                {
                    clicked_button.IsChecked = false;
                }
            } else
            {
                // Send Enable DD command.
                byte[] command = BitConverter.GetBytes(Convert.ToUInt16(MGMSG_MCM_SET_DEVICE_DETECTION));
                byte[] bytesToSend = new byte[9] {
                    command[0],
                    command[1],
                    0x03,
                    0x00,
                    (byte)(MOTHERBOARD_ID | 0x80),
                    HOST_ID,
                    slot_number,
                    0x00,
                    0x01
                };
                usb_write("system_tab", bytesToSend, 9);
            }

            _stepper_initialized[slot_number] = false;
            
        }

        // Code has been made decreciated since v5.0.0.0
#if false
        private void Set_device_type_Click(objeslot_numberct sender, RoutedEventArgs e)
        {

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;   // slot starts on 0
            Byte slot_id = (Byte)(slot_number | 0x21);

            int device_type = 0;

            switch (slot_number)
            {
                case SLOT1:
                    //device_type = cb_device_id_d1.SelectedIndex;
                    break;
                case SLOT2:
                    device_type = cb_device_id_d2.SelectedIndex;
                    break;
                case SLOT3:
                    device_type = cb_device_id_d3.SelectedIndex;
                    break;
                case SLOT4:
                    device_type = cb_device_id_d4.SelectedIndex;
                    break;
                case SLOT5:
                    device_type = cb_device_id_d5.SelectedIndex;
                    break;
                case SLOT6:
                    device_type = cb_device_id_d6.SelectedIndex;
                    break;
                case SLOT7:
                    device_type = cb_device_id_d7.SelectedIndex;
                    break;
                case SLOT8:
                    device_type = cb_device_id_d8.SelectedIndex;
                    break;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_DEVICE));
            byte[] device_part_number = BitConverter.GetBytes(device_type);

            byte[] bytesToSend = new byte[6 + 4] { command[0], command[1], 4, 0x00, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID,
            slot_number, 0x00,
            device_part_number[0], device_part_number[1]
            };
            usb_write(bytesToSend, 6 + 4);
            Thread.Sleep(100);
        }

        public string get_serial_number(byte slot)
        {
            string serial_number_s = "";
            slot += 1;

            // system tab textbox for slot device serial number
            TextBox tb_name = (TextBox)this.FindName("tbdevice_serial_num_" + slot.ToString());

            // system tab textbox for slot match device serial number
            TextBox tb_match_name = (TextBox)this.FindName("tbMatch_serial_num_" + slot.ToString());

            // system tab combobox for slot device serial number match
            ComboBox cb_match_name = (ComboBox)this.FindName("cb_device_id_m" + slot.ToString());

            // copy the serial number from the device
            if (cb_match_name.SelectedIndex == 0)
            {
                serial_number_s = "0";
                // update the boad side textbox for the serial number copied from the cable
                this.Dispatcher.Invoke(new Action(() => { tb_match_name.Text = "0"; }));
            }
            else
            {
                //serial_number_s = tbdevice_serial_num_1.Text;
                // update the boad side textbox for the serial number copied from the cable
                this.Dispatcher.Invoke(new Action(() => { tb_match_name.Text = tb_name.Text; }));
            }
            return serial_number_s;
        }

        private void Set_device_type_m_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;   // slot starts on 0
            Byte slot_id = (Byte)(slot_number | 0x21);

            int device_type = 0;
            string device_serial_num_t = "0";

           // device_serial_num_t = get_serial_number(slot_number);
            // system tab combobox for slot device serial number match
            ComboBox cb_name_match = (ComboBox)this.FindName("cb_device_id_m" + slot.ToString());
            device_type = cb_name_match.SelectedIndex;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_DEVICE_BOARD));
            byte[] device_part_number = BitConverter.GetBytes(device_type);
            byte[] device_serial_number = BitConverter.GetBytes(Convert.ToInt64(device_serial_num_t));

            byte[] bytesToSend = new byte[6 + 4 ] { command[0], command[1], 4, 0x00, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID,
            slot_number, 0x00,
            device_part_number[0], device_part_number[1],
          //  device_serial_number[0], device_serial_number[1], device_serial_number[2], device_serial_number[3],
           // device_serial_number[4], device_serial_number[5],device_serial_number[6], device_serial_number[7]
            };
            usb_write(bytesToSend, 6 + 4 );
            Thread.Sleep(100);
        }
#endif

        private void Restart_processor(bool restart_ui)
        {
            // restart the MCM6000
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_RESTART_PROCESSOR));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 1, 0x00, MOTHERBOARD_ID, HOST_ID };
            usb_write("system_tab", bytesToSend, 6);

            if (restart_ui)
            {
                // restart the application and close this window
                System.Diagnostics.Process.Start(Application.ResourceAssembly.Location);
                Application.Current.Shutdown();
            }
        }

        private void SuspendTask_Click(object sender, RoutedEventArgs e)
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_TASK_CONTROL));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 1, 0x00, MOTHERBOARD_ID, HOST_ID };
            usb_write("system_tab", bytesToSend, 6);
        }

        private void ResumeTask_Click(object sender, RoutedEventArgs e)
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_TASK_CONTROL));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, MOTHERBOARD_ID, HOST_ID };
            usb_write("system_tab", bytesToSend, 6);
        }

        private void erase_eeprom_Click(object sender, RoutedEventArgs e)
        {
            MessageBoxResult result = MessageBox.Show("Are you sure you want to erase EEPROM memory?",
                "Confirmation", MessageBoxButton.YesNoCancel);
            if (result == MessageBoxResult.Yes)
            {
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_ERASE_EEPROM));
                byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, MOTHERBOARD_ID, HOST_ID };
                usb_write("system_tab", bytesToSend, 6);
            }
            else if (result == MessageBoxResult.No)
            {
                // No code here  
            }
            else
            {
                // Cancel code here  
            }
        }

        /* Board update request info on _timer1_Elapsed*/
        public void board_update_request()
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_BOARD_REQ_STATUSUPDATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };
            usb_write("system_tab", bytesToSend, 6);
        }

        /* Board update response info on _timer1_Elapsed*/

        const int TEMP_SAMPLES = 15;
        double[] temperatures = new double[TEMP_SAMPLES];
        int temperatures_index = 0;
        bool temperature_ready = false;
        double sum_temperatures = 0;

        const double B = 3930.0;
        const double T0 = 298.0;
        const double R2 = 10000.0;
        const double R0 = 10000.0;
        const double VCC = 3.3;
        const double MAX_VAL = 4095;    // 12 bit
        const double volts_per_counts = VCC / MAX_VAL;
        const double vin_resistor_div = 0.083170; // R2/(R1+r2) // rev 004 has different resistor

        double adc1;

        public void board_update()
        {
            // temperature
            UInt16 temperature = (UInt16)(ext_data[0] + (ext_data[1] << 8));
            // Vin monitor
            UInt16 vin_monitor = (UInt16)(ext_data[2] + (ext_data[3] << 8));
            // CPU Temperature
            double cpu_temperature = (UInt16)(ext_data[4] + (ext_data[5] << 8));
            // slot error bits, one bit for each slot to indicate an error has disabled this slot
            slot_error = ext_data[6];

            double temperature_c_avg = 0;

            if (temperatures_index < TEMP_SAMPLES)
            {
                temperatures[temperatures_index++] = temperature;
            }
            else
            {
                temperatures_index = 0;
                temperature_ready = true;
            }

            // get the average
            if (temperature_ready)
            {
                sum_temperatures = 0;
                for (int i = 0; i < TEMP_SAMPLES; i++)
                {
                    sum_temperatures += temperatures[i];
                }
                temperature_c_avg = sum_temperatures / TEMP_SAMPLES;
            }
            else
            {
                temperature_c_avg = temperature;
            }

            adc1 = (temperature_c_avg * VCC) / 4096.0;

            // in Fahrenheit 
            double temperature_f = (((T0 * B) / (Math.Log(R2 * (VCC - adc1) / (R0 * adc1)) * T0 + B)) - 273) * 1.8 + 32;
            double temperature_c = (temperature_f - 32) * 5 / 9;

            double temperature_v = temperature * volts_per_counts; // voltage at resistor divider 

            // Vin monitor
            double vm = vin_monitor * volts_per_counts;

            double vin = (double)vm / 0.0661; // rev004 and up have a 976 Ohm resistor 
            if (board_type_t < (int)BoardTypes.MCM6000_REV_004)
                vin = (double)vm / 0.083170;  // rev003 and before have a 1270 Ohm resistor 

            // CPU Temperature
            // The output voltage VT = 0.72V at 27C and the temperature slope dVT/dT = 2.33 mV/C
            double cpu_temp1 = ((cpu_temperature * 3.3) / 4095) * 1000;
            double cpu_temp_c = ((cpu_temp1 - 720.00) * (100.00 / 233)) + 27.000;
            double cpu_temp_f = (cpu_temp_c * 1.8) + 32;

            this.Dispatcher.Invoke(new Action(() =>
            {
                // temperature
                lbl_tem_status.Content = "Temperature:  ";
                lbl_tem_status.Content += temperature_f.ToString("###.");
                lbl_tem_status.Content += " F   ";
                lbl_tem_status.Content += temperature_c.ToString("###.");
                lbl_tem_status.Content += " C   ";

                // Vin monitor
                lbl_tem_status.Content += "             Vin (Volts): ";
                lbl_tem_status.Content += vin.ToString("##.0");

                // CPU Temerature
                lbl_tem_status.Content += "             CPU Temperature:  ";
                lbl_tem_status.Content += cpu_temp_f.ToString("###.");
                lbl_tem_status.Content += " F   ";
                lbl_tem_status.Content += cpu_temp_c.ToString("###.");
                lbl_tem_status.Content += " C   ";

                // errors
                lbl_tem_status.Content += "         Slot Card Errors: ";
                lbl_tem_status.Content += Convert.ToString(slot_error, 2).PadLeft(8, '0');
            }));
        }

        private void Image_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left && e.ClickCount == 2)
            {
                tbx_password.Visibility = Visibility.Visible;
            }
        }

        private void tbx_password_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                // check the password
                if (tbx_password.Text == "virginia")
                {
                    add_tabs();
                }
                else
                {
                    remove_tabs();
                }
                tbx_password.Visibility = Visibility.Hidden;
            }
        }

        private void cb_enable_logging_Updated(object sender, RoutedEventArgs e)
        {
            byte enabled = 0;

            if ((bool)cb_enable_logging.IsChecked)
                enabled = 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_ENABLE_LOG));
            byte[] bytesToSend = new byte[6] { command[0], command[1], enabled, 0x00, MOTHERBOARD_ID, HOST_ID };
            usb_write("system_tab", bytesToSend, 6);
            Thread.Sleep(50);
        }

        private void BrowseProgrammingFileClicked(object sender, RoutedEventArgs e)
        {
            OpenProgrammingFileDialog();
        }

        // Response from MGMSG_MCM_GET_ENABLE_LOG
        public void system_tab_get_enable_logging()
        {
            byte enable_log_t = header[2];
            bool enable_log = false;

            if (enable_log_t == 1)
                enable_log = true;

            this.Dispatcher.Invoke(new Action(() => { cb_enable_logging.IsChecked = (bool)enable_log; }));
        }

        private void safe_set_checked(CheckBox cb, bool is_checked)
        {
            if (ow_status.Dispatcher.CheckAccess())
            {
                cb.IsChecked = is_checked;
            }
            else
            {
                cb.Dispatcher.BeginInvoke(new Action(() => safe_set_checked(cb, is_checked)));
            }
        }

        public void system_request_device_detection_Callback (uint slot_num, bool device_detection_enabled)
        {
            CheckBox target = null;

            switch (slot_num)
            {
                case SLOT1:
                    target = cbDisableDeviceDetection_1;
                    break;
                case SLOT2:
                    target = cbDisableDeviceDetection_2;
                    break;
                case SLOT3:
                    target = cbDisableDeviceDetection_3;
                    break;
                case SLOT4:
                    target = cbDisableDeviceDetection_4;
                    break;
                case SLOT5:
                    target = cbDisableDeviceDetection_5;
                    break;
                case SLOT6:
                    target = cbDisableDeviceDetection_6;
                    break;
                case SLOT7:
                    target = cbDisableDeviceDetection_7;
                    break;
                case SLOT8:
                    target = cbDisableDeviceDetection_8;
                    break;
            }

            if (target != null)
            {
                safe_set_checked(target, !device_detection_enabled);
            }
        }

        public void system_request_device_detection(uint slot_num)
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_DEVICE_DETECTION));
            byte[] bytesToSend = new byte[6] { command[0], command[1], (byte)slot_num, 0x00, MOTHERBOARD_ID, HOST_ID };
            usb_write("system_tab", bytesToSend, 6);
        }

        private bool confirm_allow_slot_config_reset()
        {
            MessageBoxResult rt = MessageBox.Show("Are you sure you want to erase parameters?", "WARNING", MessageBoxButton.YesNo);


            return rt == MessageBoxResult.Yes;
        }

        private void reset_slot_card_configuration(int slot_id)
        {
            byte[] command = BitConverter.GetBytes(Convert.ToInt16(MGMSG_MCM_ERASE_DEVICE_CONFIGURATION));
            byte[] bytesToSend = new byte[8] { command[0], command[1], 2, 0, (byte)((SLOT_ID_BASE + slot_id) | 0x80), HOST_ID, (byte)slot_id, 0x00 };
            usb_write("system_tab", bytesToSend, 8);
        }

        private void OpenProgrammingFileDialog()
        {
            OpenFileDialog dlg = new()
            {
                Title = "Select the programming file.",
                Filter = "Supported Files (*.mcmpkg;*.hex;*.jed,*.zip)|*.mcmpkg;*.hex;*.jed;*.zip"
                    + "|MCM Packages (*.mcmpkg)|*.mcmpkg"
                    + "|Intel HEX Files (*.hex)|*.hex"
                    + "|JED Files (*.jed)|*.jed"
                    + "|Zip Files (*.zip)|*.zip",
                CheckFileExists = true,
                Multiselect = false,
            };
            bool? result = dlg.ShowDialog();

            if (result.HasValue && result.Value)
            {
                _programmingPath.Path = dlg.FileName;

                Dispatcher.Invoke(() =>
                {
                    e_tbx_progf_path.Text = _programmingPath.Path;
                    e_cbx_progf_firmware.IsChecked = _programmingPath.ProvidesFirmware;
                    e_cbx_progf_cpld.IsChecked = _programmingPath.ProvidesCPLD;
                    e_cbx_progf_lut.IsChecked = _programmingPath.ProvidesLUT;

                    btn_select_firmware_file.IsEnabled = _programmingPath.ProvidesFirmware;
                    btOpen_cpld_file.IsEnabled = _programmingPath.ProvidesCPLD;
                });
            }
        }
    }
}

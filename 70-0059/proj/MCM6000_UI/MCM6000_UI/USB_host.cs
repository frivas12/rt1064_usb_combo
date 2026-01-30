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

namespace MCM6000_UI
{
    public partial class MainWindow
    {
        const byte MAX_NUM_PORTS = 7+1;
        const byte MAX_NUM_CONTROLS = 16;
        const byte HID_USAGE_PAGE_LED = 0x08;
        const byte SIZE_OF_IN = 0x15;
        const byte SIZE_OF_OUT = 0x0F;
        const byte USB_HID_SEND_DELAY = 10;

        public struct MapInfoIn
        {
            public byte number;
            //public byte port;
            public UInt16 vid;
            public UInt16 pid;
            public byte control_number;
            public byte mode;
            public byte modify_control_port;
            public UInt16 modify_control_ctl_num;
            public byte destination_slot;
            public byte destination_bit;
            public byte destination_port;
            public byte destination_virtual;
            public byte modify_speed;
            public byte revserse_dir;
            public byte dead_band;
        }
        List<List<MapInfoIn>> mapping_in_data = new List<List<MapInfoIn>>();

        public struct MapInfoOut
        {
            public byte number;
            public byte control_number;

            public byte type;
            public UInt16 vid;
            public UInt16 pid;
            public byte mode;
            public byte color_1_id;
            public byte color_2_id;
            public byte color_3_id;
            public byte source_slot;
            public byte source_bit;
            public byte source_port;
            public byte source_virtual;
        }
        List<List<MapInfoOut>> mapping_out_data = new List<List<MapInfoOut>>();

        string type_s = "";
        string type_s_mode = "";
        StackPanel[,] panel_in = new StackPanel[MAX_NUM_PORTS, MAX_NUM_CONTROLS];
        StackPanel[,] panel_out = new StackPanel[MAX_NUM_PORTS, MAX_NUM_CONTROLS];
        uint[,] input_control_type = new uint[MAX_NUM_PORTS, MAX_NUM_CONTROLS]; // 8 ports, max number of controls
        uint[,] output_control_type = new uint[MAX_NUM_PORTS, MAX_NUM_CONTROLS]; // 8 ports, max number of controls
        int[] num_of_input_controls = new int[MAX_NUM_PORTS];
        int[] num_of_output_controls = new int[MAX_NUM_PORTS];
        byte byte_to_read = 0;
        byte button_num;
        bool[] save_flags_in = new bool[MAX_NUM_PORTS];
        bool[] save_flags_out = new bool[MAX_NUM_PORTS];
        int[] controls_awaiting = Enumerable.Repeat(0, MAX_NUM_PORTS).ToArray();

        // called when tab is selected    
        public void usb_host_update()
        {
            byte[] bytesToSend = new byte[6];

            /* root port is at address 1 but display on UI as 0*/
            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {
                save_flags_in[_port] = false;
                save_flags_out[_port] = false;

                clear_all_in_map_controls_for_port(_port);
                clear_all_out_map_controls_for_port(_port);

                byte[] command;

                void Send(byte[] toSend, int length)
                {
                    schedule_usb_write("usb_host_update", toSend, length);
                }

                // MGMSG_MOD_REQ_JOYSTICK_INFO
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_REQ_JOYSTICK_INFO));
                bytesToSend = new byte[6] { command[0], command[1], _port, 0x00, MOTHERBOARD_ID, HOST_ID };
                Send(bytesToSend, 6);

                // MGMSG_MOD_REQ_JOYSTICK_MAP_IN
                for (byte _channel_num = 0; _channel_num < MAX_NUM_CONTROLS; _channel_num++)
                {
                    command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_REQ_JOYSTICK_MAP_IN));
                    bytesToSend = new byte[6] { command[0], command[1], _port, _channel_num, MOTHERBOARD_ID, HOST_ID };
                    Send(bytesToSend, 6);
                }

                // MGMSG_MOD_REQ_JOYSTICK_MAP_OUT
                for (byte _channel_num = 0; _channel_num < MAX_NUM_CONTROLS; _channel_num++)
                {
                    command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_REQ_JOYSTICK_MAP_OUT));
                    bytesToSend = new byte[6] { command[0], command[1], _port, _channel_num, MOTHERBOARD_ID, HOST_ID };
                    Send(bytesToSend, 6);
                }
            }
        }

        // called from parse_apt as callback from returned info from APT command above MGMSG_LA_REQ_JOYSTICK_INFO
        public void usb_device_info()
        {
            Int32 VID, PID;
            string speed_txt;
            string device_type;
            string port_or_control_string;
            byte port = 0;

            // add a in mapping list for each port
            if (mapping_in_data.Count == 0)
            {
                for (int i = 0; i < MAX_NUM_PORTS; i++)
                {
                    mapping_in_data.Add(new List<MapInfoIn>());
                }
            }

            // add a out mapping list for each port
            if (mapping_out_data.Count == 0)
            {
                for (int i = 0; i < MAX_NUM_PORTS; i++)
                {
                    mapping_out_data.Add(new List<MapInfoOut>());
                }
            }


            byte_to_read = 0;

            /* Port*/
            port = (byte)(ext_data[byte_to_read++]);

            /* VID */
            VID = ext_data[byte_to_read] | ext_data[byte_to_read + 1] << 8;
            byte_to_read += 2;

            /* PID */
            PID = ext_data[byte_to_read] | ext_data[byte_to_read + 1] << 8;
            byte_to_read += 2;

            /* speed and hub */
            int speed = (ext_data[byte_to_read] >> 1) & 0x01;

            if (speed == 1)
            {
                speed_txt = " | Lowspeed";
            }
            else
            {
                speed_txt = " | Highspeed";
            }

            int is_hub = ext_data[byte_to_read++] & 0x01;

            num_of_input_controls[port] = ext_data[byte_to_read++];
            num_of_output_controls[port] = ext_data[byte_to_read++];

            if (is_hub == 1)
            {
                device_type = "(HUB) | ";
                port_or_control_string = "Ports";
            }
            else
            {
                device_type = "(Device) | ";
                port_or_control_string = "Controls";

                controls_awaiting[port] = num_of_input_controls[port] + num_of_output_controls[port];

                byte[] command = new byte[9]
                {
                    0, 0, 3, 0, Thorlabs.APT.Address.MOTHERBOARD | 0x80, Thorlabs.APT.Address.HOST,
                    port, 0, 0
                };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOD_REQ_JOYSTICK_CONTROL), 0, command, 0, 2);

                command[7] = 0;
                for (int i = 0; i < num_of_input_controls[port]; ++i)
                {
                    command[8] = (byte)i;
                    usb_write("usb_device_info", command, command.Length);
                    Thread.Sleep(USB_HID_SEND_DELAY);
                }

                command[7] = 1;
                for (int i = 0; i < num_of_output_controls[port]; ++i)
                {
                    command[8] = (byte)i;
                    usb_write("usb_device_info", command, command.Length);
                    Thread.Sleep(USB_HID_SEND_DELAY);
                }
            }

            // Wait just a little bit more for a response

            Task.Run(() =>
            {
                while (controls_awaiting[port] > 0)
                {
                    Thread.Yield();
                }

                var port_panel = get_port_panel(port);    // set the stack panel we are working on

                this.Dispatcher.Invoke(new Action(() =>
                {
                    port_panel.Content = "Port: " + port.ToString() + " ";
                    port_panel.Content += device_type;
                    if (VID == 0)
                    {
                        port_panel.Content = "Port: " + port.ToString();
                        port_panel.Content += " No Device";
                        return;
                    }
                    else
                    {
                        port_panel.Content += " | VID: " + VID.ToString("X4");
                        port_panel.Content += " | PID: " + PID.ToString("X4");
                        port_panel.Content += speed_txt;
                        if (is_hub == 1)
                        {
                            port_panel.Content += " | " + num_of_input_controls[port].ToString() + " " + port_or_control_string;
                            return;
                        }
                        else
                        {
                            // number of input controls
                            port_panel.Content += "\n\n" + num_of_input_controls[port].ToString() + " Input " + port_or_control_string + "\n";
                            display_input_controls(port);
                            // number of output controls
                            port_panel.Content += "\n\n" + num_of_output_controls[port].ToString() + " Output " + port_or_control_string + "\n";
                            display_output_controls(port);
                        }
                    }

                }));
            });

        }

        public void usb_control_info(byte address, bool isOut, byte controlNumber, uint usage)
        {
            uint port = (byte)(address - 1);

            if (isOut)
            {
                output_control_type[port, controlNumber] = usage;
            }
            else
            {
                input_control_type[port, controlNumber] = usage;
            }

            --controls_awaiting[port];
        }

        // called from parse_apt as callback from returned info from APT command above MGMSG_MOD_REQ_JOYSTICK_MAP_IN
        public void usb_device_mapping_in()
        {
            byte_to_read = 0;

            int c = ext_data.Length;
            if (c < SIZE_OF_IN)
                return;

            /* Port*/
            byte _port = (byte)(ext_data[0]);
            byte _control_num = (byte)(ext_data[1]);

            if (ext_data[14] == 0xff)
            {
                return;
            }

            MapInfoIn mi = new MapInfoIn();
            mi.number = (byte)mapping_in_data[_port].Count;

            mi.control_number = _control_num;
            mi.vid = (UInt16)(ext_data[2] + (ext_data[3] << 8));
            mi.pid = (UInt16)(ext_data[4] + (ext_data[5] << 8));
            mi.modify_control_port = ext_data[6];
            mi.modify_control_ctl_num = (UInt16)(ext_data[7] + (ext_data[8] << 8));
            mi.destination_slot = ext_data[9];
            mi.destination_bit = ext_data[10];
            mi.destination_port = ext_data[11];
            mi.destination_virtual = ext_data[12];
            mi.modify_speed = ext_data[13];
            mi.revserse_dir = ext_data[14];
            mi.dead_band = ext_data[15];
            mi.mode = ext_data[16];
            add_map_in(_port, ref mi);
            add_in_map_to_ui(_port, mi.number);
        }

        // called from parse_apt as callback from returned info from APT command above MGMSG_MOD_REQ_JOYSTICK_MAP_OUT
        public void usb_device_mapping_out()
        {
            byte_to_read = 0;

            int c = ext_data.Length;
            if (c < SIZE_OF_OUT)
                return;

            /* Port*/
            byte _port = (byte)(ext_data[0]);
            byte _control_num = (byte)(ext_data[1]);

            if (ext_data[2] == 0xff)
            {
                return;
            }

            MapInfoOut mi = new MapInfoOut();
            mi.number = (byte)mapping_out_data[_port].Count;

            mi.control_number = _control_num;
            mi.type = ext_data[2];
            mi.vid = (UInt16)(ext_data[3] + (ext_data[4] << 8));
            mi.pid = (UInt16)(ext_data[5] + (ext_data[6] << 8));
            mi.mode = ext_data[7];
            mi.color_1_id = ext_data[8];
            mi.color_2_id = ext_data[9];
            mi.color_3_id = ext_data[10];
            mi.source_slot = ext_data[11];
            mi.source_bit = ext_data[12];
            mi.source_port = ext_data[13];
            mi.source_virtual = ext_data[14];
            add_map_out(_port, ref mi);
            add_out_map_to_ui(_port, mi.number);
        }

        private void Refresh_Click(object sender, RoutedEventArgs e)
        {
            
            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {
                var port_panel = get_port_panel(_port);    // set the stack panel we are working on
                this.Dispatcher.Invoke(new Action(() =>
                {
                    port_panel.Content = "Port " + _port.ToString() + " (querying...)";
                    }));

                num_of_input_controls[_port] = 0;
                num_of_output_controls[_port] = 0;
            }

            usb_host_update();
        }

        public Label get_port_panel(byte _port)
        {
            switch (_port)
            {
                case 0:
                    return p0;
                case 1:
                    return p1;
                case 2:
                    return p2;
                case 3:
                    return p3;
                case 4:
                    return p4;
                case 5:
                    return p5;
                case 6:
                    return p6;
                case 7:
                    return p7;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public WrapPanel get_map_panel_in(byte _port)
        {
            switch (_port)
            {
                case 0:
                    return map_0;
                case 1:
                    return map_1;
                case 2:
                    return map_2;
                case 3:
                    return map_3;
                case 4:
                    return map_4;
                case 5:
                    return map_5;
                case 6:
                    return map_6;
                case 7:
                    return map_7;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public WrapPanel get_map_panel_out(byte _port)
        {
            switch (_port)
            {
                case 0:
                    return map_out_0;
                case 1:
                    return map_out_1;
                case 2:
                    return map_out_2;
                case 3:
                    return map_out_3;
                case 4:
                    return map_out_4;
                case 5:
                    return map_out_5;
                case 6:
                    return map_out_6;
                case 7:
                    return map_out_7;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public void get_type_string(uint usage)
        {
            ushort usagePage = (ushort)(usage >> 16);
            ushort usageId = (ushort)(usage & 0x00FF);

            switch(usagePage)
            {
                case 0x1:
                    switch (usageId)
                    {
                        case 0x30:
                            type_s = "X | ";
                            break;
                        case 0x31:
                            type_s = "Y | ";
                            break;
                        case 0x32:
                            type_s = "Z | ";
                            break;
                        case 0x33:
                            type_s = "Rx | ";
                            break;
                        case 0x34:
                            type_s = "Ry | ";
                            break;
                        case 0x35:
                            type_s = "Rz | ";
                            break;
                        case 0x37:
                            type_s = "Dial | ";
                            break;
                        case 0x38:
                            type_s = "Wheel | ";
                            break;
                        default:
                            type_s = "? | ";
                            break;
                    }
                    break;
                case 0x9:
                    type_s = "Button";
                    button_num++;
                    break;
                default:
                    type_s = "? | ";
                    break;
            }
        }

        public byte get_type_num(string type)
        {
            byte type_num = 0;
            switch (type)
            {
                case "Button":
                    type_num = 0x9;
                    break;
                case "X":
                    type_num = 0x30;
                    break;
                case "Y":
                    type_num = 0x31;
                    break;
                case "Z":
                    type_num = 0x32;
                    break;
                case "Rx":
                    type_num = 0x33;
                    break;
                case "Ry":
                    type_num = 0x34;
                    break;
                case "Rz":
                    type_num = 0x35;
                    break;
                case "Dial":
                    type_num = 0x37;
                    break;
                case "Wheel":
                    type_num = 0x38;
                    break;
                default:
                    type_num = 0xFF;
                    break;
            }
            return type_num;
        }

        public void get_mode_string(byte mode)
        {
#if true    // to make fit screwn width just display number
            type_s_mode = Convert.ToString(mode);
#else
            switch (mode)
            {
                case 0:
                    type_s_mode = "Axis (D) | ";
                    break;
                case 1:
                    type_s_mode = "Select (M+D) | ";
                    break;
                case 2:
                    type_s_mode = "Select Toggle (M+D) | ";
                    break;
                case 3:
                    type_s_mode = "Disable (M) | ";
                    break;
                case 4:
                    type_s_mode = "Toggle Disable (D) | ";
                    break;
                case 5:
                    type_s_mode = "Disable (D) | ";
                    break;
                case 6:
                    type_s_mode = "Speed (D) | ";
                    break;
                case 7:
                    type_s_mode = "Jog CW (D) | ";
                    break;
                case 8:
                    type_s_mode = "Jog CCW (D) | ";
                    break;
                case 9:
                    type_s_mode = "Position Toggle (D) | ";
                    break;
                case 10:
                    type_s_mode = "Home (D) | ";
                    break;
                case 11:
                    type_s_mode = "Position 1 (D) | ";
                    break;
                case 12:
                    type_s_mode = "Position 2 (D) | ";
                    break;
                case 13:
                    type_s_mode = "Position 3 (D) | ";
                    break;
                case 14:
                    type_s_mode = "Position 4 (D) | ";
                    break;
                case 15:
                    type_s_mode = "Position 5 (D) | ";
                    break;
                case 16:
                    type_s_mode = "Position 6 (D) | ";
                    break;
                case 17:
                    type_s_mode = "Position 7 (D) | ";
                    break;
                case 18:
                    type_s_mode = "Position 8 (D) | ";
                    break;
                case 19:
                    type_s_mode = "Position 9 (D) | ";
                    break;
                case 20:
                    type_s_mode = "Position 10 (D) | ";
                    break;
            }
#endif
        }

        public void display_input_controls(byte _port)
        {
            uint usage;
            int items_on_line_cnt = 0;
            button_num = 0;
            var port_panel = get_port_panel(_port);
            for (int i = 0; i < num_of_input_controls[_port]; i++)
            {

                // add control number
                port_panel.Content += i.ToString() + ": ";

                // get type
                usage = input_control_type[_port, i];
                get_type_string(usage);

                if (items_on_line_cnt > 24)
                {
                    port_panel.Content += "\n";
                    items_on_line_cnt = 0;
                }
                items_on_line_cnt++;

                if ((usage >> 16) == 0x9) // add button number
                {
                    port_panel.Content += type_s + " " + button_num.ToString() + " | ";
                }
                else
                {
                    port_panel.Content += type_s;
                }
            }
        }

        public void display_output_controls(byte _port)
        {
            uint usage;
            string type_s;

            byte led_num = 0;

            var port_panel = get_port_panel(_port);

            for (int i = 0; i < num_of_output_controls[_port]; i++)
            {
                // add control number
                port_panel.Content += i.ToString() + ": ";

                usage = output_control_type[_port, i];
                switch (usage >> 16)
                {
                    case 0x8:
                        type_s = "LED";
                        led_num++;
                        break;
                    default:
                        type_s = "? | ";
                        break;
                }

                port_panel.Content += type_s + " " + led_num.ToString() + " | ";
            }
        }

        private void add_VID_TextChanged(object sender, TextChangedEventArgs e)
        {
            char[] allowedChars = new char[] { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

            foreach (char character in add_VID.Text.ToUpper().ToArray())
            {
                if (!allowedChars.Contains(character))
                {
                    MessageBox.Show(string.Format("'{0}' is not a hexadecimal character", character));
                }
            }
        }

        private void add_PID_TextChanged(object sender, TextChangedEventArgs e)
        {
            char[] allowedChars = new char[] { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

            foreach (char character in add_PID.Text.ToUpper().ToArray())
            {
                if (!allowedChars.Contains(character))
                {
                    MessageBox.Show(string.Format("'{0}' is not a hexadecimal character", character));
                }
            }
        }

        private void clear_all_in_map_controls_for_port(byte _port)
        {
            try
            {
                int number_of_controls = mapping_in_data[_port].Count;
                byte number;


                for (byte i = 0; i < number_of_controls; i++)
                {
                    // find the control number
                    number = mapping_in_data[_port][0].number;

                    clear_in_map_control(_port, number);
                }
            }
            catch (Exception)
            {

            }
        }

        private void clear_all_out_map_controls_for_port(byte _port)
        {
            try
            {
                int number_of_controls = mapping_out_data[_port].Count;
                byte number;


                for (byte i = 0; i < number_of_controls; i++)
                {
                    // find the control number
                    number = mapping_out_data[_port][0].number;

                    clear_out_map_control(_port, number);
                }
            }
            catch (Exception)
            {

            }
        }

        private void clear_in_map_control(byte _port, byte number)
        {
            // get the list item that matches the clear button name number
            byte num = 0;

            for (num = 0; num < mapping_in_data[_port].Count; num++)
            {
                if (mapping_in_data[_port][num].number == number)
                    break;
            }

            var map_panel = get_map_panel_in(_port);
            this.Dispatcher.Invoke(new Action(() =>
            {
                map_panel.Children.Remove(panel_in[_port, number]);
                mapping_in_data[_port].RemoveAt(num);
            }));
        }

        private void clear_out_map_control(byte _port, byte number)
        {
            // get the list item that matches the clear button name number
            byte num = 0;

            for (num = 0; num < mapping_out_data[_port].Count; num++)
            {
                if (mapping_out_data[_port][num].number == number)
                    break;
            }

            var map_panel = get_map_panel_out(_port);
            this.Dispatcher.Invoke(new Action(() =>
            {
                map_panel.Children.Remove(panel_out[_port , number]);
                mapping_out_data[_port].RemoveAt(num);
            }));
        }

        private void clear_map_in_control_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string _port_s = button_name.Substring(button_name.Length - 4, 2);
            string control_s = button_name.Substring(button_name.Length - 2, 2);
            byte _port = Convert.ToByte(_port_s);
            int control = Convert.ToInt32(control_s);

            // get the list item that matches the clear button name number
            byte num = 0;

            for (num = 0; num < mapping_in_data[_port].Count; num++)
            {
                if (mapping_in_data[_port][num].number == control)
                    break;
            }

            clear_in_map_control(_port, (byte)control);
            save_flags_in[_port] = true;
            save_in_button_color(_port);
        }

        private void clear_map_out_control_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string _port_s = button_name.Substring(button_name.Length - 4, 2);
            string control_s = button_name.Substring(button_name.Length - 2, 2);
            byte _port = Convert.ToByte(_port_s);
            int control = Convert.ToInt32(control_s);

            // get the list item that matches the clear button name number
            byte num = 0;

            for (num = 0; num < mapping_out_data[_port].Count; num++)
            {
                if (mapping_out_data[_port][num].number == control)
                    break;
            }

            clear_out_map_control(_port, (byte)control);
            save_flags_out[_port] = true;
            save_out_button_color(_port);
        }

        private void save_port_mapping_in(byte _port, bool save_to_eeprom)
        {
            /*MGMSG_MOD_SET_JOYSTICK_MAP_IN //32 bytes total, extended data is 26 bytes
            CMD_LSB | CMD_MSB | LEN_LSB | LEN_MSB | DEST | 0x80 | SOURCE
            PORT | CONTROL | VID_LSB | VID_MSB | PID_LSB | PID_MSB
            MOD_CTL_PORT | MOD_CTL_NUM_LSB | MOD_CTL_NUM_MSB | DEST_SLOT | DEST_BIT | DEST_PORT
            DEST_VIRT | SPEED | REV_DIR | MODE | RESERVED | RESERVED
            RESERVED | RESERVED | RESERVED | RESERVED | RESERVED | RESERVED
            RESERVED | RESERVED */

            byte length_of_extended_data = (byte)(SIZE_OF_IN);

            byte[] bytesToSend = new byte[SIZE_OF_IN + 6];
            for (int c = 0; c < (SIZE_OF_IN + 6); c++)
            {
                bytesToSend[c] = 0xff;
            }

            // MGMSG_MOD_SET_JOYSTICK_MAP_IN
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_JOYSTICK_MAP_IN));
            byte[] header = new byte[] { command[0], command[1], length_of_extended_data, 0, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID };

            Array.Copy(header, 0, bytesToSend, 0, header.Length);

            // add port number bytesToSend buffer
            bytesToSend[6] = _port;   /* byte 6*/

            //clear all existing controls in MCM memory
             for (int c = 0; c < MAX_NUM_CONTROLS; c++)
             {
                 bytesToSend[7] = (byte)c;
                 usb_write("save_port_mapping_in", bytesToSend, length_of_extended_data + 6);
                 Thread.Sleep(USB_HID_SEND_DELAY);
             }

            int map_count = mapping_in_data[_port].Count;

            // add all the mapping data
            for (int i = 0; i < map_count; i++)
            {
                if (i > MAX_NUM_CONTROLS - 1) //only MAX_NUM_CONTROLS controls are allowed
                {
                    return;
                }

                bytesToSend[7] = mapping_in_data[_port][i].control_number;
                bytesToSend[8] = (byte)(mapping_in_data[_port][i].vid);
                bytesToSend[9] = (byte)(mapping_in_data[_port][i].vid >> 8);
                bytesToSend[10] = (byte)(mapping_in_data[_port][i].pid);
                bytesToSend[11] = (byte)(mapping_in_data[_port][i].pid >> 8);

                bytesToSend[12] = mapping_in_data[_port][i].modify_control_port;
                bytesToSend[13] = (byte)(mapping_in_data[_port][i].modify_control_ctl_num);
                bytesToSend[14] = (byte)(mapping_in_data[_port][i].modify_control_ctl_num >> 8);
                bytesToSend[15] = mapping_in_data[_port][i].destination_slot;
                bytesToSend[16] = mapping_in_data[_port][i].destination_bit;
                bytesToSend[17] = mapping_in_data[_port][i].destination_port;
                bytesToSend[18] = mapping_in_data[_port][i].destination_virtual;

                bytesToSend[19] = mapping_in_data[_port][i].modify_speed;
                bytesToSend[20] = mapping_in_data[_port][i].revserse_dir;
                bytesToSend[21] = mapping_in_data[_port][i].dead_band;
                bytesToSend[22] = mapping_in_data[_port][i].mode;
                bytesToSend[26] = 0x00; // Always enable the control.
                usb_write("save_port_mapping_in", bytesToSend, length_of_extended_data + 6);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }

            save_flags_in[_port] = false;
            save_in_button_color(_port);

            if (save_to_eeprom)
            {
                bytesToSend = save_command(Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.MGMSG_MOD_SET_JOYSTICK_MAP_IN, _port);

                usb_write("save_port_mapping_in", bytesToSend, 10);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }
        }

        private void save_port_mapping_out(byte _port, bool save_to_eeprom)
        {
            /*MGMSG_LA_SET_JOYSTICK_MAP_OUT //32 bytes total, extended data is 26 bytes
            CMD_LSB | CMD_MSB | LEN_LSB | LEN_MSB | DEST | 0x80 | SOURCE
            PORT | CONTROL | VID_LSB | VID_MSB | PID_LSB | PID_MSB
            MOD_CTL_PORT | MOD_CTL_NUM_LSB | MOD_CTL_NUM_MSB | DEST_SLOT | DEST_BIT | DEST_PORT
            DEST_VIRT | SPEED | MODE | RESERVED | RESERVED | RESERVED
            RESERVED | RESERVED | RESERVED | RESERVED | RESERVED | RESERVED
            RESERVED | RESERVED */

            byte length_of_extended_data = (byte)(SIZE_OF_OUT);

            byte[] bytesToSend = new byte[SIZE_OF_OUT + 6];
            for (int c = 0; c < (SIZE_OF_OUT + 6); c++)
            {
                bytesToSend[c] = 0xff;
            }

            // MGMSG_LA_SET_JOYSTICK_MAP_OUT
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_JOYSTICK_MAP_OUT));
            byte[] header = new byte[] { command[0], command[1], length_of_extended_data, 0, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID };

            Array.Copy(header, 0, bytesToSend, 0, header.Length);

            // add port number bytesToSend buffer
            bytesToSend[6] = _port;   /* byte 6*/

            //clear all existing controls in MCM memory
               for (int c = 0; c < MAX_NUM_CONTROLS; c++)
               {
                   bytesToSend[7] = (byte)c;
                   usb_write("save_port_mapping_out", bytesToSend, length_of_extended_data + 6);
                   Thread.Sleep(USB_HID_SEND_DELAY);
               }

            int map_count = mapping_out_data[_port].Count;

            // add all the mapping data to the 
            for (int i = 0; i < map_count; i++)
            {
                if (i > MAX_NUM_CONTROLS - 1) //only MAX_NUM_CONTROLS controls are allowed
                {
                    return;
                }

                bytesToSend[7] = mapping_out_data[_port][i].control_number;

                // below are part of Hid_Mapping_control_out struct
                // type
                bytesToSend[8] = HID_USAGE_PAGE_LED;

                // idVendor
                bytesToSend[9] = (byte)(mapping_out_data[_port][i].vid);
                bytesToSend[10] = (byte)(mapping_out_data[_port][i].vid >> 8);

                // idProduct
                bytesToSend[11] = (byte)(mapping_out_data[_port][i].pid);
                bytesToSend[12] = (byte)(mapping_out_data[_port][i].pid >> 8);

                // mode
                bytesToSend[13] = mapping_out_data[_port][i].mode;

                // color_1_id
                bytesToSend[14] = (byte)(mapping_out_data[_port][i].color_1_id);

                // color_2_id
                bytesToSend[15] = (byte)(mapping_out_data[_port][i].color_2_id);

                // color_3_id
                bytesToSend[16] = (byte)(mapping_out_data[_port][i].color_3_id);

                // source_slots
                bytesToSend[17] = mapping_out_data[_port][i].source_slot;

                // source_bit
                bytesToSend[18] = mapping_out_data[_port][i].source_bit;

                // source_port
                bytesToSend[19] = mapping_out_data[_port][i].source_port;

                // source_virtual
                bytesToSend[20] = mapping_out_data[_port][i].source_virtual;

                usb_write("save_port_mapping_out", bytesToSend, length_of_extended_data + 6);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }

            save_flags_out[_port] = false;
            save_out_button_color(_port);

            if (save_to_eeprom)
            {
                bytesToSend = save_command(Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.MGMSG_MOD_SET_JOYSTICK_MAP_OUT, _port);

                usb_write("save_port_mapping_out", bytesToSend, 10);
                Thread.Sleep(USB_HID_SEND_DELAY);

                usb_write("save_port_mapping_out", bytesToSend, 10);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }
        }

        private void in_save_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string _port_s = button_name.Substring(button_name.Length - 1, 1);
            byte _port = Convert.ToByte(_port_s);

            bool save_to_eeprom = false;
            Dispatcher.Invoke(() => {
                CheckBox? target = FindName("e_chk_usb_host_save_eeprom_p" + _port.ToString()) as CheckBox;
                if (target is not null)
                {
                    save_to_eeprom = target.IsChecked == true;
                }
            });

            save_port_mapping_in(_port, save_to_eeprom);
        }

        private void out_save_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string _port_s = button_name.Substring(button_name.Length - 1, 1);
            byte _port = Convert.ToByte(_port_s);

            bool save_to_eeprom = false;
            Dispatcher.Invoke(() => {
                CheckBox? target = FindName("e_chk_usb_host_save_eeprom_p" + _port.ToString()) as CheckBox;
                if (target is not null)
                {
                    save_to_eeprom = target.IsChecked == true;
                }
            });

            save_port_mapping_out(_port, save_to_eeprom);
        }


        private void add_map_in(byte _port, ref MapInfoIn mi)
        {
            byte i = (byte)mapping_in_data[_port].Count;

            // add the mapping info to the list
            mapping_in_data[_port].Add(new MapInfoIn
            {
                number = i, // used only for clear
                control_number = mi.control_number,
                vid = mi.vid,
                pid = mi.pid,
                mode = mi.mode,
                modify_control_port = mi.modify_control_port,
                modify_control_ctl_num = mi.modify_control_ctl_num,
                destination_slot = mi.destination_slot,
                destination_bit = mi.destination_bit,
                destination_port = mi.destination_port,
                destination_virtual = mi.destination_virtual,
                modify_speed = mi.modify_speed,
                revserse_dir = mi.revserse_dir,
                dead_band = mi.dead_band
            });
        }

        private void add_map_out(byte _port, ref MapInfoOut mi)
        {
            byte i = (byte)mapping_out_data[_port].Count;

            // add the mapping info to the list
            mapping_out_data[_port].Add(new MapInfoOut
            {
                number = i, // used only for clear
                control_number = mi.control_number,
                type = mi.type,
                vid = mi.vid,
                pid = mi.pid,
                mode = mi.mode,
                color_1_id = mi.color_1_id,
                color_2_id = mi.color_2_id,
                color_3_id = mi.color_3_id,
                source_slot = mi.source_slot,
                source_bit = mi.source_bit,
                source_port = mi.source_port,
                source_virtual = mi.source_virtual
            });
        }

        private void save_in_button_color(byte _port)
        {

            if (save_flags_in[_port] == true)
                Foreground = new SolidColorBrush(Colors.Red);
            else
                Foreground = new SolidColorBrush(Colors.White);

            switch (_port)
            {
                case 0:
                    in_save_p0.Foreground = Foreground;
                    break;
                case 1:
                    save_p1.Foreground = Foreground;
                    break;

                case 2:
                    in_save_p2.Foreground = Foreground;
                    break;

                case 3:
                    in_save_p3.Foreground = Foreground;
                    break;

                case 4:
                    in_save_p4.Foreground = Foreground;
                    break;

                case 5:
                    in_save_p5.Foreground = Foreground;
                    break;

                case 6:
                    in_save_p6.Foreground = Foreground;
                    break;

                case 7:
                    in_save_p7.Foreground = Foreground;
                    break;

                default:
                    break;
            }
        }

        private void save_out_button_color(byte _port)
        {

            if (save_flags_out[_port] == true)
                Foreground = new SolidColorBrush(Colors.Red);
            else
                Foreground = new SolidColorBrush(Colors.White);

            switch (_port)
            {
                case 0:
                    out_save_p0.Foreground = Foreground;
                    break;
                case 1:
                    out_save_p1.Foreground = Foreground;
                    break;

                case 2:
                    out_save_p2.Foreground = Foreground;
                    break;

                case 3:
                    out_save_p3.Foreground = Foreground;
                    break;

                case 4:
                    out_save_p4.Foreground = Foreground;
                    break;

                case 5:
                    out_save_p5.Foreground = Foreground;
                    break;

                case 6:
                    out_save_p6.Foreground = Foreground;
                    break;

                case 7:
                    in_save_p7.Foreground = Foreground;
                    break;

                default:
                    break;
            }
        }

        private void add_in_map_to_ui(byte _port, byte number)
        {
            string modify_control_port_binary = Convert.ToString(mapping_in_data[_port][number].modify_control_port, 2);
            string modify_control_ctl_num_binary = Convert.ToString(mapping_in_data[_port][number].modify_control_ctl_num, 2);
            string modify_speed_binary = Convert.ToString(mapping_in_data[_port][number].modify_speed, 2);
            string destination_slot_binary = Convert.ToString(mapping_in_data[_port][number].destination_slot, 2);
            string destination_bit_binary = Convert.ToString(mapping_in_data[_port][number].destination_bit, 2);
            string destination_port_binary = Convert.ToString(mapping_in_data[_port][number].destination_port, 2);
            string destination_virtual_binary = Convert.ToString(mapping_in_data[_port][number].destination_virtual, 2);
            string modify_deadband_binary = Convert.ToString(mapping_in_data[_port][number].dead_band, 2);

            string rev_dir;

            if (mapping_in_data[_port][number].revserse_dir == 1)
                rev_dir = "(R) ";
            else
                rev_dir = "";

            get_mode_string(mapping_in_data[_port][number].mode);

            this.Dispatcher.Invoke(new Action(() =>
            {
                var map_panel = get_map_panel_in(_port);

                save_in_button_color(_port);

                panel_in[_port, number] = new StackPanel();
                panel_in[_port, number].Orientation = Orientation.Horizontal;
                map_panel.Children.Add(panel_in[_port, number]);

                Label label = new Label();
                label.Content += "Ctrl In: " + rev_dir + mapping_in_data[_port][number].control_number.ToString();
                label.Content += " | VID: " + mapping_in_data[_port][number].vid.ToString("X4");
                label.Content += " | PID: " + mapping_in_data[_port][number].pid.ToString("X4");
                label.Content += " | Mode: " + type_s_mode;// map_add_mode.Text;
                label.Content += " | Port (M): [" + modify_control_port_binary.PadLeft(8, '0') + "]";
                label.Content += " | Control (M): [" + modify_control_ctl_num_binary.PadLeft(16, '0') + "]";
                label.Content += " | DB: " + mapping_in_data[_port][number].dead_band.ToString();
                label.Content += " | Speed (M): [" + modify_speed_binary.PadLeft(8, '0') + "]";
                label.Content += " | Slot (D): [" + destination_slot_binary.PadLeft(8, '0') + "]";
                label.Content += " | Bit (D): [" + destination_bit_binary.PadLeft(8, '0') + "]";
                label.Content += " | Port (D): [" + destination_port_binary.PadLeft(8, '0') + "]";
                label.Content += " | Virtual (D): [" + destination_virtual_binary.PadLeft(8, '0') + "]";
                panel_in[_port, number].Children.Add(label);

                // Clear button
                Button bt_clear = new Button();
                bt_clear.Name = "clear" + _port.ToString("00") + number.ToString("00");
                bt_clear.Click += new RoutedEventHandler(clear_map_in_control_Click);
                bt_clear.Margin = new Thickness(5, 5, 0, 0);
                bt_clear.Content = "Clear";
                panel_in[_port, number].Children.Add(bt_clear);

            }));
        }

        private void add_out_map_to_ui(byte _port, byte number)
        {
            string mode_binary = Convert.ToString(mapping_out_data[_port][number].mode, 2);
            string source_slot_binary = Convert.ToString(mapping_out_data[_port][number].source_slot, 2);
            string source_bit_binary = Convert.ToString(mapping_out_data[_port][number].source_bit, 2);
            string source_port_binary = Convert.ToString(mapping_out_data[_port][number].source_port, 2);
            string source_virtual_binary = Convert.ToString(mapping_out_data[_port][number].source_virtual, 2);

            get_mode_string(mapping_out_data[_port][number].mode);

            this.Dispatcher.Invoke(new Action(() =>
            {
                var map_panel = get_map_panel_out(_port);

                save_out_button_color(_port);

                panel_out[_port, number] = new StackPanel();
                panel_out[_port, number].Orientation = Orientation.Horizontal;
                map_panel.Children.Add(panel_out[_port, number]);

                Label label = new Label();
                label.Content += "Ctrl Out: " + mapping_out_data[_port][number].control_number.ToString();
                label.Content += " | VID: " + mapping_out_data[_port][number].vid.ToString("X4");
                label.Content += " | PID: " + mapping_out_data[_port][number].pid.ToString("X4");
                label.Content += " | Mode: [" + mode_binary.PadLeft(8, '0') + "]";
                label.Content += " | Color 1: [" + get_color_string(mapping_out_data[_port][number].color_1_id) + "]";
                label.Content += " | Color 2: [" + get_color_string(mapping_out_data[_port][number].color_2_id) + "]";
                label.Content += " | Color 3: [" + get_color_string(mapping_out_data[_port][number].color_3_id) + "]";
                label.Content += " | Slot (D): [" + source_slot_binary.PadLeft(8, '0') + "]";
                label.Content += " | Bit (D): [" + source_bit_binary.PadLeft(8, '0') + "]";
                label.Content += " | Port (D): [" + source_port_binary.PadLeft(8, '0') + "]";
                label.Content += " | Virtual (D): [" + source_virtual_binary.PadLeft(8, '0') + "]";
                panel_out[_port, number].Children.Add(label);

                // Clear button
                Button bt_clear = new Button();
                bt_clear.Name = "clear" + _port.ToString("00") + number.ToString("00");
                bt_clear.Click += new RoutedEventHandler(clear_map_out_control_Click);
                bt_clear.Margin = new Thickness(5, 5, 0, 0);
                bt_clear.Content = "Clear";
                panel_out[_port, number].Children.Add(bt_clear);

            }));

        }

        private void add_in_Click(object sender, RoutedEventArgs e)
        {
            byte _port = Convert.ToByte(in_add_port_num.Text);

            byte i = (byte)mapping_in_data[_port].Count;
            save_flags_in[_port] = true;

            byte rev_dir = 0;
            if (in_reverse_dir.IsChecked == true)
                rev_dir |= 1;

            mapping_in_data[_port].Add(new MapInfoIn
            {
                number = i, // used only for clear
                control_number = Convert.ToByte(add_map_num.Text),
                vid = Convert.ToUInt16(add_VID.Text, 16),
                pid = Convert.ToUInt16(add_PID.Text, 16),
                mode = Convert.ToByte(map_add_mode.SelectedIndex),
                modify_control_port = map_add_get_in_port_modifications(),
                modify_control_ctl_num = map_add_get_in_control_modifications(),
                destination_slot = map_add_get_in_slot_destinations(),
                destination_bit = map_add_get_in_bit_destinations(),
                destination_port = map_add_get_in_port_destinations(),
                destination_virtual = map_add_get_in_virtual_destinations(),
                modify_speed = map_add_get_speed_modifications(),
                revserse_dir = rev_dir,
                dead_band = Convert.ToByte(tb_joystick_deadband.Text)
            });

            add_in_map_to_ui(_port, i);
        }

        private string get_color_string( int selected_index)
        {
            string temp_string;

            switch (selected_index)
            {
                case 0:
                    temp_string = "OFF";
                    break;
                case 1:
                    temp_string = "WHT";
                    break;
                case 2:
                    temp_string = "RED";
                    break;
                case 3:
                    temp_string = "GRN";
                    break;
                case 4:
                    temp_string = "BLU";
                    break;
                case 5:
                    temp_string = "YEL";
                    break;
                case 6:
                    temp_string = "AQU";
                    break;
                case 7:
                    temp_string = "MAG";
                    break;
                case 8:
                    temp_string = "WHT_DIM";
                    break;
                case 9:
                    temp_string = "RED_DIM";
                    break;
                case 10:
                    temp_string = "GRN_DIM";
                    break;
                case 11:
                    temp_string = "BLU_DIM";
                    break;
                case 12:
                    temp_string = "YEL_DIM";
                    break;
                case 13:
                    temp_string = "AQU_DIM";
                    break;
                case 14:
                    temp_string = "MAG_DIM";
                    break;
                default:
                    temp_string = "ERROR";
                    break;
            }
            return temp_string;
        }

        private void add_out_Click(object sender, RoutedEventArgs e)
        {
            byte _port = Convert.ToByte(out_add_port_num.Text);

            byte i = (byte)mapping_out_data[_port].Count;
            save_flags_out[_port] = true;

            mapping_out_data[_port].Add(new MapInfoOut
            {
                number = i, // used only for clear
                vid = Convert.ToUInt16(out_add_VID.Text, 16),
                pid = Convert.ToUInt16(out_add_PID.Text, 16),
                control_number = Convert.ToByte(out_add_map_num.Text),
                mode = Convert.ToByte(map_add_out_mode.SelectedIndex),
                color_1_id = Convert.ToByte(map_out_color_id_1.SelectedIndex),
                color_2_id = Convert.ToByte(map_out_color_id_2.SelectedIndex),
                color_3_id = Convert.ToByte(map_out_color_id_3.SelectedIndex),
                source_slot = map_add_get_out_slot_sources(),
                source_bit = map_add_get_out_bit_sources(),
                source_port = map_add_get_out_port_sources(),
                source_virtual = map_add_get_out_virtual_sources()
            });

            add_out_map_to_ui(_port, i);
        }

        private void add_in_to_all_ports_Click(object sender, RoutedEventArgs e)
        {
            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {

                byte i = (byte)mapping_in_data[_port].Count;
                save_flags_in[_port] = true;

                mapping_in_data[_port].Add(new MapInfoIn
                {
                    number = i, // used only for clear
                    control_number = Convert.ToByte(add_map_num.Text),
                    vid = Convert.ToUInt16(add_VID.Text, 16),
                    pid = Convert.ToUInt16(add_PID.Text, 16),
                    mode = Convert.ToByte(map_add_mode.SelectedIndex),
                    modify_control_port = map_add_get_in_port_modifications(),
                    modify_control_ctl_num = map_add_get_in_control_modifications(),
                    destination_slot = map_add_get_in_slot_destinations(),
                    destination_bit = map_add_get_in_bit_destinations(),
                    destination_port = map_add_get_in_port_destinations(),
                    destination_virtual = map_add_get_in_virtual_destinations()

                });

                add_in_map_to_ui(_port, i);
            }
        }

        private void add_out_to_all_ports_Click(object sender, RoutedEventArgs e)
        {
            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {

                byte i = (byte)mapping_out_data[_port].Count;
                save_flags_out[_port] = true;

                mapping_out_data[_port].Add(new MapInfoOut
                {
                    number = i, // used only for clear
                    control_number = Convert.ToByte(add_map_num.Text),
                    type = HID_USAGE_PAGE_LED,
                    vid = Convert.ToUInt16(add_VID.Text, 16),
                    mode = Convert.ToByte(map_add_out_mode.SelectedIndex),
                    color_1_id = Convert.ToByte(map_out_color_id_1.SelectedIndex),
                    color_2_id = Convert.ToByte(map_out_color_id_2.SelectedIndex),
                    color_3_id = Convert.ToByte(map_out_color_id_3.SelectedIndex),
                    source_slot = map_add_get_out_slot_sources(),
                    source_bit = map_add_get_out_bit_sources(),
                    source_port = map_add_get_out_port_sources(),
                    source_virtual = map_add_get_out_virtual_sources()

                });

                add_out_map_to_ui(_port, i);
            }
        }

        private byte map_add_get_in_slot_destinations()
        {
            byte slot_destinations = 0;

            if (in_d_s_0.IsChecked == true)
                slot_destinations |= 1;
            if (in_d_s_1.IsChecked == true)
                slot_destinations |= 1 << 1;
            if (in_d_s_2.IsChecked == true)
                slot_destinations |= 1 << 2;
            if (in_d_s_3.IsChecked == true)
                slot_destinations |= 1 << 3;
            if (in_d_s_4.IsChecked == true)
                slot_destinations |= 1 << 4;
            if (in_d_s_5.IsChecked == true)
                slot_destinations |= 1 << 5;
            if (in_d_s_6.IsChecked == true)
                slot_destinations |= 1 << 6;
            if (in_d_s_7.IsChecked == true)
                slot_destinations |= 1 << 7;
            return slot_destinations;
        }

        private byte map_add_get_out_slot_sources()
        {
            byte slot_sources = 0;

            if (out_d_s_0.IsChecked == true)
                slot_sources |= 1;
            if (out_d_s_1.IsChecked == true)
                slot_sources |= 1 << 1;
            if (out_d_s_2.IsChecked == true)
                slot_sources |= 1 << 2;
            if (out_d_s_3.IsChecked == true)
                slot_sources |= 1 << 3;
            if (out_d_s_4.IsChecked == true)
                slot_sources |= 1 << 4;
            if (out_d_s_5.IsChecked == true)
                slot_sources |= 1 << 5;
            if (out_d_s_6.IsChecked == true)
                slot_sources |= 1 << 6;
            if (out_d_s_7.IsChecked == true)
                slot_sources |= 1 << 7;
            return slot_sources;
        }
        
        private byte map_add_get_in_bit_destinations()
        {
            byte bit_destinations = 0;

            if (in_d_b_0.IsChecked == true)
                bit_destinations |= 1;
            if (in_d_b_1.IsChecked == true)
                bit_destinations |= 1 << 1;
            if (in_d_b_2.IsChecked == true)
                bit_destinations |= 1 << 2;
            if (in_d_b_3.IsChecked == true)
                bit_destinations |= 1 << 3;
            if (in_d_b_4.IsChecked == true)
                bit_destinations |= 1 << 4;
            if (in_d_b_5.IsChecked == true)
                bit_destinations |= 1 << 5;
            if (in_d_b_6.IsChecked == true)
                bit_destinations |= 1 << 6;
            if (in_d_b_7.IsChecked == true)
                bit_destinations |= 1 << 7;
            return bit_destinations;
        }

        private byte map_add_get_out_bit_sources()
        {
            byte bit_sources = 0;

            if (out_d_b_0.IsChecked == true)
                bit_sources |= 1;
            if (out_d_b_1.IsChecked == true)
                bit_sources |= 1 << 1;
            if (out_d_b_2.IsChecked == true)
                bit_sources |= 1 << 2;
            if (out_d_b_3.IsChecked == true)
                bit_sources |= 1 << 3;
            if (out_d_b_4.IsChecked == true)
                bit_sources |= 1 << 4;
            if (out_d_b_5.IsChecked == true)
                bit_sources |= 1 << 5;
            if (out_d_b_6.IsChecked == true)
                bit_sources |= 1 << 6;
            if (out_d_b_7.IsChecked == true)
                bit_sources |= 1 << 7;
            return bit_sources;
        }

        private byte map_add_get_in_port_destinations()
        {
            byte port_destinations = 0;

            if (in_d_p_0.IsChecked == true)
                port_destinations |= 1;
            if (in_d_p_1.IsChecked == true)
                port_destinations |= 1 << 1;
            if (in_d_p_2.IsChecked == true)
                port_destinations |= 1 << 2;
            if (in_d_p_3.IsChecked == true)
                port_destinations |= 1 << 3;
            if (in_d_p_4.IsChecked == true)
                port_destinations |= 1 << 4;
            if (in_d_p_5.IsChecked == true)
                port_destinations |= 1 << 5;
            if (in_d_p_6.IsChecked == true)
                port_destinations |= 1 << 6;
            if (in_d_p_7.IsChecked == true)
                port_destinations |= 1 << 7;
            return port_destinations;
        }

        private byte map_add_get_out_port_sources()
        {
            byte port_sources = 0;

            if (out_d_p_0.IsChecked == true)
                port_sources |= 1;
            if (out_d_p_1.IsChecked == true)
                port_sources |= 1 << 1;
            if (out_d_p_2.IsChecked == true)
                port_sources |= 1 << 2;
            if (out_d_p_3.IsChecked == true)
                port_sources |= 1 << 3;
            if (out_d_p_4.IsChecked == true)
                port_sources |= 1 << 4;
            if (out_d_p_5.IsChecked == true)
                port_sources |= 1 << 5;
            if (out_d_p_6.IsChecked == true)
                port_sources |= 1 << 6;
            if (out_d_p_7.IsChecked == true)
                port_sources |= 1 << 7;
            return port_sources;
        }

        private byte map_add_get_in_virtual_destinations()
        {
            byte virtual_destinations = 0;

            if (in_d_v_0.IsChecked == true)
                virtual_destinations |= 1;
            if (in_d_v_1.IsChecked == true)
                virtual_destinations |= 1 << 1;
            if (in_d_v_2.IsChecked == true)
                virtual_destinations |= 1 << 2;
            if (in_d_v_3.IsChecked == true)
                virtual_destinations |= 1 << 3;
            if (in_d_v_4.IsChecked == true)
                virtual_destinations |= 1 << 4;
            if (in_d_v_5.IsChecked == true)
                virtual_destinations |= 1 << 5;
            if (in_d_v_6.IsChecked == true)
                virtual_destinations |= 1 << 6;
            if (in_d_v_7.IsChecked == true)
                virtual_destinations |= 1 << 7;
            return virtual_destinations;
        }

        private byte map_add_get_out_virtual_sources()
        {
            byte virtual_sources = 0;

            if (out_d_v_0.IsChecked == true)
                virtual_sources |= 1;
            if (out_d_v_1.IsChecked == true)
                virtual_sources |= 1 << 1;
            if (out_d_v_2.IsChecked == true)
                virtual_sources |= 1 << 2;
            if (out_d_v_3.IsChecked == true)
                virtual_sources |= 1 << 3;
            if (out_d_v_4.IsChecked == true)
                virtual_sources |= 1 << 4;
            if (out_d_v_5.IsChecked == true)
                virtual_sources |= 1 << 5;
            if (out_d_v_6.IsChecked == true)
                virtual_sources |= 1 << 6;
            if (out_d_v_7.IsChecked == true)
                virtual_sources |= 1 << 7;
            return virtual_sources;
        }

        private byte map_add_get_in_port_modifications()
        {
            byte port_modifications = 0;

            if (in_m_p_0.IsChecked == true)
                port_modifications |= 1;
            if (in_m_p_1.IsChecked == true)
                port_modifications |= 1 << 1;
            if (in_m_p_2.IsChecked == true)
                port_modifications |= 1 << 2;
            if (in_m_p_3.IsChecked == true)
                port_modifications |= 1 << 3;
            if (in_m_p_4.IsChecked == true)
                port_modifications |= 1 << 4;
            if (in_m_p_5.IsChecked == true)
                port_modifications |= 1 << 5;
            if (in_m_p_6.IsChecked == true)
                port_modifications |= 1 << 6;
            if (in_m_p_6.IsChecked == true)
                port_modifications |= 1 << 7;
            return port_modifications;
        }

        private UInt16 map_add_get_in_control_modifications()
        {
            UInt16 control_modifications = 0;

            if (in_m_cn_0.IsChecked == true)
                control_modifications |= 1;
            if (in_m_cn_1.IsChecked == true)
                control_modifications |= 1 << 1;
            if (in_m_cn_2.IsChecked == true)
                control_modifications |= 1 << 2;
            if (in_m_cn_3.IsChecked == true)
                control_modifications |= 1 << 3;
            if (in_m_cn_4.IsChecked == true)
                control_modifications |= 1 << 4;
            if (in_m_cn_5.IsChecked == true)
                control_modifications |= 1 << 5;
            if (in_m_cn_6.IsChecked == true)
                control_modifications |= 1 << 6;
            if (in_m_cn_7.IsChecked == true)
                control_modifications |= 1 << 7;
            if (in_m_cn_8.IsChecked == true)
                control_modifications |= 1 << 8;
            if (in_m_cn_9.IsChecked == true)
                control_modifications |= 1 << 9;
            if (in_m_cn_10.IsChecked == true)
                control_modifications |= 1 << 10;
            if (in_m_cn_11.IsChecked == true)
                control_modifications |= 1 << 11;
            if (in_m_cn_12.IsChecked == true)
                control_modifications |= 1 << 12;
            if (in_m_cn_13.IsChecked == true)
                control_modifications |= 1 << 13;
            if (in_m_cn_14.IsChecked == true)
                control_modifications |= 1 << 14;
            if (in_m_cn_15.IsChecked == true)
                control_modifications |= 1 << 15;
            return control_modifications;
        }

        private byte map_add_get_speed_modifications()
        {
            byte speed_modifications = 0;

            double high_speed_pct = m_high_speed.Value;

            int high_speed = Convert.ToInt16(Math.Floor(high_speed_pct * 2.55));

            speed_modifications = (byte)high_speed;

            return speed_modifications;
        }

        private void clear_js_in_map(byte _port, bool save_to_eeprom)
        {
            byte length_of_extended_data = (byte)(SIZE_OF_IN);
            byte[] bytesToSend = Enumerable.Repeat((byte)0xff, 32).ToArray();

            // MGMSG_MOD_SET_JOYSTICK_MAP_IN
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_JOYSTICK_MAP_IN));
            byte[] header = new byte[] { command[0], command[1], length_of_extended_data, 0, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID };

            Array.Copy(header, 0, bytesToSend, 0, header.Length);

            bytesToSend[6] = _port;
            for (byte _control_num = 0; _control_num < MAX_NUM_CONTROLS; _control_num++)
            {
                bytesToSend[7] = _control_num;
                usb_write("clear_js_in_map", bytesToSend, length_of_extended_data + 6);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }
            clear_all_in_map_controls_for_port(_port);

            if (save_to_eeprom)
            {
                byte[] cmd = save_command(Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.MGMSG_MOD_SET_JOYSTICK_MAP_IN, _port);
                usb_write("clear_js_in_map", cmd, cmd.Length);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }
        }       
        
        private void clear_all_in(object sender, RoutedEventArgs e, bool save_to_eeprom = false)
        {

            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {
                clear_js_in_map(_port, save_to_eeprom);
            }
            mapping_in_data.Clear();
        }

        private void clear_js_out_map(byte _port, bool save_to_eeprom)
        {
            byte length_of_extended_data = (byte)(SIZE_OF_OUT);
            byte[] bytesToSend = Enumerable.Repeat((byte)0xff, 32).ToArray();

            // MGMSG_MOD_SET_JOYSTICK_MAP_OUT
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_JOYSTICK_MAP_OUT));
            byte[] header = new byte[] { command[0], command[1], length_of_extended_data, 0, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID };

            Array.Copy(header, 0, bytesToSend, 0, header.Length);

            bytesToSend[6] = _port;
            for (byte _control_num = 0; _control_num < MAX_NUM_CONTROLS; _control_num++)
            {
                bytesToSend[7] = _control_num;
                usb_write("clear_js_out_map", bytesToSend, length_of_extended_data + 6);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }
            clear_all_out_map_controls_for_port(_port);

            if (save_to_eeprom)
            {
                byte[] cmd = save_command(Thorlabs.APT.Address.MOTHERBOARD, Thorlabs.APT.MGMSG_MOD_SET_JOYSTICK_MAP_OUT, _port);
                usb_write("clear_js_out_map", cmd, cmd.Length);
                Thread.Sleep(USB_HID_SEND_DELAY);
            }
        }

        private void clear_all_out(object sender, RoutedEventArgs e, bool save_to_eeprom = false)
        {
            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {
                clear_js_out_map(_port, save_to_eeprom);
            }
            mapping_out_data.Clear();
        }

        private void clear_all_Click(object sender, RoutedEventArgs e)
        {
            bool eeprom = apply_to_eeprom();

            clear_all_in(sender, e, eeprom);
            clear_all_out(sender, e, eeprom);
            usb_host_update();
        }

        private void clear_all_in_Click(object sender, RoutedEventArgs e)
        {
            clear_all_in(sender, e);
            usb_host_update();
        }

        private void clear_all_out_Click(object sender, RoutedEventArgs e)
        {
            clear_all_out(sender, e);
            usb_host_update();
        }


        private void save_all_Click(object sender, RoutedEventArgs e)
        {
            bool eeprom = apply_to_eeprom();

            for (byte _port = 0; _port < MAX_NUM_PORTS; _port++)
            {
                save_port_mapping_in(_port, eeprom);
                save_flags_in[_port] = false;
                save_in_button_color(_port);

                save_port_mapping_out(_port, eeprom);
                save_flags_out[_port] = false;
                save_out_button_color(_port);
            }
        }

        private bool apply_to_eeprom()
        {
            bool rt = false;
            Dispatcher.Invoke(() => {
                rt = e_chk_usb_host_top_save_to_eeprom.IsChecked == true;
            });
            return rt;
        }
    }
}
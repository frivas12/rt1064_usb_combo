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
        private void Set_in_map(byte _port, MapInfoIn mapping_in_temp)
        {
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

            bytesToSend[7] = mapping_in_temp.control_number;
            bytesToSend[8] = (byte)(mapping_in_temp.vid);
            bytesToSend[9] = (byte)(mapping_in_temp.vid >> 8);
            bytesToSend[10] = (byte)(mapping_in_temp.pid);
            bytesToSend[11] = (byte)(mapping_in_temp.pid >> 8);

            bytesToSend[12] = mapping_in_temp.modify_control_port;
            bytesToSend[13] = (byte)(mapping_in_temp.modify_control_ctl_num);
            bytesToSend[14] = (byte)(mapping_in_temp.modify_control_ctl_num >> 8);
            bytesToSend[15] = mapping_in_temp.destination_slot;
            bytesToSend[16] = mapping_in_temp.destination_bit;
            bytesToSend[17] = mapping_in_temp.destination_port;
            bytesToSend[18] = mapping_in_temp.destination_virtual;

            bytesToSend[19] = mapping_in_temp.modify_speed;
            bytesToSend[20] = mapping_in_temp.revserse_dir;
            bytesToSend[21] = mapping_in_temp.mode;
            
            usb_write(bytesToSend, length_of_extended_data + 6);
            Thread.Sleep(USB_HID_SEND_DELAY);
        }

        private void Set_out_map(byte _port, MapInfoOut mapping_out_temp)
        {
            byte length_of_extended_data = (byte)(SIZE_OF_OUT);

            byte[] bytesToSend = new byte[SIZE_OF_OUT + 6];
            for (int c = 0; c < (SIZE_OF_OUT + 6); c++)
            {
                bytesToSend[c] = 0xff;
            }
            // MGMSG_MOD_SET_JOYSTICK_MAP_OUT
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_JOYSTICK_MAP_OUT));
            byte[] header = new byte[] { command[0], command[1], length_of_extended_data, 0, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID };

            Array.Copy(header, 0, bytesToSend, 0, header.Length);

            // add port number bytesToSend buffer
            bytesToSend[6] = _port;   /* byte 6*/

            bytesToSend[7] = mapping_out_temp.control_number;
            bytesToSend[8] = HID_USAGE_PAGE_LED;
            bytesToSend[9] = (byte)(mapping_out_temp.vid);
            bytesToSend[10] = (byte)(mapping_out_temp.vid >> 8);
            bytesToSend[11] = (byte)(mapping_out_temp.pid);
            bytesToSend[12] = (byte)(mapping_out_temp.pid >> 8);

            bytesToSend[13] = mapping_out_temp.mode;
            bytesToSend[14] = mapping_out_temp.color_1_id;
            bytesToSend[15] = mapping_out_temp.color_2_id;
            bytesToSend[16] = mapping_out_temp.color_3_id;
            bytesToSend[17] = mapping_out_temp.source_slot;
            bytesToSend[18] = mapping_out_temp.source_bit;
            bytesToSend[19] = mapping_out_temp.source_port;

            bytesToSend[20] = mapping_out_temp.source_virtual;

            usb_write(bytesToSend, length_of_extended_data + 6);
            Thread.Sleep(USB_HID_SEND_DELAY);
        }

        private void Set_4_knob_map_Click(object sender, RoutedEventArgs e)
        {
            // *******************************************************************
            //  In Mapping 
            // *******************************************************************
            MapInfoIn mapping_in_temp = new MapInfoIn();

            // ****** Knob R ****** 
            mapping_in_temp.control_number = 0;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT4;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 0;   // Axis (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Knob X ****** 
            mapping_in_temp.control_number = 1;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT1;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 0;   // Axis (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Knob Y ****** 
            mapping_in_temp.control_number = 2;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT2;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 0;   // Axis (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Knob Z ****** 
            mapping_in_temp.control_number = 3;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT3;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 0;   // Axis (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Small button to switch Z and elevator ****** 
            mapping_in_temp.control_number = 8;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 1<<0;
            mapping_in_temp.modify_control_ctl_num = 1<<3;  // knob 4
            mapping_in_temp.destination_slot = 1 << SLOT5;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 1;
            mapping_in_temp.mode = 2;   //Select Toggle (M+D)
            Set_in_map(0, mapping_in_temp); // port 0
            mapping_in_temp.modify_control_port = 1 << 1;
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Button 1 Disable Knob R ****** 
            mapping_in_temp.control_number = 4;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT4;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 4;   // Toggle Disable (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Button 2 Disable Knob X ****** 
            mapping_in_temp.control_number = 5;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT1;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 4;   // Toggle Disable (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Button 3 Disable Knob Y ****** 
            mapping_in_temp.control_number = 6;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT2;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 4;   // Toggle Disable (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Button 4 Disable Knob Z ****** 
            mapping_in_temp.control_number = 7;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = 1 << SLOT3;
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 4;   // Toggle Disable (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1

            // ****** Button 6 Disable All (top small button) ****** 
            mapping_in_temp.control_number = 9;
            mapping_in_temp.vid = 0;
            mapping_in_temp.pid = 0;
            mapping_in_temp.modify_control_port = 0;
            mapping_in_temp.modify_control_ctl_num = 0;
            mapping_in_temp.destination_slot = (1 << SLOT1) | (1 << SLOT2) | (1 << SLOT3) | (1 << SLOT4) | (1 << SLOT5);
            mapping_in_temp.destination_bit = 0;
            mapping_in_temp.destination_port = 0;
            mapping_in_temp.destination_virtual = 0;
            mapping_in_temp.modify_speed = 0xF0;
            mapping_in_temp.revserse_dir = 0;
            mapping_in_temp.mode = 5;   // Disable (D)
            Set_in_map(0, mapping_in_temp); // port 0
            Set_in_map(1, mapping_in_temp); // port 1


            // *******************************************************************
            //  Out Mapping 
            // *******************************************************************
            MapInfoOut mapping_out_temp = new MapInfoOut();

            // ****** Button 1 Knob R LED ****** 
            mapping_out_temp.control_number = 0;
            mapping_out_temp.type = 0;
            mapping_out_temp.vid = 0;
            mapping_out_temp.pid = 0;
            mapping_out_temp.mode = 0x1B;
            mapping_out_temp.color_1_id = 2;
            mapping_out_temp.color_2_id = 3;
            mapping_out_temp.color_3_id = 4;
            mapping_out_temp.source_slot = 1 << SLOT4;
            mapping_out_temp.source_bit = 0;
            mapping_out_temp.source_port = 0;
            mapping_out_temp.source_virtual = 0;
            Set_out_map(0, mapping_out_temp); // port 0
            Set_out_map(1, mapping_out_temp); // port 1

            // ****** Button 2 Knob X LED ****** 
            mapping_out_temp.control_number = 1;
            mapping_out_temp.type = 0;
            mapping_out_temp.vid = 0;
            mapping_out_temp.pid = 0;
            mapping_out_temp.mode = 0x1B;
            mapping_out_temp.color_1_id = 2;
            mapping_out_temp.color_2_id = 3;
            mapping_out_temp.color_3_id = 4;
            mapping_out_temp.source_slot = 1 << SLOT1;
            mapping_out_temp.source_bit = 0;
            mapping_out_temp.source_port = 0;
            mapping_out_temp.source_virtual = 0;
            Set_out_map(0, mapping_out_temp); // port 0
            Set_out_map(1, mapping_out_temp); // port 1

            // ****** Button 3 Knob Y LED ****** 
            mapping_out_temp.control_number = 2;
            mapping_out_temp.type = 0;
            mapping_out_temp.vid = 0;
            mapping_out_temp.pid = 0;
            mapping_out_temp.mode = 0x1B;
            mapping_out_temp.color_1_id = 2;
            mapping_out_temp.color_2_id = 3;
            mapping_out_temp.color_3_id = 4;
            mapping_out_temp.source_slot = 1 << SLOT2;
            mapping_out_temp.source_bit = 0;
            mapping_out_temp.source_port = 0;
            mapping_out_temp.source_virtual = 0;
            Set_out_map(0, mapping_out_temp); // port 0
            Set_out_map(1, mapping_out_temp); // port 1

            // ****** Button 4 Knob Z LED ****** 
            mapping_out_temp.control_number = 3;
            mapping_out_temp.type = 0;
            mapping_out_temp.vid = 0;
            mapping_out_temp.pid = 0;
            mapping_out_temp.mode = 0x1B;
            mapping_out_temp.color_1_id = 2;
            mapping_out_temp.color_2_id = 3;
            mapping_out_temp.color_3_id = 4;
            mapping_out_temp.source_slot = 1 << SLOT3;
            mapping_out_temp.source_bit = 0;
            mapping_out_temp.source_port = 0;
            mapping_out_temp.source_virtual = 0;
            Set_out_map(0, mapping_out_temp); // port 0
            Set_out_map(1, mapping_out_temp); // port 1

            // ****** Button low small toggle Z/E LED ****** 
            mapping_out_temp.control_number = 4;
            mapping_out_temp.type = 0;
            mapping_out_temp.vid = 0;
            mapping_out_temp.pid = 0;
            mapping_out_temp.mode = 0x40;
            mapping_out_temp.color_1_id = 3;
            mapping_out_temp.color_2_id = 2;
            mapping_out_temp.color_3_id = 4;
            mapping_out_temp.source_slot = 1 << SLOT3;
            mapping_out_temp.source_bit = 0;
            mapping_out_temp.source_port = 0;
            mapping_out_temp.source_virtual = 0;
            Set_out_map(0, mapping_out_temp); // port 0
            Set_out_map(1, mapping_out_temp); // port 1

            // ****** Button low small disable all LED ****** 
            mapping_out_temp.control_number = 5;
            mapping_out_temp.type = 0;
            mapping_out_temp.vid = 0;
            mapping_out_temp.pid = 0;
            mapping_out_temp.mode = 0x04;
            mapping_out_temp.color_1_id = 2;
            mapping_out_temp.color_2_id = 3;
            mapping_out_temp.color_3_id = 4;
            mapping_out_temp.source_slot = (1 << SLOT1) | (1 << SLOT2) | (1 << SLOT3) | (1 << SLOT4) | (1 << SLOT5);
            mapping_out_temp.source_bit = 0;
            mapping_out_temp.source_port = 0;
            mapping_out_temp.source_virtual = 0;
            Set_out_map(0, mapping_out_temp); // port 0
            Set_out_map(1, mapping_out_temp); // port 1

        }
    }
}
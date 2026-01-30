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

        private void btn_set_noise_eater_1(object sender, RoutedEventArgs e)
        {
            Byte slot_number = SLOT4;
            Byte ne_num = 0;
            Byte slot_id = (Byte)(0x24 | 0x80); // slot 4 on otm

            float num = 0;
            num = float.Parse(cp_textbox[slot_number, 0].Text);
            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);

            if ((value >= 0) && (value <= 100))
            {
                // The Laser power setpoint (0 to 32767 -> 0% to 100% power).
                value = 32767 * (value / 100);
                byte[] data1 = BitConverter.GetBytes(Convert.ToInt16(value));

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_SET_PARAMS));

                byte[] bytesToSend = new byte[10] { command[0], command[1], 4, 0x00, slot_id, HOST_ID, 
                slot_number, ne_num,  // MsgID
                data1 [0], data1 [1]    // SetPoint
            };
                usb_write("otm_dac_functions", bytesToSend, 10);
                Thread.Sleep(20);
            }
        }

        private void btn_set_noise_eater_2(object sender, RoutedEventArgs e)
        {
            Byte slot_number = SLOT4;
            Byte ne_num = 1;
            Byte slot_id = (Byte)(0x24 | 0x80); // slot 4 on otm

            float num = 0;
            num = float.Parse(cp_textbox[slot_number, 1].Text);
            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);

            if ((value >= 0) && (value <= 100))
            {
                // The Laser power setpoint (0 to 32767 -> 0% to 100% power).
                value = 32767 * (value / 100);
                byte[] data1 = BitConverter.GetBytes(Convert.ToInt16(value));

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_SET_PARAMS));

                byte[] bytesToSend = new byte[10] { command[0], command[1], 4, 0x00, slot_id, HOST_ID, 
                slot_number, ne_num,  // MsgID
                data1 [0], data1 [1]    // SetPoint
            };
                usb_write("otm_dac_functions", bytesToSend, 10);
                Thread.Sleep(20);
            }
        }

        private void btn_set_laser_power(object sender, RoutedEventArgs e)
        {
            Byte slot_number = SLOT4;
            Byte ne_num = 2;
            Byte slot_id = (Byte)(0x24 | 0x80); // slot 4 on otm

            float num = 0;
            num = float.Parse(cp_textbox[slot_number, 2].Text);
            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);

            if ((value >= 0) && (value <= 100))
            {
                // The Laser power setpoint (0 to 32767 -> 0% to 100% power).
                value = 32767 * (value / 100);
                byte[] data1 = BitConverter.GetBytes(Convert.ToInt16(value));

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_SET_PARAMS));

                byte[] bytesToSend = new byte[10] { command[0], command[1], 4, 0x00, slot_id, HOST_ID, 
                slot_number, ne_num,  // MsgID
                data1 [0], data1 [1]    // SetPoint
            };
                usb_write("otm_dac_functions", bytesToSend, 10);
                Thread.Sleep(20);
            }
        }

        private void bt_laser_power_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            button_num = 2;
            byte[] command;

            if (cp_button[slot_number, button_num].Content.ToString() == "Laser Off")
            {
                // turn laser on
                cp_button[slot_number, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, button_num].Content = "Laser On"; }));
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_ENABLEOUTPUT)); 
            }
            else
            {
                // turn laser off
                cp_button[slot_number, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, button_num].Content = "Laser Off"; }));
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_DISABLEOUTPUT)); 
            }

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, 0x00, slot_id, HOST_ID };
            usb_write("otm_dac_functions", bytesToSend, 6);
        }

        public void build_otm_dac_controls(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                string slot_name = (_slot + 1).ToString();
                string panel_name = "brd_slot_" + slot_name;
                byte label_num = 0;
                byte button_num = 0;
                byte textbox_num = 0;

                StackPanel dac_sel = (StackPanel)this.FindName(panel_name);

                // Top stack panel
                StackPanel top = new StackPanel();
                top.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(top);

                // slot label
                Label label = new Label();
                label.Content += "Slot " + slot_name;
                top.Children.Add(label);

                //  DAC label
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 90;
                cp_labels[_slot, label_num].Content += "OTM DAC";
                top.Children.Add(cp_labels[_slot, label_num]);
                // End Top stack panel

                // 2nd row stack panel
                StackPanel row_2 = new StackPanel();
                row_2.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(row_2);

                // End 2nd row stack panel

                // Laser path 1 label
                label_num = 0;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 180;
                cp_labels[_slot, label_num].Content += "Noise Eater Path 1 (0-100%)";
                row_2.Children.Add(cp_labels[_slot, label_num]);

                // Laser path 1 power textbox
                textbox_num = 0;
                cp_textbox[_slot, textbox_num] = new TextBox();
                cp_textbox[_slot, textbox_num].Name = "ne_1";
                cp_textbox[_slot, textbox_num].Text = "0";
                cp_textbox[_slot, textbox_num].Width = 80;
                cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_textbox[_slot, textbox_num]);

                // Laser path 1 power set Button
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Set";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "dac_set" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_set_noise_eater_1);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // 3rd row stack panel
                StackPanel row_3 = new StackPanel();
                row_3.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(row_3);

                // Laser path 2 label
                label_num = 1;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 180;
                cp_labels[_slot, label_num].Content += "Noise Eater Path 2 (0-100%)";
                row_3.Children.Add(cp_labels[_slot, label_num]);

                // Laser path 1 power textbox
                textbox_num = 1;
                cp_textbox[_slot, textbox_num] = new TextBox();
                cp_textbox[_slot, textbox_num].Name = "ne_2";
                cp_textbox[_slot, textbox_num].Text = "0";
                cp_textbox[_slot, textbox_num].Width = 80;
                cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_textbox[_slot, textbox_num]);

                // Laser path 1 power set Button
                button_num = 1;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Set";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "dac_set" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_set_noise_eater_2);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_button[_slot, button_num]);

                // 4th row stack panel
                StackPanel row_4 = new StackPanel();
                row_4.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(row_4);

                // Laser power set Button
                button_num = 2;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Laser Off";
                cp_button[_slot, button_num].Width = 100;
                cp_button[_slot, button_num].Name = "laser_power" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(bt_laser_power_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_4.Children.Add(cp_button[_slot, button_num]);

                // Laser level label
                label_num = 2;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 130;
                cp_labels[_slot, label_num].Content += "Laser Power (0-100%)";
                row_4.Children.Add(cp_labels[_slot, label_num]);

                // Laser power textbox
                textbox_num = 2;
                cp_textbox[_slot, textbox_num] = new TextBox();
                cp_textbox[_slot, textbox_num].Name = "laser_set" + slot_name;
                cp_textbox[_slot, textbox_num].Text = "0";
                cp_textbox[_slot, textbox_num].Width = 80;
                cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                row_4.Children.Add(cp_textbox[_slot, textbox_num]);

                // Laser power set Button
                button_num = 3;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Set";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "laser_power_set" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_set_laser_power);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_4.Children.Add(cp_button[_slot, button_num]);


            }));

        }



    }

}
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
        public void request_laser_params( byte MsgID, byte slot)
        {
            slot = (byte)(Convert.ToByte(slot + 1) + 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_REQ_PARAMS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], MsgID, 0x00, slot, HOST_ID };
            usb_write("otm_rs232_functions", bytesToSend, 6);
            Thread.Sleep(10);
        }

        private void get_laser_params()
        {
            byte MsgID = ext_data[0];
            byte _slot = ext_data[1];
            int textbox_num = 0;
            int label_num = 0;

            if (MsgID == 1)
            {
                // The Laser power setpoint (0 to 32767 -> 0% to 100% power).
                Int16 laser_power = (Int16)(ext_data[2] | (ext_data[3] << 8));
                double laser_power_f = Math.Round((laser_power / 32767.0), 2) * 100;

                this.Dispatcher.Invoke(new Action(() =>
                {
                    cp_textbox[_slot, textbox_num].Text = laser_power_f.ToString();
                }));
            }
            if (MsgID == 12)
            {
                double laser_temperature = Math.Round((Double)System.BitConverter.ToSingle(ext_data, 2), 4);
                double laser_temperature_f = (laser_temperature * 9) / 5 + 32;


                double laser_power = Math.Round((Double)System.BitConverter.ToSingle(ext_data, 6), 4);

                this.Dispatcher.Invoke(new Action(() =>
                {
                    label_num = 3;
                    cp_labels[_slot, label_num].Content = "Laser Power:  " + laser_power.ToString("0.0") + " W"; 

                    label_num = 4;
                    cp_labels[_slot, label_num].Content = "Laser Temperature: " + laser_temperature.ToString("0.0")
                        + " C (" + laser_temperature_f.ToString("0.0") + "F)";

                }));
            }
        }

        private void bt_laser_aming_off_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_DISABLEAIMING));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, 0, slot_id, HOST_ID };
            usb_write("otm_rs232_functions", bytesToSend, 6);
        }

        private void bt_laser_aming_on_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_ENABLEAIMING));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, 0, slot_id, HOST_ID };
            usb_write("otm_rs232_functions", bytesToSend, 6);
        }

        private void bt_laser_power_on_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_ENABLEOUTPUT));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, 0, slot_id, HOST_ID };
            usb_write("otm_rs232_functions", bytesToSend, 6);
        }

        private void bt_laser_power_off_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_DISABLEOUTPUT));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, 0, slot_id, HOST_ID };
            usb_write("otm_rs232_functions", bytesToSend, 6);
        }

                
        private void ipg_power_Handler(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                Byte slot_number = SLOT5;
                Byte slot_id = (Byte)(0x25 | 0x80); // slot 5 on otm

                float num = 0;
                num = float.Parse(cp_textbox[slot_number, 0].Text);
                string stringValue = num.ToString().Replace(',', '.');
                double value = Convert.ToDouble(stringValue);

                if ((value >= 40) && (value <= 100))
                {
                    // The Laser power setpoint (0 to 32767 -> 0% to 100% power).
                    value = 32767 * (value / 100);
                    byte[] data1 = BitConverter.GetBytes(Convert.ToInt16(value));

                    byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_SET_PARAMS));

                    byte[] bytesToSend = new byte[10] { command[0], command[1], 4, 0, slot_id, HOST_ID,
                    slot_number, 0,  // MsgID
                    data1 [0], data1 [1]    // SetPoint
            };
                    usb_write("otm_rs232_functions", bytesToSend, 10);
                    Thread.Sleep(20);
                }
                else
                {
                    MessageBox.Show("Laser power must be over 2 watts.  Enter values (40 - 100%)");

                }
            }
        }

        public void build_otm_rs232_controls(byte _slot)
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
                cp_labels[_slot, label_num].Width = 100;
                cp_labels[_slot, label_num].Content += "OTM IPG LASER";
                top.Children.Add(cp_labels[_slot, label_num]);
                // End Top stack panel

                // 2nd row stack panel
                StackPanel row_2 = new StackPanel();
                row_2.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(row_2);

                // End 2nd row stack panel

                // Amining beam label
                label_num = 0;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Content += "Aiming Beam";
                row_2.Children.Add(cp_labels[_slot, label_num]);

                // Amining beam Button off
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "OFF";
                cp_button[_slot, button_num].Width = 30;
                cp_button[_slot, button_num].Name = "ABF" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(bt_laser_aming_off_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Amining beam Button on
                button_num = 1;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "ON";
                cp_button[_slot, button_num].Width = 30;
                cp_button[_slot, button_num].Name = "ABN" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(bt_laser_aming_on_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Laser beam label
                label_num = 1;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Margin = new Thickness(25, 0, 0, 0);
                cp_labels[_slot, label_num].Content += "Laser";
                row_2.Children.Add(cp_labels[_slot, label_num]);

                // Laser beam Button off
                button_num = 2;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "OFF";
                cp_button[_slot, button_num].Width = 30;
                cp_button[_slot, button_num].Name = "EMOFF" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(bt_laser_power_off_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Laser beam Button on
                button_num = 3;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "ON";
                cp_button[_slot, button_num].Width = 30;
                cp_button[_slot, button_num].Name = "EMON" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(bt_laser_power_on_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Laser setpoint label
                label_num = 2;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Content += "        Laser Power Setpoint (0 - 100 %)";
                row_2.Children.Add(cp_labels[_slot, label_num]);

                // Laser setpoint textbox
                textbox_num = 0;
                cp_textbox[_slot, textbox_num] = new TextBox();
                cp_textbox[_slot, textbox_num].Name = "laser_setpoint";
                cp_textbox[_slot, textbox_num].Text = "0";
                cp_textbox[_slot, textbox_num].Width = 80;
                cp_textbox[_slot, textbox_num].KeyDown += new KeyEventHandler(ipg_power_Handler);
                cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_textbox[_slot, textbox_num]);

                // 3rd row stack panel
                StackPanel row_3 = new StackPanel();
                row_3.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(row_3);

                // Laser Temperature label
                label_num = 3;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Name = "ROP";
                cp_labels[_slot, label_num].Width = 140;
                cp_labels[_slot, label_num].Content += "Laser Power (W) 0";
                row_3.Children.Add(cp_labels[_slot, label_num]);

                // Laser Temperature label
                label_num = 4;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Name = "RCT";
                cp_labels[_slot, label_num].Width = 200;
                cp_labels[_slot, label_num].Content += "Laser Temperature 25.2 C (80.5) F";
                row_3.Children.Add(cp_labels[_slot, label_num]);

                // 4th row stack panel
                StackPanel row_4 = new StackPanel();
                row_4.Orientation = Orientation.Horizontal;
                dac_sel.Children.Add(row_4);


            }));

        }



    }

}
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
        public void request_postion(byte slot)
        {
            slot = (byte)(Convert.ToByte(slot + 1) + 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_BUTTONPARAMS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("servo_functions", bytesToSend, 6);
            Thread.Sleep(10);
        }

        public void update_pos_button_color(byte _slot, byte position)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                switch (position)
                {
                    case 0:     // 0, doesn't know where it is
                        cp_button[_slot, 2].Foreground = new SolidColorBrush(Colors.Red);       // P1
                        cp_button[_slot, 3].Foreground = new SolidColorBrush(Colors.Red);       // P2
                        break;

                    case 1:     // 1 = position 1
                        cp_button[_slot, 2].Foreground = new SolidColorBrush(Colors.Green);       // P1
                        cp_button[_slot, 3].Foreground = new SolidColorBrush(Colors.White);     // P2
                        break;

                    case 2:     // 2 = position 2
                        cp_button[_slot, 2].Foreground = new SolidColorBrush(Colors.White);       // P1
                        cp_button[_slot, 3].Foreground = new SolidColorBrush(Colors.Green);     // P2
                        break;
                }
            }));
        }

        public void servo_status_update()
        {
            byte position;

            byte _slot = ext_data[0];

            // pos 
            position = ext_data[4];

            update_pos_button_color(_slot, position);
        }

        private void servo_stop(byte slot_number)
        {
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            Byte dir = 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_STOP));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, dir, slot_id, HOST_ID };

            usb_write("servo_functions", bytesToSend, 6);
        }

        private void ButtonVel_CCW_move_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            Byte dir = 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_VELOCITY));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, dir, slot_id, HOST_ID };

            usb_write("servo_functions", bytesToSend, 6);

            Thread.Sleep(10);
            request_postion(slot_number);
        }

        private void ButtonVel_CW_move_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            Byte dir = 0;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_VELOCITY));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, dir, slot_id, HOST_ID };

            usb_write("servo_functions", bytesToSend, 6);

            Thread.Sleep(10);
            request_postion(slot_number);
        }

        private void ButtonVel_move_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            servo_stop(slot_number);
        }

        private void btnP1_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte chan_id = Convert.ToByte(slot);
            Byte slot_id = (Byte)((chan_id | 0x20) | 0x80);
            chan_id -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_BUTTONPARAMS));

            update_pos_button_color(chan_id, 1);

            byte[] bytesToSend = new byte[22] { command[0], command[1], 16, 0x00, slot_id, HOST_ID, 
                chan_id, 0x00, 
                0,0,            // Mode (not used)
                1, 0, 0, 0,     // Position 1 (for Mezoscope 1 = position 1, 2 = position 2)
                0,0,0,0,        // Position 2  (not used)
                0,0,            // TimeOut (not used)
                0,0            // (not used)
            };

            usb_write("servo_functions", bytesToSend, 22);
        }

        private void btnP2_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte chan_id = Convert.ToByte(slot);
            Byte slot_id = (Byte)((chan_id | 0x20) | 0x80);
            chan_id -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_BUTTONPARAMS));

            update_pos_button_color(chan_id, 2);


            byte[] bytesToSend = new byte[22] { command[0], command[1], 16, 0x00, slot_id, HOST_ID, 
                chan_id, 0x00, 
                0,0,            // Mode (not used)
                2, 0, 0, 0,     // Position 1 (for Mezoscope 1 = position 1, 2 = position 2)
                0,0,0,0,        // Position 2  (not used)
                0,0,            // TimeOut (not used)
                0,0            // (not used)
            };

            usb_write("servo_functions", bytesToSend, 22);
        }

        private void bt_servo_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            cb_servo_select_SelectionChanged(null, null);
        }

        private void save_servo_params_Click(object sender, RoutedEventArgs e)
        {
            // get the slot selected from the combo  box 
            byte slot_id = (byte)((Convert.ToByte(cb_servo_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_servo_select.SelectedIndex));

            Int32 lTransitTime_temp = Int32.Parse(tbServoTransit_time.Text);
            Int32 PulseWidth1_temp = Int32.Parse(tbServoSpeed.Text);

            //write limit switch params 22-6 = 16 bytes
            byte[] lTransitTime_t = BitConverter.GetBytes(lTransitTime_temp);
            byte[] PulseWidth1_t = BitConverter.GetBytes(PulseWidth1_temp);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_MFF_OPERPARAMS));
            byte[] bytesToSend = new byte[40] { command[0], command[1], 34, 0x00, slot_id, HOST_ID,
                chan_id, 0x00, 
                lTransitTime_t[0], lTransitTime_t[1], lTransitTime_t[2], lTransitTime_t[3], 
                0,0,0,0,    // lTransitTimeADC not used
                0,0,        // OperMode1 not used
                0,0,        // SigMode1 not used
                PulseWidth1_t[0], PulseWidth1_t[1], PulseWidth1_t[2], PulseWidth1_t[3], 
                0,0,        // OperMode2 not used
                0,0,        // SigMode2 not used
                0,0,0,0,    // PulseWidth2 not used
                0,0,0,0,    // not used
                0,0,0,0    // not used
                };

            usb_write("servo_functions", bytesToSend, 40);

            Thread.Sleep(10);
            request_postion(chan_id);
        }

        private void cb_servo_select_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // get the stage selected from teh combo  box 
            byte slot = (byte)(Convert.ToByte(cb_servo_select.SelectedIndex) + 0x21);

            byte slot_num = (byte)(Convert.ToByte(cb_servo_select.SelectedIndex));


            // request limit params
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_MFF_OPERPARAMS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("servo_functions", bytesToSend, 6);
            Thread.Sleep(10);
        }

        private void servo_get_params()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);

            // lTransitTime
            Int32 lTransitTime = (ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24);

            // pwm
            Int32 pwm = (ext_data[14] | ext_data[15] << 8 | ext_data[16] << 16 | ext_data[17] << 24);

            this.Dispatcher.Invoke(new Action(() =>
            {
                tbServoTransit_time.Text = lTransitTime.ToString();
                tbServoSpeed.Text = pwm.ToString();
            }));
        }

        public void build_servo_control(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                string slot_name = (_slot + 1).ToString();
                string panel_name = "brd_slot_" + slot_name;
                byte label_num = 0;
                byte button_num = 0;

                StackPanel sr_sel = (StackPanel)this.FindName(panel_name);

                // Top stack panel
                StackPanel top = new StackPanel();
                top.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(top);

                // slot label
                Label label = new Label();
                label.Content += "Slot " + slot_name;
                top.Children.Add(label);

                // Servo label
                label_num = 0;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 90;
                cp_labels[_slot, label_num].Content += "Servo Motor";
                top.Children.Add(cp_labels[_slot, label_num]);
                // End Top stack panel

                // 2nd row stack panel
                StackPanel row_2 = new StackPanel();
                row_2.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(row_2);

                // End 2nd row stack panel

                // CCW Move Button
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "<< Move";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "sr_home" + slot_name;
                cp_button[_slot, button_num].PreviewMouseLeftButtonDown += new MouseButtonEventHandler(ButtonVel_CCW_move_PreviewMouseLeftButtonDown);
                cp_button[_slot, button_num].PreviewMouseLeftButtonUp += new MouseButtonEventHandler(ButtonVel_move_PreviewMouseLeftButtonUp);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // CW Move Button
                button_num = 1;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Move >>";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "sr_home" + slot_name;
                cp_button[_slot, button_num].PreviewMouseLeftButtonDown += new MouseButtonEventHandler(ButtonVel_CW_move_PreviewMouseLeftButtonDown);
                cp_button[_slot, button_num].PreviewMouseLeftButtonUp += new MouseButtonEventHandler(ButtonVel_move_PreviewMouseLeftButtonUp);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Position 1 Button
                button_num = 2;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Position 1";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "sr_home" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnP1_Click);
                cp_button[_slot, button_num].Margin = new Thickness(55, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Position 2 Button
                button_num = 3;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Position 2";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "sr_home" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnP2_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

            }));

            // get the jog step size
            stepper_req_jog_params(_slot);
        }


    }

}
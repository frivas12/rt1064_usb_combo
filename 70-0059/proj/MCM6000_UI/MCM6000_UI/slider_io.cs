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
        const byte MIRROR_OUT = 0;
        const byte MIRROR_IN = 1;
        const byte MIRROR_UNKNOWN = 2;

        const byte MIRROR_MODE_DISABLED = 0;
        const byte MIRROR_MODE_DIRECT = 1;  // 5V
        const byte MIRROR_MODE_SIGNAL = 2;  // 24V using board on b-scope


        private void bt_io_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_io_select.SelectedIndex) + 0x21);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_MIRROR_PARAMS));
            byte[] bytesToSend = new byte[6];

            for (byte chan_id = 0; chan_id < 3; chan_id++)
            {
                bytesToSend = new byte[6] { command[0], command[1], chan_id, 0x00, slot, HOST_ID };
                usb_write("slider_io_functions", bytesToSend, 6);
                Thread.Sleep(10);
            }
        }

        // return from MGMSG_MCM_REQ_MIRROR_PARAMS
        private void mirrors_get_params()
        {
            byte chan_id = ext_data[0];

            byte mirror_initial_state = ext_data[2];
            byte mirror_mode = ext_data[3];
            byte pwm_low_val = ext_data[4];
            byte pwm_high_val = ext_data[5];

            if (mirror_mode == MIRROR_MODE_DIRECT)
            {
                default_params(chan_id, true);
                this.Dispatcher.Invoke(new Action(() =>
               {
                   if (chan_id == 0)
                   {
                       mirror_position_1.Value = 150;
                       cb_mirror_initial_state_1.SelectedIndex = mirror_initial_state;
                       cb_io_mode_1.SelectedIndex = mirror_mode;
                       tb_io_mirror_low_val_1.Text = pwm_low_val.ToString();
                       tb_io_mirror_high_val_1.Text = pwm_high_val.ToString();
                   }
                   else if (chan_id == 1)
                   {
                       mirror_position_2.Value = 150;
                       cb_mirror_initial_state_2.SelectedIndex = mirror_initial_state;
                       cb_io_mode_2.SelectedIndex = mirror_mode;
                       tb_io_mirror_low_val_2.Text = pwm_low_val.ToString();
                       tb_io_mirror_high_val_2.Text = pwm_high_val.ToString();
                   }
                   else if (chan_id == 2)
                   {
                       mirror_position_3.Value = 150;
                       cb_mirror_initial_state_3.SelectedIndex = mirror_initial_state;
                       cb_io_mode_3.SelectedIndex = mirror_mode;
                       tb_io_mirror_low_val_3.Text = pwm_low_val.ToString();
                       tb_io_mirror_high_val_3.Text = pwm_high_val.ToString();
                   }
               }));
            }
            else
            {
                this.Dispatcher.Invoke(new Action(() =>
               {
                   if (chan_id == 0)
                   {
                       cb_mirror_initial_state_1.SelectedIndex = mirror_initial_state;
                       cb_io_mode_1.SelectedIndex = mirror_mode;
                   }
                   else if (chan_id == 1)
                   {
                       cb_mirror_initial_state_2.SelectedIndex = mirror_initial_state;
                       cb_io_mode_2.SelectedIndex = mirror_mode;
                   }
                   else if (chan_id == 2)
                   {
                       cb_mirror_initial_state_3.SelectedIndex = mirror_initial_state;
                       cb_io_mode_3.SelectedIndex = mirror_mode;
                   }
               }));
                default_params(chan_id, false);
            }

        }

        private void mirror_position_1_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            byte slot;

            // get the stage selected from the combo  box 
            if (cb_io_select.SelectedIndex > 0)
                slot = (byte)(Convert.ToByte(cb_io_select.SelectedIndex) + 0x21);
            else
                return;

            Slider changedSlider = (Slider)sender;
            string slider_name = changedSlider.Name;
            string channel = slider_name.Substring(slider_name.Length - 1, 1);
            Byte chan_id = Convert.ToByte(channel);
            chan_id -= 1;

            byte pwm_val = (byte)changedSlider.Value;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_MIRROR_PWM_DUTY));
            byte[] bytesToSend = new byte[6];

            bytesToSend = new byte[6] { command[0], command[1], chan_id, pwm_val, slot, HOST_ID };
            usb_write("slider_io_functions", bytesToSend, 6);
            Thread.Sleep(20);
        }



        private void default_params( byte chan_id, bool enable)
        {
            if (chan_id == 0)
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    if (enable)
                    {
                        mirror_position_1.Value = 150;
                        mirror_position_1.IsEnabled = true;

                        tb_io_mirror_low_val_1.Text = "110";
                        tb_io_mirror_low_val_1.IsEnabled = true;

                        tb_io_mirror_high_val_1.Text = "180";
                        tb_io_mirror_high_val_1.IsEnabled = true;
                    }
                    else
                    {
                        mirror_position_1.Value = 150;
                        mirror_position_1.IsEnabled = false;

                        tb_io_mirror_low_val_1.Text = "110";
                        tb_io_mirror_low_val_1.IsEnabled = false;

                        tb_io_mirror_low_val_1.Text = "180";
                        tb_io_mirror_high_val_1.IsEnabled = false;
                    }
                }));
            }
            else if (chan_id == 1)
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    if (enable)
                    {
                        mirror_position_2.Value = 150;
                        mirror_position_2.IsEnabled = true;

                        tb_io_mirror_low_val_2.Text = "110";
                        tb_io_mirror_low_val_2.IsEnabled = true;

                        tb_io_mirror_high_val_2.Text = "180";
                        tb_io_mirror_high_val_2.IsEnabled = true;
                    }
                    else
                    {
                        mirror_position_2.Value = 150;
                        mirror_position_2.IsEnabled = false;

                        tb_io_mirror_low_val_2.Text = "110";
                        tb_io_mirror_low_val_2.IsEnabled = false;

                        tb_io_mirror_low_val_2.Text = "180";
                        tb_io_mirror_high_val_2.IsEnabled = false;
                    }
                }));
            }
            else if (chan_id == 2)
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    if (enable)
                    {
                        mirror_position_3.Value = 150;
                        mirror_position_3.IsEnabled = true;

                        tb_io_mirror_low_val_3.Text = "110";
                        tb_io_mirror_low_val_3.IsEnabled = true;

                        tb_io_mirror_high_val_3.Text = "180";
                        tb_io_mirror_high_val_3.IsEnabled = true;
                    }
                    else
                    {
                        mirror_position_3.Value = 150;
                        mirror_position_3.IsEnabled = false;

                        tb_io_mirror_low_val_3.Text = "110";
                        tb_io_mirror_low_val_3.IsEnabled = false;

                        tb_io_mirror_low_val_3.Text = "180";
                        tb_io_mirror_high_val_3.IsEnabled = false;
                    }
                }));
            }

        }

        private void cb_io_mode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox changedComboBox = (ComboBox)sender;
            string comboBox_name = changedComboBox.Name;
            string channel = comboBox_name.Substring(comboBox_name.Length - 1, 1);
            Byte chan_id = Convert.ToByte(channel);
            chan_id -= 1;
            bool enable;

            if (changedComboBox.SelectedIndex == MIRROR_MODE_DIRECT)
                enable = true;
            else
                enable = false;
            default_params(chan_id, enable);
        }


        public void request_mirror_postions(byte slot, byte chan)
        {
            slot = (byte)(Convert.ToByte(slot + 1) + 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_MIRROR_STATE));
            byte[] bytesToSend = new byte[6] { command[0], command[1], chan, 0x00, slot, HOST_ID };
            usb_write("slider_io_functions", bytesToSend, 6);
            Thread.Sleep(10);
        }

        public controls.flipper_shutter.MirrorStateEventArgs DeserializeMirrorState(byte[] data)
        {
            return new controls.flipper_shutter.MirrorStateEventArgs()
            {
                slot = (byte)(data[5] - Thorlabs.APT.Address.SLOT_1),
                channel = data[2],
                mirror_state = data[3],
            };
        }
        public void mirror_position_get()
        {
            var args = DeserializeMirrorState(header);
            var slot = args.slot;
            ++args.channel;
            button_num = Convert.ToByte(args.channel - 1);

            if (cp_button[slot, button_num] is null)
            { return; }

            if (args.IsOut())
            {
                cp_button[slot, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot, button_num].Content = "Mirror Out"; }));
            }
            else if (args.IsIn())
            {
                cp_button[slot, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot, button_num].Content = "Mirror In"; }));
            }
            else
            {
                cp_button[slot, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot, button_num].Content = "Unknown"; }));
            }
        
        }

        private void save_io_params_Click(object sender, RoutedEventArgs e)
        {
            byte mirror_initial_state = 0;
            byte mirror_mode = 0;
            byte pwm_low_val = 0;
            byte pwm_high_val = 0;

            // get the slot selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_io_select.SelectedIndex) + 0x21) | 0x80); ;

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string chan_id_s = button_name.Substring(button_name.Length - 1, 1);
            Byte chan_id = Convert.ToByte(chan_id_s);
            chan_id -= 1;   // chan _id starts at 0

            if (chan_id == 0)
            {
                mirror_initial_state = (byte)cb_mirror_initial_state_1.SelectedIndex;
                mirror_mode = (byte)cb_io_mode_1.SelectedIndex;
                pwm_low_val = byte.Parse(tb_io_mirror_low_val_1.Text);
                pwm_high_val = byte.Parse(tb_io_mirror_high_val_1.Text);
            }
            else if (chan_id == 1)
            {
                mirror_initial_state = (byte)cb_mirror_initial_state_2.SelectedIndex;
                mirror_mode = (byte)cb_io_mode_2.SelectedIndex;
                pwm_low_val = byte.Parse(tb_io_mirror_low_val_2.Text);
                pwm_high_val = byte.Parse(tb_io_mirror_high_val_2.Text);
            }
            else if (chan_id == 2)
            {
                mirror_initial_state = (byte)cb_mirror_initial_state_3.SelectedIndex;
                mirror_mode = (byte)cb_io_mode_3.SelectedIndex;
                pwm_low_val = byte.Parse(tb_io_mirror_low_val_3.Text);
                pwm_high_val = byte.Parse(tb_io_mirror_high_val_3.Text);
            }

            /*MGMSG_MORROR_SET_PWM => [CL | CH | chan_id | pwm_val | dest | source] */
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_MIRROR_PARAMS));
            byte[] bytesToSend = new byte[6+6] { command[0], command[1], 6, 0x00, slot, HOST_ID,
                chan_id, 0x00, 
                mirror_initial_state,
                mirror_mode,
                pwm_low_val,
                pwm_high_val
                };

            usb_write("slider_io_functions", bytesToSend, 6+6);

            Thread.Sleep(20);
        }


        public void build_io_control(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                string slot_name = (_slot + 1).ToString();
                string panel_name = "brd_slot_" + slot_name;
                byte label_num = 0;
                byte button_num = 0;

                StackPanel io_sel = (StackPanel)this.FindName(panel_name);

                // Top stack panel
                StackPanel top = new StackPanel();
                top.Orientation = Orientation.Horizontal;
                io_sel.Children.Add(top);

                // slot label
                Label label = new Label();
                label.Content += "Slot " + slot_name;
                top.Children.Add(label);

                // IO label
                label_num = 0;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 90;
                cp_labels[_slot, label_num].Content += "Slider IO";
                top.Children.Add(cp_labels[_slot, label_num]);
                // End Top stack panel

                // 2nd row stack panel
                StackPanel row_2 = new StackPanel();
                row_2.Orientation = Orientation.Horizontal;
                io_sel.Children.Add(row_2);

                // IO label
                label_num = 1;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 120;
                cp_labels[_slot, label_num].Content += "Galvo/Galvo Mirror:";
                row_2.Children.Add(cp_labels[_slot, label_num]);

                // Mirror 1 Button
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Unknown";
                cp_button[_slot, button_num].Width = 120;
                cp_button[_slot, button_num].Name = "io_mirror_0_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnMirror_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);
                // End 2nd row stack panel

                // 3rd row stack panel
                StackPanel row_3 = new StackPanel();
                row_3.Orientation = Orientation.Horizontal;
                io_sel.Children.Add(row_3);

                // IO label
                label_num = 2;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 120;
                cp_labels[_slot, label_num].Content += "Galvo/Res Mirror:";
                row_3.Children.Add(cp_labels[_slot, label_num]);

                // Mirror 1 Button
                button_num = 1;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Unknown";
                cp_button[_slot, button_num].Width = 120;
                cp_button[_slot, button_num].Name = "io_mirror_1_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnMirror_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_button[_slot, button_num]);
                // End 3rd row stack panel


                // 4th row stack panel
                StackPanel row_4 = new StackPanel();
                row_4.Orientation = Orientation.Horizontal;
                io_sel.Children.Add(row_4);

                // IO label
                label_num = 3;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 120;
                cp_labels[_slot, label_num].Content += "Cam/PMT Mirror:";
                row_4.Children.Add(cp_labels[_slot, label_num]);

                // Mirror 1 Button
                button_num = 2;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Unknown";
                cp_button[_slot, button_num].Width = 120;
                cp_button[_slot, button_num].Name = "io_mirror_2_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnMirror_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_4.Children.Add(cp_button[_slot, button_num]);
                // End 2nd row stack panel

            }));
        }

        bool IsBitSet(byte b, int pos)
        {
            return ((b >> pos) & 1) != 0;
        }

        private void btnMirror_Click(object sender, RoutedEventArgs e)
        {
            byte mirror_state = 0;

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            string mirror = button_name.Substring(button_name.Length - 3, 1);

            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte mirror_channel = Convert.ToByte(mirror); // 0,1,2

            if (clickedButton.Content.ToString() == "Mirror In")
            {
                cp_button[slot_number, mirror_channel].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, mirror_channel].Content = "Mirror Out"; }));
                mirror_state = 0;
            }
            else
            {
                cp_button[slot_number, mirror_channel].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, mirror_channel].Content = "Mirror In"; }));
                mirror_state = 1;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_MIRROR_STATE));
            byte[] bytesToSend = new byte[6] { command[0], command[1], mirror_channel, mirror_state, slot_id, HOST_ID };
            usb_write("slider_io_functions", bytesToSend, 6);

            Thread.Sleep(500);

        }
    }
}
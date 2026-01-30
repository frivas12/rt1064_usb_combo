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
        byte shutter_chan_next = 0;

        public void request_shutter_4_state(byte slot)
        {
            slot = (byte)(Convert.ToByte(slot + 1) + 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_SOL_STATE));
            byte[] bytesToSend = new byte[6];

            byte chan_id = shutter_chan_next;

            bytesToSend = new byte[6] { command[0], command[1], chan_id, 0x00, slot, HOST_ID };
            usb_write("shutter_4_functions", bytesToSend, 6);

            shutter_chan_next++;
            if (shutter_chan_next > 3)
                shutter_chan_next = 0;

            // get the interlock state
            if (shutter_chan_next == 0)
            {
                Thread.Sleep(15);
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_INTERLOCK_STATE));
                bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                usb_write("shutter_4_functions", bytesToSend, 6);
            }
        }

        private void save_shutter_4_params_Click(object sender, RoutedEventArgs e)
        {
            byte shutter_initial_state = 0;
            byte shutter_mode = 0;
            byte external_trigger_mode = 0;
            byte on_time = 0;
            Int32 duty_cycle_pulse = 0;
            Int32 duty_cycle_hold = 0;
            bool open_is_negative_voltage = false;
            uint holdoff_time = 0;
            
            // get the slot selected from the combo  box 
            byte slot_id = (byte)((Convert.ToByte(cb_shutter_4_select.SelectedIndex) + 0x21) | 0x80);

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string chan_id_s = button_name.Substring(button_name.Length - 1, 1);
            Byte chan_id = Convert.ToByte(chan_id_s);
            chan_id -= 1;   // chan _id starts at 0

            bool shouldSave = (chan_id switch
            {
                0 => e_chbx_4_shutter_persist_1,
                1 => e_chbx_4_shutter_persist_2,
                2 => e_chbx_4_shutter_persist_3,
                3 => e_chbx_4_shutter_persist_4,
                _ => null
            })?.IsChecked ?? false;

            if (chan_id == 0)
            {
                shutter_initial_state = (byte)cb_shutter_4_initial_state_1.SelectedIndex;
                shutter_mode = (byte)cb_shutter_4_mode_1.SelectedIndex;
                external_trigger_mode = (byte)cb_shutter_4_external_trigger_mode_1.SelectedIndex;
                on_time = byte.Parse(tbShutter_4_Pulse_time_1.Text);
                duty_cycle_pulse = Int32.Parse(tbShutter_4_Voltage_1.Text);
                duty_cycle_hold = Int32.Parse(tbShutter_4_HoldVoltage_1.Text);
                open_is_negative_voltage = "Negative".Equals(cb_shutter_4_voltage_reverse_1.SelectedItem);
                holdoff_time = uint.Parse(tbShutter_4_Holdoff_1.Text);
            }
            else if (chan_id == 1)
            {
                shutter_initial_state = (byte)cb_shutter_4_initial_state_2.SelectedIndex;
                shutter_mode = (byte)cb_shutter_4_mode_2.SelectedIndex;
                external_trigger_mode = (byte)cb_shutter_4_external_trigger_mode_2.SelectedIndex;
                on_time = byte.Parse(tbShutter_4_Pulse_time_2.Text);
                duty_cycle_pulse = Int32.Parse(tbShutter_4_Voltage_2.Text);
                duty_cycle_hold = Int32.Parse(tbShutter_4_HoldVoltage_2.Text);
                open_is_negative_voltage = "Negative".Equals(cb_shutter_4_voltage_reverse_2.SelectedItem);
                holdoff_time = uint.Parse(tbShutter_4_Holdoff_2.Text);
            }
            else if (chan_id == 2)
            {
                shutter_initial_state = (byte)cb_shutter_4_initial_state_3.SelectedIndex;
                shutter_mode = (byte)cb_shutter_4_mode_3.SelectedIndex;
                external_trigger_mode = (byte)cb_shutter_4_external_trigger_mode_3.SelectedIndex;
                on_time = byte.Parse(tbShutter_4_Pulse_time_3.Text);
                duty_cycle_pulse = Int32.Parse(tbShutter_4_Voltage_3.Text);
                duty_cycle_hold = Int32.Parse(tbShutter_4_HoldVoltage_3.Text);
                open_is_negative_voltage = "Negative".Equals(cb_shutter_4_voltage_reverse_3.SelectedItem);
                holdoff_time = uint.Parse(tbShutter_4_Holdoff_3.Text);
            }
            else if (chan_id == 3)
            {
                shutter_initial_state = (byte)cb_shutter_4_initial_state_4.SelectedIndex;
                shutter_mode = (byte)cb_shutter_4_mode_4.SelectedIndex;
                external_trigger_mode = (byte)cb_shutter_4_external_trigger_mode_4.SelectedIndex;
                on_time = byte.Parse(tbShutter_4_Pulse_time_4.Text);
                duty_cycle_pulse = Int32.Parse(tbShutter_4_Voltage_4.Text);
                duty_cycle_hold = Int32.Parse(tbShutter_4_HoldVoltage_4.Text);
                open_is_negative_voltage = "Negative".Equals(cb_shutter_4_voltage_reverse_4.SelectedItem);
                holdoff_time = uint.Parse(tbShutter_4_Holdoff_4.Text);
            }

            byte[] duty_cycle_pulse_t = BitConverter.GetBytes(duty_cycle_pulse);
            byte[] duty_cycle_hold_t = BitConverter.GetBytes(duty_cycle_hold);
            byte[] holdoff_time_t = BitConverter.GetBytes(holdoff_time);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SHUTTERPARAMS));
            byte[] bytesToSend = { command[0], command[1], 19, 0x00, slot_id, HOST_ID,
                chan_id, 0x00, // 6-7
                shutter_initial_state, // 8
                shutter_mode,   // 9
                external_trigger_mode, // 10
                on_time,   // 11
                duty_cycle_pulse_t[0], duty_cycle_pulse_t[1], duty_cycle_pulse_t[2], duty_cycle_pulse_t[3], //12 - 15
                duty_cycle_hold_t[0], duty_cycle_hold_t[1], duty_cycle_hold_t[2], duty_cycle_hold_t[3], // 15-19
                (byte)(open_is_negative_voltage ? 1 : 0), // 20
                holdoff_time_t[0], holdoff_time_t[1], holdoff_time_t[2], holdoff_time_t[3], // 21 - 24
                };

            usb_write("shutter_4_functions", bytesToSend, bytesToSend.Length);

            Thread.Sleep(20);

            if (shouldSave)
            {
                byte[] saveCommand = BitConverter.GetBytes(Convert.ToInt16(MGMSG_MOT_SET_EEPROMPARAMS));
                bytesToSend = [ saveCommand[0], saveCommand[1], 4, 0x00, slot_id, HOST_ID,
                chan_id, 0x00, // 6-7
                command[0], command[1] ];

                usb_write("shutter_4_functions", bytesToSend, 10);
            }

        }

        private void bt_shutter_4_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_shutter_4_select.SelectedIndex) + 0x21);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_SHUTTERPARAMS));
            byte[] bytesToSend = new byte[6];

            for (byte chan_id = 0; chan_id < 4; chan_id++)
            {
                bytesToSend = new byte[6] { command[0], command[1], chan_id, 0x00, slot, HOST_ID };
                usb_write("shutter_4_functions", bytesToSend, 6);
                Thread.Sleep(10);
            }
        }

        private void shutter_4_get_params(byte slot, CardTypes type)
        {
            switch(type)
            {
                case CardTypes.MCM_Flipper_Shutter:
                case CardTypes.MCM_Flipper_Shutter_REVA:
                    shutter_4_get_params(slot, ext_data[0]);
                    break;

                default:
                    shutter_4_get_params(slot, ext_data[1]);
                    break;
            }
        }

        // return from MGMSG_MCM_REQ_SHUTTERPARAMS
        private void shutter_4_get_params(byte slot, byte chan_id)
        {
            byte shutter_initial_state = ext_data[2];
            byte shutter_mode = ext_data[3];
            byte external_trigger_mode = ext_data[4];
            byte on_time = ext_data[5];
            Int32 duty_cycle_pulse = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);
            Int32 duty_cycle_hold = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);
            bool open_is_negative_voltage = false;
            uint holdoff_time = 0;

            if (ext_data.Length >= 15)
            {
                open_is_negative_voltage = ext_data[14] != 0;
            }

            if (ext_data.Length >= 19)
            {
                holdoff_time = (uint)(ext_data[15] | ext_data[16] << 8 | ext_data[17] << 16 | ext_data[18] << 24);
            }

            if ((int)slot == Dispatcher.Invoke(() => cb_shutter_4_select.SelectedIndex))
            {
                if (chan_id == 0)
                {
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        if (shutter_initial_state == SHUTTER_CLOSED)
                            cb_shutter_4_initial_state_1.SelectedIndex = SHUTTER_CLOSED;
                        else
                            cb_shutter_4_initial_state_1.SelectedIndex = SHUTTER_OPENED;

                        cb_shutter_4_mode_1.SelectedIndex = shutter_mode;

                        cb_shutter_4_external_trigger_mode_1.SelectedIndex = external_trigger_mode;

                        tbShutter_4_Pulse_time_1.Text = on_time.ToString();

                        tbShutter_4_Voltage_1.Text = duty_cycle_pulse.ToString();

                        tbShutter_4_HoldVoltage_1.Text = duty_cycle_hold.ToString();

                        if (open_is_negative_voltage)
                            cb_shutter_4_voltage_reverse_1.SelectedIndex = 1;
                        else
                            cb_shutter_4_voltage_reverse_1.SelectedIndex = 0;

                        tbShutter_4_Holdoff_1.Text = holdoff_time.ToString();
                    }));
                }
                else if (chan_id == 1)
                {
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        if (shutter_initial_state == SHUTTER_CLOSED)
                            cb_shutter_4_initial_state_2.SelectedIndex = SHUTTER_CLOSED;
                        else
                            cb_shutter_4_initial_state_2.SelectedIndex = SHUTTER_OPENED;

                        cb_shutter_4_mode_2.SelectedIndex = shutter_mode;

                        cb_shutter_4_external_trigger_mode_2.SelectedIndex = external_trigger_mode;

                        tbShutter_4_Pulse_time_2.Text = on_time.ToString();

                        tbShutter_4_Voltage_2.Text = duty_cycle_pulse.ToString();

                        tbShutter_4_HoldVoltage_2.Text = duty_cycle_hold.ToString();

                        if (open_is_negative_voltage)
                            cb_shutter_4_voltage_reverse_2.SelectedIndex = 1;
                        else
                            cb_shutter_4_voltage_reverse_2.SelectedIndex = 0;

                        tbShutter_4_Holdoff_2.Text = holdoff_time.ToString();
                    }));
                }
                else if (chan_id == 2)
                {
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        if (shutter_initial_state == SHUTTER_CLOSED)
                            cb_shutter_4_initial_state_3.SelectedIndex = SHUTTER_CLOSED;
                        else
                            cb_shutter_4_initial_state_3.SelectedIndex = SHUTTER_OPENED;

                        cb_shutter_4_mode_3.SelectedIndex = shutter_mode;

                        cb_shutter_4_external_trigger_mode_3.SelectedIndex = external_trigger_mode;

                        tbShutter_4_Pulse_time_3.Text = on_time.ToString();

                        tbShutter_4_Voltage_3.Text = duty_cycle_pulse.ToString();

                        tbShutter_4_HoldVoltage_3.Text = duty_cycle_hold.ToString();

                        if (open_is_negative_voltage)
                            cb_shutter_4_voltage_reverse_3.SelectedIndex = 1;
                        else
                            cb_shutter_4_voltage_reverse_3.SelectedIndex = 0;

                        tbShutter_4_Holdoff_3.Text = holdoff_time.ToString();
                    }));
                }
                else if (chan_id == 3)
                {
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        if (shutter_initial_state == SHUTTER_CLOSED)
                            cb_shutter_4_initial_state_4.SelectedIndex = SHUTTER_CLOSED;
                        else
                            cb_shutter_4_initial_state_4.SelectedIndex = SHUTTER_OPENED;

                        cb_shutter_4_mode_4.SelectedIndex = shutter_mode;

                        cb_shutter_4_external_trigger_mode_4.SelectedIndex = external_trigger_mode;

                        tbShutter_4_Pulse_time_4.Text = on_time.ToString();

                        tbShutter_4_Voltage_4.Text = duty_cycle_pulse.ToString();

                        tbShutter_4_HoldVoltage_4.Text = duty_cycle_hold.ToString();

                        if (open_is_negative_voltage)
                            cb_shutter_4_voltage_reverse_4.SelectedIndex = 1;
                        else
                            cb_shutter_4_voltage_reverse_4.SelectedIndex = 0;

                        tbShutter_4_Holdoff_4.Text = holdoff_time.ToString();
                    }));
                }
            }

            foreach (var window in flipper_shutter_windows)
            {
                window.HandleEvent(new controls.flipper_shutter.ShutterParamsEventArgs()
                {
                    channel = chan_id,
                    slot = slot,
                    power_up_state = (controls.drivers.IShutterDriver.States)shutter_initial_state,
                    type = (controls.drivers.IShutterDriver.Types)shutter_mode,
                    external_trigger_mode = (controls.drivers.IShutterDriver.TriggerModes)external_trigger_mode,
                    duty_cycle_percentage_hold = duty_cycle_hold / 1900d * 100d,
                    duty_cycle_percentage_pulse = duty_cycle_pulse / 1900d * 100d,
                    pulse_width = TimeSpan.FromMilliseconds(10 * on_time),
                    open_uses_negative_voltage = open_is_negative_voltage,
                    holdoff_time = TimeSpan.FromMilliseconds((double)holdoff_time / 10),
                });
            }
        }

        //  return from MGMSG_MOT_REQ_SOL_STATE
        private void shutter_4_get_state()
        {
            byte slot = (byte)(Convert.ToByte(header[5]) - 0x21);
            byte chan_id = (byte)(Convert.ToByte(header[2]) + 1);
            byte state = header[3];
            button_num = Convert.ToByte(chan_id-1);

            if (cp_button[slot, button_num] is not null)
            {
                if (state == SHUTTER_OPENED)
                {
                    cp_button[slot, button_num].Dispatcher.Invoke(new Action(() =>
                    { cp_button[slot, button_num].Content = "Shutter " + chan_id.ToString() + " Opened"; }));
                }
                else
                {
                    cp_button[slot, button_num].Dispatcher.Invoke(new Action(() =>
                    { cp_button[slot, button_num].Content = "Shutter " + chan_id.ToString() + " Closed"; }));
                }
            }

            foreach (var item in flipper_shutter_windows)
            {
                item.HandleEvent(new controls.flipper_shutter.SolenoidStateEventArgs()
                {
                    channel = chan_id - 1,
                    slot = slot,
                    solenoid_on = state == SHUTTER_OPENED,
                });
            }
        }

        //  return from MGMSG_MCM_GET_INTERLOCK_STATE
        private void shutter_get_interlock_state()
        {
            byte slot = (byte)(Convert.ToByte(header[5]) - 0x21);
            bool interlock_state = (bool)(Convert.ToBoolean(header[2]));

            // 0 means interlock not installed or trinoc in eyepiece mode
            // 1 means interlock good and trinoc in camera mode

            this.Dispatcher.Invoke(new Action(() =>
            {
                cp_radio[slot, 0].IsChecked = interlock_state;
            }));
        }

        private void btn_shutter_4_Click(object sender, RoutedEventArgs e)
        {
            // Shutter 1 is on SLOT 2
            // Shutter 2 is on SLOT 3

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            string channel = button_name.Substring(button_name.Length - 3, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte chan_id = Convert.ToByte(channel);
            chan_id -= 1;   // chan _id starts at 0
            button_num = Convert.ToByte(chan_id);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte state = 0;

            if (cp_button[slot_number, button_num].Content.ToString() == "Shutter " + channel + " Closed")
            {
                // open the shutter
                cp_button[slot_number, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, button_num].Content = "Shutter " + channel + " Opened"; }));
                state = SHUTTER_OPENED;
            }
            else
            {
                // close the shutter
                cp_button[slot_number, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, button_num].Content = "Shutter " + channel + " Closed"; }));
                state = SHUTTER_CLOSED;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_SOL_STATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], chan_id, state, slot_id, HOST_ID };
            usb_write("shutter_4_functions", bytesToSend, 6);
        }



        public void build_shutter_4_controls(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                string slot_name = (_slot + 1).ToString();
                string panel_name = "brd_slot_" + slot_name;
                byte label_num = 0;
                byte button_num = 0;
                byte radio_num = 0;

                StackPanel sr_sel = (StackPanel)this.FindName(panel_name);

                // Top stack panel
                StackPanel top = new StackPanel();
                top.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(top);

                // slot label
                Label label = new Label();
                label.Content += "Slot " + slot_name;
                top.Children.Add(label);

                // Shutter label
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 90;
                cp_labels[_slot, label_num].Content += "Shutter";
                top.Children.Add(cp_labels[_slot, label_num]);
                // End Top stack panel

                // 2nd row stack panel
                StackPanel row_2 = new StackPanel();
                row_2.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(row_2);

                // Interlock radio button
                cp_radio[_slot, radio_num] = new RadioButton();
                cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                cp_radio[_slot, radio_num].Content = "Interlock Good";
                cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                cp_radio[_slot, radio_num].Margin = new Thickness(5, 1, 5, 0);
                row_2.Children.Add(cp_radio[_slot, radio_num]);
                // End 2nd row stack panel

                // 3nd row stack panel
                StackPanel row_3 = new StackPanel();
                row_3.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(row_3);


                // Shutter 1 Button
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Shutter 1 Closed";
                cp_button[_slot, button_num].Width = 130;
                cp_button[_slot, button_num].Name = "shutter_1_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_shutter_4_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_button[_slot, button_num]);

                // Shutter 2 Button
                button_num = 1;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Shutter 2 Closed";
                cp_button[_slot, button_num].Width = 130;
                cp_button[_slot, button_num].Name = "shutter_2_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_shutter_4_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_button[_slot, button_num]);

                // Shutter 3 Button
                button_num = 2;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Shutter 3 Closed";
                cp_button[_slot, button_num].Width = 130;
                cp_button[_slot, button_num].Name = "shutter_3_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_shutter_4_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_button[_slot, button_num]);

                // Shutter 4 Button
                button_num = 3;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Shutter 4 Closed";
                cp_button[_slot, button_num].Width = 130;
                cp_button[_slot, button_num].Name = "shutter_4_" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_shutter_4_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_3.Children.Add(cp_button[_slot, button_num]);
                // End 3nd row stack panel

            }));

        }



    }

}
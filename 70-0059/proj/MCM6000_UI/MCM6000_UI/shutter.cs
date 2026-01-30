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
       public void request_shutter_state(byte slot)
        {
            slot = (byte)(Convert.ToByte(slot + 1) + 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_SOL_STATE));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("shutter_functions", bytesToSend, 6);
            Thread.Sleep(10);
        }

        private void save_shutter_params_Click(object sender, RoutedEventArgs e)
        {
            // get the slot selected from the combo  box 
            byte slot_id = (byte)((Convert.ToByte(cb_shutter_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_shutter_select.SelectedIndex));

            byte shutter_initial_state = (byte)cb_sutter_initial_state.SelectedIndex;
            byte shutter_mode = (byte)cb_sutter_mode.SelectedIndex;
            byte external_trigger_mode = (byte)cb_sutter_external_trigger_mode.SelectedIndex;
            byte on_time = byte.Parse(tbShutterPulse_time.Text);
            Int32 duty_cycle_pulse = Int32.Parse(tbShutterVoltage.Text);
            Int32 duty_cycle_hold = Int32.Parse(tbShutterHoldVoltage.Text);

            byte[] duty_cycle_pulse_t = BitConverter.GetBytes(duty_cycle_pulse);
            byte[] duty_cycle_hold_t = BitConverter.GetBytes(duty_cycle_hold);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SHUTTERPARAMS));
            byte[] bytesToSend = new byte[20] { command[0], command[1], 14, 0x00, slot_id, HOST_ID,
                chan_id, 0x00, 
                shutter_initial_state,
                shutter_mode,
                external_trigger_mode,
                on_time,   
                duty_cycle_pulse_t[0], duty_cycle_pulse_t[1], duty_cycle_pulse_t[2], duty_cycle_pulse_t[3],
                duty_cycle_hold_t[0], duty_cycle_hold_t[1], duty_cycle_hold_t[2], duty_cycle_hold_t[3] 
                };

            usb_write("shutter_functions", bytesToSend, 20);

            Thread.Sleep(20);

        }

        private void bt_shutter_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_shutter_select.SelectedIndex) + 0x21);

            byte slot_num = (byte)(Convert.ToByte(cb_shutter_select.SelectedIndex));

            // request params
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_SHUTTERPARAMS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("shutter_functions", bytesToSend, 6);
            Thread.Sleep(10);
        }

        private void shutter_get_params()
        {
            byte slot = ext_data[0];

            byte shutter_initial_state = ext_data[2];
            byte shutter_mode = ext_data[3];
            byte external_trigger_mode = ext_data[4];
            byte on_time = ext_data[5];
            Int32 duty_cycle_pulse = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);
            Int32 duty_cycle_hold = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);

            this.Dispatcher.Invoke(new Action(() =>
            {
                if (shutter_initial_state == SHUTTER_CLOSED)
                    cb_sutter_initial_state.SelectedIndex = SHUTTER_CLOSED;
                else
                    cb_sutter_initial_state.SelectedIndex = SHUTTER_OPENED;

                cb_sutter_mode.SelectedIndex = shutter_mode;

                cb_sutter_external_trigger_mode.SelectedIndex = external_trigger_mode;

                tbShutterPulse_time.Text = on_time.ToString();

                tbShutterVoltage.Text = duty_cycle_pulse.ToString();

                tbShutterHoldVoltage.Text = duty_cycle_hold.ToString();
            }));

        }

        private void shutter_get_state()
        {
            byte slot = header[2];
            byte state = header[3];

            this.Dispatcher.Invoke(new Action(() =>
            {
                if (cp_button[slot, button_num] is null)
                { return; }
                if (state == SHUTTER_OPENED)
                {
                    cp_button[slot, button_num].Content = "Shutter Opened";
                    if (current_main_tab == MesoTab)
                    {
                        btnToggle_shutter.Content = "Shutter Opened";
                    }
                }
                else
                {
                    cp_button[slot, button_num].Content = "Shutter Closed";
                    if (current_main_tab == MesoTab)
                    {
                        btnToggle_shutter.Content = "Shutter Closed";
                    }
                }
            }));

        }

        private void btn_shutter_Click(object sender, RoutedEventArgs e)
        {
            // Shutter 1 is on SLOT 2
            // Shutter 2 is on SLOT 3

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;
            Byte slot_id = (Byte)((slot_number + 1) | 0x20);

            byte state = 0;

            if (cp_button[slot_number, button_num].Content.ToString() == "Shutter Closed")
            {
                // open the shutter
                cp_button[slot_number, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, button_num].Content = "Shutter Opened"; }));
                state = SHUTTER_OPENED;
            }
            else
            {
                // close the shutter
                cp_button[slot_number, button_num].Dispatcher.Invoke(new Action(() =>
                { cp_button[slot_number, button_num].Content = "Shutter Closed"; }));
                state = SHUTTER_CLOSED;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_SOL_STATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, state, slot_id, HOST_ID };
            usb_write("shutter_functions", bytesToSend, 6);
            clear_focus();
        }



        public void build_shutter_controls(byte _slot)
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

                // Shutter Button
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Shutter Closed";
                cp_button[_slot, button_num].Width = 150;
                cp_button[_slot, button_num].Name = "shutter" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_shutter_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);
                // End 2nd row stack panel

            }));

        }



    }

}
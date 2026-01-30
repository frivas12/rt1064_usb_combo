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
        // variables need which are grabbed when the UI is started
        static double[] slot_nm_per_count = new double[NUM_OF_SLOTS];
        static double[] slot_counts_per_unit = new double[NUM_OF_SLOTS];
        static Int16[] cw_hardlimit_save = new Int16[NUM_OF_SLOTS];
        static Int16[] ccw_hardlimit_save = new Int16[NUM_OF_SLOTS];
        static Int16[] limit_mode_save = new Int16[NUM_OF_SLOTS];

        static bool[] _stepper_initialized = new bool[NUM_OF_SLOTS];  // Flags used to identify that a device's data has been populated.

        /* Stepper update on _timer1_Elapsed*/
        public void stepper_update(byte _slot)
        {
            Byte slot_id = (Byte)((_slot + 1) | 0x20);
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_STATUSUPDATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
            usb_write("stepper_functions", bytesToSend, 6);
        }

        /* MCM Stepper update on _timer1_Elapsed*/
        public void mcm_stepper_update(byte _slot)
        {
            Byte slot_id = (Byte)((_slot + 1) | 0x20);
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_STATUSUPDATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
            usb_write("stepper_functions", bytesToSend, 6);
            Thread.Sleep(30);
        }

        public void stepper_status_update()
        {
            double step, enc;

            byte _slot = ext_data[0];

            // step counts 
            step = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
            double nm_per_step = (slot_nm_per_count[_slot] / (slot_counts_per_unit[_slot]) * 100000);
            step = step * nm_per_step / 1e3;

            // encoder counts 
            enc = ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24;
            enc = enc * slot_nm_per_count[_slot] / 1e3;

        }

        public void mcm_stepper_status_update()
        {
            double step, enc, enc_raw;
            byte current_stored_position;
            byte _slot = ext_data[0];

            try
            {
                // step counts 
                int int_step =  ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
                double nm_per_step = (slot_nm_per_count[_slot] / (slot_counts_per_unit[_slot]) * 100000);
                step = int_step * nm_per_step / 1e3;

                // encoder counts 
                int int_enc = ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24;
                enc = int_enc * slot_nm_per_count[_slot] / 1e3;

                byte chan_enable = ext_data[13];
                bool stage_connected = (ext_data[11] & 0x01) > 0;

                // Current store position, 0 to 10 , if not at any stored position 0xff
                current_stored_position = ext_data[14];

                // encoder counts raw
                int int_enc_raw = ext_data[15] | ext_data[16] << 8 | ext_data[17] << 16 | ext_data[18] << 24;
                enc_raw = int_enc_raw * slot_nm_per_count[_slot] / 1e3;

                this.Dispatcher.Invoke(new Action(() =>
                {
                    // *** for mesoscope tab ***
                    if (current_main_tab == MesoTab)
                    {
                        if (_slot == 0)
                        {
                            lblXPos.Text = enc.ToString("0,0.0");
                            rd_CCW_limit_X.IsChecked = (ext_data[10] & (1 << 1)) > 0;
                            rd_CW_limit_X.IsChecked = (ext_data[10] & (1 << 0)) > 0;
                            rd_CCW_moving_X.IsChecked = (ext_data[10] & (1 << 5)) > 0;
                            rd_CW_moving_X.IsChecked = (ext_data[10] & (1 << 4)) > 0;
                            rd_meso_CW_limit_X.IsChecked = (ext_data[10] & (1 << 2)) > 0;
                            rd_meso_CCW_limit_X.IsChecked = (ext_data[10] & (1 << 3)) > 0;

                        }
                        else if (_slot == 1)
                        {
                            lblYPos.Text = enc.ToString("0,0.0");
                            rd_CCW_limit_Y.IsChecked = (ext_data[10] & (1 << 1)) > 0;
                            rd_CW_limit_Y.IsChecked = (ext_data[10] & (1 << 0)) > 0;
                            rd_CCW_moving_Y.IsChecked = (ext_data[10] & (1 << 5)) > 0;
                            rd_CW_moving_Y.IsChecked = (ext_data[10] & (1 << 4)) > 0;
                            rd_meso_CW_limit_Y.IsChecked = (ext_data[10] & (1 << 2)) > 0;
                            rd_meso_CCW_limit_Y.IsChecked = (ext_data[10] & (1 << 3)) > 0;
                        }

                        else if (_slot == 2)
                        {
                            lblZPos.Text = enc.ToString("0,0.0");
                            rd_CCW_limit_Z.IsChecked = (ext_data[10] & (1 << 1)) > 0;
                            rd_CW_limit_Z.IsChecked = (ext_data[10] & (1 << 0)) > 0;
                            rd_CCW_moving_Z.IsChecked = (ext_data[10] & (1 << 5)) > 0;
                            rd_CW_moving_Z.IsChecked = (ext_data[10] & (1 << 4)) > 0;
                            rd_meso_CW_limit_Z.IsChecked = (ext_data[10] & (1 << 2)) > 0;
                            rd_meso_CCW_limit_Z.IsChecked = (ext_data[10] & (1 << 3)) > 0;
                        }
                        else if (_slot == 3)
                        {
                            lblRPos.Text = enc.ToString("0,0.000");
                            rd_CCW_limit_R.IsChecked = (ext_data[10] & (1 << 1)) > 0;
                            rd_CW_limit_R.IsChecked = (ext_data[10] & (1 << 0)) > 0;
                            rd_CCW_moving_R.IsChecked = (ext_data[10] & (1 << 5)) > 0;
                            rd_CW_moving_R.IsChecked = (ext_data[10] & (1 << 4)) > 0;
                            rd_meso_CW_limit_R.IsChecked = (ext_data[10] & (1 << 2)) > 0;
                            rd_meso_CCW_limit_R.IsChecked = (ext_data[10] & (1 << 3)) > 0;
                        }
                    }

                    SolidColorBrush Color = new SolidColorBrush();

                    // if on limit
                    if (((ext_data[10] & 0x02) > 0) || ((ext_data[10] & 0x01) > 0) || ((ext_data[10] & 0x08) > 0) || ((ext_data[10] & 0x04) > 0) || !stage_connected)
                    {
                        Color = System.Windows.Media.Brushes.Red;
                    }
                    // cw or ccw moving
                    else if (((ext_data[10] & 0x20) > 0) || ((ext_data[10] & 0x10) > 0))
                    {
                        Color = System.Windows.Media.Brushes.Blue;
                    }
                    else
                    {
                        Color = System.Windows.Media.Brushes.Green;
                    }

                    // *** for mesoscope tab ***
                    if (current_main_tab == MesoTab)
                    {
                        if (_slot == 0)
                        {
                            _meso_border_1.BorderBrush = Color;
                        }
                        else if (_slot == 1)
                        {
                            _meso_border_2.BorderBrush = Color;
                        }
                        else if (_slot == 2)
                        {
                            _meso_border_3.BorderBrush = Color;
                        }
                        else if (_slot == 3)
                        {
                            _meso_border_4.BorderBrush = Color;
                        }
                    }

                    controls.stepper.OnStatusUpdateArgs args = new();
                    args.Slot = _slot;
                    args.StepperCount = int_step;
                    args.EncoderCount = int_enc;
                    args.EncoderRawCount = int_enc_raw;
                    args.OnCWHardLimit = (ext_data[10] & (1 << 0)) > 0;
                    args.OnCCWHardLimit = (ext_data[10] & (1 << 1)) > 0;
                    args.OnCWSoftLimit = (ext_data[10] & (1 << 2)) > 0;
                    args.OnCCWSoftLimit = (ext_data[10] & (1 << 3)) > 0;
                    args.MovingCW = (ext_data[10] & (1 << 4)) > 0;
                    args.MovingCCW = (ext_data[10] & (1 << 5)) > 0;
                    args.IsHoming = (ext_data[11] & (1 << 1)) > 0;
                    args.IsHomed = (ext_data[11] & (1 << 2)) > 0;
                    args.MagnetTooClose = (ext_data[11] & (1 << 6)) > 0;
                    args.MagnetTooFar = (ext_data[11] & (1 << 7)) > 0;
                    args.MagnetNotReady = (ext_data[11] & (1 << 8)) > 0;
                    args.ChannelEnabled = (ext_data[13] & 0x80) > 0;
                    args.MotorConnected = (ext_data[11] & 0x01) > 0;
                    args.StoredPositionOn = ext_data[14];

                    foreach (var v in stepper_windows)
                    {
                        if (v.Item2.Slot == _slot)
                        {
                            v.Item1.InvokeStatusUpdate(this, args);
                        }
                    }
                }));
            }
            catch (Exception)
            {

            }


        }

        private void btnHome_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_HOME));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            //clear_focus();
        }

        private void btnZero_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_ENCCOUNTER));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
                slot_number, 0x00,
                0x00, 0x00, 0x00, 0x00 };

            usb_write("stepper_functions", bytesToSend, 12);
            //clear_focus();
        }

        private void btnStop_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_STOP));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, slot_id, HOST_ID };
            usb_write("stepper_functions", bytesToSend, 6);
            clear_focus_fast();
        }

        private void btn_set_cw_soft_lim_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SOFT_LIMITS));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 2, 0x00, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            clear_focus();
        }

        private void btn_set_ccw_soft_lim_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SOFT_LIMITS));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 1, 0x00, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            clear_focus();
        }

        private void btn_clear_soft_lim_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SOFT_LIMITS));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 3, 0x00, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            clear_focus();
        }

        private void btnGoto_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            float num = float.Parse(cp_textbox[slot_number, 2].Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_number];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_ABSOLUTE));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
               slot_number, 0x00,
               data1[0], data1[1], data1[2], data1[3]
            };

            usb_write("stepper_functions", bytesToSend, 12);
            //clear_focus();
        }

        private void btnJogCCW_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            // DIR_CCW = 0
            byte dir = 0;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_JOG));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, dir, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            //clear_focus();
        }

        private void btnJogCW_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            // DIR_CW = 1
            byte dir = 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_JOG));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, dir, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            //clear_focus();
        }

        private void btnJogSave_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            float num = float.Parse(cp_textbox[slot_number, 3].Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = Math.Abs(1000 * value / slot_nm_per_count[slot_number]);
            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_JOGPARAMS));

            byte[] bytesToSend = new byte[28] { command[0], command[1], 22, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
                slot_number, 0,         // Chan Ident
                2,0,                    // Jog Mode - single step jogging
                data1 [0], data1 [1], data1 [2], data1 [3],    // Jog Step Size in encoder counts
                0x00,0x00,0x00,0x00,    // Jog Min Velocity (not used)
                0x00,0x00,0x00,0x00,    // Jog Acceleration (not used)
                0x00,0x00,0x00,0x00,    // Jog Max Velocity (not used)
                0,0                    // Stop Mode (not used) - 1 for immediate (abrupt) stop or 2 for profiled stop (with controlled deceleration).
                };
            usb_write("stepper_functions", bytesToSend, 28);
            //clear_focus();
        }

        private void disable_slot_controls(Byte slot_number, Byte enable)
        {
            // stackpanel name meso tab
            string s_meso_name = "sp_meso_" + (slot_number + 1).ToString();
            StackPanel sp_meso_name = (StackPanel)this.FindName(s_meso_name);
            try
            {
                if (enable == 1)
                {
                    cp_button[slot_number, 10].Content = "Enable";
                    cp_subpanels[slot_number, 0].IsEnabled = false;
                    cp_subpanels[slot_number, 1].IsEnabled = false;
                    cp_subpanels[slot_number, 2].IsEnabled = false;
                    cp_subpanels[slot_number, 3].IsEnabled = false;

                    if (sp_meso_name is not null)
                        sp_meso_name.IsEnabled = false;
                }
                else
                {
                    cp_button[slot_number, 10].Content = "Disable";
                    cp_subpanels[slot_number, 0].IsEnabled = true;
                    cp_subpanels[slot_number, 1].IsEnabled = true;
                    cp_subpanels[slot_number, 2].IsEnabled = true;
                    cp_subpanels[slot_number, 3].IsEnabled = true;

                    if (sp_meso_name is not null)
                        sp_meso_name.IsEnabled = true;
                }
            }
            catch (Exception)
            {

            }
        }

        private void btn_disable_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            Byte enable = 0;

            if (cp_button[slot_number, 10].Content.ToString() == "Disable")
            {
                disable_slot_controls(slot_number, 1);
                enable = 0;
            }
            else
            {
                disable_slot_controls(slot_number, 0);
                enable = 1;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOD_SET_CHANENABLESTATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, enable, slot_id, HOST_ID };

            _timer1.Enabled = false;
            usb_write("stepper_functions", bytesToSend, 6);
            Thread.Sleep(50);
            _timer1.Enabled = true;
            //clear_focus();
        }

        public void build_stepper_control(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
                       {
                           string slot_name = (_slot + 1).ToString();
                           string panel_name = "brd_slot_" + slot_name;
                           byte sub_panel_num = 0;
                           byte label_num = 0;
                           byte radio_num = 0;
                           byte button_num = 0;
                           byte textbox_num = 0;

                           StackPanel st_sel = (StackPanel)this.FindName(panel_name);

                           // Top stack panel
                           sub_panel_num = 0;
                           cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                           cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                           cp_subpanels[_slot, sub_panel_num].Name = "brd_slot_top_" + slot_name;
                           st_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                           // slot label
                           Label label = new Label();
                           label.Content += "Slot " + slot_name;
                           cp_subpanels[_slot, sub_panel_num].Children.Add(label);

                           // Axis label
                           label_num = 0;
                           cp_labels[_slot, label_num] = new Label();
                           cp_labels[_slot, label_num].Width = 90;
                           cp_labels[_slot, label_num].Content += "";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_labels[_slot, label_num]);

                           // Encoder text label
                           Label label3 = new Label();
                           label3.Content = "Encoder (um):";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(label3);

                           // Encoder Position textbox
                           textbox_num = 0;
                           cp_textbox[_slot, textbox_num] = new TextBox();
                           cp_textbox[_slot, textbox_num].Text = "";
                           cp_textbox[_slot, textbox_num].Width = 85;
                           cp_textbox[_slot, textbox_num].TextAlignment = TextAlignment.Right;
                           cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_textbox[_slot, textbox_num]);

                           // Encoder ABS Position textbox
                           textbox_num = 1;
                           cp_textbox[_slot, textbox_num] = new TextBox();
                           cp_textbox[_slot, textbox_num].Text = "";
                           cp_textbox[_slot, textbox_num].Width = 85;
                           cp_textbox[_slot, textbox_num].TextAlignment = TextAlignment.Right;
                           cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_textbox[_slot, textbox_num]);

                           // Stepper text label
                           Label label4 = new Label();
                           label4.Margin = new Thickness(10, 0, 0, 0);
                           label4.Content = "Step Position (um):";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(label4);

                           // Stepper Position label
                           label_num = 1;
                           cp_labels[_slot, label_num] = new Label();
                           cp_labels[_slot, label_num].Width = 85;
                           cp_textbox[_slot, textbox_num].TextAlignment = TextAlignment.Right;
                           cp_labels[_slot, label_num].Content += "";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_labels[_slot, label_num]);

                           // End Top stack panel

                           // 2nd row stack panel
                           sub_panel_num = 1;
                           cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                           cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                           st_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                           // CCW Limit
                           radio_num = 0;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "CCW <Limits>";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(5, 1, 5, 1);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);

                           // CW Limit
                           radio_num = 1;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "CW   |";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(0, 1, 5, 1);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);

                           // CCW Moving
                           radio_num = 2;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "CCW <Moving>";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(5, 1, 5, 0);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);

                           // CW Moving
                           radio_num = 3;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "CW   |";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(0, 1, 5, 0);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);

                           // CCW Soft Limit
                           radio_num = 4;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "CCW <Soft Limits>";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(5, 1, 5, 0);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);

                           // CW Soft Limit
                           radio_num = 5;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "CW   |";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(0, 1, 5, 0);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);
                           
                           // Homed status
                           radio_num = 6;
                           cp_radio[_slot, radio_num] = new RadioButton();
                           cp_radio[_slot, radio_num].Foreground = new SolidColorBrush(Colors.White);
                           cp_radio[_slot, radio_num].Content = "Homed";
                           cp_radio[_slot, radio_num].GroupName = (_slot * radio_num).ToString();
                           cp_radio[_slot, radio_num].Margin = new Thickness(10, 1, 5, 0);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_radio[_slot, radio_num]);

                           // End 2nd row stack panel

                           // 3nd row stack panel
                           sub_panel_num = 2;
                           cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                           cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                           st_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                           // Home Button
                           button_num = 0;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Home";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_home" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnHome_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Zero Button
                           button_num = 1;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Zero";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_zero" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnZero_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Stop Button
                           button_num = 2;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Stop";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_stop" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnStop_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Set CCW Soft Limit Button
                           button_num = 3;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Set CCW Soft Limit";
                           cp_button[_slot, button_num].Width = 110;
                           cp_button[_slot, button_num].Name = "st_ccw_soft_limit" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_set_ccw_soft_lim_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(20, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Set CW Soft Limit Button
                           button_num = 4;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Set CW Soft Limit";
                           cp_button[_slot, button_num].Width = 110;
                           cp_button[_slot, button_num].Name = "st_cw_soft_limit" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_set_cw_soft_lim_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Clear Soft Limits Button
                           button_num = 5;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Clear Soft Limits";
                           cp_button[_slot, button_num].Width = 110;
                           cp_button[_slot, button_num].Name = "st_clear_soft_limit" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_clear_soft_lim_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // End 3nd row stack panel

                           // 4nd row stack panel
                           sub_panel_num = 3;
                           cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                           cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                           st_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                           // Goto Button
                           button_num = 6;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Goto (um)";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_goto" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnGoto_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Goto textbox
                           textbox_num = 2;
                           cp_textbox[_slot, textbox_num] = new TextBox();
                           cp_textbox[_slot, textbox_num].Text = "0";
                           cp_textbox[_slot, textbox_num].Width = 120;
                           cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_textbox[_slot, textbox_num]);

                           // Jog CCW Button
                           button_num = 7;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Jog CCW";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_jog_ccw" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnJogCCW_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(10, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Jog CW Button
                           button_num = 8;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Jog CW";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_jog_cw" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnJogCW_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Jog textbox
                           textbox_num = 3;
                           cp_textbox[_slot, textbox_num] = new TextBox();
                           cp_textbox[_slot, textbox_num].Text = "";
                           cp_textbox[_slot, textbox_num].Width = 100;
                           cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_textbox[_slot, textbox_num]);

                           // Jog Save step Button
                           button_num = 9;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Jog Save Step Size";
                           cp_button[_slot, button_num].Width = 120;
                           cp_button[_slot, button_num].Name = "st_jog_save" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btnJogSave_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // 5th row stack panel
                           StackPanel row_5 = new StackPanel();
                           row_5.Orientation = Orientation.Horizontal;
                           st_sel.Children.Add(row_5);

                           // Enable/disable Button
                           button_num = 10;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Disable";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "st_disable" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_disable_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Goto position Button
                           button_num = 11;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P1";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "st_goto_p1_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Goto position Button
                           button_num = 12;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P2";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "st_goto_p2_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Goto position Button
                           button_num = 13;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P3";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "st_goto_p3_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Goto position Button
                           button_num = 14;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P4";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "st_goto_p4_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Goto position Button
                           button_num = 15;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P5";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "st_goto_p5_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);
                           
                           // Goto position Button
                           button_num = 16;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P6";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "st_goto_p6_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Move by Button
                           button_num = 17;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Move By";
                           cp_button[_slot, button_num].Width = 70;
                           cp_button[_slot, button_num].Name = "st_move_by_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_move_by_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(15, 5, 5, 5);
                           row_5.Children.Add(cp_button[_slot, button_num]);

                           // Move By textbox
                           textbox_num = 4;
                           cp_textbox[_slot, textbox_num] = new TextBox();
                           cp_textbox[_slot, textbox_num].Text = "0";
                           cp_textbox[_slot, textbox_num].Width = 100;
                           cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                           row_5.Children.Add(cp_textbox[_slot, textbox_num]);

                           // 6th row stack panel
                           StackPanel row_6 = new StackPanel();
                           row_6.Orientation = Orientation.Horizontal;
                           st_sel.Children.Add(row_6);

                           // Move CCW fast Button
                           button_num = 18;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "<<";
                           cp_button[_slot, button_num].Width = 70;
                           cp_button[_slot, button_num].Name = "st_move_CCW_F_" + slot_name;
                           cp_button[_slot, button_num].PreviewMouseLeftButtonDown += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonDown);
                           cp_button[_slot, button_num].PreviewMouseLeftButtonUp += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonUp);
                           //cp_button[_slot, button_num].PreviewTouchDown += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                           //cp_button[_slot, button_num].PreviewTouchUp += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchUp);

                           //cp_button[_slot, button_num].MouseLeave += new MouseEventHandler(btn_Velocity_Move_MouseLeave);
                           //Stylus.SetIsPressAndHoldEnabled(cp_button[_slot, button_num], true);
                           // cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_velocity_move_Click);
                           // cp_button[_slot, button_num].TouchEnter += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                           // cp_button[_slot, button_num].TouchLeave += new EventHandler<TouchEventArgs>(btn_Velocity_Move_TouchLeave);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_6.Children.Add(cp_button[_slot, button_num]);

                           // Move CCW slow Button
                           button_num = 19;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "<";
                           cp_button[_slot, button_num].Width = 70;
                           cp_button[_slot, button_num].Name = "st_move_CCW_S_" + slot_name;
                           cp_button[_slot, button_num].PreviewMouseLeftButtonDown += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonDown);
                           cp_button[_slot, button_num].PreviewMouseLeftButtonUp += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonUp);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_6.Children.Add(cp_button[_slot, button_num]);



                           //Stylus.SetIsPressAndHoldEnabled(cp_button[_slot, button_num], true);
                           // cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_velocity_move_Click);
                           //cp_button[_slot, button_num].PreviewTouchDown += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                           // cp_button[_slot, button_num].TouchEnter += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                           //cp_button[_slot, button_num].PreviewTouchUp += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchUp);
                           // cp_button[_slot, button_num].TouchLeave += new EventHandler<TouchEventArgs>(btn_Velocity_Move_TouchLeave);

                           // Move CCW fast Button
                           button_num = 20;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = ">";
                           cp_button[_slot, button_num].Width = 70;
                           cp_button[_slot, button_num].Name = "st_move_CW_S_" + slot_name;
                           //Stylus.SetIsPressAndHoldEnabled(cp_button[_slot, button_num], true);
                          // cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_velocity_move_Click);
                           cp_button[_slot, button_num].PreviewMouseLeftButtonDown += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonDown);
                           cp_button[_slot, button_num].PreviewMouseLeftButtonUp += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonUp);
                           //cp_button[_slot, button_num].PreviewTouchDown += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                          /// cp_button[_slot, button_num].TouchEnter += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                           //cp_button[_slot, button_num].PreviewTouchUp += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchUp);
                          // cp_button[_slot, button_num].TouchLeave += new EventHandler<TouchEventArgs>(btn_Velocity_Move_TouchLeave);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_6.Children.Add(cp_button[_slot, button_num]);

                           // Move CCW fast Button
                           button_num = 21;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = ">>";
                           cp_button[_slot, button_num].Width = 70;
                           cp_button[_slot, button_num].Name = "st_move_CW_F_" + slot_name;
                           //Stylus.SetIsPressAndHoldEnabled(cp_button[_slot, button_num], true);
                         //  cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_velocity_move_Click);
                           cp_button[_slot, button_num].PreviewMouseLeftButtonDown += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonDown);
                           cp_button[_slot, button_num].PreviewMouseLeftButtonUp += new MouseButtonEventHandler(btn_Velocity_Move_PreviewMouseLeftButtonUp);
                           //cp_button[_slot, button_num].PreviewTouchDown += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                          // cp_button[_slot, button_num].TouchEnter += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchDown);
                           //cp_button[_slot, button_num].PreviewTouchUp += new EventHandler<TouchEventArgs>(btn_Velocity_Move_PreviewTouchUp);
                           //cp_button[_slot, button_num].TouchLeave += new EventHandler<TouchEventArgs>(btn_Velocity_Move_TouchLeave);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           row_6.Children.Add(cp_button[_slot, button_num]);

                       }));

            // get the jog step size
            stepper_req_jog_params(_slot);
        }

        private void stepper_req_jog_params(byte slot_number)
        {
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_JOGPARAMS));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
        }

        private void stepper_get_jog_params()
        {
            double temp;

            byte slot = (byte)(ext_data[0] | ext_data[1]);

            // jog_mode
            Int16 jog_mode = (Int16)(ext_data[2] | ext_data[3] << 8);

            // step_size
            Int32 step_size = (ext_data[4] | ext_data[5] << 8 | ext_data[6] << 16 | ext_data[7] << 24);

            // min_vel
            Int32 min_vel = (ext_data[8] | ext_data[9] << 8 | ext_data[10] << 16 | ext_data[11] << 24);

            // acc
            Int32 acc = (ext_data[12] | ext_data[13] << 8 | ext_data[14] << 16 | ext_data[15] << 24);

            // max_vel
            Int32 max_vel = (ext_data[16] | ext_data[17] << 8 | ext_data[18] << 16 | ext_data[19] << 24);

            // stop_mode
            Int16 stop_mode = (Int16)(ext_data[20] | ext_data[21] << 8);


            // Step size is in encoder counts so we need to convert from um to encoder counts
            temp = (step_size * slot_nm_per_count[slot]) / 1000;
            temp = Math.Round(temp, 2);

            this.Dispatcher.Invoke(new Action(() =>
            {
                try
                {
                    cp_textbox[slot, 3].Text = temp.ToString();
                }
                catch (Exception)
                {

                }
            }));

            foreach (var v in stepper_windows)
            {
                if (v.Item2.Slot == slot)
                {
                    v.Item2.JogStepSize = step_size;
                    v.Item2.OptionalSpeedLimit.JoggingMovementSpeedLimit = (uint)max_vel;
                    v.Item1.PostConfiguration(v.Item2);
                }
            }
        }

        private void btn_goto_stored_position_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            // get the position number
            string pos_num_t = button_name.Substring(button_name.Length - 3, 1);
            Byte pos_num = Convert.ToByte(pos_num_t);
            pos_num -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_GOTO_STORE_POSITION));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, pos_num, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);
            //clear_focus();
        }

        private void btn_move_by_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            float num = float.Parse(cp_textbox[slot_number, 4].Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_number];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_MOVE_BY));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
               slot_number, 0x00,
               data1[0], data1[1], data1[2], data1[3]
            };

            usb_write("stepper_functions", bytesToSend, 12);
            //clear_focus();
        }


        /// ///////////////////////


        private void btn_Velocity_Move_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _timer1.Enabled = false;
            byte dir = 0;   // dir CCW = 0, CW = 1; 
            byte speed = 0;    // speed percent of max speed for stage 0 = stopped, 100 = max speed of stage

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);

            string dir_t = button_name.Substring(button_name.Length - 7, 3);

            if (dir_t == "CCW")
                dir = 0;
            else
                dir = 1;

            string speed_t = button_name.Substring(button_name.Length - 3, 1);
            if (speed_t == "F")
                speed = 100;
            else
                speed = 30;

            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            velocity_move(slot_id, dir, speed);
            _timer1.Enabled = true;
        }


        private void btn_Velocity_Move_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            _timer1.Enabled = false;
            btnStop_Click(sender, null);
            Thread.Sleep(50);
            btnStop_Click(sender, null);
            _timer1.Enabled = true;

        }

        private void btn_Velocity_Move_PreviewTouchUp(object sender, TouchEventArgs e)
        {
            btn_Velocity_Move_PreviewMouseLeftButtonUp(sender, null);

        }

        private void btn_Velocity_Move_TouchLeave(object sender, TouchEventArgs e)
        {
            btn_Velocity_Move_PreviewMouseLeftButtonUp(sender, null);
        }

        private void btn_Velocity_Move_PreviewTouchDown(object sender, TouchEventArgs e)
        {
            btn_Velocity_Move_PreviewMouseLeftButtonDown(sender, null);
        }

        private void velocity_move(byte slot_id, byte dir, byte speed)
        {
            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(speed));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_SET_VELOCITY));

            byte[] bytesToSend = new byte[6] { command[0], command[1], dir, speed, slot_id, HOST_ID };

            usb_write("stepper_functions", bytesToSend, 6);

        }
        private void btn_velocity_move_Click(object sender, RoutedEventArgs e)
        {
            //clear_focus_fast();
            //// Send STOP on mouse up again to make sure it stops
            //_timer1.Enabled = false;
            //byte dir = 0;   // dir CCW = 0, CW = 1; 
            //byte speed = 0;    // speed percent of max speed for stage 0 = stopped, 100 = max speed of stage

            //Button clickedButton = (Button)sender;
            //string button_name = clickedButton.Name;
            //string slot = button_name.Substring(button_name.Length - 1, 1);

            //Byte slot_number = Convert.ToByte(slot);
            //Byte slot_id = (Byte)(slot_number | 0x20);
            //slot_number -= 1;

            //velocity_move(slot_id, dir, speed);
            //_timer1.Enabled = true;
        }

        private void bt_stepper_erase_params_Click (object sender, RoutedEventArgs e)
        {
            if (confirm_allow_slot_config_reset())
            {
                int slot_id = cb_stage_select.SelectedIndex;

                if (slot_id >= 0)
                {
                    string str = ((ComboBoxItem)cb_stage_select.Items[slot_id]).Content.ToString();
                    slot_id = int.Parse(str.Substring(str.Length - 1));
                    reset_slot_card_configuration(slot_id - 1);
                }
            }
        }

        private bool IsSoftStopFeatureAvailable()
        {
            return _firmwareVersion is not null && _firmwareVersion >= new Version(7, 2, 1);
        }
    }
}